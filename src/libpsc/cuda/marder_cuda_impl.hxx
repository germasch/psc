
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
      item_rho_{grid},
      item_dive_{grid},
      bnd_{grid, grid.ibn},
      bnd_mf_{grid, grid.ibn},
      rho_{grid, 1, grid.ibn},
      res_{grid, 1, grid.ibn}
  {
    if (dump_) {
      io_.open("marder");
    }
  }

  void calc_aid_fields(MfieldsState& mflds, Mfields& rho)
  {
    item_dive_(mflds.grid(), mflds);
    auto& dive = item_dive_.result();
	       
    if (dump_) {
      static int cnt;
      io_.begin_step(cnt, cnt);//ppsc->timestep, ppsc->timestep * ppsc->dt);
      cnt++;
      io_.write(rho, rho.grid(), "rho", {"rho"});
      io_.write(dive, dive.grid(), "dive", {"dive"});
      io_.end_step();
    }

    res_.copy_comp_yz(0, dive, 0);
    res_.axpy_comp_yz(0, -1., rho, 0);
    // FIXME, why is this necessary?
    bnd_mf_.fill_ghosts(res_, 0, 1);
  }

  // ----------------------------------------------------------------------
  // psc_marder_cuda_correct
  //
  // Do the modified marder correction (See eq.(5, 7, 9, 10) in Mardahl and Verboncoeur, CPC, 1997)

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
  void correct_patch(const Grid_t& grid, fields_view_t flds, fields_view_t f, int p,
		     real_t diffusion, real_t& max_err)
  {
    define_dxdydz(dx, dy, dz);

    // FIXME: how to choose diffusion parameter properly?
    //double deltax = ppsc->patch[p].dx[0];
    double deltay = grid.domain.dx[1]; // FIXME double/float
    double deltaz = grid.domain.dx[2];

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

#if 0
  void correct(MfieldsState& mflds)
  {
    auto& h_mflds = mflds.get_as<MfieldsStateSingle>(EX, EX + 3);
    auto& h_res = res_.get_as<MfieldsSingle>(0, 1);

    auto& grid = mflds.grid();
    double inv_sum = 0.;
    for (int d = 0; d < 3; d++) {
      if (!grid.isInvar(d)) {
	inv_sum += 1. / sqr(grid.domain.dx[d]);
      }
    }
    double diffusion_max = 1. / 2. / (.5 * grid.dt) / inv_sum;
    double diffusion     = diffusion_max * diffusion_;
    real_t max_err = 0.;
    for (int p = 0; p < h_res.n_patches(); p++) {
      correct_patch(h_mflds.grid(), h_mflds[p], h_res[p], p, diffusion, max_err);
    }
    res_.put_as(h_res, 0, 0);
    mflds.put_as(h_mflds, EX, EX + 3);

    MPI_Allreduce(MPI_IN_PLACE, &max_err, 1, MPI_FLOAT, MPI_MAX, grid_.comm());
    mpi_printf(grid_.comm(), "marder: err %g\n", max_err);
  }

#else
  void correct(MfieldsState& mflds)
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
    cuda_mfields *cmf = res_.cmflds();

    // OPT, do all patches in one kernel
    for (int p = 0; p < mflds.n_patches(); p++) {
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
  
  void operator()(MfieldsStateCuda& mflds, MparticlesCuda<BS>& mprts)
  {
    static int pr, pr_A, pr_B, pr_C, pr_D;
    if (!pr) {
      pr = prof_register("marder", 1., 0, 0);
      pr_A = prof_register("marder prep", 1., 0, 0);
      pr_B = prof_register("marder aid", 1., 0, 0);
      pr_C = prof_register("marder correct", 1., 0, 0);
      pr_D = prof_register("marder fillg", 1., 0, 0);
    }

    prof_start(pr);
    prof_start(pr_A);
    // need to fill ghost cells first (should be unnecessary with only variant 1) FIXME
    bnd_.fill_ghosts(mflds, EX, EX+3);

    item_rho_(mprts);
    auto &rho = item_rho_.result();
    prof_stop(pr_A);

    for (int i = 0; i < loop_; i++) {
      prof_start(pr_B);
      calc_aid_fields(mflds, rho);
      prof_stop(pr_B);
      prof_start(pr_C);
      correct(mflds);
      prof_stop(pr_C);
      prof_start(pr_D);
      bnd_.fill_ghosts(mflds, EX, EX+3);
      prof_stop(pr_D);
    }
    prof_stop(pr);
  }
  
private:
  const Grid_t& grid_;
  real_t diffusion_; //< diffusion coefficient for Marder correction
  int loop_; //< execute this many relaxation steps in a loop
  bool dump_; //< dump div_E, rho

  WriterMRC io_; //< for debug dumping

  Bnd_<MfieldsState> bnd_;
  Bnd_<Mfields> bnd_mf_;
  Mfields rho_;
  Mfields res_;
  
  Moment_rho_1st_nc_cuda<MparticlesCuda<BS144>, dim_yz> item_rho_; // FIXME, hardcoded dim_yz
  FieldsItemFields<Item_dive_cuda> item_dive_;
};

