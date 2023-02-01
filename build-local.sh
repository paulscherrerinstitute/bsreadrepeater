mkdir -p ./build

cmake3 -S . -B ./build -D ZMQ_BUILD_DIR=$HOME/software/zeromq-4.3.4/build

cmake3 --build ./build
