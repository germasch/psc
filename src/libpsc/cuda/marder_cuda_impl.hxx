
#pragma once

#include "fields_item_dive_cuda.hxx"
#include "fields_item_moments_1st_cuda.hxx"

#include <mrc_io.h>

// FIXME: checkpointing won't properly restore state
// FIXME: if the subclass creates objects, it'd be cleaner to have them
// be part of the subclass

template<typename BS>
struct MarderCuda : MarderBase
{
  using MfieldsState = MfieldsStateCuda;
  using Mfields = MfieldsCuda;
  using Mparticles = MparticlesCuda<BS>;
  using real_t = MfieldsState::real_t;
  
  MarderCuda(const Grid_t& grid, real_t diffusion, int loop, bool dump)
    : grid_{grid},
      diffusion_{diffusion},
      loop_{loop},
      dump_{dump},
      m_(grid, diffusion, loop, dump)
#if 1
#else
    ,
      item_rho_{grid},
      item_div_e_{grid},
      div_e_{grid, 1, grid.ibn},
      bnd_{grid, grid.ibn}
#endif
  {
#if 0
    // FIXME, output_fields should be taking care of their own psc_bnd?
    if (dump_) {
      writer_.open("marder");
    }
#endif
  }

#if 0
  void calc_aid_fields(MfieldsState& mflds, Mparticles& mprts)
  {
    //item_div_e_(mprts.grid(), mflds, mprts); // FIXME, should accept NULL for particles

    auto& grid = mflds.grid();
    auto& dev_rho = item_rho_.result();
    auto& rho = dev_rho.template get_as<MfieldsC>(0, 1);

    auto& h_mflds = mflds.get_as<MfieldsState>(0, mflds._n_comps());
    auto dive = Item_dive<MfieldsState>(h_mflds);
    div_e_.assign(dive);
    
    if (dump_) {
      static int cnt;
      writer_.begin_step(cnt, cnt);
      writer_.write(rho, grid, "rho", {"rho"});
      writer_.write(div_e_, grid, "dive", {"dive"});
      div_e_.axpy_comp(0, -1., rho, 0); // FIXME _yz
      writer_.write(div_e_, grid, "diff", {"diff"});
      writer_.end_step();
      cnt++;
    } else {
      div_e_.axpy_comp(0, -1., rho, 0); // FIXME _yz
    }

    bnd_.fill_ghosts(div_e_, 0, 1);
  }
#endif
  
  // ----------------------------------------------------------------------
  // psc_marder_cuda_correct
  //
  // Do the modified marder correction (See eq.(5, 7, 9, 10) in Mardahl and Verboncoeur, CPC, 1997)

  void correct(MfieldsState& mflds, Mfields& mf)
  {
#if 1
#else
    assert(mflds._grid().isInvar(0));

    const Grid_t& grid = mflds._grid();
    // FIXME: how to choose diffusion parameter properly?
    float dx[3];
    for (int d = 0; d < 3; d++) {
      dx[d] = grid.domain.dx[d];
    }
    float inv_sum = 0.;
    for (int d = 0; d < 3; d++) {
      if (!grid.isInvar(d)) {
	inv_sum += 1. / sqr(grid.domain.dx[d]);
      }
    }
    float diffusion_max = 1. / 2. / (.5 * grid.dt) / inv_sum;
    float diffusion     = diffusion_max * diffusion_;
    
    float fac[3];
    fac[0] = 0.f;
    fac[1] = .5 * grid.dt * diffusion / dx[1];
    fac[2] = .5 * grid.dt * diffusion / dx[2];

    cuda_mfields *cmflds = mflds.cmflds();
    cuda_mfields *cmf = mf.cmflds();

    // OPT, do all patches in one kernel
    for (int p = 0; p < mf.n_patches(); p++) {
      int l_cc[3] = {0, 0, 0}, r_cc[3] = {0, 0, 0};
      int l_nc[3] = {0, 0, 0}, r_nc[3] = {0, 0, 0};
      for (int d = 0; d < 3; d++) {
	if (grid.bc.fld_lo[d] == BND_FLD_CONDUCTING_WALL && grid.atBoundaryLo(p, d)) {
	  l_cc[d] = -1;
	  l_nc[d] = -1;
	}
	if (grid.bc.fld_hi[d] == BND_FLD_CONDUCTING_WALL && grid.atBoundaryHi(p, d)) {
	  r_cc[d] = -1;
	  r_nc[d] = 0;
	}
      }
    
      const int *ldims = grid.ldims;
    
      int ly[3] = { l_nc[0], l_cc[1], l_nc[2] };
      int ry[3] = { r_nc[0] + ldims[0], r_cc[1] + ldims[1], r_nc[2] + ldims[2] };
    
      int lz[3] = { l_nc[0], l_nc[1], l_cc[2] };
      int rz[3] = { r_nc[0] + ldims[0], r_nc[1] + ldims[1], r_cc[2] + ldims[2] };
    
      cuda_marder_correct_yz(cmflds, cmf, p, fac, ly, ry, lz, rz);
    }
#endif
  }

  void operator()(MfieldsStateCuda& mflds, MparticlesCuda<BS>& mprts)
  {
#if 1
    auto& h_mflds = mflds.get_as<MfieldsStateSingle>(EX, EX + 3);
    auto& h_mprts = mprts.template get_as<MparticlesSingle>();

    m_(h_mflds, h_mprts);
    
    mflds.put_as(h_mflds, EX, EX + 3);
    mprts.put_as(h_mprts, MP_DONT_COPY);
#else
    item_rho_(mprts);

    // need to fill ghost cells first (should be unnecessary with only variant 1) FIXME
    auto& h_mflds = mflds.get_as<MfieldsStateDouble>(0, mflds._n_comps());
    /bnd_.fill_ghosts(h_mflds, EX, EX+3);

    for (int i = 0; i < loop_; i++) {
      calc_aid_fields(mflds, mprts);
      //correct(mflds);
      //bnd_.fill_ghosts(mflds, EX, EX+3);
    }
#endif
  }
  
private:
  const Grid_t& grid_;
  real_t diffusion_; //< diffusion coefficient for Marder correction
  int loop_; //< execute this many relaxation steps in a loop
  bool dump_; //< dump div_E, rho

#if 1
  Marder_<MparticlesSingle, MfieldsStateSingle, MfieldsSingle> m_;
#else
  FieldsItemFields<Item_dive_cuda> item_div_e_;
  Moment_rho_1st_nc_cuda<MparticlesCuda<BS144>, dim_yz> item_rho_; // FIXME, hardcoded dim_yz
  WriterMRC writer_;
  MfieldsC div_e_;
  Bnd_<MfieldsC> bnd_;
#endif
};

