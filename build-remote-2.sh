
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

echo "--------------------  BUILD"
pwd

uname -a
which gcc
gcc --version
which cmake3
cmake3 --version

rm -rf "$D0/package"
rm -rf "$D0/build"

mkdir -p ./build

cmake3 -S . -B ./build -D ZMQ_BUILD_DIR=$HOME/software/zeromq-4.3.4/install

cmake3 --build ./build

mkdir -p package/bsrep
cp ./build/src/bsrep ./package/bsrep/
cp $HOME/software/zeromq-4.3.4/install/lib64/libzmq*.so* ./package/bsrep/
echo $D0
cd package
tar -czf bsrep.tgz bsrep
ls -ltr
tar -tf bsrep.tgz
cd $D0
echo $D0
