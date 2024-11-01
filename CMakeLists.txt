cmake_minimum_required(VERSION 3.20.3)
project(MemSim2 VERSION 0.1)

if (CMAKE_BUILD_TYPE MATCHES "Release")
    set(COMPILE_OPTIONS
        -Ofast
        -flto=auto
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
    src/cache/controller/llc2.cpp
    src/utils/argparse.cpp
)

if (USE_DRAMSIM3)
    set(SIM_FILES ${SIM_FILES} src/ds3/interface.cpp)
else()
    set(SIM_FILES ${SIM_FILES} 
        src/dram/config.cpp
        src/dram/controller.cpp
        src/dram/rank.cpp
        src/dram/subchannel.cpp)
endif()

find_package(ZLIB)

add_executable(sim main/sim.cpp ${SIM_FILES})
target_compile_options(sim PRIVATE ${COMPILE_OPTIONS})
target_include_directories(sim PRIVATE "src")

if (USE_DRAMSIM3)
    add_subdirectory(DRAMsim3)
    target_compile_definitions(sim PRIVATE USE_DRAMSIM3=ON)
    target_link_libraries(sim PRIVATE dramsim3)
endif()

target_link_libraries(sim PRIVATE ZLIB::ZLIB)
target_compile_definitions(sim PRIVATE N_THREADS=4)
# Optional compile definitions:
if (LLC_REPL_POLICY)
    target_compile_definitions(sim PRIVATE LLC_REPL_POLICY=CacheReplPolicy::${LLC_REPL_POLICY})
endif()
