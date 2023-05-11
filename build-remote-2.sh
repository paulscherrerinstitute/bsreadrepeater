
VER=$(cat version.txt)

D0=$(pwd)

if false; then
  curl -LO "https://github.com/zeromq/libzmq/releases/download/v4.3.4/zeromq-4.3.4.tar.gz"
  tar -xf zeromq-4.3.4.tar.gz
  cd zeromq-4.3.4
  mkdir build
  cmake3 -S . -B ./build -D CMAKE_BUILD_TYPE=Release -D BUILD_TESTS:BOOL=OFF -D ENABLE_DRAFTS:BOOL=ON
  cmake3 --build ./build
fi

cd "$D0"

echo "--------------------  BUILD  VER $VER"
pwd

uname -a
which gcc
gcc --version
which cmake3
cmake3 --version

echo $D0
pwd

rm -rf "$D0/package"
rm -rf "$D0/build"

mkdir -p ./build

cmake3 -S . -B ./build -D ZMQ_BUILD_DIR=$HOME/software/zeromq-4.3.4/install

cmake3 --build ./build

PACKAGE_NAME=bsrep-$VER-amd64-rhel7

echo $PACKAGE_NAME

mkdir -p package/$PACKAGE_NAME
cp ./build/src/bsrep ./package/$PACKAGE_NAME/
cp $HOME/software/zeromq-4.3.4/install/lib64/libzmq*.so* ./package/$PACKAGE_NAME/

cd package
pwd
tar -czf $PACKAGE_NAME.tgz $PACKAGE_NAME
ls -ltr
tar -tf $PACKAGE_NAME.tgz

URL=https://data-api.psi.ch/distri/store/$PACKAGE_NAME.tgz
echo $URL
curl -T $PACKAGE_NAME.tgz $URL

cd $D0
pwd
echo $URL
