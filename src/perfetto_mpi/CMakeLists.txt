
cmake_minimum_required(VERSION 3.17)

project(perfetto_pmpi LANGUAGES C CXX)

find_package(Python2 REQUIRED COMPONENTS Interpreter)

add_library(perfetto_pmpi SHARED perfetto_pmpi.cxx)
target_compile_definitions(perfetto_pmpi PRIVATE PIC)
target_link_libraries(perfetto_pmpi PRIVATE perfetto::perfetto)
target_link_libraries(perfetto_pmpi PRIVATE MPI::MPI_CXX)
target_include_directories(perfetto_pmpi PRIVATE ../libmrc/include)

set(_output "${CMAKE_CURRENT_BINARY_DIR}/perfetto_pmpi.cxx")
set(_input "${CMAKE_CURRENT_SOURCE_DIR}/perfetto_pmpi.tpl.cxx")
set(_wrap "${CMAKE_CURRENT_SOURCE_DIR}/wrap.py")
add_custom_command(
  OUTPUT ${_output}
  COMMAND ${Python2_EXECUTABLE} ${_wrap} -f -o ${_output} ${_input}
  MAIN_DEPENDENCY ${_input}
  DEPENDS ${_wrap}
  COMMENT "Wrapping ${_input}"
  VERBATIM
  )
