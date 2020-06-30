#!/bin/bash

function enable3kaccess()
{
    sed -i "s/^#3K\ //g" project.yml
    sed -i "s/^#3K\ //g" repository.yml
    sed -i "s/^#3K\ //g" Makefile.kernel
    sed -i "s/^#3K\ //g" CMakeLists.txt
}

function disable3kaccess()
{
    sed -i '/^[^#]/ s/\(^.*3KAccess\ Only.*$\)/#3K\ \1/' project.yml
    sed -i '/^[^#]/ s/\(^.*3KAccess\ Only.*$\)/#3K\ \1/' repository.yml
    sed -i '/^[^#]/ s/\(^.*CONFIG_UWB_DW3000.*$\)/X3K\ \1/' repository.yml
    sed -i '/^[^#]/ s/\(^.*3KAccess\ Only.*$\)/#3K\ \1/' Makefile.kernel
    sed -i '/^[^#]/ s/\(^.*3KAccess\ Only.*$\)/#3K\ \1/' CMakeLists.txt
}

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
CHECKOUT_PATH=${DIR}/repos
HAS_3K_ACCESS=0

if [ "$1" == "restore" ];then
    echo -en " * Restoring files..."
    disable3kaccess
    echo -e " done"
    exit 0;
fi

if [ -d ${DIR}/../../repos ];then
    echo " * uwb-core is checked out as part of uwb-apps"
    CHECKOUT_PATH=$(dirname $DIR)
fi

if [ ! -d ${CHECKOUT_PATH} ];then
    mkdir -p ${CHECKOUT_PATH}
fi

# Just get a very shallow clone to test with
TESTCO=$(mktemp -d)
echo " * Checking access to dw3000-c0 repository"
if git clone git@github.com:Decawave/uwb-dw3000-c0 ${TESTCO} --depth 1 -q > /dev/null 2>&1; then
    echo "   - Access to dw3000-c0 OK"
    enable3kaccess
    HAS_3K_ACCESS=1
else
    echo "   - Access to dw3000-c0 Denied"
    disable3kaccess
    HAS_3K_ACCESS=0
fi
rm -rf ${TESTCO}

if [ ! -e ${CHECKOUT_PATH}/decawave-uwb-dw3000-c0 ];then
    if [ -h ${CHECKOUT_PATH}/decawave-uwb-dw3000-c0 ];then
        rm -f ${CHECKOUT_PATH}/decawave-uwb-dw3000-c0;
    fi

    echo -e "\nYou may now re-run newt upgrade in $(dirname ${CHECKOUT_PATH})"
    echo -e "to checkout the dw3000-c0 directory"
fi

# Check symlink paths used for build_generic
if [ ! -e ${DIR}/hw/drivers/uwb/uwb_dw1000 ];then
    echo "   - Fixing symlinks to dw1000"
    if [ -h ${DIR}/hw/drivers/uwb/uwb_dw1000 ];then
        rm -f ${DIR}/hw/drivers/uwb/uwb_dw1000
    fi
    ln -s ${CHECKOUT_PATH}/decawave-uwb-dw1000/hw/drivers/uwb/uwb_dw1000 ${DIR}/hw/drivers/uwb/uwb_dw1000
fi

if [ ! -e ${DIR}/lib/cir/cir_dw1000 ];then
    echo "   - Fixing symlinks to cir_dw1000"
    if [ -h ${DIR}/lib/cir/cir_dw1000 ];then
        rm -f ${DIR}/lib/cir/cir_dw1000
    fi
    ln -s ${CHECKOUT_PATH}/decawave-uwb-dw1000/lib/cir/cir_dw1000 ${DIR}/lib/cir/cir_dw1000
fi

if (( $HAS_3K_ACCESS ));then
    if [ ! -e ${DIR}/hw/drivers/uwb/uwb_dw3000-c0 ];then
        echo "   - Fixing symlinks to dw3000-c0"
        if [ -h ${DIR}/hw/drivers/uwb/uwb_dw3000-c0 ];then
            rm -f ${DIR}/hw/drivers/uwb/uwb_dw3000-c0
        fi
        ln -s ${CHECKOUT_PATH}/decawave-uwb-dw3000-c0/hw/drivers/uwb/uwb_dw3000-c0 ${DIR}/hw/drivers/uwb/uwb_dw3000-c0
    fi

    if [ ! -e ${DIR}/lib/cir/cir_dw3000-c0 ];then
        echo "   - Fixing symlinks to cir_dw3000-c0"
        if [ -h ${DIR}/lib/cir/cir_dw3000-c0 ];then
            rm -f ${DIR}/lib/cir/cir_dw3000-c0
        fi
        ln -s ${CHECKOUT_PATH}/decawave-uwb-dw3000-c0/lib/cir/cir_dw3000-c0 ${DIR}/lib/cir/cir_dw3000-c0
    fi
fi

echo " * To undo the modifications to repository files, run setup with \"restore\" as the first argument"

