cmake_minimum_required(VERSION 3.12)
project(test_libswoc CXX)
set(CMAKE_CXX_STANDARD 17)

add_executable(test_libswoc
    unit_test_main.cc

    test_BufferWriter.cc
    test_bw_format.cc
    test_Errata.cc
    test_IntrusiveDList.cc
    test_IntrusiveHashMap.cc
    test_ip.cc
    test_Lexicon.cc
    test_MemSpan.cc
    test_MemArena.cc
    test_meta.cc
    test_TextView.cc
    test_Scalar.cc
    test_swoc_file.cc

    ex_bw_format.cc
    ex_IntrusiveDList.cc
    ex_MemArena.cc
    ex_TextView.cc
    )

target_link_libraries(test_libswoc PUBLIC swoc++)
set_target_properties(test_libswoc PROPERTIES CLANG_FORMAT_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
