
#include "bk_mparticles_iface.h"

#include "psc.h"
#include "psc_particles_single_by_kind.h"

#define PARTICLE_TYPE "single_by_kind"
#define PFX(x) psc_mparticles_single_by_kind_ ## x
#define psc_mparticles_sub psc_mparticles_single_by_kind

// ======================================================================
// psc_mparticles: subclass "single_by_kind"

static struct mrc_obj_method psc_mparticles_single_by_kind_methods[] = {
  {}
};

// ----------------------------------------------------------------------
// psc_mparticles_sub_setup

static void
PFX(setup)(struct psc_mparticles *_mprts)
{
  mparticles_single_by_kind_t mprts(_mprts);

  psc_mparticles_setup_super(_mprts);

  new(mprts.sub_) psc_mparticles_single_by_kind{ppsc->grid};

  mprts->bkmprts = bk_mparticles_new(mprts->n_patches());
}

static void
PFX(destroy)(struct psc_mparticles *mprts)
{
  bk_mparticles *bkmprts = psc_mparticles_sub(mprts)->bkmprts;

  bk_mparticles_delete(bkmprts);
}

static void
PFX(reserve_all)(struct psc_mparticles *mprts, uint *n_prts_by_patch)
{
  bk_mparticles *bkmprts = psc_mparticles_sub(mprts)->bkmprts;

  bk_mparticles_reserve_all(bkmprts, n_prts_by_patch);
}

static void
PFX(resize_all)(struct psc_mparticles *mprts, uint *n_prts_by_patch)
{
  bk_mparticles *bkmprts = psc_mparticles_sub(mprts)->bkmprts;

  bk_mparticles_resize_all(bkmprts, n_prts_by_patch);
}

static void
PFX(get_size_all)(struct psc_mparticles *mprts, uint *n_prts_by_patch)
{
  bk_mparticles *bkmprts = psc_mparticles_sub(mprts)->bkmprts;

  bk_mparticles_size_all(bkmprts, n_prts_by_patch);
}

static unsigned int
PFX(get_nr_particles)(struct psc_mparticles *mprts)
{
  bk_mparticles *bkmprts = psc_mparticles_sub(mprts)->bkmprts;

  return bk_mparticles_n_prts(bkmprts);
}

// ----------------------------------------------------------------------
// psc_mparticles_ops

struct PFX(ops) : psc_mparticles_ops {
  PFX(ops)() {
    name                    = PARTICLE_TYPE;
    size                    = sizeof(struct psc_mparticles_sub);
    methods                 = PFX(methods);
    setup                   = PFX(setup);
    destroy                 = PFX(destroy);
#if 0
    write                   = PFX(write);
    read                    = PFX(read);
#endif
    reserve_all             = PFX(reserve_all);
    resize_all              = PFX(resize_all);
    get_size_all            = PFX(get_size_all);
    get_nr_particles        = PFX(get_nr_particles);
  }
} PFX(ops);

