
#include <array>

namespace kg
{

using Mode = adios2::Mode;
using Dims = adios2::Dims;
template<typename T>
using Box = adios2::Box<T>;
  
struct IO;
struct Engine;

template<typename T>
struct Variable;

template<typename T, typename Enable = void>
struct Attribute;

// ======================================================================
// Engine

struct Engine
{
  Engine(adios2::Engine engine, adios2::IO io, MPI_Comm comm)
    : engine_{engine}, io_{io}
  {
    MPI_Comm_rank(comm, &mpi_rank_);
    MPI_Comm_size(comm, &mpi_size_);
  }

  // ----------------------------------------------------------------------
  // put for adios2 variables
  
  template<typename T>
  void put(adios2::Variable<T> variable, const T* data, const Mode launch = Mode::Deferred)
  {
    engine_.Put(variable, data, launch);
  }
  
  template<typename T>
  void put(adios2::Variable<T> variable, const T& datum, const Mode launch = Mode::Deferred)
  {
    engine_.Put(variable, datum, launch);
  }
  
  // ----------------------------------------------------------------------
  // put in general
  
  template<class T, class... Args>
  void put(T& variable, Args&&... args)
  {
    variable.put(*this, std::forward<Args>(args)...);
  }

  // ----------------------------------------------------------------------
  // get for adios2 variables
  
  template<typename T>
  void get(adios2::Variable<T> variable, T& datum, const Mode launch = Mode::Deferred)
  {
    engine_.Get(variable, datum, launch);
  }
  
  template<typename T>
  void get(adios2::Variable<T> variable, T* data, const Mode launch = Mode::Deferred)
  {
    engine_.Get(variable, data, launch);
  }
  
  template<typename T>
  void get(adios2::Variable<T> variable, std::vector<T>& data, const Mode launch = Mode::Deferred)
  {
    engine_.Get(variable, data, launch);
  }
  
  // ----------------------------------------------------------------------
  // get in general
  
  template <class T, class... Args>
  void get(T& variable, Args&&... args)
  {
    variable.get(*this, std::forward<Args>(args)...);
  }

  // ----------------------------------------------------------------------
  // performPuts
  
  void performPuts()
  {
    engine_.PerformPuts();
  }
  
  // ----------------------------------------------------------------------
  // performGets
  
  void performGets()
  {
    engine_.PerformGets();
  }
  
  // ----------------------------------------------------------------------
  // close
  
  void close()
  {
    engine_.Close();
  }

  template<typename T>
  void putAttribute(const std::string& name, const T* data, size_t size)
  {
    auto attr = io_.InquireAttribute<T>(name);
    if (attr) {
      mprintf("attr '%s' already exists -- ignoring it!", name.c_str());
    } else {
      io_.DefineAttribute<T>(name, data, size);
    }
  }
  
  template<typename T>
  void getAttribute(const std::string& name, std::vector<T>& data)
  {
    auto attr = io_.InquireAttribute<T>(name);
    assert(attr);
    data = attr.Data();
  }
  
  int mpiRank() const { return mpi_rank_; }
  int mpiSize() const { return mpi_size_; }

private:
  adios2::Engine engine_;
  adios2::IO io_;
  int mpi_rank_;
  int mpi_size_;
};

// ======================================================================
// IO

struct IO
{
  IO(adios2::ADIOS& ad, const char* name)
    : io_{ad.DeclareIO(name)}
  {}

  Engine open(const std::string& name, const adios2::Mode mode)
  {
     // FIXME, assumes that the ADIOS2 object underlying io_ was created on MPI_COMM_WORLD
    auto comm = MPI_COMM_WORLD;
    
    return {io_.Open(name, mode), io_, comm};
  }

  template<typename T,
	   typename std::enable_if<Variable<T>::is_adios_variable::value, int>::type = 0>
  Variable<T> defineVariable(const std::string &name, const Dims &shape = Dims(),
			     const Dims &start = Dims(), const Dims &count = Dims(),
			     const bool constantDims = false)
  {
    auto var = io_.InquireVariable<T>(name);
    if (var) {
      return var;
    } else {
      return io_.DefineVariable<T>(name, shape, start, count, constantDims);
    }
  }
  
  template<typename T,
	   typename std::enable_if<!Variable<T>::is_adios_variable::value, int>::type = 0>
  Variable<T> defineVariable(const std::string &name, const Dims &shape = Dims(),
		 const Dims &start = Dims(), const Dims &count = Dims(),
		 const bool constantDims = false)
  {
    return {name, *this};
  }

  template <class T>
  Variable<T> inquireVariable(const std::string &name)
  {
    return io_.InquireVariable<T>(name);
  }
  
private:
  adios2::IO io_;
};

// ======================================================================
// Variable
//
// This general version handles T being one of the base adios2 types (only!)

template<typename T>
struct Variable
{
  using value_type = T;
  using is_adios_variable = std::true_type;
  
  Variable(adios2::Variable<T> var)
    : var_{var}
  {}

  void put(Engine& writer, const T datum, const Mode launch = Mode::Deferred)
  {
    writer.put(var_, datum, launch);
  }
  
  void put(Engine& writer, const T* data, const Mode launch = Mode::Deferred)
  {
    writer.put(var_, data, launch);
  }
  
  void get(Engine& reader, T& datum, const Mode launch = Mode::Deferred)
  {
    reader.get(var_, datum, launch);
  }

  void get(Engine& reader, T* data, const Mode launch = Mode::Deferred)
  {
    reader.get(var_, data, launch);
  }

  void get(Engine& reader, std::vector<T>& data, const Mode launch = Mode::Deferred)
  {
    reader.get(var_, data, launch);
  }

  explicit operator bool() const { return static_cast<bool>(var_); }
  
  void setSelection(const Box<Dims>& selection)
  {
    var_.SetSelection(selection);
  }

  void setShape(const Dims &shape)
  {
    var_.SetShape(shape);
  }

  Dims shape() const
  {
    return var_.Shape();
  }

private:
  adios2::Variable<T> var_;
};
  
// ======================================================================
// VariableGlobalSingleValue

template<typename T>
struct VariableGlobalSingleValue
{
  using value_type = T;
  using is_adios_variable = std::false_type;

  VariableGlobalSingleValue(const std::string& name, IO& io)
    : var_{io.defineVariable<T>(name)}
  {}
  
  void put(Engine& writer, const T datum, const Mode launch = Mode::Deferred)
  {
    if (writer.mpiRank() == 0) {
      writer.put(var_, datum, launch);
    }
  }
  
  void get(Engine& reader, T& val, const Mode launch = Mode::Deferred)
  {
    reader.get(var_, val, launch);
  }

  explicit operator bool() const { return static_cast<bool>(var_); }
  
private:
  Variable<T> var_;
};
  
template<typename T>
struct VariableGlobalSingleValue<Vec3<T>>
{
  using value_type = Vec3<T>;
  
  VariableGlobalSingleValue(const std::string& name, IO& io)
    : var_{io.defineVariable<T>(name, {3}, {0}, {0})} // adios2 FIXME {3} {} {} gives no error, but problems
  {}

  void put(Engine& writer, const Vec3<T>& vec3, const Mode launch = Mode::Deferred)
  {
    if (writer.mpiRank() == 0) {
      var_.setSelection({{0}, {3}}); // adios2 FIXME, would be nice to specify {}, {3}
      writer.put(var_, vec3.data(), launch);
    }
  };

  void get(Engine& reader, Vec3<T>& vec3, const Mode launch = Mode::Deferred)
  {
    var_.setSelection({{0}, {3}}); // adios2 FIXME, would be nice to specify {}, {3}
    reader.get(var_, vec3.data(), launch);
  };
  
  explicit operator bool() const { return static_cast<bool>(var_); }

private:
  Variable<T> var_;
};

// ======================================================================
// VariableGlobalSingleArray

template<typename T>
struct VariableGlobalSingleArray
{
  using value_type = T;
  using is_adios_variable = std::false_type;

  VariableGlobalSingleArray(const std::string& name, IO& io)
    : var_{io.defineVariable<T>(name, {1}, {0}, {1})}
  {}

  void put(Engine& writer, const T* data, const Dims& shape, const Mode launch = Mode::Deferred)
  {
    var_.setShape(shape);
    if (writer.mpiRank() == 0) {
      var_.setSelection({Dims(shape.size()), shape});
      writer.put(var_, data, launch);
    }
  }

  void put(Engine& writer, const std::vector<T>& vec, const Mode launch = Mode::Deferred)
  {
    put(writer, vec.data(), {vec.size()});
  }
  
  void get(Engine& reader, T* data, const Mode launch = Mode::Deferred)
  {
    // FIXME, without a setSelection, is it guaranteed that the default selection is {{}, shape}?
    reader.get(var_, data, launch);
  }

  void get(Engine& reader, std::vector<T>& data, const Mode launch = Mode::Deferred)
  {
    // FIXME, without a setSelection, is it guaranteed that the default selection is {{}, shape}?
    reader.get(var_, data, launch);
  }

  Dims shape() const
  {
    return var_.shape();
  }

  explicit operator bool() const { return static_cast<bool>(var_); }
  
private:
  Variable<T> var_;
};

namespace detail
{
  
// ======================================================================
// Attribute
//
// Handles T being one of the base adios2 types (or arrays thereof)

template<typename T>
struct Attribute
{
  Attribute(const std::string& name, IO& io)
    : name_{name}
  {}

  void put(Engine& writer, const T* data, size_t size)
  {
    if (writer.mpiRank() == 0) { // FIXME, should I do this?
      writer.putAttribute(name_, data, size);
    }
  }

  void get(Engine& reader, std::vector<T>& data)
  {
    reader.getAttribute(name_, data);
  }

protected:
  const std::string name_;
};

};

template <typename T, typename Enable>
class Attribute;

// ======================================================================
// Attribute<std::vector>

namespace detail
{

template <typename T>
struct is_vector : std::false_type {};

template <typename T>
struct is_vector<std::vector<T>> : std::true_type {};

};

template <class T>
class Attribute<T, typename std::enable_if<detail::is_vector<T>::value>::type>
{
  using DataType = typename T::value_type;
public:
  using value_type = T;

  Attribute(const std::string& name, IO& io)
    : attr_{name, io}
  {}

  void put(Engine& writer, const T& vec)
  {
    attr_.put(writer, vec.data(), vec.size());
  }

  void get(Engine& reader, T& vec)
  {
    attr_.get(reader, vec);
  }

private:
  detail::Attribute<DataType> attr_;
};
  
// ======================================================================
// Attribute<Vec3>

namespace detail
{

template <typename T>
struct is_Vec3 : std::false_type {};

template <typename T>
struct is_Vec3<Vec3<T>> : std::true_type {};

};

template <class T>
class Attribute<T, typename std::enable_if<detail::is_Vec3<T>::value>::type>
{
  using DataType = typename T::value_type;
public:
  using value_type = T;

  Attribute(const std::string& name, IO& io)
    : attr_{name, io}
  {}

  void put(Engine& writer, const T& vec)
  {
    attr_.put(writer, vec.data(), 3);
  }
  
  void get(Engine& reader, T& data)
  {
    std::vector<DataType> vals;
    attr_.get(reader, vals);
    data = {vals[0], vals[1], vals[2]};
  }

private:
  detail::Attribute<DataType> attr_;
};
  
// ======================================================================
// VariableLocalSingleValue

template<typename T>
struct VariableLocalSingleValue
{
  using value_type = T;
  using is_adios_variable = std::false_type;

  VariableLocalSingleValue(const std::string& name, IO& io)
    : var_{io.defineVariable<T>(name, {adios2::LocalValueDim})}
  {}
  
  void put(Engine& writer, const T datum, const Mode launch = Mode::Deferred)
  {
    writer.put(var_, datum, launch);
  }
  
  void get(Engine& reader, T& val, const Mode launch = Mode::Deferred)
  {
    auto shape = var_.shape();
    assert(shape.size() == 1);
    auto dim0 = shape[0];
    assert(dim0 == reader.mpiSize());
    
    // FIXME, setSelection doesn't work, so read the whole thing
    std::vector<T> vals(shape[0]);
    reader.get(var_, vals.data(), launch);
    //for (auto val : vals) mprintf("val %d\n", val);
    val = vals[reader.mpiRank()];
  }

  Dims shape() const
  {
    return var_.shape();
  }

  explicit operator bool() const { return static_cast<bool>(var_); }
  
private:
  Variable<T> var_;
};
  
};

// ======================================================================
// VariableByPatch

template<typename T>
struct VariableByPatch;

template<typename T>
struct VariableByPatch<Vec3<T>>
{
  using value_type = Vec3<T>;
  using is_adios_variable = std::false_type;

  VariableByPatch(const std::string& name, kg::IO& io)
    : var_{io.defineVariable<T>(name, {1, 3}, {0, 0}, {0, 0})}
  {}
  
  void put(kg::Engine& writer, const value_type* data, const Grid_t& grid,
	   const kg::Mode launch = kg::Mode::Deferred)
  {
    size_t patches_n_local = grid.n_patches();
    size_t patches_n_global = grid.nGlobalPatches();
    size_t patches_start = grid.localPatchInfo(0).global_patch;
    var_.setShape({patches_n_global, 3});
    var_.setSelection({{patches_start, 0}, {patches_n_local, 3}});
    writer.put(var_, data[0].data(), launch);
  }
  
  void get(kg::Engine& reader, value_type* data, const Grid_t& grid,
	   const kg::Mode launch = kg::Mode::Deferred)
  {
    size_t patches_n_local = grid.n_patches();
    size_t patches_n_global = grid.nGlobalPatches();
    size_t patches_start = grid.localPatchInfo(0).global_patch;
    assert(var_.shape() == kg::Dims({patches_n_global, 3}));
    var_.setSelection({{patches_start, 0}, {patches_n_local, 3}});
    reader.get(var_, data[0].data(), launch);
  }
  
private:
  kg::Variable<T> var_;
};
  
// ======================================================================
// Variable<Grid_t::Domain>

// FIXME, this should be templated by Grid_<T>::Domain, but can't do that...

template<>
struct kg::Variable<Grid_t::Domain>
{
  using Grid = Grid_t;
  using value_type = typename Grid::Domain;
  using is_adios_variable = std::false_type;

  using real_t = typename Grid::real_t;
  using Real3 = typename Grid::Real3;

  Variable(const std::string& name, kg::IO& io)
    : var_gdims_{name + ".gdims", io},
      var_length_{name + ".length", io},
      var_corner_{name + ".corner", io},
      var_np_{name + ".np", io},
      var_ldims_{name + ".ldims", io},
      var_dx_{name + ".dx", io}
  {}

  void put(kg::Engine& writer, const Grid::Domain& domain, const kg::Mode launch = kg::Mode::Deferred)
  {
    writer.put(var_gdims_, domain.gdims, launch);
    writer.put(var_length_, domain.length, launch);
    writer.put(var_corner_, domain.corner, launch);
    writer.put(var_np_, domain.np, launch);
    writer.put(var_ldims_, domain.ldims, launch);
    writer.put(var_dx_, domain.dx, launch);
  }

  void get(Engine& reader, Grid::Domain& domain, const Mode launch = Mode::Deferred)
  {
    reader.get(var_gdims_, domain.gdims, launch);
    reader.get(var_length_, domain.length, launch);
    reader.get(var_corner_, domain.corner, launch);
    reader.get(var_np_, domain.np, launch);
    reader.get(var_ldims_, domain.ldims, launch);
    reader.get(var_dx_, domain.dx, launch);
  }

  explicit operator bool() const
  {
    return (var_gdims_ && var_length_ && var_corner_ &&
	    var_np_ && var_ldims_ && var_dx_);
  }

private:
  kg::VariableGlobalSingleValue<Int3> var_gdims_;
  kg::VariableGlobalSingleValue<Real3> var_length_;
  kg::VariableGlobalSingleValue<Real3> var_corner_;
  kg::VariableGlobalSingleValue<Int3> var_np_;
  kg::VariableGlobalSingleValue<Int3> var_ldims_;
  kg::VariableGlobalSingleValue<Real3> var_dx_;
};

// ======================================================================
// Variable<GridBc>

template<>
struct kg::Variable<GridBc>
{
  using value_type = GridBc;
  using is_adios_variable = std::false_type;

  Variable(const std::string& name, kg::IO& io)
    : var_fld_lo_{name + ".fld_lo", io},
      var_fld_hi_{name + ".fld_hi", io},
      var_prt_lo_{name + ".prt_lo", io},
      var_prt_hi_{name + ".prt_hi", io}
  {}

  void put(kg::Engine& writer, const GridBc& bc, const kg::Mode launch = kg::Mode::Deferred)
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

  explicit operator bool() const
  {
    return (var_fld_lo_ && var_fld_hi_ && var_prt_lo_ && var_prt_hi_);
  }

private:
  kg::VariableGlobalSingleValue<Int3> var_fld_lo_;
  kg::VariableGlobalSingleValue<Int3> var_fld_hi_;
  kg::VariableGlobalSingleValue<Int3> var_prt_lo_;
  kg::VariableGlobalSingleValue<Int3> var_prt_hi_;
};

// ======================================================================
// Variable<Normalization>

template<>
struct kg::Variable<Grid_t::Normalization>
{
  using value_type = Grid_t::Normalization;
  using is_adios_variable = std::false_type;

  using real_t = Grid_t::real_t;

  Variable(const std::string& name, kg::IO& io)
    : var_cc_{name + ".cc", io},
      var_fnqs_{name + ".fnqs", io},
      var_eta_{name + ".eta", io},
      var_beta_{name + ".beta", io},
      var_cori_{name + ".cori", io},
      var_b0_{name + ".b0", io},
      var_rho0_{name + ".rho0", io},
      var_phi0_{name + ".phi0", io},
      var_a0_{name + ".a0", io}
  {}

  void put(kg::Engine& writer, const Grid_t::Normalization& norm, const kg::Mode launch = kg::Mode::Deferred)
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

  void get(Engine& reader, Grid_t::Normalization& norm, const Mode launch = Mode::Deferred)
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
  kg::VariableGlobalSingleValue<real_t> var_cc_;
  kg::VariableGlobalSingleValue<real_t> var_fnqs_;
  kg::VariableGlobalSingleValue<real_t> var_eta_;
  kg::VariableGlobalSingleValue<real_t> var_beta_;
  kg::VariableGlobalSingleValue<real_t> var_cori_;

  kg::VariableGlobalSingleValue<real_t> var_b0_;
  kg::VariableGlobalSingleValue<real_t> var_rho0_;
  kg::VariableGlobalSingleValue<real_t> var_phi0_;
  kg::VariableGlobalSingleValue<real_t> var_a0_;
};

// ======================================================================
// Variable<Grid::Kinds>

template<>
struct kg::Variable<Grid_t::Kinds>
{
  using value_type = Grid_t::Kinds;
  using is_adios_variable = std::false_type;
  using real_t = Grid_t::real_t;

  static const size_t NAME_LEN = 10;

  Variable(const std::string& name, kg::IO& io)
    : attr_q_{name + ".q", io},
      attr_m_{name + ".m", io},
      attr_names_{name + ".names", io}
  {}

  void put(kg::Engine& writer, const Grid_t::Kinds& kinds, const kg::Mode launch = kg::Mode::Deferred)
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

  void get(Engine& reader, Grid_t::Kinds& kinds, const Mode launch = Mode::Deferred)
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
  kg::Attribute<std::vector<real_t>> attr_q_;
  kg::Attribute<std::vector<real_t>> attr_m_;
  kg::Attribute<std::vector<std::string>> attr_names_;
};

// ======================================================================
// Variable<Grid_<T>>

template<typename T>
struct kg::Variable<Grid_<T>>
{
  using Grid = Grid_<T>;
  using value_type = Grid;
  using is_adios_variable = std::false_type;

  using real_t = typename Grid::real_t;
  using Real3 = typename Grid::Real3;
  
  Variable(const std::string& name, kg::IO& io)
    : var_ldims_{name + ".ldims", io},
      var_domain_{name + ".domain", io},
      var_bc_{name + ".bc", io},
      var_norm_{name + ".norm", io},
      var_dt_{name + ".dt", io},
      var_patches_n_local_{name + ".patches.n_local", io},
      var_patches_off_{name + ".patches.off", io},
      var_patches_xb_{name + ".patches.xb", io},
      var_patches_xe_{name + ".patches.xe", io},
      var_kinds_{name + ".kinds", io},
      var_ibn_{name + ".ibn", io},
      var_timestep_{name + ".timestep", io}
  {}

  void put(kg::Engine& writer, const Grid& grid, const kg::Mode launch = kg::Mode::Deferred)
  {
    writer.put(var_ldims_, grid.ldims);
    writer.put(var_domain_, grid.domain, launch);
    writer.put(var_bc_, grid.bc, launch);
    writer.put(var_norm_, grid.norm, launch);
    writer.put(var_dt_, grid.dt, launch);

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
    writer.put(var_ibn_, grid.ibn, launch);
    writer.put(var_timestep_, grid.timestep_, launch);

    writer.performPuts(); // because we're writing temp local vars (the patches_*)
  }
  
  void get(kg::Engine& reader, Grid& grid, const kg::Mode launch = kg::Mode::Deferred)
  {
    reader.get(var_ldims_, grid.ldims);
    reader.get(var_domain_, grid.domain, launch);
    reader.get(var_bc_, grid.bc, launch);
    reader.get(var_norm_, grid.norm, launch);
    reader.get(var_dt_, grid.dt, launch);

    int patches_n_local;
    reader.get(var_patches_n_local_, patches_n_local, launch);

    reader.performGets(); // need patches_n_local, domain, bc to be read
    grid.mrc_domain_ = grid.make_mrc_domain(grid.domain, grid.bc, patches_n_local);

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
    reader.get(var_ibn_, grid.ibn, launch);
    reader.get(var_timestep_, grid.timestep_, launch);
  }
  
  explicit operator bool() const
  {
    return (var_domain_ && var_dt_);
  }

private:
  kg::Attribute<Int3> var_ldims_;
  kg::Variable<typename Grid::Domain> var_domain_;
  kg::Variable<GridBc> var_bc_;
  kg::Variable<typename Grid::Normalization> var_norm_;
  kg::VariableGlobalSingleValue<real_t> var_dt_;

  kg::VariableLocalSingleValue<int> var_patches_n_local_; // FIXME, should be size_t, adios2 bug?
  VariableByPatch<Int3> var_patches_off_;
  VariableByPatch<Real3> var_patches_xb_;
  VariableByPatch<Real3> var_patches_xe_;

  kg::Variable<typename Grid::Kinds> var_kinds_;
  kg::VariableGlobalSingleValue<Int3> var_ibn_;
  kg::VariableGlobalSingleValue<int> var_timestep_;
  // the mrc_domain_ member is not written and handled specially on read
};

