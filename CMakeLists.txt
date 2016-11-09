cmake_minimum_required(VERSION 2.0)
project(Stronk)

# Change -DHAVE_LIBUUID to either one of these;
# -DHAVE_WINDOWS for Windows
# -DHAVE_LIBUUID For libuuid
# -DHAVE_APPLE for Apple.
# -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"' -std=c11 -pedantic -ggdb -ftrapv -pipe -Wall -Wextra -Wno-unused-function -D_POSIX_C_SOURCE -DUSE_GCC_UNION_CAST -DHAVE_LIBUUID -DMCPR_DO_LOGGING -Wl,--no-as-needed -lssl -lcrypto -lz -lpthread -luuid -lzlog -lcurl")

include_directories(
    lib/
    src/
)

file(GLOB_RECURSE SOURCES src/*)
file(GLOB_RECURSE HEADERS src/*.h)
file(GLOB_RECURSE LIBRARY_SOURCES lib/*)
add_executable(Stronk ${SOURCES} ${LIBRARY_SOURCES})

# target_link_libraries(Stronk libz.a)            # zlib license
# target_link_libraries(Stronk libssl.a)          # OpenSSL license
# target_link_libraries(Stronk libcurl.dll.a)     # MIT license
