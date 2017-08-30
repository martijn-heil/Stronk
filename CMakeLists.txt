cmake_minimum_required(VERSION 2.5)
#set(CMAKE_LEGACY_CYGWIN_WIN32 0) # Remove when CMake >= 2.8.4 is required
project(Stronk C)

# define HAVE_FOPENCOOKIE if you have fopencookie()
# -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"' -std=c11 -pedantic -ggdb -Og -ftrapv -fno-stack-protector -pipe -Wall -Wextra -Wno-unused-function -Wno-pointer-arith -Wno-unused-parameter -Wno-unused-variable -D_POSIX_C_SOURCE -DDEBUG -DHAVE_FOPENCOOKIE -DHAVE_LIBUUID -DNANOSLEEP_HACKY_FIX -DFORCE_C11_THREAD_EMULATION -D_REENTRANT -D_POSIX_C_SOURCE -D_GNU_SOURCE -pthread -Wl,--no-as-needed -lcrypto -lssl -lz -lcurl -lzlog") # -Wl,--no-as-needed

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
