#Parameter
%define ker_ver %(uname -r | sed -e 's/.aarch64//g' -e 's/.arm64//g' -e 's/.amd64//g' -e 's/.x86_64//g' | sed 's/-/_/g')
%define ver %(uname -r)
%define global_ver %(cat %{_sourcedir}/GLOBAL_VERSION_NEW | grep driver | awk -F ':' '{print $2}')
%{!?KVERSION: %define KVERSION %(uname -r)}
%define __strip /bin/true
%define arch %(uname -m)

Name:           hibifur5
Version:        %{global_ver}
Release:        %{ker_ver}
Group:          System/Kernel
Vendor:         Huawei Technologies Co., Ltd.
License:        GPLv2
Summary:        Huawei hibifur5 kernel module
URL:            http://support.huawei.com/enterprise
ExclusiveArch:  %{?arch}

#Sources
Source0:    %{name}.ko
Source1:    hibifur5-modules.conf

%description
This package contains the Huawei hibifur5.ko kernel module of Network Interface Card.

%prep
echo "override %{name} %(echo %(uname -r) | awk -F "-" '{print $1}')-* weak-updates/%{name}" > %{name}.conf

%build
#we use binary ko to make rpm package, so needn't build

%install
%{__rm} -rf %{buildroot}
%{__install} -d %{buildroot}/lib/modules/%{KVERSION}/extra/%{name}
%{__install} %{SOURCE0} %{buildroot}/lib/modules/%{KVERSION}/extra/%{name}/
find %{buildroot}/lib/modules -iname 'modules.*' -exec rm {} \;
%{__install} -d %{buildroot}%{_sysconfdir}/depmod.d/
%{__install} %{name}.conf %{buildroot}%{_sysconfdir}/depmod.d/
%{__install} -d %{buildroot}%{_sysconfdir}/modules-load.d/
%{__install} %{SOURCE1} %{buildroot}%{_sysconfdir}/modules-load.d/
# Set the module(s) to be executable, so that they will be stripped when packaged.
find %{buildroot} -type f -name \*.ko -exec chmod u+x {} \;
# Remove the unrequired files.
%{__rm} -f %{buildroot}/lib/modules/%{KVERSION}/modules.*

%clean
%{__rm} -rf %{buildroot}

%description   -n %{name}
This package provides the hibifur.ko kernel module from Huawei Technologies Co., Ltd. built
for the Linux kernel %{ker_ver} for the %{_target_cpu} family of processors.

%files
%defattr (644,root,root,755)
/lib/modules/%(uname -r)/extra/%{name}/
%attr(640,root,root) /etc/depmod.d/%{name}.conf
%attr(640,root,root) /etc/modules-load.d/hibifur5-modules.conf

%pre

%post
if [[ %{KVERSION} != $(uname -r) ]]; then
	if [ ! -d "/lib/modules/$(uname -r)/extra" ];then
		mkdir /lib/modules/$(uname -r)/extra
	fi
	cp -rf /lib/modules/%{KVERSION}/extra/%{name} /lib/modules/$(uname -r)/extra/
fi

/sbin/depmod $(uname -r) > /dev/null 2>&1

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

%changelog
* Tue Nov 19 2024 Huawei HIBIFUR5 Driver Develop Team - %{version}
- Initial %(uname -r) build of the package.