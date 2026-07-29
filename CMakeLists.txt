cmake_minimum_required(VERSION 3.16)
project(szip VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(szip
    src/main.cpp
    src/bit_stream.cpp
    src/delta.cpp
    src/mtf.cpp
    src/bwt.cpp
    src/lz77.cpp
    src/rle.cpp
    src/huffman.cpp
    src/range_coder.cpp
    src/neural.cpp
)

if(MSVC)
    target_compile_options(szip PRIVATE /W4 /O2 /fp:fast /GL)
else()
    target_compile_options(szip PRIVATE -Wall -Wextra -O3 -ffast-math -funroll-loops -march=native -flto)
endif()
set_target_properties(szip PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)

# ---------------------------------------------------------
# Cross-Platform Installation & Unix Man Page Integration
# ---------------------------------------------------------
include(GNUInstallDirs)

# Install the executable to the system bin directory (e.g. /usr/local/bin)
install(TARGETS szip 
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install the man page to the system man directory (e.g. /usr/local/share/man/man1)
if(NOT WIN32)
    install(FILES docs/szip.1 
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
    )
endif()
