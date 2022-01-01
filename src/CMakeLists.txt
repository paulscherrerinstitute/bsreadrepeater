add_executable(bsrep
../include/bsreadrepeater/err.h
main.c
../include/bsreadrepeater/bsr_poller.h
bsr_poller.c
../include/bsreadrepeater/bsr_cmdchn.h
bsr_cmdchn.c
)
target_compile_options(bsrep PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_include_directories(bsrep PRIVATE "../include/bsreadrepeater")
#target_link_libraries(bsrep kappa)
target_link_libraries(bsrep libzmq)
target_link_libraries(bsrep PkgConfig::glib2pkg)
