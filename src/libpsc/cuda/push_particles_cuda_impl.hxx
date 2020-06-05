
#pragma once

#include "push_particles.hxx"
#include "mparticles_cuda.hxx"
#include "psc_fields_cuda.h"
#include "cuda_iface.h"
#include "cuda_push_particles.cuh"
#include "interpolate.hxx"

#include "../libpsc/psc_push_particles/1vb/psc_push_particles_1vb.h"

struct DepositVb3d : std::integral_constant<int, DEPOSIT_VB_3D> {};
struct DepositVb2d : std::integral_constant<int, DEPOSIT_VB_2D> {};

struct CurrmemGlobal;
struct CurrmemShared;

template<typename DIM, typename BS, typename IP, typename DEPOSIT, typename CURRMEM>
struct CudaPushpConfig
{
  using dim = DIM;
  using Bs = BS;
  using Ip = IP;
  using Deposit = DEPOSIT;
  using Currmem = CURRMEM;
};

template<typename dim, typename BS>
using CudaConfig1vbec3d = CudaPushpConfig<dim, BS, opt_ip_1st_ec, DepositVb3d, CurrmemShared>;

template<typename dim>
using CudaConfig1vb = CudaPushpConfig<dim, BS144, opt_ip_1st, DepositVb2d, CurrmemShared>;

template<typename dim, typename BS>
using CudaConfig1vbec3dGmem = CudaPushpConfig<dim, BS, opt_ip_1st_ec, DepositVb3d, CurrmemGlobal>;

// ======================================================================
// psc_push_particles: "1vb_4x4_cuda"

template<typename Config>
class PushParticlesCuda : PushParticlesBase
{
public:
  using BS = typename Config::Bs;
  using Mparticles = MparticlesCuda<BS>;
  using MfieldsState = MfieldsStateCuda;
  
  void push_mprts(Mparticles& mprts, MfieldsState& mflds)
  {
#if 1
    CudaPushParticles_<Config>::push_mprts(mprts.cmprts(), mflds.cmflds());
#else
    auto& h_mprts = mprts.template get_as<MparticlesSingle>();
    auto& h_mflds = mflds.get_as<MfieldsStateSingle>(0, mflds._n_comps());
    ppush_.push_mprts(h_mprts, h_mflds);
    mprts.put_as(h_mprts);
    mflds.put_as(h_mflds, JXI, JXI+3);
#endif
  }

  PushParticlesVb<Config1vbecSplit<MparticlesSingle, MfieldsStateSingle, dim_xyz>> ppush_;
};

