
cmake_minimum_required(VERSION 3.17)

project(Test LANGUAGES CXX CUDA)

include(FeatureSummary)

# find_package(Thrust REQUIRED)
# thrust_create_target(Thrust HOST OMP DEVICE CUDA)

find_package(rmm REQUIRED)
#set(BUILD_TESTS OFF CACHE BOOL "rmm tests" FORCE)
#add_subdirectory(/root/src/rmm build-rmm)

add_executable(test test.cxx cu.cu)
set_source_files_properties(test.cxx PROPERTIES LANGUAGE CUDA)
target_link_libraries(test PRIVATE rmm::rmm)

#add_executable(t2 t2.cu)
#target_link_libraries(t2 PRIVATE Thrust::Thrust)
