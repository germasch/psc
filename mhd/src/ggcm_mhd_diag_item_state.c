
#include "ggcm_mhd_diag_item_private.h"

#include "ggcm_mhd_defs.h"
#include "ggcm_mhd_private.h"
#include "ggcm_mhd_diag_private.h"

#include <mrc_fld_as_double.h>
#include <mrc_domain.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

// ======================================================================
// ggcm_mhd_diag_item subclass "rr1"

// ----------------------------------------------------------------------
// ggcm_mhd_diag_item_rr1_run

static void
ggcm_mhd_diag_item_rr1_run(struct ggcm_mhd_diag_item *item,
			   struct mrc_io *io, struct mrc_fld *fld,
			   int diag_type, float plane)
{
  float scale_rr = 1.;
  ggcm_mhd_diag_c_write_one_field(io, fld, RR, "rr1", scale_rr, diag_type, plane);
}

// ----------------------------------------------------------------------
// ggcm_mhd_diag_item subclass "rr1"

struct ggcm_mhd_diag_item_ops ggcm_mhd_diag_item_ops_rr1 = {
  .name             = "rr1",
  .run              = ggcm_mhd_diag_item_rr1_run,
};

// ======================================================================
// ggcm_mhd_diag_item subclass "uu1"

// ----------------------------------------------------------------------
// ggcm_mhd_diag_item_uu1_run

static void
ggcm_mhd_diag_item_uu1_run(struct ggcm_mhd_diag_item *item,
			   struct mrc_io *io, struct mrc_fld *fld,
			   int diag_type, float plane)
{
  struct ggcm_mhd *mhd = item->diag->mhd;

  int mhd_type;
  mrc_fld_get_param_int(fld, "mhd_type", &mhd_type);

  struct mrc_fld *fld_r = mrc_domain_fld_create(mhd->domain, SW_2, "uu1");
  mrc_fld_set_type(fld_r, FLD_TYPE);
  mrc_fld_setup(fld_r);

  struct mrc_fld *r = mrc_fld_get_as(fld_r, FLD_TYPE);
  struct mrc_fld *f = mrc_fld_get_as(fld, FLD_TYPE);

  if (mhd_type == MT_SEMI_CONSERVATIVE_GGCM ||
      mhd_type == MT_SEMI_CONSERVATIVE) {
    mrc_fld_foreach(f, ix,iy,iz, 0, 0) {
      F3(r, 0, ix,iy,iz) = UU(f, ix,iy,iz);
    } mrc_fld_foreach_end;
  } else if (mhd_type == MT_FULLY_CONSERVATIVE) {
    mrc_fld_foreach(f, ix,iy,iz, 0, 0) {
      float b2  = (sqr(.5f * (BX(f, ix,iy,iz) + BX(f, ix+1,iy  ,iz  ))) +
		   sqr(.5f * (BY(f, ix,iy,iz) + BY(f, ix  ,iy+1,iz  ))) +
		   sqr(.5f * (BZ(f, ix,iy,iz) + BZ(f, ix  ,iy  ,iz+1))));
      F3(r, 0, ix,iy,iz) = EE(f, ix,iy,iz) -.5f * b2;
    } mrc_fld_foreach_end;
  } else {
    assert(0);
  }

  float max = 0.;
  mrc_fld_foreach(f, ix,iy,iz, 0, 0) {
    max = mrc_fld_max(max, mrc_fld_abs(F3(r, 0, ix,iy,iz)));
    if (!isfinite(UU(f, ix,iy,iz))) max = 9999.;
  } mrc_fld_foreach_end;
  mprintf("max uu1 = %g\n", max);

  mrc_fld_put_as(r, fld_r);
  mrc_fld_put_as(f, fld);

  float scale_uu = 1.;
  ggcm_mhd_diag_c_write_one_field(io, fld_r, 0, "uu1", scale_uu, diag_type, plane);
  mrc_fld_destroy(fld_r);
}

// ----------------------------------------------------------------------
// ggcm_mhd_diag_item subclass "uu1"

struct ggcm_mhd_diag_item_ops ggcm_mhd_diag_item_ops_uu1 = {
  .name             = "uu1",
  .run              = ggcm_mhd_diag_item_uu1_run,
};

// ======================================================================
// ggcm_mhd_diag_item subclass "rv1"

// ----------------------------------------------------------------------
// ggcm_mhd_diag_item_rv1_run

static void
ggcm_mhd_diag_item_rv1_run(struct ggcm_mhd_diag_item *item,
			   struct mrc_io *io, struct mrc_fld *fld,
			   int diag_type, float plane)
{
  float scale_rv = 1.;
  ggcm_mhd_diag_c_write_one_field(io, fld, RVX, "rv1x", scale_rv, diag_type, plane);
  ggcm_mhd_diag_c_write_one_field(io, fld, RVY, "rv1y", scale_rv, diag_type, plane);
  ggcm_mhd_diag_c_write_one_field(io, fld, RVZ, "rv1z", scale_rv, diag_type, plane);
}

// ----------------------------------------------------------------------
// ggcm_mhd_diag_item subclass "rv1"

struct ggcm_mhd_diag_item_ops ggcm_mhd_diag_item_ops_rv1 = {
  .name             = "rv1",
  .run              = ggcm_mhd_diag_item_rv1_run,
};

// ======================================================================
// ggcm_mhd_diag_item subclass "b1"

// ----------------------------------------------------------------------
// ggcm_mhd_diag_item_b1_run

static void
ggcm_mhd_diag_item_b1_run(struct ggcm_mhd_diag_item *item,
			  struct mrc_io *io, struct mrc_fld *fld,
			  int diag_type, float plane)
{
  float scale_bb = 1.;
  ggcm_mhd_diag_c_write_one_field(io, fld, BX, "b1x", scale_bb, diag_type, plane);
  ggcm_mhd_diag_c_write_one_field(io, fld, BY, "b1y", scale_bb, diag_type, plane);
  ggcm_mhd_diag_c_write_one_field(io, fld, BZ, "b1z", scale_bb, diag_type, plane);
}

// ----------------------------------------------------------------------
// ggcm_mhd_diag_item subclass "b1"

struct ggcm_mhd_diag_item_ops ggcm_mhd_diag_item_ops_b1 = {
  .name             = "b1",
  .run              = ggcm_mhd_diag_item_b1_run,
};

