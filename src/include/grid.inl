
#include <array>

#include "kg/io.h"

// ======================================================================
// VariableByPatch

template <typename T>
struct VariableByPatch;

template <typename T>
struct VariableByPatch<std::vector<Vec3<T>>>
{
  using value_type = std::vector<Vec3<T>>;

  VariableByPatch(const std::string& name, kg::io::Engine& engine) {}

  void put(kg::io::Engine& writer, const value_type& datum, const Grid_t& grid,
           const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    kg::io::Dims shape = {static_cast<size_t>(grid.nGlobalPatches()), 3};
    kg::io::Dims start = {
      static_cast<size_t>(grid.localPatchInfo(0).global_patch), 0};
    kg::io::Dims count = {static_cast<size_t>(grid.n_patches()), 3};
    auto var = writer.makeVariable<T>(shape);
    var.setSelection({start, count});
    var.put(writer, datum[0].data(), launch);
  }

  void get(kg::io::Engine& reader, value_type& datum, const Grid_t& grid,
           const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    kg::io::Dims shape = {static_cast<size_t>(grid.nGlobalPatches()), 3};
    kg::io::Dims start = {
      static_cast<size_t>(grid.localPatchInfo(0).global_patch), 0};
    kg::io::Dims count = {static_cast<size_t>(grid.n_patches()), 3};
    auto var = reader.makeVariable<T>();
    assert(var.shape() == shape);
    var.setSelection({start, count});
    datum.resize(count[0]);
    var.get(reader, datum[0].data(), launch);
  }
};

// ======================================================================
// Variable<Grid_t::Domain>

// FIXME, this should be templated by Grid_<T>::Domain, but can't do that...

template <>
class kg::io::Descr<Grid_t::Domain>
{
public:
  using value_type = typename Grid_t::Domain;

  Descr(const std::string& pfx, Engine& engine) {}

  static void put(kg::io::Engine& writer, const value_type& domain,
                  const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    writer.putAttribute("gdims", domain.gdims, launch);
    writer.putAttribute("length", domain.length, launch);
    writer.putAttribute("corner", domain.corner, launch);
    writer.putAttribute("np", domain.np, launch);
    writer.putAttribute("ldims", domain.ldims, launch);
    writer.putAttribute("dx", domain.dx, launch);
  }

  static void get(Engine& reader, value_type& domain,
                  const Mode launch = Mode::Deferred)
  {
    reader.getAttribute("gdims", domain.gdims, launch);
    reader.getAttribute("length", domain.length, launch);
    reader.getAttribute("corner", domain.corner, launch);
    reader.getAttribute("np", domain.np, launch);
    reader.getAttribute("ldims", domain.ldims, launch);
    reader.getAttribute("dx", domain.dx, launch);
  }
};

// ======================================================================
// Variable<GridBc>

template <>
class kg::io::Descr<GridBc>
{
public:
  using value_type = GridBc;

  Descr(const std::string& pfx, Engine& engine) {}

  static void put(kg::io::Engine& writer, const value_type& bc,
                  const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    writer.putAttribute("fld_lo", bc.fld_lo, launch);
    writer.putAttribute("fld_hi", bc.fld_hi, launch);
    writer.putAttribute("prt_lo", bc.prt_lo, launch);
    writer.putAttribute("prt_hi", bc.prt_hi, launch);
  }

  static void get(Engine& reader, value_type& bc,
                  const Mode launch = Mode::Deferred)
  {
    reader.getAttribute("fld_lo", bc.fld_lo, launch);
    reader.getAttribute("fld_hi", bc.fld_hi, launch);
    reader.getAttribute("prt_lo", bc.prt_lo, launch);
    reader.getAttribute("prt_hi", bc.prt_hi, launch);
  }
};

// ======================================================================
// Variable<Normalization>

template <>
class kg::io::Descr<Grid_t::Normalization>
{
public:
  using value_type = Grid_t::Normalization;

  Descr(const std::string& pfx, Engine& engine) {}

  static void put(kg::io::Engine& writer, const Grid_t::Normalization& norm,
                  const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    writer.putAttribute("cc", norm.cc, launch);
    writer.putAttribute("fnqs", norm.fnqs, launch);
    writer.putAttribute("eta", norm.eta, launch);
    writer.putAttribute("beta", norm.beta, launch);
    writer.putAttribute("cori", norm.cori, launch);
    writer.putAttribute("b0", norm.b0, launch);
    writer.putAttribute("rho0", norm.rho0, launch);
    writer.putAttribute("phi0", norm.phi0, launch);
    writer.putAttribute("a0", norm.a0, launch);
  }

  static void get(Engine& reader, Grid_t::Normalization& norm,
                  const Mode launch = Mode::Deferred)
  {
    reader.getAttribute("cc", norm.cc, launch);
    reader.getAttribute("fnqs", norm.fnqs, launch);
    reader.getAttribute("eta", norm.eta, launch);
    reader.getAttribute("beta", norm.beta, launch);
    reader.getAttribute("cori", norm.cori, launch);
    reader.getAttribute("b0", norm.b0, launch);
    reader.getAttribute("rho0", norm.rho0, launch);
    reader.getAttribute("phi0", norm.phi0, launch);
    reader.getAttribute("a0", norm.a0, launch);
  }
};

// ======================================================================
// Variable<Grid::Kinds>

template <>
class kg::io::Descr<Grid_t::Kinds>
{
  using real_t = Grid_t::real_t;

public:
  Descr(const std::string& pfx, Engine& engine) {}

  using value_type = Grid_t::Kinds;

  static void put(kg::io::Engine& writer, const Grid_t::Kinds& kinds,
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

    writer.putAttribute("names", names, kg::io::Mode::Sync);
    writer.putAttribute("q", q, kg::io::Mode::Sync);
    writer.putAttribute("m", m, kg::io::Mode::Sync);
  }

  static void get(Engine& reader, Grid_t::Kinds& kinds,
                  const Mode launch = Mode::Deferred)
  {
    auto q = std::vector<real_t>{};
    auto m = std::vector<real_t>{};
    auto names = std::vector<std::string>{};
    reader.getAttribute("names", names, kg::io::Mode::Sync);
    reader.getAttribute("q", q, kg::io::Mode::Sync);
    reader.getAttribute("m", m, kg::io::Mode::Sync);
    reader.getAttribute("names", names);

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
class kg::io::Descr<Grid_<T>>
{
  using Grid = Grid_<T>;
  using real_t = typename Grid::real_t;
  using Real3 = typename Grid::Real3;

public:
  using value_type = Grid;

  Descr(const std::string& name, kg::io::Engine& engine) {}

  void put(kg::io::Engine& writer, const Grid& grid,
           const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    writer.putAttribute("ldims", grid.ldims);
    writer.put("domain", grid.domain, launch);
    writer.put("bc", grid.bc, launch);
    writer.put("norm", grid.norm, launch);
    writer.putAttribute("dt", grid.dt);

    size_t patches_n_local = grid.patches.size();
    writer.putLocal("n_local", patches_n_local);

    auto patches_off = std::vector<Int3>(patches_n_local);
    auto patches_xb = std::vector<Real3>(patches_n_local);
    auto patches_xe = std::vector<Real3>(patches_n_local);
    for (int p = 0; p < patches_n_local; p++) {
      auto& patch = grid.patches[p];
      patches_off[p] = patch.off;
      patches_xb[p] = patch.xb;
      patches_xe[p] = patch.xe;
    }

    writer.put<VariableByPatch>("off", patches_off, grid, launch);
    writer.put<VariableByPatch>("xb", patches_xb, grid, launch);
    writer.put<VariableByPatch>("xe", patches_xe, grid, launch);

    writer.put("kinds", grid.kinds, launch);
    writer.putAttribute("ibn", grid.ibn);
    writer.putAttribute("timestep", grid.timestep_);

    writer.performPuts(); // because we're writing temp local vars (the
                          // patches_*)
  }

  void get(kg::io::Engine& reader, Grid& grid,
           const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    reader.getAttribute("ldims", grid.ldims);
    reader.get("domain", grid.domain, launch);
    reader.get("bc", grid.bc, launch);
    reader.get("norm", grid.norm, launch);
    reader.getAttribute("dt", grid.dt);

    size_t patches_n_local;
    reader.getLocal("n_local", patches_n_local, launch);

    reader.performGets(); // need patches_n_local, domain, bc to be read
    grid.mrc_domain_ =
      grid.make_mrc_domain(grid.domain, grid.bc, patches_n_local);

    grid.patches.resize(patches_n_local);
    auto patches_off = std::vector<Int3>(patches_n_local);
    auto patches_xb = std::vector<Real3>(patches_n_local);
    auto patches_xe = std::vector<Real3>(patches_n_local);
    reader.get<VariableByPatch>("off", patches_off, grid, launch);
    reader.get<VariableByPatch>("xb", patches_xb, grid, launch);
    reader.get<VariableByPatch>("xe", patches_xe, grid, launch);

    reader.performGets(); // need to actually read the temp local vars
    for (int p = 0; p < patches_n_local; p++) {
      auto& patch = grid.patches[p];
      patch.off = patches_off[p];
      patch.xb = patches_xb[p];
      patch.xe = patches_xe[p];
    }

    reader.get("kinds", grid.kinds, launch);
    reader.getAttribute("ibn", grid.ibn);
    reader.getAttribute("timestep", grid.timestep_);
  }
};
