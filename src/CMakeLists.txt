add_executable(bsrep
../include/bsreadrepeater/err.h
../include/bsreadrepeater/bsr_str.h
bsr_str.c
../include/bsreadrepeater/bsr_startupcmd.h
bsr_startupcmd.c
main.c
../include/bsreadrepeater/bsr_poller.h
bsr_poller.c
../include/bsreadrepeater/bsr_sockhelp.h
bsr_sockhelp.c
../include/bsreadrepeater/bsr_cmdchn.h
bsr_cmdchn.c
../include/bsreadrepeater/bsr_chnhandler.h
bsr_chnhandler.c
../include/bsreadrepeater/hex.h
hex.c
bsr_json.cpp
./bitshuffle/lz4.c
./bitshuffle/lz4.h
./bitshuffle/iochain.c
./bitshuffle/iochain.h
./bitshuffle/bitshuffle_core.c
./bitshuffle/bitshuffle_core.h
)
set_property(TARGET bsrep PROPERTY CXX_STANDARD 20)
target_compile_options(bsrep PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter)
target_include_directories(bsrep PRIVATE "../include/bsreadrepeater")
target_include_directories(bsrep PRIVATE "../../../software/rapidjson/include")
target_include_directories(bsrep PRIVATE "./bitshuffle")
#target_link_libraries(bsrep kappa)
target_link_libraries(bsrep libzmq)
target_link_libraries(bsrep PkgConfig::glib2pkg)
target_link_libraries(bsrep m)
