#!/bin/bash
#
#   @Ephemeral
#
#   USAGE:
#
#   sudo bash create-debian-rpi-package.sh create PACKAGENAME
#   sudo dpkg -i PACKAGENAME.deb
#   sudo bash create-debian-rpi-package.sh clean PACKAGENAME
#
# sudo tail -f /var/log/dpkg.log


ARCHITECTURE="armhf" # RPi3B

KERNEL_RELEASE=$(uname -r)
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
VERSION="0.1-0"

function CREATE() {

    PROJECT=$(mktemp -d -p /tmp/ rpideb.XXXXXXXXXXX)
    if [ ! -d "${PROJECT}/package/DEBIAN" ];then
        mkdir -p "${PROJECT}/package/DEBIAN"
    else
        echo "A temporary directory ${PROJECT}/package already exists !"
        echo "You can use 'sudo bash create-debian-rpi-package.sh clean'"
        exit 0
    fi;
    cd "${PROJECT}/package"

    if [ ! -e ${DIR}/../uwbhal.dtbo ];then
        echo "creating device-tree overlay"
        dtc -W no-unit_address_vs_reg -@ -I dts -O dtb -o ${DIR}/../uwbhal.dtbo ${DIR}/../rpi_uwbhal_overlay.dts
    fi


    echo "creating embedded files..."
    mkdir -p ./boot/overlays
    mkdir -p ./lib/modules/${KERNEL_RELEASE}/kernel/net/uwb/
    cp ${DIR}/../uwbhal.dtbo ./boot/overlays/
    find ${DIR}/../../../../ -type f -name "*.ko" |while read KONAME;do
        cp ${KONAME} ./lib/modules/${KERNEL_RELEASE}/kernel/net/uwb/
    done
    #mkdir -p ./usr/bin/
    #echo "ip a" > ./usr/bin/mytool.sh

    cd "DEBIAN"

    echo "creating pre install file, use this for saving KERNEL..."
    cat << EOF > preinst
#!/bin/sh
echo "pre install job..."
exit 0
EOF
    chmod 0755 preinst

    echo "creating control file..."
    cat << EOF > control
Package: ${PACKAGENAME}-${KERNEL_RELEASE}
Version: ${VERSION}
Architecture: ${ARCHITECTURE}
Maintainer: Niklas Casaril <${PACKAGENAME}@debian.org>
Description: UWB kernel modules
EOF

    echo "creating post install file..."
    cat << EOF > postinst
#!/bin/sh
echo "Running 'depmod -a' ..."
depmod -a
if [ -e /DietPi/config.txt ];then
    if ! grep -q "dtoverlay=uwbhal" /DietPi/config.txt;then
        echo "Adding dtoverlay=uwbhal to /DietPi/config.txt"
        echo "NOTE: Reboot needed to activate"
        echo "dtoverlay=uwbhal" >> /DietPi/config.txt;
    fi
fi
exit 0
EOF
    chmod 0755 postinst

    echo "creating post rm file..."
    cat << EOF > postrm
#!/bin/sh
echo "Running 'depmod -a' ..."
depmod -a
if [ -e /DietPi/config.txt ];then
    if grep -q "dtoverlay=uwbhal" /DietPi/config.txt;then
        echo "Removing dtoverlay=uwbhal from /DietPi/config.txt"
        sed -i '/^dtoverlay.uwbhal/d' /DietPi/config.txt
    fi
fi
exit 0
EOF
    chmod 0755 postrm

    echo "building debian package ${PACKAGENAME}.deb ..."
    cd ${PROJECT}
    dpkg-deb --build package
    if [ ${?} -eq "0" ];then
        mv ${PROJECT}/package.deb /tmp/${PACKAGENAME}-${KERNEL_RELEASE}_${VERSION}_${ARCHITECTURE}.deb
        echo "SUCCESS building /tmp/${PACKAGENAME}.deb"
        echo -e "\nINSTALL\nsudo dpkg -i /tmp/${PACKAGENAME}-${KERNEL_RELEASE}_${VERSION}_${ARCHITECTURE}.deb\n\n"
    fi
}

function CLEAN() {
    rm -R ${PROJECT}
    rm -R /tmp/${PACKAGENAME}.deb
    dpkg --purge ${PACKAGENAME}
}


case ${1} in
    create|CREATE|Create)
        if [ -z "${2}" ];then
            echo "You must provide the package name.";
            exit 0;
        else
            PACKAGENAME="${2}";
        fi;
        if [ ! -z "${3}" ];then
            VERSION=${3}
        fi
        CREATE;;
    clean|CLEAN|Clean)
        if [ -z "${2}" ];then
            echo "You must provide the package name.";
            exit 0;
        else
            PACKAGENAME="${2}";
        fi;
        CLEAN;;
    *) echo -e "Unknown option ${1}\nOptions are: CREATE or CLEAN and optional [VERSION]";;
esac
