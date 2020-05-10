
#include <type_traits>
#include "bs.hxx"

#include "cuda_mfields.h"
#include "cuda_mfields.inl"
#include "cuda_mparticles.cuh"
#include "cuda_mparticles.inl"

#include <kg/io.h>
#include <iostream>

int debug_patch_ = -1;

void debug_ab(DMparticlesCuda<BS144> d_mprts, DMFields d_mflds);
void debug_rho(DMparticlesCuda<BS144> d_mprts, DMFields d_mflds);

std::ostream& operator<<(std::ostream& of, const float4 &xi)
{
  of << "{" << xi.x << ", " << xi.y << ", " << xi.z << ", " << xi.w << "}";
  return of;
}

bool operator==(const float4& a, const float4& b)
{
  return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

bool operator!=(const float4& a, const float4& b)
{
  return !(a == b);
}

class MF
{
  using T = float;

public:
  MF(const DMFields& dmf) : dmf_(dmf) {}

  T operator()(int i, int j, int k, int m, int p) const
  {
    auto data = thrust::device_pointer_cast(dmf_.storage_.data());
    i -= dmf_.box().ib()[0];
    j -= dmf_.box().ib()[1];
    k -= dmf_.box().ib()[2];
    int idx = (((p * dmf_.n_comps() + m) * dmf_.box().im()[2] + k) * dmf_.box().im()[1] + j) * dmf_.box().im()[0] + i;
    return data[idx];
  }

private:
  DMFields dmf_;
};

using DMparticles = DMparticlesCuda<BS144>;

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  DMFields d_mflds_1, d_mflds_2;
  DMparticles d_mprts_1, d_mprts_2;

  auto io = kg::io::IOAdios2{};
  auto reader = io.open("flatfoil-summit129/bef-proc-27-time-1207.bp", kg::io::Mode::Read);
  //auto reader = io.open("bef-proc-0-time-0.bp", kg::io::Mode::Read);
  reader.get("d_mflds", d_mflds_1);
  reader.get("d_mprts", d_mprts_1);
  reader.close();

  auto reader2 = io.open("flatfoil-summit129/aft-proc-27-time-1207.bp", kg::io::Mode::Read);
  //auto reader2 = io.open("aft-proc-0-time-0.bp", kg::io::Mode::Read);
  reader2.get("d_mflds", d_mflds_2);
  reader2.get("d_mprts", d_mprts_2);
  reader2.close();
  
  auto d_xi4_1 = thrust::device_pointer_cast<float4>(d_mprts_1.storage.xi4);
  auto d_xi4_2 = thrust::device_pointer_cast<float4>(d_mprts_2.storage.xi4);
  auto d_pxi4_1 = thrust::device_pointer_cast<float4>(d_mprts_1.storage.pxi4);
  auto d_pxi4_2 = thrust::device_pointer_cast<float4>(d_mprts_2.storage.pxi4);

  // for (int n = 0; n < N; n++) {
  //   std::cout << "0: " << d_xi4_1[n] << "\n";
  //   std::cout << "2: " << d_xi4_2[n] << "\n";
  // }
  // std::cout << "\n";

  for (int p = 0; p < d_mflds_1.n_patches_; p++) {
    uint size = d_mflds_1.box().size();
    cudaError ierr = cudaMemset(d_mflds_1.storage_.data() + (p * 9 + JXI) * size, 0,
    				3 * size * sizeof(fields_cuda_real_t));
    cudaCheck(ierr);
  }

  d_mprts_1.fnqzs_ = d_mprts_1.fnqys_; // fixing wrong data

  DMFields d_rho_p;
  d_rho_p.box_ = d_mflds_1.box();
  d_rho_p.n_patches_ = d_mflds_1.n_patches();
  d_rho_p.n_fields_ = 1;
  thrust::device_vector<float> d_rho_p_data(d_rho_p.box().size() * d_rho_p.n_comps() * d_rho_p.n_patches());
  d_rho_p.storage_.data_ = d_rho_p_data.data().get();
  d_rho_p.storage_.size_ = d_rho_p_data.size();

  DMFields d_rho_m;
  d_rho_m.box_ = d_mflds_1.box();
  d_rho_m.n_patches_ = d_mflds_1.n_patches();
  d_rho_m.n_fields_ = 1;
  thrust::device_vector<float> d_rho_m_data(d_rho_m.box().size() * d_rho_m.n_comps() * d_rho_m.n_patches());
  d_rho_m.storage_.data_ = d_rho_m_data.data().get();
  d_rho_m.storage_.size_ = d_rho_m_data.size();

  
  debug_rho(d_mprts_1, d_rho_m);
  debug_ab(d_mprts_1, d_mflds_1);
  debug_rho(d_mprts_1, d_rho_p);
  
  auto writer = io.open("flatfoil-summit129/after.bp", kg::io::Mode::Write, MPI_COMM_SELF);
  writer.put("d_mflds", d_mflds_1);
  writer.put("d_mprts", d_mprts_1);
  writer.put("d_rho_m", d_rho_m);
  writer.put("d_rho_p", d_rho_p);
  writer.close();

  int n_prts = d_mprts_1.size_;
  for (int n = 0; n < n_prts; n++) {
    if (d_xi4_1[n] != d_xi4_2[n]) {
      std::cout << "1 : " << d_xi4_1[n] << "\n";
      std::cout << "2 : " << d_xi4_2[n] << "\n";
    }
    if (d_pxi4_1[n] != d_pxi4_2[n]) {
      std::cout << "1p: " << d_pxi4_1[n] << "\n";
      std::cout << "2p: " << d_pxi4_2[n] << "\n";
    }
  }

#if 0
  MF mf1(d_mflds_1), mf2(d_mflds_2);
  auto& ib = d_mflds_1.box().ib();
  auto& im = d_mflds_1.box().im();
  for (int p = 0; p < d_mflds_1.n_patches(); p++) {
    for (int m = JXI; m <= JZI; m++) {
      for (int k = ib[2]; k < ib[2] + im[2]; k++) {
	for (int j = ib[1]; j < ib[1] + im[1]; j++) {
	  for (int i = ib[0]; i < ib[0] + im[0]; i++) {
	    if (mf1(i, j, k, m, p) != mf2(i, j, k, m, p)) {
	      std:: cout << "!!! m" << m << " " << mf1(i, j, k, m, p) << " -- " << mf2(i, j, k, m, p) << "\n";
	    }
	  }
	}
      }
    }
  }
#endif
  
  MPI_Finalize();
  return 0;
}