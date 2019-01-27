cmake_minimum_required(VERSION 3.12)

project(lib-swoc++ CXX)
set(CMAKE_CXX_STANDARD 17)
include(GNUInstallDirs)

set(HEADER_FILES
    include/swoc/BufferWriter.h
    include/swoc/bwf_base.h
    include/swoc/bwf_std.h
    include/swoc/bwf_ex.h
    include/swoc/Errata.h
    include/swoc/IntrusiveDList.h
    include/swoc/IntrusiveHashMap.h
    include/swoc/swoc_ip.h
    include/swoc/Lexicon.h
    include/swoc/MemArena.h
    include/swoc/MemSpan.h
    include/swoc/Scalar.h
    include/swoc/TextView.h
    include/swoc/swoc_file.h
    include/swoc/swoc_meta.h
    include/swoc/bwf_ip.h include/swoc/swoc_version.h)

# These are external but required.
set(EXTERNAL_HEADER_FILES
    include/swoc/ext/HashFNV.h
)

set(CC_FILES
    src/bw_format.cc
    src/Errata.cc
    src/swoc_ip.cc
    src/MemArena.cc
    src/swoc_file.cc
    src/TextView.cc
    )

add_library(swoc++ STATIC ${CC_FILES})

# Not quite sure how this works, but I think it generates one of two paths depending on the context.
# That is, the generator functions return non-empty strings only in the corresponding context.
target_include_directories(swoc++
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    )

# These install target variables are created by GNUInstallDirs.
install(TARGETS swoc++
    EXPORT swoc++-config
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
install(DIRECTORY include/swoc DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT swoc++-config
    NAMESPACE swoc++::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/swoc++
    )

# Alledgedly this makes the targets "importable from the build directory" but I see no evidence of that.
# AFAICT the file isn't created at all even with this enabled.
#export(TARGETS swoc++ FILE libswoc++-config.cmake)

set(CLANG_DIRS )

set_target_properties(swoc++ PROPERTIES CLANG_FORMAT_DIRS "${PROJECT_SOURCE_DIR}/src;${PROJECT_SOURCE_DIR}/include")