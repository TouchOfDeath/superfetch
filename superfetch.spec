Name:           superfetch
Version:        1.0.0
Release:        1%{?dist}
Summary:        A blazing fast, sub-millisecond system information tool for Linux

License:        MIT
URL:            https://github.com/TouchOfDeath/superfetch
Source0:        https://github.com/TouchOfDeath/superfetch/archive/refs/tags/v%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make

%description
Superfetch is an ultra-optimized system information fetch tool written in raw C.
It bypasses standard libraries like glibc to read directly from the Linux kernel
using POSIX syscalls, achieving execution times under 1 millisecond.

%prep
%autosetup

%build
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
# We use DESTDIR and PREFIX=/usr
make install DESTDIR=$RPM_BUILD_ROOT PREFIX=/usr

%files
%license LICENSE
%{_bindir}/superfetch
%{_mandir}/man1/superfetch.1*
%config(noreplace) /etc/bash_completion.d/superfetch

%changelog
* Wed Jun 10 2026 TouchOfDeath <a.sahoo.work@gmail.com> - 1.0.0-1
- Initial RPM release
