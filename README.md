TCPWebLog - README
====================

+ Name: TCPWebLog

+ Version: 3.2.4

+ Release date: 2012-11-12

+ Author: Nicola Asuni

+ Copyright (2012-2012):

> > Fubra Limited  
> > Manor Coach House  
> > Church Hill  
> > Aldershot  
> > Hampshire  
> > GU12 4RQ  
> > <http://www.fubra.com>  
> > <support@fubra.com>  


SOFTWARE LICENSE:
-----------------

This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.

See LICENSE.TXT file for more information.


DESCRIPTION:
------------

TCPWebLog is a Free Open Source Software system to collect and aggregate Web logs (i.e. Apache and Varnish) from multiple GNU/Linux computers running on a Cloud.
This system is basically composed by a simple "client" program used to directly pipe the Web logs to a central server via TCP connection, and a "server" program to receive the logs and quickly aggregate/split them by case:


## TCPWebLog-Client ##

This section contains the software to be installed on the cluster nodes. It is essentially composed by the tcpweblog_client.bin program to transmit the input log data to a remote server through a TCP connection.


## TCPWebLog-Server ##

The TCPWebLog-Server program listens on a TCP port for incoming log data from multiple TCPWebLog-Client clients and stores the logs on the local filesystem.
Once installed and configured, this system can be easily started and stopped using the provided SysV init script.


## GENERAL USAGE SCHEMA ##
	
	(TCPWebLog-Client) - - - TCP connection  - - -> (TCPWebLog-Server)



HOW-TO CREATE TCPWebLog RPMs
------------------------------

This is a short hands-on tutorial on creating RPM files for the TCPWebLog project.
For an automatic building script for CentOS and the latest RPM packages please check the CatN Repository: https://github.com/fubralimited/CatN-Repo

NOTE: The sever configuration for TCPWebLog-Client and TCPWebLog-Server may be different, so this process must be executed in different environments.


## DEVELOPMENT ENVIRONMENT ##

To build RPMs we need a set of development tools.
This is a one-time-only setup, installed by running those commands from a system administration (root) account.
NOTE: You may need to change the the 
	
Install the EPEL repository:

	# rpm -Uvh http://download.fedoraproject.org/pub/epel/6/$(uname -m)/epel-release-6-7.noarch.rpm

Install development tools and Fedora packager:

	# yum install @development-tools
	# yum install fedora-packager

The following packages are required to create TCPWebLog RPMs:

	# yum install kernel-devel elfutils-devel

Create a dummy user specifically for creating RPM packages:

	# /usr/sbin/useradd makerpm
	# passwd makerpm

Reboot the machine, log as makerpm user and create the required directory structure in your home directory by executing: 

	$ rpmdev-setuptree

The rpmdev-setuptree program will create the ~/rpmbuild directory and a set of subdirectories (e.g. SPECS and BUILD), which you will use for creating your packages. The ~/.rpmmacros file is also created, which can be used for setting various options. 


## CREATE THE TCPWebLog RPMs ##

Download the TCPWebLog sources:

	$ cd ~
	$ git clone git://github.com/fubralimited/TCPWebLog.git

Copy the SPEC files and source files to rpmbuild dir:
	
	$ cd ~/TCPWebLog
	$ export SUVER=$(cat VERSION) 
	
	$ cd ~/TCPWebLog/client
	$ cp tcplogsender.spec ~/rpmbuild/SPECS/
	$ tar -zcvf ~/rpmbuild/SOURCES/tcpweblog_client-$SUVER.tar.gz  *
	
	$ cd ~/TCPWebLog/server
	$ cp tcpweblog_server.spec ~/rpmbuild/SPECS/
	$ tar -zcvf ~/rpmbuild/SOURCES/tcpweblog_server-$SUVER.tar.gz  *

Create the RPMs:

	$ cd ~/rpmbuild/SPECS/
	$ rpmbuild -ba tcpweblog_client.spec
	$ rpmbuild -ba tcpweblog_server.spec


The RPMs are now located at ~/rpmbuild/RPMS/$(uname -m)


INSTALL SERVERUSAGE SERVER:
---------------------------

As root install the TCPWebLog-Server RPM file:

	# rpm -i tcpweblog_server-3.2.4-1.el6.$(uname -m).rpm 
	
Configure the TCPWebLog-Server

	# nano /etc/tcpweblog_server.conf

Start the service

	# /etc/init.d/tcpweblog_server start

Start the service at boot:

	# chkconfig tcpweblog_server on

INSTALL SERVERUSAGE CLIENT:
---------------------------

As root install the SystemTap runtime and TCPWebLog-Client RPM files:

	# rpm -i tcpweblog_client-3.2.4-1.el6.$(uname -m).rpm
	
Configure the logs

	The 7 arguments of tcpweblog_client.bin are:

	- remote_ip_address: the IP address of the listening remote log server;
	- remote_port: the TCP	port of the listening remote log server;
	- local_cache_file: the local cache file to temporarily store the logs when the TCP connection is not available;
	- logname: the last part of the log file name (i.e.: access.log);
	- cluster_number: the cluster number;
	- client_ip: the client (local) IP address;
	- client_hostname: the client (local) hostname.

EXAMPLES:

	APACHE (configuration per virtual host)
		CustomLog \"| /usr/bin/tcpweblog_client.bin 10.0.3.15 9940 /var/log/tcpweblog_cache.log access.log 1 10.0.2.15 xhost\" combined
		ErrorLog \"| /usr/bin/tcpweblog_client.bin 10.0.3.15 9940 /var/log/tcpweblog_cache.log error.log 1 10.0.2.15 xhost\"

	APACHE SSL (configuration per virtual host)
		CustomLog \"| /usr/bin/tcpweblog_client.bin 10.0.3.15 9940 /var/log/tcpweblog_cache.log ssl.access.log 1 10.0.2.15 xhost\" combined
		ErrorLog \"| /usr/bin/tcpweblog_client.bin 10.0.3.15 9940 /var/log/tcpweblog_cache.log ssl.error.log 1 10.0.2.15 xhost\"

	APACHE (general CustomLog)
		# you must prefix the log format with \"%A %V\", for example:
		LogFormat \"%A %V %{X-Forwarded-For}i %l %u %t \\\"%r\\\" %>s %b \\\"%{Referer}i\\\" \\\"%{User-Agent}i\\\"\" common
		CustomLog \"| /usr/bin/tcpweblog_client.bin 10.0.3.15 9940 /var/log/tcpweblog_cache.log access.log 1 - -\" common

	VARNISHNCSA
		You must prefix the log format with \"%A %V\", for example:
		varnishncsa -F \"%A %V %{X-Forwarded-For}i %l %u %t \\\"%r\\\" %>s %b \\\"%{Referer}i\\\" \\\"%{User-Agent}i\\\"\" | /usr/bin/tcpweblog_client.bin 10.0.3.15 9940 /var/log/tcpweblog_cache.log varnish.log 1 - -

	USING A NAMED PIPE TO FORWARD LOGS (pure-ftpd example)
		- Create a named pipe:
			mkfifo /var/log/pureftpd.log -Z system_u:object_r:var_log_t:s0
		- Create a /root/ftplogpipe.sh file:
			#!/bin/sh
			(setsid bash -c '(while cat /var/log/pureftpd.log; do : Nothing; done | /usr/bin/tcpweblog_client.bin 10.0.3.15 9940 /var/log/tcpweblog_ftp_cache.log ftp.log 1 - -) & disown %%') </dev/null >&/dev/null &
		- Add the following line to the end of /etc/rc.d/rc.local:
			root/ftplogpipe.sh"
		- Edit /etc/pure-ftpd/pure-ftpd.conf:
			Altlog clf:/var/log/pureftpd.log

	USING RSYSLOG AS A CLIENT FOR TCPWebLog-Server (pure-ftpd example)
		-  create a /etc/rsyslog.d/ftp.conf
				# Rsyslog config file to forward pure-ftp logs to TCPWebLog-Server
				$ModLoad imfile
				# read the input ftp log file
				$InputFileName /var/log/pureftpd.log
				$InputFileTag :ftplog: # mark rows with a custom tag
				$InputFileStateFile stat-ftp
				$InputFileSeverity notice
				$InputFileFacility ftp
				$InputRunFileMonitor
				# define a message template compatible with TCPWebLog-Server
				# @@logname<TAB>cluster<TAB>clientip<TAB>clienthost<TAB>rawbuf
				$template tcpweblog_format,"@@ftp.log	1	-	-	%msg%\n"
				# configure TCP connection and local cache
				$WorkDirectory /var/lib/rsyslog # where to place spool files
				$ActionQueueFileName FTPfwdRule # unique name prefix for spool files
				$ActionQueueMaxDiskSpace 1g   # 1gb space limit (use as much as possible)
				$ActionQueueSaveOnShutdown on # save messages to disk on shutdown
				$ActionQueueType LinkedList   # run asynchronously
				$ActionResumeRetryCount -1    # infinite retries if host is down
				# send data to TCPWebLog-Server via TCP
				:syslogtag, isequal, ":ftplog:" @@10.0.3.15:9940;tcpweblog_format
		-  restart rsyslog and pure-ftpd:
				service rsyslog restart
				service pure-ftpd restart
		
		
	If using SELinux, run the following command to allow the Apache daemon to open network connections:
		setsebool -P httpd_can_network_connect=1

On the above examples we are simply "piping" the log data to our program.
