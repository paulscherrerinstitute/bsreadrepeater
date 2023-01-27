
D0=$(pwd)

if false; then
  curl -LO "https://github.com/zeromq/libzmq/releases/download/v4.3.4/zeromq-4.3.4.tar.gz"
  tar -xf zeromq-4.3.4.tar.gz
  cd zeromq-4.3.4
  mkdir build
  cmake3 -S . -B ./build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS:BOOL=OFF -DENABLE_DRAFTS:BOOL=ON
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

rm -rf "$D0/build"

mkdir -p ./build

cmake3 -S . -B ./build -DZMQ_BUILD_DIR=$(pwd)/zeromq-4.3.4/build

cmake3 --build ./build
