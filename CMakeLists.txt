cmake_minimum_required(VERSION 3.20.3)
project(MemSim2 VERSION 0.1)

if (CMAKE_BUILD_TYPE MATCHES "Release")
    set(COMPILE_OPTIONS
        -O3
        -fno-strict-aliasing
        -Wno-deprecated
        -Wno-unknown-warming-option
        -Wno-self-assign
        -Wno-unused-parameter
        -Wno-unused-but-set-variable
        -Wunused)
elseif (CMAKE_BUILD_TYPE MATCHES "Profile")
    set(COMPILE_OPTIONS
        -fno-strict-aliasing
        -Wno-deprecated
        -Wno-unknown-warming-option
        -Wno-self-assign
        -Wno-unused-parameter
        -Wno-unused-but-set-variable
        -Wunused
        -O1 -g)
else()
    set(COMPILE_OPTIONS
        -g
        -Wall)
endif()

set(CMAKE_CXX_STANDARD 17)

set(SIM_FILES
    src/core.cpp
    src/os.cpp
    src/cache/replacement.cpp
    src/ds3/interface.cpp
)

find_package(ZLIB)

add_executable(sim main/sim.cpp ${SIM_FILES})
target_compile_options(sim PRIVATE ${COMPILE_OPTIONS})
target_include_directories(sim PRIVATE "src")

add_subdirectory(DRAMsim3)

target_link_libraries(sim PRIVATE ZLIB::ZLIB dramsim3)
if (CMAKE_BUILD_TYPE MATCHES "Profile")
#   target_link_libraries(sim PRIVATE -pg)
endif()

target_compile_definitions(sim PRIVATE -DN_THREADS=4)
