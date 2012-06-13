/*
//=============================================================================+
// File name   : tcpweblog_server.c
// Begin       : 2012-02-14
// Last Update : 2012-06-13
// Version     : 1.1.0
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
//				 UK
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

// TO COMPILE (requires sqlite-devel):
// gcc -O3 -g -pipe -Wp,-D_THREAD_SAFE -D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector -fno-strict-aliasing -fwrapv -fPIC --param=ssp-buffer-size=4 -D_GNU_SOURCE -o tcpweblog_server.bin tcpweblog_server.c -lpthread

// USAGE EXAMPLES:
// ./tcpweblog_server.bin PORT MAX_CONNECTIONS ROOT_DIR
// ./tcpweblog_server.bin 9940 100 "/cluster/"

#include <pthread.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

// DEBUG OPTION TO PRINT EXECUTION TIME
//#define _DEBUG
#ifdef _DEBUG
	#include <time.h>
	time_t starttime;
	time_t endtime;
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
 * Append the input string on a log file
 * @param s string to append on log.
 */
void appendlog(const char *s, const char *file) {
	FILE *fp = NULL;
	int ret = EOF;
	fp = fopen(file, "a");
	if (fp != NULL) {
		ret = fputs(s, fp);
		fclose(fp);
	}
	if (ret == EOF) {
		// output an error message
		perror("TCPWebLog-Server (appendlog)");
	}
}

/**
 * Process log rows.
 * @param row Raw row to insert.
 */
void process_row(char *row) {

	// cluster number
	int cluster = 0;

	// log type
	int logtype = 0;

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

	// file path where to store the log line
	char file[BUFLEN];

	// pointer for strtok_r
	char *endstr = NULL;

	if ((row[0] != '@') || (row[1] != '@')) {
		perror("TCPWebLog-Server (invalid line)");
		return;
	}

	// remove first 2 characters "@@"
	memmove(row, (row + 2), strlen(row));

	// get log type
	logtype = atoi(strtok_r(row, "\t", &endstr));

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

	// process lines by type
	switch (logtype) {
		// Apache Access Log
		case 1: {
			// compose file name and path
			sprintf(file, "%s%03d/logs/ip/%s/%s.access.log", rootdir, cluster, clientip, clienthost);
			break;
		}
		// Apache Error Log
		case 2: {
			// compose file name and path
			sprintf(file, "%s%03d/logs/ip/%s/%s.error.log", rootdir, cluster, clientip, clienthost);
			break;
		}
		// Apache SSL Access Log
		case 3: {
			// compose file name and path
			sprintf(file, "%s%03d/logs/ip/%s/%s.ssl.access.log", rootdir, cluster, clientip, clienthost);
			break;
		}
		// Apache SSL Error Log
		case 4: {
			// compose file name and path
			sprintf(file, "%s%03d/logs/ip/%s/%s.ssl.error.log", rootdir, cluster, clientip, clienthost);
			break;
		}
		// Varnish NCSA Log
		// You must prefix the log format with "%h %V", for example:
		// "%h %V %{X-Forwarded-For}i %l %u %t \"%r\" %>s %b \"%{Referer}i\" \"%{User-Agent}i\""
		case 5: {
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
			sprintf(file, "%s%03d/logs/ip/%s/%s.varnish.log", rootdir, cluster, clientip, clienthost);
			break;
		}
		// FTP Error Log
		case 6: {
			strtok_r(logline, " ", &endstr);
			strtok_r(NULL, " ", &endstr);
			// get the FTP username (ident)
			strcpy(clientident, strtok_r(NULL, " ", &endstr));
			// compose file name and path
			sprintf(file, "%s%03d/logs/ident/%s/%s.ftp.log", rootdir, cluster, clientident, clientident);
			break;
		}
		// PHP Error Log
		case 7: {
			// compose file name and path
			sprintf(file, "%s%03d/logs/ip/%s/%s.php.error.log", rootdir, cluster, clientip, clienthost);
			break;
		}
		// Unknow log type
		default: {
			perror("TCPWebLog-Server (unknown log type)");
			return;
			break;
		}
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
		starttime = time(NULL);
		printf("  START TIME [sec]: %d\n", starttime);
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
		endtime = time(NULL);
		printf("    END TIME [sec]: %d\n", endtime);
		printf("ELAPSED TIME [sec]: %d\n\n", (endtime - starttime));
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
		diep("This program listen on specified IP:PORT for incoming TCP messages from tcpweblog_client.bin, and split them on local filesystem by IP address and type.\nYou must provide 3 arguments: port, max_conenctions, root_directory \nFOR EXAMPLE:\n./tcpweblog_server.bin 9940 100 \"/cluster/\"");
	}

	// listening TCP port
	int port = atoi(argv[1]);

	// max number of connections
	int maxconn = atoi(argv[2]);

	// root directory where to put log files
	rootdir = (char *)argv[3];

	// thread identifier
	pthread_t tid;

	// thread attributes
	pthread_attr_t tattr;

	// thread number
	int tn = 0;

	// arguments be passes on thread
	targs cargs[maxconn];

	// option for SOL_SOCKET
	int optval = 1;

	// structure containing an Internet socket address: an address family (always AF_INET for our purposes), a port number, an IP address
	// si_server defines the socket where the server will listen.
	struct sockaddr_in6 si_server;

	// defines the socket at the other end of the link (that is, the client)
	struct sockaddr_in6 si_client;

	// size of si_client
	int slen = sizeof(si_client);

	// socket
	int s = -1;

	// new socket
	int ns = -1;

	// initialize the si_server structure filling it with binary zeros
	memset((char *) &si_server, 0, slen);

	// use internet address
	si_server.sin6_family = AF_INET6;

	// listen to any IP address
	si_server.sin6_addr = in6addr_any;

	// set the port to listen to, and ensure the correct byte order
	si_server.sin6_port = htons(port);

	// Create a network socket.
	// AF_INET says that it will be an Internet socket.
	// SOCK_STREAM Provides sequenced, reliable, two-way, connection-based byte streams.
	if ((s = socket(si_server.sin6_family, SOCK_STREAM, 0)) == -1) {
		diep("TCPWebLog-Server (socket)");
	}

	// set socket to listen only IPv6
	if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof(optval)) == -1) {
		diep("TCPWebLog-Server (setsockopt : IPPROTO_IPV6 - IPV6_V6ONLY)");
	}

	// set SO_REUSEADDR on socket to true (1):
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
		diep("TCPWebLog-Server (setsockopt : SOL_SOCKET - SO_REUSEADDR)");
	}

	// bind the socket s to the address in si_server.
	if (bind(s, (struct sockaddr *) &si_server, slen) == -1) {
		diep("TCPWebLog-Server (bind)");
	}

	// listen for connections
	listen(s, maxconn);

	// forever
	while (1)  {

		// accept a connection on a socket
		if ((ns = accept(s, (struct sockaddr *) &si_client, &slen)) == -1) {
			// print an error message
			perror("TCPWebLog-Server (accept)");
			// retry after 1 second
			sleep(1);
		} else {

			// prepare data for the thread
			cargs[tn].socket_conn = ns;

			// handle each connection on a separate thread
			pthread_attr_init(&tattr);
			pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
			pthread_create(&tid, &tattr, connection_thread, (void *)&cargs[tn]);

			tn++;

			if (tn >= maxconn) {
				// reset connection number
				tn = 0;
			}
		}

	} // end of for loop

	// close socket
	close(s);

	// close program and return 0
	return 0;
}

//=============================================================================+
// END OF FILE
//=============================================================================+
