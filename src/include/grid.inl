
namespace kg
{

struct IO;
struct Engine;
  
template<typename T>
struct ScalarWriter
{
  ScalarWriter(const std::string& name, IO& io, MPI_Comm comm);

  void put(Engine& writer, T val);

private:
  adios2::Variable<T> var_;
};

struct Engine
{
  Engine(adios2::Engine engine, MPI_Comm comm)
    : engine_{engine}
  {
    MPI_Comm_rank(comm, &mpi_rank_);
  }

  template<typename T>
  void put(adios2::Variable<T> variable, const T* data, const adios2::Mode launch = adios2::Mode::Deferred)
  {
    engine_.Put(variable, data, launch);
  }

  template <class T>
  void put(adios2::Variable<T> variable, const T& datum, const adios2::Mode launch = adios2::Mode::Deferred)
  {
    engine_.Put(variable, datum, launch);
  }

  template<class T>
  void put(ScalarWriter<T>& var, const T& datum, const adios2::Mode launch = adios2::Mode::Deferred)
  {
    var.put(*this, datum);
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
  using Dims = adios2::Dims;
  
  IO(adios2::ADIOS& ad, const char* name)
    : io_{ad.DeclareIO(name)}
  {}

  Engine open(const std::string& name, const adios2::Mode mode)
  {
     // FIXME, assumes that the ADIOS2 object underlying io_ was created on MPI_COMM_WORLD
    auto comm = MPI_COMM_WORLD;
    
    return {io_.Open(name, mode), comm};
  }

  template<typename T>
  adios2::Variable<T> defineVariable(const std::string &name, const Dims &shape = Dims(),
				     const Dims &start = Dims(), const Dims &count = Dims(),
				     const bool constantDims = false)
  {
    return io_.DefineVariable<T>(name, shape, start, count, constantDims);
  }

private:
  adios2::IO io_;
};

template<typename T>
struct Vec3Writer
{
  Vec3Writer(const std::string& name, kg::IO& io, MPI_Comm comm)
  {
    var_ = io.defineVariable<T>(name, {3}, {0}, {0});  // adios2 FIXME {3} {} {} gives no error, but problems
  }

  void put(kg::Engine& writer, const Vec3<T>& val)
  {
    if (writer.mpiRank() == 0) {
      var_.SetSelection({{0}, {3}}); // adios2 FIXME, would be nice to specify {}, {3}
      writer.put(var_, val.data());
    }
  }

private:
  adios2::Variable<T> var_;
};

using Int3Writer = Vec3Writer<int>;

// ======================================================================

template<typename T>
ScalarWriter<T>::ScalarWriter(const std::string& name, IO& io, MPI_Comm comm)
{
  var_ = io.defineVariable<T>(name);
}
  
template<typename T>
void ScalarWriter<T>::put(Engine& writer, T val)
{
  if (writer.mpiRank() == 0) {
    writer.put(var_, val);
  }
}
  
};

template<typename T>
struct Grid_<T>::Adios2
{
  using RealWriter = kg::ScalarWriter<real_t>;
  using Real3Writer = kg::Vec3Writer<real_t>;
  
  Adios2(const Grid_& grid, kg::IO& io)
    : grid_{grid},
      w_ldims_{"grid.ldims", io, MPI_COMM_WORLD}, // FIXME hardcoded comm
      w_dt_{"grid.dt", io, MPI_COMM_WORLD},
      w_domain_gdims_{"grid.domain.gdims", io, MPI_COMM_WORLD},
      w_domain_length_{"grid.domain.length", io, MPI_COMM_WORLD},
      w_domain_corner_{"grid.domain.corner", io, MPI_COMM_WORLD},
      w_domain_np_{"grid.domain.np", io, MPI_COMM_WORLD},
      w_domain_ldims_{"grid.domain.ldims", io, MPI_COMM_WORLD},
      w_domain_dx_{"grid.domain.dx", io, MPI_COMM_WORLD}
  {}

  void put(kg::Engine& writer, const Grid_& grid)
  {
    w_ldims_.put(writer, grid.ldims);
    writer.put(w_dt_, grid.dt);
    
    w_domain_gdims_.put(writer, grid.domain.gdims);
    w_domain_length_.put(writer, grid.domain.length);
    w_domain_corner_.put(writer, grid.domain.corner);
    w_domain_np_.put(writer, grid.domain.np);
    w_domain_ldims_.put(writer, grid.domain.ldims);
    w_domain_dx_.put(writer, grid.domain.dx);
  }
  
private:
  const Grid_& grid_;
  kg::Int3Writer w_ldims_;
  RealWriter w_dt_;

  kg::Int3Writer w_domain_gdims_;
  Real3Writer w_domain_length_;
  Real3Writer w_domain_corner_;
  kg::Int3Writer w_domain_np_;
  kg::Int3Writer w_domain_ldims_;
  Real3Writer w_domain_dx_;
};

template<typename T>
auto Grid_<T>::writer(kg::IO& io) -> Adios2
{
  return {*this, io};
}

