
#include <type_traits>
#include "bs.hxx"

#include "cuda_mfields.h"
#include "cuda_mfields.inl"
#include "cuda_mparticles.cuh"
#include "cuda_mparticles.inl"

#include <kg/io.h>

void debug_ab(DMparticlesCuda<BS144>& d_mprts, DMFields& d_mflds);

using DMparticles = DMparticlesCuda<BS144>;

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  DMFields d_mflds;
  DMparticles d_mprts;

  auto io = kg::io::IOAdios2{};
  auto reader = io.open("before-proc-51-time-1207.bp", kg::io::Mode::Read);
  reader.get("d_mflds", d_mflds);
  reader.get("d_mprts", d_mprts);
  reader.close();

  auto d_xi4 = thrust::device_pointer_cast<float4>(d_mprts.storage.xi4);
  float4 xi4 = d_xi4[4504];
  printf("4504 %g %g %g\n", xi4.x, xi4.y, xi4.z);
  
  debug_ab(d_mprts, d_mflds);
  
  MPI_Finalize();
  return 0;
}