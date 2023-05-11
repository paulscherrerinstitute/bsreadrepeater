#ssh sf-daqbuf-34 sudo yum install -y libpcap libpcap-devel
#ssh sf-daqbuf-34 mkdir dtop

VER=$(cat version.txt)
EXE_NAME=bsreadrepeater
TARGET_TRIPLE=amd64-rhel7
BUILD_HOST=sf-daqbuf-34
REMOTE_DIR="bsreadrepeater-$VER"

ssh ${BUILD_HOST} mkdir -p ${REMOTE_DIR}

# This Redhat release has a separate cmake3 package.

#ssh ${BUILD_HOST} sudo yum install -y devtoolset-12-gcc
#ssh ${BUILD_HOST} sudo yum install -y devtoolset-12-gcc-c++
#ssh ${BUILD_HOST} sudo yum install -y cmake3
#ssh ${BUILD_HOST} sudo yum install -y zeromq zeromq-devel

#ssh ${BUILD_HOST} sudo yum search cmake3

rsync -rti include src CMakeLists.txt version.txt build-remote-2.sh ${BUILD_HOST}:${REMOTE_DIR}/
if [ "$?" != "0" ]; then return -1; fi

ssh ${BUILD_HOST} cd ${REMOTE_DIR} '&&' scl enable devtoolset-12 -- bash build-remote-2.sh $VER
if [ "$?" != "0" ]; then return -1; fi

#ssh ${BUILD_HOST} cd ${REMOTE_DIR}\; pwd\; gcc -std=gnu11 -O3 -o dtop main.c -I. -lpcap
#if [ "$?" != "0" ]; then return -1; fi

#scp ${BUILD_HOST}:${REMOTE_DIR}/${EXE_NAME} ${EXE_NAME}-${TARGET_TRIPLE}
#if [ "$?" != "0" ]; then return -1; fi

#scp ${EXE_NAME}-${TARGET_TRIPLE} sf-daqbuf-24:.
#if [ "$?" != "0" ]; then return -1; fi
#echo "copied to sf-daqbuf-24"

#scp ${EXE_NAME}-${TARGET_TRIPLE} data-api:.
#if [ "$?" != "0" ]; then return -1; fi

#ssh data-api sudo chown nginx:nginx ${EXE_NAME}-${TARGET_TRIPLE} \; sudo mv ${EXE_NAME}-${TARGET_TRIPLE} /opt/distri/
#if [ "$?" != "0" ]; then return -1; fi
#echo "copied to distri"
