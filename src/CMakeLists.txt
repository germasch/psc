
add_subdirectory(kg)
add_subdirectory(libpsc)

macro(add_psc_executable name)
  add_executable(${name} ${name}.cxx)
  target_link_libraries(${name} psc)
endmacro(add_psc_executable)

add_psc_executable(psc_bubble_yz)
add_psc_executable(psc_flatfoil_yz)
#add_psc_executable(psc_bigniff_yz)
add_psc_executable(psc_whistler)
add_psc_executable(psc_harris_xz)

if (RMM_FOUND)
  add_executable(cuda_test cuda_test.cu)
  target_link_libraries(cuda_test PUBLIC RMM::RMM)
endif()
