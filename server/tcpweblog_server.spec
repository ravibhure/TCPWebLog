%define release 1

Name:           tcpweblog_server
Version:        3.1.0
Release:        %{release}%{?dist}
Summary:        TCPWebLog-Server collects logs data via TCP from TCPWebLog-Client

Group:          Applications/System
License:        AGPLv3
URL:            https://github.com/fubralimited/TCPWebLog
Source0:        %{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  elfutils-devel

Requires(preun): chkconfig
Requires(preun): initscripts
Requires(postun): initscripts

%description
The TCPWebLog-Server is a program to collect and process log data sent by
TCPWebLog-Client instances on remote computers. This program listen on a TCP
port for incoming log data and store them on a SQLite table. An shell script
(tcpweblog_dbagg.sh) is executed periodically by a cron job to aggregate data
on another SQLite table.

%prep
%setup -q -c %{name}-%{version}

%build
make

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT
make clean

%files
%defattr(-,root,root,-)
%doc README LICENSE
%{_bindir}/tcpweblog_server.bin
%config(noreplace) %{_sysconfdir}/tcpweblog_server.conf
%{_initrddir}/tcpweblog_server
%{_mandir}/man8/tcpweblog_server.8.gz

%preun
if [ $1 -eq 0 ] ; then
	# uninstall: stop service
	/sbin/service tcpweblog_server stop >/dev/null 2>&1
	/sbin/chkconfig --del tcpweblog_server
fi

%postun
if [ $1 -eq 1 ] ; then
	# upgrade: restart service if was running
	/sbin/service tcpweblog_server condrestart >/dev/null 2>&1 || :
fi

%changelog
* Thu Aug 21 2012 Nicola Asuni
- Added %config(noreplace)
- Added preun and postun sections

* Mon May 28 2012 Nicola Asuni
- First version.

