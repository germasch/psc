
#include <type_traits>
#include "bs.hxx"

#include "cuda_mfields.h"
#include "cuda_mfields.inl"
#include "cuda_mparticles.cuh"
#include "cuda_mparticles.inl"

#include <kg/io.h>
#include <iostream>

void debug_ab(DMparticlesCuda<BS144> d_mprts, DMFields d_mflds);

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
  //auto reader = io.open("flatfoil-summit113/bef-proc-27-time-1207.bp", kg::io::Mode::Read);
  auto reader = io.open("bef-proc-0-time-0.bp", kg::io::Mode::Read);
  reader.get("d_mflds", d_mflds_1);
  reader.get("d_mprts", d_mprts_1);
  reader.close();

  auto reader2 = io.open("aft-proc-0-time-0.bp", kg::io::Mode::Read);
  //auto reader2 = io.open("flatfoil-summit113/aft-proc-27-time-1207.bp", kg::io::Mode::Read);
  reader2.get("d_mflds", d_mflds_2);
  reader2.get("d_mprts", d_mprts_2);
  reader2.close();
  
  auto d_xi4_1 = thrust::device_pointer_cast<float4>(d_mprts_1.storage.xi4);
  auto d_xi4_2 = thrust::device_pointer_cast<float4>(d_mprts_2.storage.xi4);
  auto d_pxi4_1 = thrust::device_pointer_cast<float4>(d_mprts_1.storage.pxi4);
  auto d_pxi4_2 = thrust::device_pointer_cast<float4>(d_mprts_2.storage.pxi4);

  const int N = 1;
  for (int n = 0; n < N; n++) {
    std::cout << "0: " << d_xi4_1[n] << "\n";
    std::cout << "2: " << d_xi4_2[n] << "\n";
  }
  std::cout << "\n";

  for (int p = 0; p < d_mflds_1.n_patches_; p++) {
    uint size = d_mflds_1.box().size();
    cudaError ierr = cudaMemset(d_mflds_1.storage_.data() + (p * 9 + JXI) * size, 0,
    				3 * size * sizeof(fields_cuda_real_t));
    cudaCheck(ierr);
  }
  
  debug_ab(d_mprts_1, d_mflds_1);

  for (int n = 0; n < N; n++) {
    std::cout << "1 : " << d_xi4_1[n] << "\n";
    std::cout << "1p: " << d_pxi4_1[n] << "\n";
    std::cout << "2 : " << d_xi4_2[n] << "\n";
    std::cout << "2p: " << d_pxi4_2[n] << "\n";
  }
  
  MPI_Finalize();
  return 0;
}