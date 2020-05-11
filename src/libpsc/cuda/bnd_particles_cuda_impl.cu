
#include "bnd_particles_cuda_impl.hxx"
#include "cuda_bndp.h"
#include "bnd_cuda_3_impl.hxx"
#include "psc_fields_cuda.h"

extern BndCuda3<MfieldsStateCuda> *g_bnd;

// ----------------------------------------------------------------------
// ctor

template<typename Mparticles, typename DIM>
BndParticlesCuda<Mparticles, DIM>::BndParticlesCuda(const Grid_t& grid)
  : Base(grid),
    cbndp_(new cuda_bndp<typename Mparticles::CudaMparticles, DIM>(grid))
{}

// ----------------------------------------------------------------------
// dtor

template<typename Mparticles, typename DIM>
BndParticlesCuda<Mparticles, DIM>::~BndParticlesCuda()
{
  delete cbndp_;
}

// ----------------------------------------------------------------------
// reset

template<typename Mparticles, typename DIM>
void BndParticlesCuda<Mparticles, DIM>::reset(const Grid_t& grid)
{
  Base::reset(grid);
  delete(cbndp_);
  cbndp_ = new cuda_bndp<typename Mparticles::CudaMparticles, DIM>(grid);
}

// ----------------------------------------------------------------------
// operator()

template<typename Mparticles, typename DIM>
void BndParticlesCuda<Mparticles, DIM>::operator()(Mparticles& mprts)
{
  if (psc_balance_generation_cnt > this->balance_generation_cnt_) {
    this->balance_generation_cnt_ = psc_balance_generation_cnt;
    reset(mprts.grid());
  }
  
  static int pr_A, pr_B;
  if (!pr_A) {
    pr_A = prof_register("xchg_mprts_prep", 1., 0, 0);
    pr_B = prof_register("xchg_mprts_post", 1., 0, 0);
  }
  
  prof_restart(pr_time_step_no_comm);
  prof_start(pr_A);
  g_bnd->check(HX, HX + 3, __LINE__);
  auto&& bufs = cbndp_->prep(mprts.cmprts());
  prof_stop(pr_A);
  
  g_bnd->check(HX, HX + 3, __LINE__);
  this->process_and_exchange(mprts, bufs);
  
  prof_restart(pr_time_step_no_comm);
  prof_start(pr_B);
  g_bnd->check(HX, HX + 3, __LINE__);
  cbndp_->post(mprts.cmprts());
  g_bnd->check(HX, HX + 3, __LINE__);
  prof_stop(pr_B);
  prof_stop(pr_time_step_no_comm);
}

template struct BndParticlesCuda<MparticlesCuda<BS144>, dim_yz>;
template struct BndParticlesCuda<MparticlesCuda<BS444>, dim_xyz>;
