#Parameter
%define ker_ver %(uname -r | sed -e 's/.aarch64//g' -e 's/.x86_64//g' -e 's/.arm64//g' -e 's/.amd64//g' | sed 's/-/_/g')
%define arch %(uname -m)
%define global_ver %(cat %{_sourcedir}/GLOBAL_VERSION_NEW | grep driver | awk -F ':' '{print $2}')
%{!?KVERSION: %define KVERSION %(uname -r)}
%define __strip /bin/true
%define _build_id_links none

Name:           hisdk5
Version:        %{global_ver}
Release:        %{ker_ver}
Group:          System/Kernel
Vendor:         Huawei Technologies Co., Ltd.
License:        GPLv2
Summary:        Huawei hisdk5 kernel module
URL:            http://support.huawei.com/enterprise
ExclusiveArch:  %{?arch}

#Sources
Source0:    %{name}.ko
Source1:    hiudk5.ko
Source2:    hisdk5-modules.conf
Source3:    hiudk5-modules.conf

%description
This package contains the Huawei hisdk5.ko kernel module of Network Interface Card.

%prep
echo "override %{name} %(echo %(uname -r) | awk -F "-" '{print $1}')-* weak-updates/%{name}" > %{name}.conf

%build
#we use binary ko to make rpm package, so needn't build

%install
%{__rm} -rf %{buildroot}
%{__install} -d %{buildroot}/lib/modules/%{KVERSION}/extra/%{name}
%{__install} %{SOURCE0} %{buildroot}/lib/modules/%{KVERSION}/extra/%{name}/
%{__install} %{SOURCE1} %{buildroot}/lib/modules/%{KVERSION}/extra/%{name}/
find %{buildroot}/lib/modules -iname 'modules.*' -exec rm {} \;
%{__install} -d %{buildroot}%{_sysconfdir}/depmod.d/
%{__install} %{name}.conf %{buildroot}%{_sysconfdir}/depmod.d/
%{__install} -d %{buildroot}%{_sysconfdir}/modules-load.d/
%{__install} %{SOURCE2} %{buildroot}%{_sysconfdir}/modules-load.d/
%{__install} -d %{buildroot}%{_sysconfdir}/modules-load.d/
%{__install} %{SOURCE3} %{buildroot}%{_sysconfdir}/modules-load.d/
# Set the module(s) to be executable, so that they will be stripped when packaged.
find %{buildroot} -type f -name \*.ko -exec chmod u+x {} \;
# Remove the unrequired files.
%{__rm} -f %{buildroot}/lib/modules/%{KVERSION}/modules.*

%clean
%{__rm} -rf %{buildroot}

%files
%defattr (644,root,root,755)
/lib/modules/%(uname -r)/extra/%{name}/
%attr(640,root,root) /etc/depmod.d/%{name}.conf
%attr(640,root,root) /etc/modules-load.d/hisdk5-modules.conf
%attr(640,root,root) /etc/modules-load.d/hiudk5-modules.conf

%pre

%post
if [[ %{KVERSION} != $(uname -r) ]]; then
	if [ ! -d "/lib/modules/$(uname -r)/extra" ];then
		mkdir /lib/modules/$(uname -r)/extra
	fi
	cp -rf /lib/modules/%{KVERSION}/extra/%{name} /lib/modules/$(uname -r)/extra/
fi

if [ -f /etc/sdi_nanoos_version ]; then
	echo "options hisdk5 use_hinic5_vram=1" > /etc/modprobe.d/hisdk-modules.conf
fi

/sbin/depmod $(uname -r) > /dev/null 2>&1
dracut --add-drivers "hiudk5 hisdk5" --force /boot/initramfs-$(uname -r).tmp > /dev/null 2>&1
\mv /boot/initramfs-$(uname -r).tmp /boot/initramfs-$(uname -r).img > /dev/null 2>&1

%preun

%postun
if [ "$1" -eq 0 ]; then
	if [[ %{KVERSION} != $(uname -r) ]]; then
		rm -rf /lib/modules/$(uname -r)/extra/%{name} > /dev/null 2>&1
	fi
	rm -rf /lib/modules/%{KVERSION}/extra/%{name} > /dev/null 2>&1
	rm -rf /etc/dracut.conf.d/%{name}.conf > /dev/null 2>&1
fi

/sbin/depmod $(uname -r) > /dev/null 2>&1
dracut --add-drivers "hiudk5 hisdk5" --force /boot/initramfs-$(uname -r).tmp > /dev/null 2>&1
\mv /boot/initramfs-$(uname -r).tmp /boot/initramfs-$(uname -r).img > /dev/null 2>&1

%changelog
* Sat Jan 22 2022 Huawei HISDK5 Driver Develop Team - %{version}
- Initial %(uname -r)%{dist} build of the package.