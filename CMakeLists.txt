# Set the minimum version of CMake that can be used
# To find the cmake version run
# $ cmake --version
cmake_minimum_required(VERSION 3.5)

# Set the project name
project (resynthesis)

set(RESYNTHESIS_CXX_STANDARD "17" CACHE STRING "C++ standard")
set(CMAKE_CXX_STANDARD ${RESYNTHESIS_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS On)

add_compile_options ( -std=c++17)

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(flute3)
add_subdirectory(OpenTimer)
add_subdirectory(abacus)
# Add an executable
# add_executable(main src/main.cpp)
# add_executable(resynthesis.o src/resynthesis.cpp)

find_package( OpenMP REQUIRED)
if(OPENMP_FOUND)
message("OPENMP FOUND")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

set(OPENTIMER_DIR "OpenTimer/")
include_directories(${OPENTIMER_DIR})
link_directories(${OPENTIMER_DIR}/lib)

file(GLOB SOURCES "src/*.cpp" "src/row_base.cpp")
add_executable(main ${SOURCES})
add_executable(timing_report "src/timing_report/get_timing_report.cpp")
add_executable(gen_verilog "src/gen_verilog/gen_verilog.cpp")
add_executable(timing_after_remap "src/timing_after_remap/timing_after_remap.cpp")
add_executable(testmain "src/test/test.cpp")


# target_link_libraries(mockturtle PRIVATE json)
# target_link_libraries(OpenTimer PRIVATE json)

target_link_libraries(main PUBLIC mockturtle)
target_link_libraries(main PUBLIC flute)
target_link_libraries(main PUBLIC OpenTimer)
target_link_libraries(main PUBLIC abacus)

target_link_libraries(timing_report PUBLIC mockturtle)
target_link_libraries(timing_report PUBLIC flute)
target_link_libraries(timing_report PUBLIC OpenTimer)
target_link_libraries(timing_report PUBLIC abacus)

target_link_libraries(gen_verilog PUBLIC mockturtle)

target_link_libraries(timing_after_remap PUBLIC mockturtle)
target_link_libraries(timing_after_remap PUBLIC OpenTimer)

target_link_libraries(testmain PUBLIC OpenTimer)
target_link_libraries(testmain PUBLIC flute)
target_link_libraries(testmain PUBLIC mockturtle)