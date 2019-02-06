
struct PscIO
{
  PscIO(adios2::ADIOS& ad, const char* name)
    : io_{ad.DeclareIO(name)}
  {}

  
  adios2::Engine open(const std::string &name, const adios2::Mode mode)
  {
    return io_.Open(name, mode);
  }

  adios2::IO io_;
};

template<typename T>
struct ScalarWriter
{
  ScalarWriter(const std::string& name, PscIO& io, MPI_Comm comm)
  {
    var_ = io.io_.DefineVariable<T>(name);
    MPI_Comm_rank(comm, &mpi_rank_);
  }

  void put(adios2::Engine& writer, T val)
  {
    if (mpi_rank_ == 0) {
      writer.Put(var_, val);
    }
  }

private:
  adios2::Variable<T> var_;
  int mpi_rank_;
};

template<typename T>
struct Vec3Writer
{
  Vec3Writer(const std::string& name, PscIO& io, MPI_Comm comm)
  {
    var_ = io.io_.DefineVariable<T>(name, {3}, {0}, {0});  // adios2 FIXME {3} {} {} gives no error, but problems
    MPI_Comm_rank(comm, &mpi_rank_);
  }

  void put(adios2::Engine& writer, const Vec3<T>& val)
  {
    if (mpi_rank_ == 0) {
      var_.SetSelection({{0}, {3}}); // adios2 FIXME, would be nice to specify {}, {3}
      writer.Put(var_, val.data());
    }
  }

private:
  adios2::Variable<T> var_;
  int mpi_rank_;
};

using Int3Writer = Vec3Writer<int>;

template<typename T>
struct Grid_<T>::Adios2
{
  using RealWriter = ScalarWriter<real_t>;
  using Real3Writer = Vec3Writer<real_t>;
  
  Adios2(const Grid_& grid, PscIO& io)
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

  void put(adios2::Engine& writer, const Grid_& grid)
  {
    w_ldims_.put(writer, grid.ldims);
    w_dt_.put(writer, grid.dt);
    
    w_domain_gdims_.put(writer, grid.domain.gdims);
    w_domain_length_.put(writer, grid.domain.length);
    w_domain_corner_.put(writer, grid.domain.corner);
    w_domain_np_.put(writer, grid.domain.np);
    w_domain_ldims_.put(writer, grid.domain.ldims);
    w_domain_dx_.put(writer, grid.domain.dx);
  }
  
private:
  const Grid_& grid_;
  Int3Writer w_ldims_;
  RealWriter w_dt_;

  Int3Writer w_domain_gdims_;
  Real3Writer w_domain_length_;
  Real3Writer w_domain_corner_;
  Int3Writer w_domain_np_;
  Int3Writer w_domain_ldims_;
  Real3Writer w_domain_dx_;
};

template<typename T>
auto Grid_<T>::writer(PscIO& io) -> Adios2
{
  return {*this, io};
}

