
#pragma once

template<typename Mparticles>
struct ChecksVpic : ChecksParams, ChecksBase
{
  ChecksVpic(const Grid_t& grid, MPI_Comm comm, const ChecksParams& params)
    : ChecksParams(params)
  {}
  
  void continuity_before_particle_push(Mparticles& mprts) {}
  void continuity_after_particle_push(Mparticles& mprts, MfieldsState& mflds) {}
  void gauss(Mparticles& mprts, MfieldsState& mflds) {}

  void continuity_before_particle_push(MparticlesBase& mprts) override {}
  void continuity_after_particle_push(MparticlesBase& mprts, MfieldsStateBase& mflds) override {}
  void gauss(MparticlesBase& mprts, MfieldsStateBase& mflds) override {}
};
