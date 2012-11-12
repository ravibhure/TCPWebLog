/*
//=============================================================================+
// File name   : tcpweblog_server.c
// Begin       : 2012-02-14
// Last Update : 2012-11-12
// Version     : 3.2.3
//
// Website     : https://github.com/fubralimited/TCPWebLog
//
// Description : This program listen on specified IP:PORT for incoming TCP
//               messages from tcpweblog_client.bin, and split them on local
//               filesystem by type and IP address
//
// Author: Nicola Asuni
//
// (c) Copyright:
//               Fubra Limited
//               Manor Coach House
//               Church Hill
//               Aldershot
//               Hampshire
//               GU12 4RQ
//				       UK
//               http://www.fubra.com
//               support@fubra.com
//
// License:
//    Copyright (C) 2012-2012 Fubra Limited
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Affero General Public License as
//    published by the Free Software Foundation, either version 3 of the
//    License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Affero General Public License for more details.
//
//    You should have received a copy of the GNU Affero General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//    See LICENSE.TXT file for more information.
//=============================================================================+
*/

/*
TO COMPILE (requires sqlite-devel):
	gcc -O3 -g -pipe -Wp,-D_THREAD_SAFE -D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector -fno-strict-aliasing -fwrapv -fPIC --param=ssp-buffer-size=4 -D_GNU_SOURCE -o tcpweblog_server.bin tcpweblog_server.c -lpthread

USAGE EXAMPLES:
	./tcpweblog_server.bin PORT MAX_CONNECTIONS ROOT_DIR
	./tcpweblog_server.bin 9940 100 "/cluster/"

NOTES:
	You must implement some firewall rules in your server to avoid receiving TCP messages from unauthorized clients.
*/

#include <pthread.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

// DEBUG OPTION TO PRINT EXECUTION TIME
//#define _DEBUG
#ifdef _DEBUG
	#include <time.h>
	clock_t starttime;
	clock_t endtime;
#endif

/**
 * Max size of the TCP buffer lenght.
 */
#define BUFLEN 65536

/**
 * Read size for TCP buffer
 */
#define READBUFLEN 65534

/**
 * Number of sockets (IPv4 + IPv6)
 */
#define MAXSOCK 2

/**
 * Root directory used to store log files
 */
char *rootdir = NULL;

/**
 * Struct to contain thread arguments.
 */
typedef struct _targs {
	int socket_conn;
} targs;

/**
 * Report error message and close the program.
 */
void diep(const char *s) {
	perror(s);
	exit(1);
}

/**
 * Daemonize this process.
 */
static void daemonize(void) {
	pid_t pid, sid;
	if (getppid() == 1) {
		// this is already a daemon
		return;
	}
	// fork off the parent process
	pid = fork();
	if (pid < 0) {
		exit(1);
	}
	// if we got a good PID, then we can exit the parent process
	if (pid > 0) {
		exit(0);
	}
	// at this point we are executing as the child process
	// change the file mode mask
	umask(0);
	// create a new SID for the child process
	sid = setsid();
	if (sid < 0) {
		exit(1);
	}
	// change the current working directory to prevents the current directory from being locked
	if ((chdir("/")) < 0) {
		exit(1);
	}
	// redirect standard files to /dev/null
	FILE *ignore;
	ignore = freopen( "/dev/null", "r", stdin);
	ignore = freopen( "/dev/null", "w", stdout);
	ignore = freopen( "/dev/null", "w", stderr);
}

/**
 * Append the input string on a log file
 * @param s string to append on log.
 * @param file file name (including full path).
 */
void appendlog(const char *s, const char *file) {
	FILE *fp = NULL;
	int ret = EOF;
	if ((fp = fopen(file, "a")) != NULL) {
		ret = fputs(s, fp);
		fclose(fp);
	} else { // try to create the missing directories
		// duplicate path
		char *dirpath = strdup(file);
		int status = 0;
		char *pp;
		char *sp;
		pp = dirpath;
		while ((sp = strchr(pp, '/')) != 0) {
			if (sp != pp) {
				// terminate string
				*sp = '\0';
				// create dir
				mkdir(dirpath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
				// restore original character
				*sp = '/';
			}
			// move pointer
			pp = (sp + 1);
		}
		free(dirpath);
		// try once again to write the file
		if ((fp = fopen(file, "a")) != NULL) {
			ret = fputs(s, fp);
			fclose(fp);
		}
	}
	if (ret == EOF) {
		// output an error message
		perror("TCPWebLog-Server (appendlog)");
		perror(file);
	}
}

/**
 * Process log rows.
 * @param row Raw row to insert.
 */
void process_row(char *row) {

	// cluster number
	int cluster = 0;

	// log name
	char logname[BUFLEN];

	// original log line
	char logline[BUFLEN];

	// temporary log line
	char tmpline[BUFLEN];

	// ip address
	char clientip[40];

	// hostname
	char clienthost[256];

	// client ident (FTP username)
	char clientident[256];

	// full file path
	char file[BUFLEN];

	// pointer for strtok_r
	char *endstr = NULL;

	if ((row[0] != '@') || (row[1] != '@')) {
		perror("TCPWebLog-Server (invalid line)");
		return;
	}

	// remove first 2 characters "@@"
	memmove(row, (row + 2), strlen(row));

	// get log name
	strcpy(logname, strtok_r(row, "\t", &endstr));

	// get cluster number prefix
	cluster = atoi(strtok_r(NULL, "\t", &endstr));

	// get the client ip
	strcpy(clientip, strtok_r(NULL, "\t", &endstr));

	// get the client hostname
	strcpy(clienthost, strtok_r(NULL, "\t", &endstr));

	// get original log line
	strcpy(logline, strtok_r(NULL, "\n", &endstr));

	// add newline char
	strcat(logline, "\n");

	// reset strtok_r string pointer
	endstr = NULL;

	// check if client IP and host are defined on parameters
	if ((clientip == NULL) || (strlen(clientip) < 3)) {
		// try to extract info from log line
		if (strstr(logname, "ftp") != NULL) {
      // special case for FTP
      // pure-ftpd clf format: 10.199.255.254 - 2x7eyq [12/Nov/2012:09:00:55 -0000] "PUT /cluster/001/vclusters/2x7eyq/sites/2x7eyq.alpha.dev2.lab/http/phpinfo.php" 200 17
			strtok_r(logline, " ", &endstr); // skip IP
			strtok_r(NULL, " ", &endstr); // skip host
			// get the FTP username (ident)
			strcpy(clientident, strtok_r(NULL, " ", &endstr));
      // get original log line
			strcpy(tmpline, strtok_r(NULL, "\n", &endstr));
			// add newline char
			strcat(tmpline, "\n");
			strcpy(logline, tmpline);
			// compose file name and path
			sprintf(file, "%s%03d/logs/ident/%s/%s.%s", rootdir, cluster, clientident, clientident, logname);
		} else { // extract IP and host name from log line
			// You must prefix the log format with "%A %V", for example:
			// "%A %V %{X-Forwarded-For}i %l %u %t \"%r\" %>s %b \"%{Referer}i\" \"%{User-Agent}i\""
			// get the IP address
			strcpy(clientip, strtok_r(logline, " ", &endstr));
			// get the hostname
			strcpy(clienthost, strtok_r(NULL, " ", &endstr));
			// get original log line
			strcpy(tmpline, strtok_r(NULL, "\n", &endstr));
			// add newline char
			strcat(tmpline, "\n");
			strcpy(logline, tmpline);
			// compose file name and path
			sprintf(file, "%s%03d/logs/ip/%s/%s.%s", rootdir, cluster, clientip, clienthost, logname);
		}
	} else {
		// compose file name and path
		sprintf(file, "%s%03d/logs/ip/%s/%s.%s", rootdir, cluster, clientip, clienthost, logname);
	}

	// save the log
	appendlog(logline, file);
}

/**
 * Thread to handle one connection.
 * @param cargs Arguments.
 */
void *connection_thread(void *cargs) {

	// decode parameters
	targs arg = *((targs *) cargs);

	// buffer used to receive TCP messages
	char buf[BUFLEN];

	// store last row portion
	char lastrow[BUFLEN];

	// line delimiter
	char delims[] = "\n";

	// single log line
	char *row = NULL;

	// string pointer for strtok_r
	char *endstr = NULL;

	// position inside the row buffer
	int pos = 0;

	// check if a row is splitted across two reads
	int splitline = 0;

	// lenght of buffer
	int buflen = 0;

	// clear (initialize) the strings
	memset(buf, 0, BUFLEN);
	memset(lastrow, 0, BUFLEN);

	#ifdef _DEBUG
		starttime = clock();
		printf("  START TIME [sec]: %.3f\n", (double)starttime / CLOCKS_PER_SEC);
	#endif

	// receive a message from ns and put data int buf (limited to BUFLEN characters)
	while ((buflen = read(arg.socket_conn, buf, READBUFLEN)) > 0) {

		// mark the end of buffer (used to recompose splitted lines)
		buf[buflen] = 2;

		// split stream into lines
		row = strtok_r(buf, delims, &endstr);

		// for each line on buf
		while (row != NULL) {
			pos = (strlen(row) - 1);
			if (row[pos] == 2) { // the line is incomplete
				if (pos > 0) { // avoid lines that contains only the terminator character
					// store uncompleted row part
					strcpy(lastrow, row);
					lastrow[pos] = '\0';
					splitline = 1;
				}
			} else { // we reached the end of a line
				if (splitline == 1) { // reconstruct splitted line
					if ((row[0] != '@') || (row[1] != '@')) {
						// recompose the line
						strcat(lastrow, row);
						// insert row on database
						process_row(lastrow);
					} else { // the lastrow contains a full line (only the newline character was missing)
						// insert row on database
						process_row(lastrow);
						// insert row on database
						process_row(row);
					}
				} else { // we have an entire line
					// insert row on database
					process_row(row);
				}
				memset(lastrow, 0, BUFLEN);
				splitline = 0;
			}

			// read next line
			row = strtok_r(NULL, delims, &endstr);

		} // end for each row

		// clear the buffer
		memset(buf, 0, BUFLEN);

	} // end read TCP

	#ifdef _DEBUG
		endtime = clock();
		printf("    END TIME [sec]: %.3f\n", (double)endtime / CLOCKS_PER_SEC);
		printf("ELAPSED TIME [sec]: %.3f\n\n", (double)(endtime - starttime) / CLOCKS_PER_SEC);
	#endif

	// close connection
	close(arg.socket_conn);

	// free memory
	free(row);

} // end connection_thread

/**
 * Main function.
 * @param argc Argument counter.
 * @param argv[] Array of arguments: port, max_connections, sqlite_database.
 */
int main(int argc, char *argv[]) {

	// decode arguments
	if (argc != 4) {
		diep("\
This program listen on specified IP:PORT for incoming TCP messages from tcpweblog_client.bin, and split them on local filesystem by IP address and type.\n\
You must provide 3 arguments: port, max_conenctions, root_directory \n\
FOR EXAMPLE:\n\
./tcpweblog_server.bin 9940 100 \"/cluster/\"");
	}

	// listening TCP port
	char *port = (char *)argv[1];

	// max number of connections
	int maxconn = atoi(argv[2]);

	// root directory where to put log files
	rootdir = (char *)argv[3];

	// daemonize this program
	daemonize();

	// thread identifier
	pthread_t tid;

	// thread attributes
	pthread_attr_t tattr;

	// thread number
	int tn = 0;

	// arguments be passes on thread
	targs cargs[maxconn];

	// true option for setsockopt
	int opttrue = 1;


	// file descriptor sets for the select function
	fd_set mset, wset;

	// initialize descriptor for select
    FD_ZERO(&mset);

	// structures to handle address information
	struct addrinfo hints, *res, *aip;

	// defines the socket at the other end of the link (that is, the client)
	struct sockaddr_storage si_client;

	// size of si_client
	socklen_t slen = sizeof(si_client);

	// sockets array
	int sockfd[MAXSOCK];

	// max socket number
	int maxsockfd = 0;

	// socket number
	int nsock=0;

	// socket options
	int opts=-1;

	// new socket
	int ns = -1;

	// initialize structure
	memset(&hints, 0, sizeof(hints));

	// set parameters
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	// get address info
	if (getaddrinfo(NULL, port, &hints, &res) != 0) {
		diep("TCPWebLog-Server (getaddrinfo)");
	}

	// for each socket type
	for (aip = res; (aip && (nsock < MAXSOCK)); aip = aip->ai_next) {

		// try to create a network socket.
		sockfd[nsock] = socket(aip->ai_family, aip->ai_socktype, aip->ai_protocol);

		if (sockfd[nsock] < 0) {
			switch (errno) {
				case EAFNOSUPPORT:
				case EPROTONOSUPPORT: {
					// skip the errors until the last address family
					if (aip->ai_next) {
						continue;
					} else {
						// handle unknown protocol errors
						diep("TCPWebLog-Server (socket)");
						break;
					}
				}
				default: {
					// handle other socket errors
					diep("TCPWebLog-Server (socket)");
					break;
				}
			}
		} else {
			if (aip->ai_family == AF_INET6) {
				// set socket to listen only IPv6
				if (setsockopt(sockfd[nsock], IPPROTO_IPV6, IPV6_V6ONLY, &opttrue, sizeof(opttrue)) == -1) {
					perror("TCPWebLog-Server (setsockopt : IPPROTO_IPV6 - IPV6_V6ONLY)");
					continue;
				}
			}
			// make socket reusable
			if (setsockopt(sockfd[nsock], SOL_SOCKET, SO_REUSEADDR, &opttrue, sizeof(opttrue)) == -1) {
				perror("TCPWebLog-Server (setsockopt : SOL_SOCKET - SO_REUSEADDR)");
				continue;
			}
			// make socket non-blocking
			opts = fcntl(sockfd[nsock], F_GETFL);
			if (opts < 0) {
				perror("TCPWebLog-Server (fcntl(F_GETFL))");
				continue;
			}
			opts = (opts | O_NONBLOCK);
			if (fcntl(sockfd[nsock], F_SETFL, opts) < 0) {
				perror("TCPWebLog-Server (fcntl(F_SETFL))");
				continue;
			}
			// bind the socket to the address
			if (bind(sockfd[nsock], aip->ai_addr, aip->ai_addrlen) < 0) {
				close(sockfd[nsock]);
				continue;
			}
			// listen for incoming connections on each stack
			if (listen(sockfd[nsock], maxconn) < 0) {
				close(sockfd[nsock]);
				continue;
			} else {
				// set select for this socket
				FD_SET(sockfd[nsock], &mset);
				if (sockfd[nsock] > maxsockfd) {
					// set the maximum socket number
					maxsockfd = sockfd[nsock];
				}
			}
		}
		// move to next socket
		nsock++;
	}
	// free resource
	freeaddrinfo(res);

	// forever
	while (1) {

		// copy master sd_set to the working sd_set
		memcpy(&wset, &mset, sizeof(mset));

		// call select() to wait for connection from all defined sockets
		if (select((maxsockfd + 1), &wset, NULL, NULL, NULL) < 0) {
			if (errno == EINTR) {
				// ignore this error and go back
				continue;
			}
			diep("TCPWebLog-Server (select)");
		}

		// for each socket
		for (nsock = 0; nsock < MAXSOCK; nsock++) {
			// if we have a connection on the socket
			if (FD_ISSET(sockfd[nsock], &wset)) {

				// accept a connection on a socket
				if ((ns = accept(sockfd[nsock], (struct sockaddr *) &si_client, &slen)) == -1) {
					// print an error message
					perror("TCPWebLog-Server (accept)");
					continue;
				} else {
					// make socket blocking
					opts = fcntl(sockfd[nsock], F_GETFL);
					if (opts < 0) {
						perror("TCPWebLog-Server (accept() > fcntl(F_GETFL))");
						continue;
					}
					opts = (opts & (~O_NONBLOCK));
					if (fcntl(sockfd[nsock], F_SETFL, opts) < 0) {
						perror("TCPWebLog-Server (accept() > fcntl(F_SETFL))");
						continue;
					}

					// prepare data for the thread
					cargs[tn].socket_conn = ns;

					// handle each connection on a separate thread
					pthread_attr_init(&tattr);
					pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
					pthread_create(&tid, &tattr, connection_thread, (void *)&cargs[tn]);

					// increase thread number
					tn++;

					if (tn >= maxconn) {
						// reset connection number
						tn = 0;
					}
				}
			}
		}
	} // end of while loop

	// close connections
	for (nsock = 0; nsock < MAXSOCK; nsock++) {
		close(sockfd[nsock]);
	}

	// close program and return 0
	return 0;
}

//=============================================================================+
// END OF FILE
//=============================================================================+
