%global pkg hisdk5
%global kernel kernel version
%define pkg_version 100.0.1.202
%define anolis_release 1

Name:        kmod-%{pkg}
Version:     %(echo %{kernel} | sed -E 's/-/~/g; s/\.(an|al)[0-9]+$//g')
Release:     %{pkg_version}~%{anolis_release}%{?dist}
Summary:     Huawei HISDK5 Kernel Module
License:     GPLv2
URL:         http://support.huawei.com/enterprise
Source0:     kmod-%{pkg}-%{pkg_version}.tar.gz
Epoch:       1

%define debug_package %{nil}
%define _build_id_links none

Requires:            kernel >= %{kernel}
Requires:            kmod
Requires:            dracut
Requires(posttrans): %{_sbindir}/depmod
Requires(postun):    %{_sbindir}/depmod
Requires(posttrans): %{_sbindir}/weak-modules
Requires(postun):    %{_sbindir}/weak-modules
Requires(posttrans): %{_bindir}/sort
Requires(postun):    %{_bindir}/sort

BuildRequires:    kernel-devel   = %{kernel}
BuildRequires:    kernel-headers = %{kernel}
BuildRequires:    elfutils-libelf-devel
BuildRequires:    gcc
BuildRequires:    kmod
BuildRequires:    make
BuildRequires:    system-rpm-config

Provides: kmod-%{pkg}-%{kernel}.%{_arch} = %{version}-%{release}
Obsoletes: kmod-%{pkg}-%{kernel}.%{_arch} < %{version}-%{release}

%description
RPM Package for Huawei HISDK5 Driver

%prep
%setup -q -n kmod-%{pkg}-%{pkg_version}

%build
export HI1823_BUILD_DIR=%{_builddir}/kmod-%{pkg}-%{pkg_version}/build
export HI1823_TRUNK_DIR=%{_builddir}/kmod-%{pkg}-%{pkg_version}
export KERNEL_VER=%{kernel}
export TOP_DIR=%{_builddir}/kmod-%{pkg}-%{pkg_version}

bash ./build.sh -d sdk

%install
mkdir -p %{buildroot}/lib/modules/%{kernel}.%{_arch}/extra/%{pkg}
%{__install} -D -t %{buildroot}/lib/modules/%{kernel}.%{_arch}/extra/%{pkg} output/binary/hisdk5.ko

find %{buildroot}/lib/modules -type f -name \*.ko -exec chmod u+x \{\} \+

%{__install} -d %{buildroot}/%{_sysconfdir}/depmod.d/
for kmod in $(find %{buildroot}/lib/modules/%{kernel}.%{_arch}/extra -type f -name \*.ko -printf "%%P\n" | sort)
do
    echo "override $(basename $kmod .ko) * weak-updates/$(dirname $kmod)" >> %{buildroot}/%{_sysconfdir}/depmod.d/%{pkg}.conf
    echo "override $(basename $kmod .ko) * extra/$(dirname $kmod)" >> %{buildroot}/%{_sysconfdir}/depmod.d/%{pkg}.conf
done

%{__install} -d %{buildroot}/%{_sysconfdir}/modules-load.d/
echo "hisdk5" > %{buildroot}/%{_sysconfdir}/modules-load.d/hisdk5-modules.conf

%clean
%{__rm} -rf %{buildroot}

%post
depmod -a > /dev/null 2>&1

%preun
echo "/lib/modules/%{kernel}.%{_arch}/extra/%{pkg}/hisdk5.ko" >> /var/run/rpm-%{pkg}-modules.list

%postun
depmod -a > /dev/null 2>&1

if [ -x "/usr/sbin/weak-modules" ]; then
    modules=( $(cat /var/run/rpm-%{pkg}-modules.list) )
    printf '%s\n' "${modules[@]}" | /usr/sbin/weak-modules --no-initramfs --remove-modules
fi
rm /var/run/rpm-%{pkg}-modules.list

%files
%defattr(644,root,root,755)
/lib/modules/%{kernel}.%{_arch}/extra/%{pkg}/*.ko
/etc/modules-load.d/hisdk5-modules.conf
%config(noreplace) %{_sysconfdir}/depmod.d/%{pkg}.conf

%changelog
* Tue Jun 16 2026 Huawei HISDK5 Driver Develop Team - %{version}
- Initial build of the package.
