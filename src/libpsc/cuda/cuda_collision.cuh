
#pragma once

#include "cuda_base.cuh"
#include "cuda_mparticles.cuh"
#include "cuda_mparticles_sort.cuh"
#include "rng_state.cuh"
// FIXME, horrible hack...
#define DEVICE __device__
#include "binary_collision.hxx"

#include <curand_kernel.h>

#define THREADS_PER_BLOCK 128

template <typename cuda_mparticles, typename RngState>
struct CudaCollision;

template <typename cuda_mparticles, typename RngState>
__global__ static void k_collide(
  DMparticlesCuda<typename cuda_mparticles::BS> dmprts, uint* d_off, uint* d_id,
  float nudt0, typename RngState::Device rng_state, uint n_cells, uint n_cells_per_patch)
{
  CudaCollision<cuda_mparticles, RngState>::d_collide(
    dmprts, d_off, d_id, nudt0, rng_state, n_cells, n_cells_per_patch);
}

class MyParticle
{
public:
  using real_t = DParticleCuda::real_t;
  using Real3 = DParticleCuda::Real3;
  
  template<typename DMparticlesCuda>
  __device__
  MyParticle(const DParticleCuda& prt, const DMparticlesCuda& dmprts)
    : prt_{prt}, q_(dmprts.q(prt.kind)), m_(dmprts.m(prt.kind))
  {}

  __device__ Real3  x() const { return prt_.x; }
  __device__ Real3& x()       { return prt_.x; }
  __device__ Real3  u() const { return prt_.u; }
  __device__ Real3& u()       { return prt_.u; }
  __device__ int kind() const { return prt_.kind; }
  __device__ real_t qni_wni() const { return prt_.qni_wni; }

  __device__ real_t q() const { return q_; }
  __device__ real_t m() const { return m_; }

  __device__ float4 xi4() const { return float4{x()[0], x()[1], x()[2], cuda_int_as_float(kind())}; }
  __device__ float4 pxi4() const { return float4{u()[0], u()[1], u()[2], qni_wni()}; }
  
private:
  DParticleCuda prt_;
  float q_;
  float m_;
};

template <typename cuda_mparticles, typename RngState>
__global__ static void k_collide2(
  DMparticlesCuda<typename cuda_mparticles::BS> dmprts, uint* d_off, uint* d_id,
  float nudt0, typename RngState::Device rng_state, uint n_cells, uint n_cells_per_patch)
{
  using real_t = typename cuda_mparticles::real_t;
  using DMparticles = DMparticlesCuda<typename cuda_mparticles::BS>;
  using DParticle = DParticleProxy<DMparticles>;

  int id = threadIdx.x + blockIdx.x * THREADS_PER_BLOCK;
  /* Copy state to local memory for efficiency */
  auto rng = rng_state[id];
  BinaryCollision<MyParticle> bc;
  
  for (uint bidx = blockIdx.x; bidx < n_cells; bidx += gridDim.x) {
    uint beg = d_off[bidx];
    uint end = d_off[bidx + 1];
    real_t nudt1 =
      nudt0 * (end - beg) * (end - beg) / ((end - beg) & ~1); // somewhat counteract that we don't collide
    // the last particle if odd
    for (uint n = beg + 2 * threadIdx.x; n + 1 < end;
	 n += 2 * THREADS_PER_BLOCK) {
      // printf("%d/%d: n = %d off %d\n", blockIdx.x, threadIdx.x, n,
      // d_off[blockIdx.x]);
      float4 xi4_1 = dmprts.storage.xi4[d_id[n]];
      float4 pxi4_1 = dmprts.storage.pxi4[d_id[n]];
      float4 xi4_2 = dmprts.storage.xi4[d_id[n+1]];
      float4 pxi4_2 = dmprts.storage.pxi4[d_id[n+1]];
      MyParticle prt1(ParticleCudaStorage(xi4_1, pxi4_1), dmprts);
      MyParticle prt2(ParticleCudaStorage(xi4_2, pxi4_2), dmprts);
#ifndef NDEBUG
      int p = bidx / n_cells_per_patch;
      int cidx1 = dmprts.validCellIndex(dmprts.storage.xi4[d_id[n]], p);
      int cidx2 = dmprts.validCellIndex(dmprts.storage.xi4[d_id[n + 1]], p);
      assert(cidx1 == cidx2);
#endif
      bc(prt1, prt2, nudt1, rng);
      // xi4 is not modified, don't need to store
      dmprts.storage.pxi4[d_id[n]] = prt1.pxi4();
      dmprts.storage.pxi4[d_id[n+1]] = prt2.pxi4();
    }
  }
  
  rng_state[id] = rng;
}

// ======================================================================
// cuda_collision

template <typename cuda_mparticles, typename RngState>
struct CudaCollision
{
  using real_t = typename cuda_mparticles::real_t;
  using DMparticles = DMparticlesCuda<typename cuda_mparticles::BS>;
  using DParticle = DParticleProxy<DMparticles>;

  CudaCollision(int interval, double nu, int nicell, double dt)
    : interval_{interval}, nu_{nu}, nicell_(nicell), dt_(dt)
  {}

  int interval() const { return interval_; }

  void operator()(cuda_mparticles& cmprts)
  {
    if (cmprts.n_prts == 0) {
      return;
    }
    cmprts.reorder();
    sort_.find_indices_ids(cmprts);
    sort_.sort();
    sort_.find_offsets();
    // for (int c = 0; c <= cmprts.n_cells(); c++) {
    //   printf("off[%d] = %d\n", c, int(sort_by_cell.d_off[c]));
    // }

    int blocks = cmprts.n_cells();
    if (blocks > 32768)
      blocks = 32768;
    dim3 dimGrid(blocks);

    if (blocks * THREADS_PER_BLOCK > rng_state_.size()) {
      rng_state_.resize(blocks * THREADS_PER_BLOCK);
    }

    int n_cells_per_patch = cmprts.grid().ldims[0] * cmprts.grid().ldims[1] * cmprts.grid().ldims[2];

    // all particles need to have same weight!
    real_t wni = 1.; // FIXME, there should at least be some assert to enforce
                     // this //prts[n_start].w());
    real_t nudt0 = wni / nicell_ * interval_ * dt_ * nu_;

#if 0
    k_collide<cuda_mparticles, RngState><<<dimGrid, THREADS_PER_BLOCK>>>(
      cmprts, sort_.d_off.data().get(), sort_.d_id.data().get(), nudt0,
      rng_state_, cmprts.n_cells(), n_cells_per_patch);
    cuda_sync_if_enabled();
#else
    k_collide2<cuda_mparticles, RngState><<<dimGrid, THREADS_PER_BLOCK>>>(
      cmprts, sort_.d_off.data().get(), sort_.d_id.data().get(), nudt0,
      rng_state_, cmprts.n_cells(), n_cells_per_patch);
    cuda_sync_if_enabled();
#endif
  }

  __device__ static void d_collide(DMparticles dmprts, uint* d_off, uint* d_id,
                                   float nudt0,
                                   typename RngState::Device rng_state,
                                   uint n_cells, uint n_cells_per_patch)
  {

    int id = threadIdx.x + blockIdx.x * THREADS_PER_BLOCK;
    /* Copy state to local memory for efficiency */
    auto rng = rng_state[id];
    BinaryCollision<DParticle> bc;

    for (uint bidx = blockIdx.x; bidx < n_cells; bidx += gridDim.x) {
      uint beg = d_off[bidx];
      uint end = d_off[bidx + 1];
      real_t nudt1 =
        nudt0 * (end - beg) * (end - beg) / ((end - beg) & ~1); // somewhat counteract that we don't collide
                                  // the last particle if odd
      for (uint n = beg + 2 * threadIdx.x; n + 1 < end;
           n += 2 * THREADS_PER_BLOCK) {
        // printf("%d/%d: n = %d off %d\n", blockIdx.x, threadIdx.x, n,
        // d_off[blockIdx.x]);
        auto prt1 = DParticle{dmprts.storage.load_proxy(dmprts, d_id[n])};
        auto prt2 = DParticle{dmprts.storage.load_proxy(dmprts, d_id[n + 1])};
#ifndef NDEBUG
	int p = bidx / n_cells_per_patch;
	int cidx1 = dmprts.validCellIndex(dmprts.storage.xi4[d_id[n]], p);
	int cidx2 = dmprts.validCellIndex(dmprts.storage.xi4[d_id[n + 1]], p);
	assert(cidx1 == cidx2);
#endif
        bc(prt1, prt2, nudt1, rng);
        // xi4 is not modified, don't need to store
        dmprts.storage.store_momentum(prt1, d_id[n]);
        dmprts.storage.store_momentum(prt2, d_id[n + 1]);
      }
    }

    rng_state[id] = rng;
  }

private:
  int interval_;
  double nu_;
  int nicell_;
  double dt_;
  RngState rng_state_;
  cuda_mparticles_randomize_sort sort_;
};
