
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
  using Moment_t = Moment_rho_1st_nc<MparticlesSingle, MfieldsSingle>;
  
  MarderCuda(const Grid_t& grid, real_t diffusion, int loop, bool dump)
    : grid_{grid},
      diffusion_{diffusion},
      loop_{loop},
      dump_{dump},
      m_(grid, diffusion, loop, dump)
#if 1
    , bnd_{grid, grid.ibn},
      bnd_mf_{grid, grid.ibn},
      rho_{grid, 1, grid.ibn},
      res_{grid, 1, grid.ibn}
#else
    ,
      item_rho_{grid},
      item_div_e_{grid},
      div_e_{grid, 1, grid.ibn},
      bnd_{grid, grid.ibn}
#endif
  {
    if (dump_) {
      io_.open("marder");
    }
  }

#if 1
  void calc_aid_fields(MfieldsStateSingle& mflds)
  {
    auto dive = Item_dive<MfieldsStateSingle>(mflds);
	       
    if (dump_) {
      static int cnt;
      io_.begin_step(cnt, cnt);//ppsc->timestep, ppsc->timestep * ppsc->dt);
      cnt++;
      io_.write(rho_, rho_.grid(), "rho", {"rho"});
      io_.write(adaptMfields(dive), dive.grid(), "dive", {"dive"});
      io_.end_step();
    }

    res_.assign(dive);
    res_.axpy_comp(0, -1., rho_, 0);
    // // FIXME, why is this necessary?
    bnd_mf_.fill_ghosts(res_, 0, 1);
  }

#else
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

#if 1
#else
  void correct(MfieldsState& mflds, Mfields& mf)
  {
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
  }
#endif

  // ----------------------------------------------------------------------
  // correct_patch
  //
  // Do the modified marder correction (See eq.(5, 7, 9, 10) in Mardahl and Verboncoeur, CPC, 1997)

#define define_dxdydz(dx, dy, dz)				\
  int dx _mrc_unused = (grid.isInvar(0)) ? 0 : 1;		\
  int dy _mrc_unused = (grid.isInvar(1)) ? 0 : 1;		\
  int dz _mrc_unused = (grid.isInvar(2)) ? 0 : 1

#define psc_foreach_3d_more(psc, p, ix, iy, iz, l, r) {	\
  int __ilo[3] = { -l[0], -l[1], -l[2] };		\
  int __ihi[3] = { grid.ldims[0] + r[0],		\
		   grid.ldims[1] + r[1],		\
		   grid.ldims[2] + r[2] };		\
  for (int iz = __ilo[2]; iz < __ihi[2]; iz++) {	\
  for (int iy = __ilo[1]; iy < __ihi[1]; iy++) {	\
  for (int ix = __ilo[0]; ix < __ihi[0]; ix++)

#define psc_foreach_3d_more_end			\
  } } }

  using fields_view_t = typename MfieldsSingle::fields_view_t;
  void correct_patch(const Grid_t& grid, fields_view_t flds, fields_view_t f, int p, real_t& max_err)
  {
    define_dxdydz(dx, dy, dz);

    // FIXME: how to choose diffusion parameter properly?
    //double deltax = ppsc->patch[p].dx[0];
    double deltay = grid.domain.dx[1]; // FIXME double/float
    double deltaz = grid.domain.dx[2];
    double inv_sum = 0.;
    int nr_levels;
    for (int d = 0; d < 3; d++) {
      if (!grid.isInvar(d)) {
	inv_sum += 1. / sqr(grid.domain.dx[d]);
      }
    }
    double diffusion_max = 1. / 2. / (.5 * grid.dt) / inv_sum;
    double diffusion     = diffusion_max * diffusion_;

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

#if 0
    psc_foreach_3d_more(ppsc, p, ix, iy, iz, l, r) {
      // FIXME: F3 correct?
      flds(EX, ix,iy,iz) += 
	(f(DIVE_MARDER, ix+dx,iy,iz) - f(DIVE_MARDER, ix,iy,iz))
	* .5 * ppsc->dt * diffusion / deltax;
      flds(EY, ix,iy,iz) += 
	(f(DIVE_MARDER, ix,iy+dy,iz) - f(DIVE_MARDER, ix,iy,iz))
	* .5 * ppsc->dt * diffusion / deltay;
      flds(EZ, ix,iy,iz) += 
	(f(DIVE_MARDER, ix,iy,iz+dz) - f(DIVE_MARDER, ix,iy,iz))
	* .5 * ppsc->dt * diffusion / deltaz;
    } psc_foreach_3d_more_end;
#endif

    assert(grid.isInvar(0));

    {
      int l[3] = { l_nc[0], l_cc[1], l_nc[2] };
      int r[3] = { r_nc[0], r_cc[1], r_nc[2] };
      psc_foreach_3d_more(ppsc, p, ix, iy, iz, l, r) {
	max_err = std::max(max_err, std::abs(f(0, ix,iy,iz)));
	flds(EY, ix,iy,iz) += 
	  (f(0, ix,iy+dy,iz) - f(0, ix,iy,iz))
	  * .5 *grid.dt * diffusion / deltay;
      } psc_foreach_3d_more_end;
    }

    {
      int l[3] = { l_nc[0], l_nc[1], l_cc[2] };
      int r[3] = { r_nc[0], r_nc[1], r_cc[2] };
      psc_foreach_3d_more(ppsc, p, ix, iy, iz, l, r) {
	flds(EZ, ix,iy,iz) += 
	  (f(0, ix,iy,iz+dz) - f(0, ix,iy,iz))
	  * .5 * grid.dt * diffusion / deltaz;
      } psc_foreach_3d_more_end;
    }
  }

#undef psc_foreach_3d_more
#undef psc_foreach_3d_more_end

  // ----------------------------------------------------------------------
  // correct

  void correct(MfieldsStateSingle& mf)
  {
    auto& mf_div_e = res_;

    real_t max_err = 0.;
    for (int p = 0; p < mf_div_e.n_patches(); p++) {
      correct_patch(mf.grid(), mf[p], mf_div_e[p], p, max_err);
    }

    MPI_Allreduce(MPI_IN_PLACE, &max_err, 1, Mfields_traits<MfieldsSingle>::mpi_dtype(), MPI_MAX, grid_.comm());
    mpi_printf(grid_.comm(), "marder: err %g\n", max_err);
  }

  void operator()(MfieldsStateCuda& mflds, MparticlesCuda<BS>& mprts)
  {
    auto& h_mflds = mflds.get_as<MfieldsStateSingle>(EX, EX + 3);
    auto& h_mprts = mprts.template get_as<MparticlesSingle>();

#if 1
    rho_.assign(Moment_t{h_mprts});
    // need to fill ghost cells first (should be unnecessary with only variant 1) FIXME
    bnd_.fill_ghosts(h_mflds, EX, EX+3);

    for (int i = 0; i < loop_; i++) {
      calc_aid_fields(h_mflds);
      correct(h_mflds);
      bnd_.fill_ghosts(h_mflds, EX, EX+3);
    }
    
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

  Bnd_<MfieldsStateSingle> bnd_;
  Bnd_<MfieldsSingle> bnd_mf_;
  MfieldsSingle rho_;
  MfieldsSingle res_;
  WriterMRC io_; //< for debug dumping
#else
  FieldsItemFields<Item_dive_cuda> item_div_e_;
  Moment_rho_1st_nc_cuda<MparticlesCuda<BS144>, dim_yz> item_rho_; // FIXME, hardcoded dim_yz
  WriterMRC writer_;
  MfieldsC div_e_;
  Bnd_<MfieldsC> bnd_;
#endif
};

