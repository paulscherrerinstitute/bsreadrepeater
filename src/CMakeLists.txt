set(D0 ".")
set(D1 "${CMAKE_SOURCE_DIR}/include/bsreadrepeater")
add_executable(bsrep
${D0}/main.c
${D1}/err.h
${D1}/bsr_log.h
${D1}/hex.h
${D0}/hex.c
${D1}/bsr_ema.h
${D0}/bsr_ema.c
${D1}/bsr_str.h
${D0}/bsr_str.c
${D1}/bsr_stats.h
${D0}/bsr_stats.c
${D1}/bsr_startupcmd.h
${D0}/bsr_startupcmd.c
${D1}/bsr_poller.h
${D0}/bsr_poller.c
${D1}/bsr_sockhelp.h
${D0}/bsr_sockhelp.c
${D1}/bsr_cmdchn.h
${D0}/bsr_cmdchn.c
${D1}/bsr_chnhandler.h
${D0}/bsr_chnhandler.c
${D1}/bsr_memreq.h
${D0}/bsr_memreq.c
${D1}/bsr_ctx1.h
${D0}/bsr_ctx1.c
${D1}/bsr_rep.h
${D0}/bsr_rep.c
${D0}/bsr_json.cpp
${D1}/bsr_json_commands.h
${D0}/bsr_json_commands.cpp
${D1}/bsr_timed_events.h
${D0}/bsr_timed_events.c
${D1}/bsr_dummy_source_s.h
${D1}/bsr_dummy_source.h
${D0}/bsr_dummy_source.c
./bitshuffle/lz4.c
./bitshuffle/lz4.h
./bitshuffle/iochain.c
./bitshuffle/iochain.h
./bitshuffle/bitshuffle_core.c
./bitshuffle/bitshuffle_core.h
./bitshuffle/bitshuffle.c
./bitshuffle/bitshuffle.h
)
set_property(TARGET bsrep PROPERTY C_STANDARD 11)
set_property(TARGET bsrep PROPERTY CXX_STANDARD 20)
target_compile_options(bsrep PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter)
target_include_directories(bsrep PRIVATE "../include")
target_include_directories(bsrep PRIVATE "../include/bsreadrepeater")
target_include_directories(bsrep PRIVATE "../../../software/rapidjson/include")
target_include_directories(bsrep PRIVATE "./bitshuffle")
target_include_directories(bsrep PRIVATE "${ZeroMQ_ROOT}/../include")
#target_link_libraries(bsrep libzmq)
target_link_libraries(bsrep ${ZMQ_LIB})
target_link_libraries(bsrep PkgConfig::glib2pkg)
target_link_libraries(bsrep m)
