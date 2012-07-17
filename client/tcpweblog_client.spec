%define release 1

Name:           tcpweblog_client
Version:        1.9.0
Release:        %{release}%{?dist}
Summary:        TCPWebLog-Client program accepts a text input and sends each line to a remote server via TCP

Group:          Applications/System
License:        AGPLv3
URL:            https://github.com/fubralimited/TCPWebLog
Source0:        %{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  elfutils-devel

%description
TCPWebLog-Client is a tool to send Apache-style log data to a remote log server
via TCP.

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
%{_bindir}/*
%{_mandir}/man8/*

%changelog
* Tue May 08 2012 Nicola Asuni
- First version for RPM packaging.
