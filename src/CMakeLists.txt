add_executable(bsrep main.c)
target_include_directories(bsrep PRIVATE "../include/bsreadrepeater")
target_link_libraries(bsrep kappa)
target_link_libraries(bsrep libzmq)
