TCPWebLog - README
====================

+ Name: TCPWebLog

+ Version: 1.0.0

+ Release date: 2012-06-06

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

The TCPWebLog system is designed to collect and process Apache-style logs.

This project is composed by 2 sub-projects:


## TCPWebLog-Client ##

The TCPWebLog-Client program accepts a log text input and sends each line to a 
remote server via TCP. It can be used to pipe Apache, Varnish, PHP and FTP logs.


## TCPWebLog-Server ##

The TCPWebLog-Server program listen on a TCP port for incoming log data 
(from TCPWebLog-Client) and process them.


## GENERAL USAGE SCHEMA ##
	
	(TCPWebLog-Client) - - - - - - - - - - - -> (TCPWebLog-Server)



HOW-TO CREATE TCPWebLog RPMs
------------------------------

This is a short hands-on tutorial on creating RPM files for the TCPWebLog project.
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

Copy the SPEC files and source files to rpmbuild dir (replace the version number with the correct value):
	
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

	# rpm -i tcpweblog_server-1.0.0-1.el6.$(uname -m).rpm 
	
Configure the TCPWebLog-Server

	# nano /etc/tcpweblog_server.conf

Start the service

	# /etc/init.d/tcpweblog_server start

Start the service at boot:

	# chkconfig tcpweblog_server on

INSTALL SERVERUSAGE CLIENT:
---------------------------

As root install the SystemTap runtime and TCPWebLog-Client RPM files:

	# rpm -i tcpweblog_client-1.0.0-1.el6.$(uname -m).rpm
	
Configure the logs

	The arguments of tcpweblog_client.bin are:

	 - the IP address of the listening remote log server;
	 - the TCP	port of the listening remote log server;
	 - the local cache file to temporarily store the logs when the TCP
	   connection is not available;
	 - the log type:
		1 : Apache Access Log;
		2 : Apache Error Log;
		3 : Apache SSL Access Log;
		4 : Apache SSL Error Log;
		5 : Varnish NCSA Log (you must prefix the log format with: "%h %V");
		6 : FTP log;
		7 : PHP error log;
	 - the cluster number;
	 - the client (local) IP address;
	 - the client (local) hostname.

Examples

	APACHE
		CustomLog "| /usr/bin/tcpweblog_client.bin 10.0.3.15 9940 /var/log/tcpweblog_cache.log 1 1 10.0.2.15 xhost" combined
		ErrorLog "| /usr/bin/tcpweblog_client.bin 10.0.3.15 9940 /var/log/tcpweblog_cache.log 2 1 10.0.2.15 xhost"

	APACHE SSL
		CustomLog "| /usr/bin/tcpweblog_client.bin 10.0.3.15 9940 /var/log/tcpweblog_cache.log 3 1 10.0.2.15 xhost" combined
		ErrorLog "| /usr/bin/tcpweblog_client.bin 10.0.3.15 9940 /var/log/tcpweblog_cache.log 4 1 10.0.2.15 xhost"

	VARNISHNCSA
		You must prefix the log format with "%h %V", for example:
		varnishncsa -F "%h %V %{X-Forwarded-For}i %l %u %t \"%r\" %>s %b \"%{Referer}i\" \"%{User-Agent}i\"" | /usr/bin/tcpweblog_client.bin 10.0.3.15 9940 /var/log/tcpweblog_cache.log 5 1 - -


