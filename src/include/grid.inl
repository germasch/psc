
namespace kg
{

using Mode = adios2::Mode;
using Dims = adios2::Dims;
template<typename T>
using Box = adios2::Box<T>;
  
struct IO;
struct Engine;

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

  void put(Engine& writer, const T val, const Mode launch = Mode::Deferred);
  void put(Engine& writer, const T* val, const Mode launch = Mode::Deferred);
  
  void setSelection(const Box<Dims>& selection)
  {
    var_.SetSelection(selection);
  }
  
  adios2::Variable<T> var_;
};
  
template<typename T>
struct VariableGlobalSingleValue
{
  using value_type = T;
  using is_adios_variable = std::false_type;
  
  VariableGlobalSingleValue(const std::string& name, IO& io);
  
  void put(Engine& writer, const T val, const Mode launch = Mode::Deferred);
  
private:
  Variable<T> var_;
};
  
template<typename T>
struct VariableGlobalSingleValue<Vec3<T>>
{
  using value_type = Vec3<T>;
  
  VariableGlobalSingleValue(const std::string& name, IO& io);
  
  void put(Engine& writer, const Vec3<T>& val, const Mode launch = Mode::Deferred);
  
private:
  Variable<T> var_;
};

template<typename T>
using Vec3Writer = VariableGlobalSingleValue<T>;

using Int3Writer = Vec3Writer<Int3>;

struct Engine
{
  Engine(adios2::Engine engine, MPI_Comm comm)
    : engine_{engine}
  {
    MPI_Comm_rank(comm, &mpi_rank_);
  }

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
  
  template<class T>
  void put(T& variable, const typename T::value_type* data, const Mode launch = Mode::Deferred)
  {
    variable.put(*this, data, launch);
  }

  template<class T>
  void put(T& variable, const typename T::value_type& datum, const Mode launch = Mode::Deferred)
  {
    variable.put(*this, datum, launch);
  }

  void close()
  {
    engine_.Close();
  }

  int mpiRank() const { return mpi_rank_; }

private:
  adios2::Engine engine_;
  int mpi_rank_;
};

struct IO
{
  IO(adios2::ADIOS& ad, const char* name)
    : io_{ad.DeclareIO(name)}
  {}

  Engine open(const std::string& name, const adios2::Mode mode)
  {
     // FIXME, assumes that the ADIOS2 object underlying io_ was created on MPI_COMM_WORLD
    auto comm = MPI_COMM_WORLD;
    
    return {io_.Open(name, mode), comm};
  }

  template<typename T,
	   typename std::enable_if<Variable<T>::is_adios_variable::value, int>::type = 0>
  Variable<T> defineVariable(const std::string &name, const Dims &shape = Dims(),
			     const Dims &start = Dims(), const Dims &count = Dims(),
			     const bool constantDims = false)
  {
    return io_.DefineVariable<T>(name, shape, start, count, constantDims);
  }
  
  template<typename T,
	   typename std::enable_if<!Variable<T>::is_adios_variable::value, int>::type = 0>
  Variable<T> defineVariable(const std::string &name, const Dims &shape = Dims(),
		 const Dims &start = Dims(), const Dims &count = Dims(),
		 const bool constantDims = false)
  {
    return {name, *this};
  }

private:
  adios2::IO io_;
};

// ======================================================================
// implementations

template<typename T>
void Variable<T>::put(Engine& writer, const T val, const Mode launch)
{
  writer.put(var_, val, launch);
}
  
template<typename T>
void Variable<T>::put(Engine& writer, const T* val, const Mode launch)
{
  writer.put(var_, val, launch);
}
  
template<typename T>
VariableGlobalSingleValue<T>::VariableGlobalSingleValue(const std::string& name, IO& io)
  : var_{io.defineVariable<T>(name)}
{}
  
template<typename T>
void VariableGlobalSingleValue<T>::put(Engine& writer, T val, const Mode launch)
{
  if (writer.mpiRank() == 0) {
    writer.put(var_, val, launch);
  }
}

template<typename T>
VariableGlobalSingleValue<Vec3<T>>::VariableGlobalSingleValue(const std::string& name, IO& io)
  : var_{io.defineVariable<T>(name, {3}, {0}, {0})} // adios2 FIXME {3} {} {} gives no error, but problems
{}
  
template<typename T>
void VariableGlobalSingleValue<Vec3<T>>::put(Engine& writer, const Vec3<T>& val, const Mode launch)
{
  if (writer.mpiRank() == 0) {
    var_.setSelection({{0}, {3}}); // adios2 FIXME, would be nice to specify {}, {3}
    writer.put(var_, val.data(), launch);
  }
};

};
  
template<typename T>
struct kg::Variable<Grid_<T>>
{
  using Grid = Grid_<T>;
  using value_type = Grid;
  using is_adios_variable = std::false_type;

  using real_t = typename Grid::real_t;
  using Real3 = typename Grid::Real3;
  
  Variable(const std::string& name, kg::IO& io)
    : var_ldims_{"grid.ldims", io},
      var_dt_{"grid.dt", io},
      var_domain_gdims_{"grid.domain.gdims", io},
      var_domain_length_{"grid.domain.length", io},
      var_domain_corner_{"grid.domain.corner", io},
      var_domain_np_{"grid.domain.np", io},
      var_domain_ldims_{"grid.domain.ldims", io},
      var_domain_dx_{"grid.domain.dx", io}
  {}

  void put(kg::Engine& writer, const Grid& grid, const kg::Mode launch = kg::Mode::Deferred)
  {
    writer.put(var_ldims_, grid.ldims);
    writer.put(var_dt_, grid.dt);
    
    writer.put(var_domain_gdims_, grid.domain.gdims);
    writer.put(var_domain_length_, grid.domain.length);
    writer.put(var_domain_corner_, grid.domain.corner);
    writer.put(var_domain_np_, grid.domain.np);
    writer.put(var_domain_ldims_, grid.domain.ldims);
    writer.put(var_domain_dx_, grid.domain.dx);
  }
  
private:
  kg::VariableGlobalSingleValue<Int3> var_ldims_;
  kg::VariableGlobalSingleValue<real_t> var_dt_;

  kg::VariableGlobalSingleValue<Int3> var_domain_gdims_;
  kg::VariableGlobalSingleValue<Real3> var_domain_length_;
  kg::VariableGlobalSingleValue<Real3> var_domain_corner_;
  kg::VariableGlobalSingleValue<Int3> var_domain_np_;
  kg::VariableGlobalSingleValue<Int3> var_domain_ldims_;
  kg::VariableGlobalSingleValue<Real3> var_domain_dx_;
};

