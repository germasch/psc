
#include <array>

#include "kg/io.h"

namespace kg
{
namespace io
{

// ======================================================================
// VariableGlobalSingleValue<Vec3<T>>

template <typename T>
struct VariableGlobalSingleValue<Vec3<T>>
{
  using value_type = Vec3<T>;

  VariableGlobalSingleValue(const std::string& name, Engine& engine)
    : var_{engine.defineVariable<T>(name, {3}, {0}, {0})}
  // adios2 FIXME {3} {} {} gives no error, but problems
  {}

  void put(Engine& writer, const Vec3<T>& vec3,
           const Mode launch = Mode::Deferred)
  {
    if (writer.mpiRank() == 0) {
      var_.setSelection(
        {{0}, {3}}); // adios2 FIXME, would be nice to specify {}, {3}
      writer.put(var_, vec3.data(), launch);
    }
  };

  void get(Engine& reader, Vec3<T>& vec3, const Mode launch = Mode::Deferred)
  {
    var_.setSelection(
      {{0}, {3}}); // adios2 FIXME, would be nice to specify {}, {3}
    reader.get(var_, vec3.data(), launch);
  };

  explicit operator bool() const { return static_cast<bool>(var_); }

private:
  Variable<T> var_;
};

} // namespace io
} // namespace kg

// ======================================================================
// VariableByPatch

template <typename T>
struct VariableByPatch;

template <typename T>
struct VariableByPatch<Vec3<T>>
{
  using value_type = Vec3<T>;
  using is_adios_variable = std::false_type;

  VariableByPatch(const std::string& name, kg::io::Engine& engine)
    : var_{engine._defineVariable<T>(name, {1, 3}, {0, 0}, {0, 0})}
  {}

  void put(kg::io::Engine& writer, const value_type* data, const Grid_t& grid,
           const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    size_t patches_n_local = grid.n_patches();
    size_t patches_n_global = grid.nGlobalPatches();
    size_t patches_start = grid.localPatchInfo(0).global_patch;
    var_.setShape({patches_n_global, 3});
    var_.setSelection({{patches_start, 0}, {patches_n_local, 3}});
    writer.put(var_, data[0].data(), launch);
  }

  void get(kg::io::Engine& reader, value_type* data, const Grid_t& grid,
           const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    size_t patches_n_local = grid.n_patches();
    size_t patches_n_global = grid.nGlobalPatches();
    size_t patches_start = grid.localPatchInfo(0).global_patch;
    assert(var_.shape() == kg::io::Dims({patches_n_global, 3}));
    var_.setSelection({{patches_start, 0}, {patches_n_local, 3}});
    reader.get(var_, data[0].data(), launch);
  }

private:
  kg::io::detail::Variable<T> var_;
};

// ======================================================================
// Variable<Grid_t::Domain>

// FIXME, this should be templated by Grid_<T>::Domain, but can't do that...

template <>
struct kg::io::Variable<Grid_t::Domain>
{
  using Grid = Grid_t;
  using value_type = typename Grid::Domain;
  using is_adios_variable = std::false_type;

  using real_t = typename Grid::real_t;
  using Real3 = typename Grid::Real3;

  Variable(const std::string& name, kg::io::Engine& engine)
    : var_gdims_{name + ".gdims", engine},
      var_length_{name + ".length", engine},
      var_corner_{name + ".corner", engine},
      var_np_{name + ".np", engine},
      var_ldims_{name + ".ldims", engine},
      var_dx_{name + ".dx", engine}
  {}

  void put(kg::io::Engine& writer, const Grid::Domain& domain,
           const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    writer.put(var_gdims_, domain.gdims, launch);
    writer.put(var_length_, domain.length, launch);
    writer.put(var_corner_, domain.corner, launch);
    writer.put(var_np_, domain.np, launch);
    writer.put(var_ldims_, domain.ldims, launch);
    writer.put(var_dx_, domain.dx, launch);
  }

  void get(Engine& reader, Grid::Domain& domain,
           const Mode launch = Mode::Deferred)
  {
    reader.get(var_gdims_, domain.gdims, launch);
    reader.get(var_length_, domain.length, launch);
    reader.get(var_corner_, domain.corner, launch);
    reader.get(var_np_, domain.np, launch);
    reader.get(var_ldims_, domain.ldims, launch);
    reader.get(var_dx_, domain.dx, launch);
  }

private:
  kg::io::Attribute<Int3> var_gdims_;
  kg::io::Attribute<Real3> var_length_;
  kg::io::Attribute<Real3> var_corner_;
  kg::io::Attribute<Int3> var_np_;
  kg::io::Attribute<Int3> var_ldims_;
  kg::io::Attribute<Real3> var_dx_;
};

// ======================================================================
// Variable<GridBc>

template <>
struct kg::io::Variable<GridBc>
{
  using value_type = GridBc;
  using is_adios_variable = std::false_type;

  Variable(const std::string& name, kg::io::Engine& engine)
    : var_fld_lo_{name + ".fld_lo", engine},
      var_fld_hi_{name + ".fld_hi", engine},
      var_prt_lo_{name + ".prt_lo", engine},
      var_prt_hi_{name + ".prt_hi", engine}
  {}

  void put(kg::io::Engine& writer, const GridBc& bc,
           const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    writer.put(var_fld_lo_, bc.fld_lo, launch);
    writer.put(var_fld_hi_, bc.fld_hi, launch);
    writer.put(var_prt_lo_, bc.prt_lo, launch);
    writer.put(var_prt_hi_, bc.prt_hi, launch);
  }

  void get(Engine& reader, GridBc& bc, const Mode launch = Mode::Deferred)
  {
    reader.get(var_fld_lo_, bc.fld_lo, launch);
    reader.get(var_fld_hi_, bc.fld_hi, launch);
    reader.get(var_prt_lo_, bc.prt_lo, launch);
    reader.get(var_prt_hi_, bc.prt_hi, launch);
  }

private:
  kg::io::Attribute<Int3> var_fld_lo_;
  kg::io::Attribute<Int3> var_fld_hi_;
  kg::io::Attribute<Int3> var_prt_lo_;
  kg::io::Attribute<Int3> var_prt_hi_;
};

// ======================================================================
// Variable<Normalization>

template <>
struct kg::io::Variable<Grid_t::Normalization>
{
  using value_type = Grid_t::Normalization;
  using is_adios_variable = std::false_type;

  using real_t = Grid_t::real_t;

  Variable(const std::string& name, kg::io::Engine& engine)
    : var_cc_{name + ".cc", engine},
      var_fnqs_{name + ".fnqs", engine},
      var_eta_{name + ".eta", engine},
      var_beta_{name + ".beta", engine},
      var_cori_{name + ".cori", engine},
      var_b0_{name + ".b0", engine},
      var_rho0_{name + ".rho0", engine},
      var_phi0_{name + ".phi0", engine},
      var_a0_{name + ".a0", engine}
  {}

  void put(kg::io::Engine& writer, const Grid_t::Normalization& norm,
           const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    writer.put(var_cc_, norm.cc, launch);
    writer.put(var_fnqs_, norm.fnqs, launch);
    writer.put(var_eta_, norm.eta, launch);
    writer.put(var_beta_, norm.beta, launch);
    writer.put(var_cori_, norm.cori, launch);
    writer.put(var_b0_, norm.b0, launch);
    writer.put(var_rho0_, norm.rho0, launch);
    writer.put(var_phi0_, norm.phi0, launch);
    writer.put(var_a0_, norm.a0, launch);
  }

  void get(Engine& reader, Grid_t::Normalization& norm,
           const Mode launch = Mode::Deferred)
  {
    reader.get(var_cc_, norm.cc, launch);
    reader.get(var_fnqs_, norm.fnqs, launch);
    reader.get(var_eta_, norm.eta, launch);
    reader.get(var_beta_, norm.beta, launch);
    reader.get(var_cori_, norm.cori, launch);
    reader.get(var_b0_, norm.b0, launch);
    reader.get(var_rho0_, norm.rho0, launch);
    reader.get(var_phi0_, norm.phi0, launch);
    reader.get(var_a0_, norm.a0, launch);
  }

private:
  kg::io::Attribute<real_t> var_cc_;
  kg::io::Attribute<real_t> var_fnqs_;
  kg::io::Attribute<real_t> var_eta_;
  kg::io::Attribute<real_t> var_beta_;
  kg::io::Attribute<real_t> var_cori_;

  kg::io::Attribute<real_t> var_b0_;
  kg::io::Attribute<real_t> var_rho0_;
  kg::io::Attribute<real_t> var_phi0_;
  kg::io::Attribute<real_t> var_a0_;
};

// ======================================================================
// Variable<Grid::Kinds>

template <>
struct kg::io::Variable<Grid_t::Kinds>
{
  using value_type = Grid_t::Kinds;
  using is_adios_variable = std::false_type;
  using real_t = Grid_t::real_t;

  static const size_t NAME_LEN = 10;

  Variable(const std::string& name, kg::io::Engine& engine)
    : attr_q_{name + ".q", engine},
      attr_m_{name + ".m", engine},
      attr_names_{name + ".names", engine}
  {}

  void put(kg::io::Engine& writer, const Grid_t::Kinds& kinds,
           const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    auto n_kinds = kinds.size();
    auto names = std::vector<std::string>(n_kinds);
    auto q = std::vector<real_t>(n_kinds);
    auto m = std::vector<real_t>(n_kinds);
    for (int kind = 0; kind < n_kinds; kind++) {
      q[kind] = kinds[kind].q;
      m[kind] = kinds[kind].m;
      names[kind] = kinds[kind].name;
    }

    writer.put(attr_names_, names);
    writer.put(attr_q_, q);
    writer.put(attr_m_, m);
    writer.performPuts();
  }

  void get(Engine& reader, Grid_t::Kinds& kinds,
           const Mode launch = Mode::Deferred)
  {
    auto q = std::vector<real_t>{};
    auto m = std::vector<real_t>{};
    auto names = std::vector<std::string>{};
    reader.get(attr_names_, names);
    reader.get(attr_q_, q);
    reader.get(attr_m_, m);
    reader.performGets();

    kinds.resize(q.size());
    for (int kind = 0; kind < q.size(); kind++) {
      kinds[kind].q = q[kind];
      kinds[kind].m = m[kind];
      kinds[kind].name = strdup(names[kind].c_str());
    }
  }

private:
  kg::io::Attribute<std::vector<real_t>> attr_q_;
  kg::io::Attribute<std::vector<real_t>> attr_m_;
  kg::io::Attribute<std::vector<std::string>> attr_names_;
};

// ======================================================================
// Variable<Grid_<T>>

template <typename T>
struct kg::io::Variable<Grid_<T>>
{
  using Grid = Grid_<T>;
  using value_type = Grid;
  using is_adios_variable = std::false_type;

  using real_t = typename Grid::real_t;
  using Real3 = typename Grid::Real3;

  Variable(const std::string& name, kg::io::Engine& engine)
    : var_ldims_{name + ".ldims", engine},
      var_domain_{name + ".domain", engine},
      var_bc_{name + ".bc", engine},
      var_norm_{name + ".norm", engine},
      var_dt_{name + ".dt", engine},
      var_patches_n_local_{name + ".patches.n_local", engine},
      var_patches_off_{name + ".patches.off", engine},
      var_patches_xb_{name + ".patches.xb", engine},
      var_patches_xe_{name + ".patches.xe", engine},
      var_kinds_{name + ".kinds", engine},
      var_ibn_{name + ".ibn", engine},
      var_timestep_{name + ".timestep", engine}
  {}

  void put(kg::io::Engine& writer, const Grid& grid,
           const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    writer.put(var_ldims_, grid.ldims);
    writer.put(var_domain_, grid.domain, launch);
    writer.put(var_bc_, grid.bc, launch);
    writer.put(var_norm_, grid.norm, launch);
    writer.put(var_dt_, grid.dt);

    size_t patches_n_local = grid.patches.size();
    writer.put(var_patches_n_local_, patches_n_local);

    auto patches_off = std::vector<Int3>(patches_n_local);
    auto patches_xb = std::vector<Real3>(patches_n_local);
    auto patches_xe = std::vector<Real3>(patches_n_local);
    for (int p = 0; p < patches_n_local; p++) {
      auto& patch = grid.patches[p];
      patches_off[p] = patch.off;
      patches_xb[p] = patch.xb;
      patches_xe[p] = patch.xe;
    }

    writer.put(var_patches_off_, patches_off.data(), grid, launch);
    writer.put(var_patches_xb_, patches_xb.data(), grid, launch);
    writer.put(var_patches_xe_, patches_xe.data(), grid, launch);

    writer.put(var_kinds_, grid.kinds, launch);
    writer.put(var_ibn_, grid.ibn);
    writer.put(var_timestep_, grid.timestep_);

    writer.performPuts(); // because we're writing temp local vars (the
                          // patches_*)
  }

  void get(kg::io::Engine& reader, Grid& grid,
           const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    reader.get(var_ldims_, grid.ldims);
    reader.get(var_domain_, grid.domain, launch);
    reader.get(var_bc_, grid.bc, launch);
    reader.get(var_norm_, grid.norm, launch);
    reader.get(var_dt_, grid.dt);

    int patches_n_local;
    reader.get(var_patches_n_local_, patches_n_local, launch);

    reader.performGets(); // need patches_n_local, domain, bc to be read
    grid.mrc_domain_ =
      grid.make_mrc_domain(grid.domain, grid.bc, patches_n_local);

    grid.patches.resize(patches_n_local);
    auto patches_off = std::vector<Int3>(patches_n_local);
    auto patches_xb = std::vector<Real3>(patches_n_local);
    auto patches_xe = std::vector<Real3>(patches_n_local);
    reader.get(var_patches_off_, patches_off.data(), grid, launch);
    reader.get(var_patches_xb_, patches_xb.data(), grid, launch);
    reader.get(var_patches_xe_, patches_xe.data(), grid, launch);

    reader.performGets(); // need to actually read the temp local vars
    for (int p = 0; p < patches_n_local; p++) {
      auto& patch = grid.patches[p];
      patch.off = patches_off[p];
      patch.xb = patches_xb[p];
      patch.xe = patches_xe[p];
    }

    reader.get(var_kinds_, grid.kinds, launch);
    reader.get(var_ibn_, grid.ibn);
    reader.get(var_timestep_, grid.timestep_);
  }

private:
  kg::io::Attribute<Int3> var_ldims_;
  kg::io::Variable<typename Grid::Domain> var_domain_;
  kg::io::Variable<GridBc> var_bc_;
  kg::io::Variable<typename Grid::Normalization> var_norm_;
  kg::io::Attribute<real_t> var_dt_;

  kg::io::VariableLocalSingleValue<int>
    var_patches_n_local_; // FIXME, should be size_t, adios2 bug?
  VariableByPatch<Int3> var_patches_off_;
  VariableByPatch<Real3> var_patches_xb_;
  VariableByPatch<Real3> var_patches_xe_;

  kg::io::Variable<typename Grid::Kinds> var_kinds_;
  kg::io::Attribute<Int3> var_ibn_;
  kg::io::Attribute<int> var_timestep_;
  // the mrc_domain_ member is not written and handled specially on read
};
