%define release 1

Name:           tcpweblog_server
Version:        1.6.0
Release:        %{release}%{?dist}
Summary:        TCPWebLog-Server collects logs data via TCP from TCPWebLog-Client

Group:          Applications/System
License:        AGPLv3
URL:            https://github.com/fubralimited/TCPWebLog
Source0:        %{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  elfutils-devel

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
%{_sysconfdir}/tcpweblog_server.conf
%{_initrddir}/tcpweblog_server
%{_mandir}/man8/tcpweblog_server.8.gz

%changelog
* Mon May 28 2012 Nicola Asuni
- First version.

