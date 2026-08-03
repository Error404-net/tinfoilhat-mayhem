cmake_minimum_required(VERSION 3.14)
project(tinfoilhat-sim CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional ASAN build: cmake -DASAN=ON -B build -S .
option(ASAN "Enable AddressSanitizer + UBSan" OFF)

add_executable(tinfoilhat-sim
    sim_main.cpp
    ../external/tinfoilhat/ui_tinfoilhat.cpp
)

target_include_directories(tinfoilhat-sim PRIVATE
    includes                         # stub Mayhem headers (take precedence)
    ../external/tinfoilhat           # app headers (tinfoilhat_logic.hpp etc.)
)

target_compile_options(tinfoilhat-sim PRIVATE -Wall -Wextra -Wno-unused-parameter)

if(ASAN)
    target_compile_options(tinfoilhat-sim PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(tinfoilhat-sim PRIVATE -fsanitize=address,undefined)
endif()
