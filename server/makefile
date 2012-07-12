
#==============================================================================+
# File name   : makefile
# Begin       : 2012-04-16
# Last Update : 2012-05-28
# Version     : 1.6.0
#
# Website     : https://github.com/fubralimited/TCPWebLog
#
# Description : Makefile to build TCPWebLog-Server.
#
# Author: Nicola Asuni
#
# (c) Copyright:
#               Fubra Limited
#               Manor Coach House
#               Church Hill
#               Aldershot
#               Hampshire
#               GU12 4RQ
#				UK
#               http://www.fubra.com
#               support@fubra.com
#
# License:
#    Copyright (C) 2012-2012 Fubra Limited
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU Affero General Public License as
#    published by the Free Software Foundation, either version 3 of the
#    License, or (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Affero General Public License for more details.
#
#    You should have received a copy of the GNU Affero General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#    See LICENSE.TXT file for more information.
#==============================================================================+

# --- MACROS ---
CC=gcc
CFLAGS=-O3 -g -pipe -Wp,-D_THREAD_SAFE -D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector -fno-strict-aliasing -fwrapv -fPIC --param=ssp-buffer-size=4 -D_GNU_SOURCE
CLIBS=-lpthread

# --- PATHS ---
PATHINSTALL=$(DESTDIR)/usr/bin/
PATHCONFIG=$(DESTDIR)/etc/
PATHMAN=$(DESTDIR)/usr/share/man/man8/
ifeq ($(wildcard /etc/rc.d/init.d),)
	PATHINIT=$(DESTDIR)/etc/init.d/
else
	PATHINIT=$(DESTDIR)/etc/rc.d/init.d/
endif

# --- TARGETS ---

# COMPILE THE TCP RECEIVER
all:
	$(CC) $(CFLAGS) -o tcpweblog_server.bin tcpweblog_server.c $(CLIBS)

# INSTALL ALL EXECUTABLES AND SQL FILES
install:
	mkdir -p $(PATHCONFIG)
	mkdir -p $(PATHINSTALL)
	mkdir -p $(PATHINIT)
	mkdir -p $(PATHMAN)
	install -m 0644 tcpweblog_server.conf $(PATHCONFIG)
	install -m 0755 tcpweblog_server.bin $(PATHINSTALL)
	install -m 0755 tcpweblog_server $(PATHINIT)
	install -m 0644 tcpweblog_server.8 $(PATHMAN)
	gzip -f $(PATHMAN)tcpweblog_server.8

# REMOVE ALL INSTALLED FILES
uninstall:
	rm -f $(PATHMAN)tcpweblog_server.8.gz
	rm -f $(PATHINIT)tcpweblog_server
	rm -f $(PATHINSTALL)tcpweblog_server.bin
	rm -f $(PATHCONFIG)tcpweblog_server.conf

# REMOVE COMPILED FILE
clean:
	rm -f tcpweblog_server.bin

#==============================================================================+
# END OF FILE
#==============================================================================+
