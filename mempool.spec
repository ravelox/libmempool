Name:           mempool
Version:        0.0.3
Release:        1%{?dist}
Summary:        Minimal memory pool allocator

License:        MIT
URL:            https://example.com/mempool
Packager:       %{?_maintainer}
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc, make

%description
Minimal memory pool allocator built as a shared library with a small test driver.

%prep
%setup -q

%build
make all

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install
mkdir -p %{buildroot}%{_includedir}
mkdir -p %{buildroot}%{_libdir}
install -m 0644 libmempool.h %{buildroot}%{_includedir}/libmempool.h
install -m 0755 libmempool.so %{buildroot}%{_libdir}/libmempool.so

%files
%{_includedir}/libmempool.h
%{_libdir}/libmempool.so

%changelog
* Thu Jan 01 1970 Mempool Maintainer <maintainer@example.com> 0.0.3-1
- Initial RPM packaging
