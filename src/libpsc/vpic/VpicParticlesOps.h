
#pragma once

template<typename Particles, typename FieldArray, typename Interpolator, typename Accumulator, typename HydroArray>
struct VpicParticlesOps
{
  typedef typename Particles::ParticleBcList ParticleBcList;
  
  static void advance_p(Particles& vmprts, Accumulator& accumulator,
			Interpolator& interpolator)
  {
    for (auto sp = vmprts.begin(); sp != vmprts.end(); ++sp) {
      TIC ::advance_p(&*sp, &accumulator, &interpolator); TOC(advance_p, 1);
    }
  }
  
  static void boundary_p(const ParticleBcList &pbc_list, Particles& vmprts, FieldArray& fa,
			 Accumulator& accumulator)
  {
    const particle_bc_t *pbc = pbc_list;
    ::boundary_p(const_cast<particle_bc_t*>(pbc), vmprts.head(), &fa, &accumulator);
  }
  
  static void accumulate_rho_p(Particles& vmprts, FieldArray &vmflds)
  {
    for (auto sp = vmprts.begin(); sp != vmprts.end(); ++sp) {
      TIC ::accumulate_rho_p(&vmflds, &*sp); TOC(accumulate_rho_p, 1);
    }
  }

  static void accumulate_rhob(FieldArray& fa, const particle_t* p, float qsp)
  {
    ::accumulate_rhob(fa.f, p, fa.g, qsp);
  }

  // ----------------------------------------------------------------------
  // drop_p

  void drop_p(Particles& vmprts, FieldArray& vmflds)
  {
    for (auto sp = vmprts.begin(); sp != vmprts.end(); ++sp) {
      if (sp->nm) {
	LOG_WARN("Removing %i particles associated with unprocessed %s movers (increase num_comm_round)",
		 sp->nm, sp->name);
      }
      // Drop the particles that have unprocessed movers due to a user defined
      // boundary condition. Particles of this type with unprocessed movers are
      // in the list of particles and move_p has set the voxel in the particle to
      // 8*voxel + face. This is an incorrect voxel index and in many cases can
      // in fact go out of bounds of the voxel indexing space. Removal is in
      // reverse order for back filling. Particle charge is accumulated to the
      // mesh before removing the particle.
      int nm = sp->nm;
      particle_mover_t * RESTRICT ALIGNED(16)  pm = sp->pm + sp->nm - 1;
      particle_t * RESTRICT ALIGNED(128) p0 = sp->p;
      for (; nm; nm--, pm--) {
	int i = pm->i; // particle index we are removing
	p0[i].i >>= 3; // shift particle voxel down
	// accumulate the particle's charge to the mesh
	this->accumulate_rhob(vmflds, p0 + i, sp->q);
	p0[i] = p0[sp->np - 1]; // put the last particle into position i
	sp->np--; // decrement the number of particles
      }
      sp->nm = 0;
    }
  }

  static void accumulate_hydro_p(HydroArray& ha, typename Particles::const_iterator sp,
				 const Interpolator& interpolator)
  {
    ::accumulate_hydro_p(&ha, &*sp, &interpolator);
  }

  static void uncenter_p(species_t *sp, const Interpolator& interpolator)
  {
    ::uncenter_p(sp, &interpolator);
  }

  static void sort_p(species_t *sp)
  {
    ::sort_p(sp);
  }

  static double energy_p(typename Particles::const_iterator sp, const Interpolator& interpolator)
  {
    return ::energy_p(&*sp, &interpolator);
  }
};


template<class ParticlesBase>
struct VpicParticles : ParticlesBase
{
  typedef ParticlesBase Base;

  using Base::Base;
};

