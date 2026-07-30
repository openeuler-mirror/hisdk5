#Parameter  纯内核版本命名，无任何固定版本号
%define ker_ver %(uname -r | sed -e 's/.aarch64//g' -e 's/.x86_64//g' -e 's/.amd64//g' -e 's/.arm64//g' | sed 's/-/_/g')
%{!?KVERSION: %define KVERSION %(uname -r)}
%define __strip /bin/true
%define _build_id_links none
%define global_ver %(cat %{_sourcedir}/GLOBAL_VERSION_NEW | grep driver | awk -F ':' '{print $2}')

%global __requires_exclude libibverbs\\.so\\.1.*
Name:           hihtr5
Version:        %{global_ver}
Release:        %{ker_ver}
Group:          System/Kernel
Vendor:         Huawei Technologies Co., Ltd.
License:        Proprietary
Summary:        Hihtr RoCE User-Space Driver and Kernel Module

Source0:        hihtr5.ko
Source1:        libhihtr.so
Source2:        hihtr5
Source3:        hihtr5-modules.conf
Source4:        libhihtr-rdmav*.so

%description
Hihtr5 is a custom RoCE driver.
Includes kernel module, user library, and libibverbs plugin.

%prep
echo "override %{name} %(echo %(uname -r) | awk -F "-" '{print $1}')-* weak-updates/%{name}" > %{name}.conf

%build
#we use binary files to make rpm package, so needn't build

%install
%{__rm} -rf %{buildroot}

# 创建目录
%{__install} -d %{buildroot}/usr/lib64/
%{__install} -d %{buildroot}/usr/lib64/libibverbs/
%{__install} -d %{buildroot}/etc/libibverbs.d/
%{__install} -d %{buildroot}/lib/modules/%{KVERSION}/extra/hihtr/
%{__install} -d %{buildroot}/usr/local/bin/
%{__install} -d %{buildroot}%{_sysconfdir}/modules-load.d/

echo "DEBUG: SOURCE1 = [%{SOURCE1}]" >&2
echo "DEBUG: buildroot = [%{buildroot}]" >&2

# 安装用户库
%{__install} -m 755 %{SOURCE1} %{buildroot}/usr/lib64/libhihtr.so
ln -sf libhihtr.so %{buildroot}/usr/lib64/libhihtr.so.1

# 安装 libibverbs 插件
%{__install} -m 755 %{SOURCE1} %{buildroot}/usr/lib64/libibverbs/libhihtr.so
%{__install} -m 755 %{SOURCE4} %{buildroot}/usr/lib64/

# 注册驱动
echo "driver hihtr" > %{buildroot}/etc/libibverbs.d/hihtr5.driver
chmod 644 %{buildroot}/etc/libibverbs.d/hihtr5.driver

# 安装内核模块
%{__install} -m 644 %{SOURCE0} %{buildroot}/lib/modules/%{KVERSION}/extra/hihtr/hihtr5.ko
find %{buildroot}/lib/modules -iname 'modules.*' -exec rm {} \;

# 安装用户工具
%{__install} -m 755 %{SOURCE2} %{buildroot}/usr/local/bin/hihtr5

# 安装modules-load.d配置文件
%{__install} -m 644 %{SOURCE3} %{buildroot}%{_sysconfdir}/modules-load.d/hihtr5-modules.conf

# 创建dracut配置文件
%{__install} -d %{buildroot}/etc/dracut.conf.d/
touch %{buildroot}/etc/dracut.conf.d/hihtr5.conf
chmod 644 %{buildroot}/etc/dracut.conf.d/hihtr5.conf

%clean
%{__rm} -rf %{buildroot}

%files
%defattr (550,root,root,755)
%attr(755,root,root) /usr/lib64/libhihtr.so
%attr(755,root,root) /usr/lib64/libhihtr.so.1
%attr(755,root,root) /usr/lib64/libibverbs/libhihtr.so
%attr(755,root,root) /usr/lib64/libhihtr-rdmav*.so
%attr(644,root,root) /etc/libibverbs.d/hihtr5.driver
%attr(644,root,root) /lib/modules/%{KVERSION}/extra/hihtr/hihtr5.ko
%attr(755,root,root) /usr/local/bin/hihtr5
%attr(644,root,root) %{_sysconfdir}/modules-load.d/hihtr5-modules.conf
%attr(644,root,root) /etc/dracut.conf.d/hihtr5.conf

%pre

%post
if [[ %{KVERSION} != $(uname -r) ]]; then
	if [ ! -d "/lib/modules/$(uname -r)/extra" ];then
		mkdir /lib/modules/$(uname -r)/extra
	fi
	cp -rf /lib/modules/%{KVERSION}/extra/hihtr /lib/modules/$(uname -r)/extra/
fi

/sbin/depmod $(uname -r) 2>/dev/null || :
ldconfig

# hihtr5 专属 dracut 配置
cat > /etc/dracut.conf.d/hihtr5.conf << EOF
add_drivers+=" ib_core ib_cm ib_umad ib_uverbs rdma_cm iw_cm rdma_ucm hihtr5 "
install_items+=" /etc/modules-load.d/hihtr5-modules.conf "
EOF
chmod 644 /etc/dracut.conf.d/hihtr5.conf

dracut --add-drivers "ib_core ib_cm ib_umad ib_uverbs rdma_cm iw_cm rdma_ucm hihtr5" --force /boot/initramfs-$(uname -r).tmp > /dev/null 2>&1
\mv /boot/initramfs-$(uname -r).tmp /boot/initramfs-$(uname -r).img > /dev/null 2>&1
echo "/lib/modules/$(uname -r)/extra/hihtr/hihtr5.ko" | /sbin/weak-modules --add-modules --no-initramfs

lsinitrd /boot/initramfs-$(uname -r).img | grep "lib/modules/$(uname -r)/extra/hihtr/hihtr5.ko" > /dev/null 2>&1
if [[ $? -ne 0 ]]; then
    echo "ERROR: hihtr5 module insert initrd failed in post"
fi

%preun
if [ $1 -eq 0 ]; then
    depmod -a %{KVERSION} 2>/dev/null || :
    ldconfig
fi

%postun
CURRENT_KERNEL=$(uname -r)
if [ "$1" -eq 0 ]; then
    rm -rf /etc/dracut.conf.d/hihtr5.conf > /dev/null 2>&1
    if [[ %{KVERSION} != $CURRENT_KERNEL ]]; then
        rm -rf /lib/modules/$CURRENT_KERNEL/extra/hihtr > /dev/null 2>&1
    fi
    rm -rf /lib/modules/%{KVERSION}/extra/hihtr > /dev/null 2>&1

    /sbin/depmod $CURRENT_KERNEL > /dev/null 2>&1
    dracut --force > /dev/null 2>&1
fi

if [ "$1" -eq 1 ]; then
    /sbin/depmod $CURRENT_KERNEL > /dev/null 2>&1
    dracut --add-drivers "ib_core ib_cm ib_umad ib_uverbs rdma_cm iw_cm rdma_ucm hihtr5" --force /boot/initramfs-$CURRENT_KERNEL.tmp > /dev/null 2>&1
    \mv /boot/initramfs-$CURRENT_KERNEL.tmp /boot/initramfs-$CURRENT_KERNEL.img > /dev/null 2>&1
    echo "/lib/modules/$CURRENT_KERNEL/extra/hihtr/hihtr5.ko" | /sbin/weak-modules --add-modules --no-initramfs

    lsinitrd /boot/initramfs-$CURRENT_KERNEL.img | grep "lib/modules/$CURRENT_KERNEL/extra/hihtr/hihtr5.ko" > /dev/null 2>&1
    if [[ $? -ne 0 ]]; then
        echo "ERROR: hihtr5 module insert initrd failed in postun"
    fi
fi

%changelog
* Tue Feb 03 2026 hihtr5 Driver Team
- Align with hinic5 naming rule, only kernel version, no fixed version number
