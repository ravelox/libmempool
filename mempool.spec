# Copyright (c) 2024 Dave Kelly <github@ravelox.co.uk>
# Licensed under the GNU General Public License v3.0 or later.

Name:           mempool
Version:        0.0.3
Release:        1%{?dist}
Summary:        Minimal memory pool allocator

License:        GPL-3.0-or-later
URL:            https://example.com/mempool
Packager:       %{?_maintainer}
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc, make
BuildRequires:  pkgconfig

%description
Minimal memory pool allocator built as a shared library with a small test driver.

%prep
%setup -q

%build
make all

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install
install -D -m 0644 libmempool.pc %{buildroot}%{_libdir}/pkgconfig/libmempool.pc
mkdir -p %{buildroot}%{_includedir}
mkdir -p %{buildroot}%{_libdir}
install -m 0644 libmempool.h %{buildroot}%{_includedir}/libmempool.h
install -m 0755 libmempool.so %{buildroot}%{_libdir}/libmempool.so

%files
%{_includedir}/libmempool.h
%{_libdir}/libmempool.so
%{_libdir}/pkgconfig/libmempool.pc

%changelog
* Thu Jan 01 1970 Mempool Maintainer <maintainer@example.com> 0.0.3-1
- Initial RPM packaging
