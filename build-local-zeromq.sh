FETCHDIR="$HOME/software"
SRCDIR="$FETCHDIR/zeromq-4.3.4"

function fetch_source {
  cd $FETCHDIR
  curl -LO "https://github.com/zeromq/libzmq/releases/download/v4.3.4/zeromq-4.3.4.tar.gz"
  tar -xf zeromq-4.3.4.tar.gz
}

function configure {
  cd $SRCDIR
  mkdir -p build
  mkdir -p install
  cmake3 -S . -B ./build -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=$(pwd)/install -D BUILD_TESTS:BOOL=OFF -D ENABLE_DRAFTS:BOOL=ON
}

function build {
  cd $SRCDIR
  cmake3 --build ./build
  cmake3 --install ./build
}

fetch_source
configure
build
