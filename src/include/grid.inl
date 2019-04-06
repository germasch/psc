
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
  using value_type = typename Grid_t::Domain;

  static void put(kg::io::Engine& writer, const value_type& domain,
                  const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    writer.put1("gdims", domain.gdims, launch);
    writer.put1("length", domain.length, launch);
    writer.put1("corner", domain.corner, launch);
    writer.put1("np", domain.np, launch);
    writer.put1("ldims", domain.ldims, launch);
    writer.put1("dx", domain.dx, launch);
  }

  static void get(Engine& reader, value_type& domain,
                  const Mode launch = Mode::Deferred)
  {
    reader.get1("gdims", domain.gdims, launch);
    reader.get1("length", domain.length, launch);
    reader.get1("corner", domain.corner, launch);
    reader.get1("np", domain.np, launch);
    reader.get1("ldims", domain.ldims, launch);
    reader.get1("dx", domain.dx, launch);
  }
};

// ======================================================================
// Variable<GridBc>

template <>
struct kg::io::Variable<GridBc>
{
  using value_type = GridBc;

  static void put(kg::io::Engine& writer, const value_type& bc,
                  const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    writer.put1("fld_lo", bc.fld_lo, launch);
    writer.put1("fld_hi", bc.fld_hi, launch);
    writer.put1("prt_lo", bc.prt_lo, launch);
    writer.put1("prt_hi", bc.prt_hi, launch);
  }

  static void get(Engine& reader, value_type& bc,
                  const Mode launch = Mode::Deferred)
  {
    reader.get1("fld_lo", bc.fld_lo, launch);
    reader.get1("fld_hi", bc.fld_hi, launch);
    reader.get1("prt_lo", bc.prt_lo, launch);
    reader.get1("prt_hi", bc.prt_hi, launch);
  }
};

// ======================================================================
// Variable<Normalization>

template <>
struct kg::io::Variable<Grid_t::Normalization>
{
  using value_type = Grid_t::Normalization;

  static void put(kg::io::Engine& writer, const Grid_t::Normalization& norm,
                  const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    writer.put1("cc", norm.cc, launch);
    writer.put1("fnqs", norm.fnqs, launch);
    writer.put1("eta", norm.eta, launch);
    writer.put1("beta", norm.beta, launch);
    writer.put1("cori", norm.cori, launch);
    writer.put1("b0", norm.b0, launch);
    writer.put1("rho0", norm.rho0, launch);
    writer.put1("phi0", norm.phi0, launch);
    writer.put1("a0", norm.a0, launch);
  }

  static void get(Engine& reader, Grid_t::Normalization& norm,
                  const Mode launch = Mode::Deferred)
  {
    reader.get1("cc", norm.cc, launch);
    reader.get1("fnqs", norm.fnqs, launch);
    reader.get1("eta", norm.eta, launch);
    reader.get1("beta", norm.beta, launch);
    reader.get1("cori", norm.cori, launch);
    reader.get1("b0", norm.b0, launch);
    reader.get1("rho0", norm.rho0, launch);
    reader.get1("phi0", norm.phi0, launch);
    reader.get1("a0", norm.a0, launch);
  }
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

    writer.put1("names", names, kg::io::Mode::Sync);
    writer.put1("q", q, kg::io::Mode::Sync);
    writer.put1("m", m, kg::io::Mode::Sync);
  }

  void get(Engine& reader, Grid_t::Kinds& kinds,
           const Mode launch = Mode::Deferred)
  {
    auto q = std::vector<real_t>{};
    auto m = std::vector<real_t>{};
    auto names = std::vector<std::string>{};
    reader.get1("names", names, kg::io::Mode::Sync);
    reader.get1("q", q, kg::io::Mode::Sync);
    reader.get1("m", m, kg::io::Mode::Sync);
    reader.get1("names", names);

    kinds.resize(q.size());
    for (int kind = 0; kind < q.size(); kind++) {
      kinds[kind].q = q[kind];
      kinds[kind].m = m[kind];
      kinds[kind].name = strdup(names[kind].c_str());
    }
  }
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
    writer.putVar("domain", grid.domain, launch);
    writer.putVar("bc", grid.bc, launch);
    writer.putVar("norm", grid.norm, launch);
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
    reader.getVar("domain", grid.domain, launch);
    reader.getVar("bc", grid.bc, launch);
    reader.getVar("norm", grid.norm, launch);
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
