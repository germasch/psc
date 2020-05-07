
#include <type_traits>
#include "bs.hxx"

#include "cuda_mfields.h"
#include "cuda_mfields.inl"
#include "cuda_mparticles.cuh"
#include "cuda_mparticles.inl"

#include <kg/io.h>
#include <iostream>

void debug_ab(DMparticlesCuda<BS144>& d_mprts, DMFields& d_mflds);

std::ostream& operator<<(std::ostream& of, const float4 &xi)
{
  of << "{" << xi.x << ", " << xi.y << ", " << xi.z << ", " << xi.w << "}";
  return of;
}

using DMparticles = DMparticlesCuda<BS144>;

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  DMFields d_mflds_1, d_mflds_2;
  DMparticles d_mprts_1, d_mprts_2;

  auto io = kg::io::IOAdios2{};
  auto reader = io.open("before-proc-68-time-1206.bp", kg::io::Mode::Read);
  reader.get("d_mflds", d_mflds_1);
  reader.get("d_mprts", d_mprts_1);
  reader.close();

  auto reader2 = io.open("after-proc-68-time-1206.bp", kg::io::Mode::Read);
  reader2.get("d_mflds", d_mflds_2);
  reader2.get("d_mprts", d_mprts_2);
  reader2.close();
  
  auto d_xi4_1 = thrust::device_pointer_cast<float4>(d_mprts_1.storage.xi4);
  auto d_xi4_2 = thrust::device_pointer_cast<float4>(d_mprts_2.storage.xi4);
  
  debug_ab(d_mprts_1, d_mflds_1);

  for (int n = 0; n < 5; n++) {
    std::cout << "1: " << d_xi4_1[n] << "\n";
    std::cout << "2: " << d_xi4_2[n] << "\n";
  }
  
  MPI_Finalize();
  return 0;
}