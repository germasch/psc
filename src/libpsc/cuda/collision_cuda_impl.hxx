
#pragma once

#include "collision.hxx"
#include "psc_particles_single.h"
#include "psc_fields_single.h"
#include "../libpsc/psc_collision/psc_collision_impl.hxx"


template <typename cuda_mparticles, typename RngState>
struct CudaCollision;

struct RngStateCuda;
struct RngStateFake;

// ----------------------------------------------------------------------
// CollisionCuda

template <typename MP, typename RngState = RngStateCuda>
struct CollisionCuda : CollisionBase
{
  using Mparticles = MP;

  CollisionCuda(const Grid_t& grid, int interval, double nu);
  void operator()(Mparticles& _mprts);
  int interval() const;

private:
  CudaCollision<typename Mparticles::CudaMparticles, RngState>* fwd_;
  Collision_<MparticlesSingle, MfieldsStateSingle, MfieldsSingle> c_;
};
