add_executable(bsrep
../include/bsreadrepeater/err.h
../include/bsreadrepeater/bsr_log.h
../include/bsreadrepeater/hex.h
hex.c
../include/bsreadrepeater/bsr_ema.h
bsr_ema.c
../include/bsreadrepeater/bsr_str.h
bsr_str.c
../include/bsreadrepeater/bsr_stats.h
bsr_stats.c
../include/bsreadrepeater/bsr_startupcmd.h
bsr_startupcmd.c
../include/bsreadrepeater/bsr_poller.h
bsr_poller.c
../include/bsreadrepeater/bsr_sockhelp.h
bsr_sockhelp.c
../include/bsreadrepeater/bsr_cmdchn.h
bsr_cmdchn.c
../include/bsreadrepeater/bsr_chnhandler.h
bsr_chnhandler.c
../include/bsreadrepeater/bsr_memreq.h
bsr_memreq.c
../include/bsreadrepeater/bsr_ctx1.h
bsr_ctx1.c
../include/bsreadrepeater/bsr_rep.h
bsr_rep.c
bsr_json.cpp
../include/bsreadrepeater/bsr_json_commands.h
bsr_json_commands.cpp
../include/bsreadrepeater/bsr_timed_events.h
bsr_timed_events.c
./bitshuffle/lz4.c
./bitshuffle/lz4.h
./bitshuffle/iochain.c
./bitshuffle/iochain.h
./bitshuffle/bitshuffle_core.c
./bitshuffle/bitshuffle_core.h
./bitshuffle/bitshuffle.c
./bitshuffle/bitshuffle.h
main.c
)
set_property(TARGET bsrep PROPERTY C_STANDARD 11)
set_property(TARGET bsrep PROPERTY CXX_STANDARD 20)
target_compile_options(bsrep PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter)
target_include_directories(bsrep PRIVATE "../include")
target_include_directories(bsrep PRIVATE "../include/bsreadrepeater")
target_include_directories(bsrep PRIVATE "../../../software/rapidjson/include")
target_include_directories(bsrep PRIVATE "./bitshuffle")
target_include_directories(bsrep PRIVATE "${ZeroMQ_ROOT}/../include")
target_link_libraries(bsrep libzmq)
target_link_libraries(bsrep PkgConfig::glib2pkg)
target_link_libraries(bsrep m)
