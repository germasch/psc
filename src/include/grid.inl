
template<typename T>
struct Vec3Writer
{
  Vec3Writer(const std::string& name, adios2::IO& io, MPI_Comm comm)
  {
    var_ = io.DefineVariable<T>(name, {3}, {0}, {0});  // adios2 FIXME {3} {} {} gives no error, but problems
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
  using Real3Writer = Vec3Writer<real_t>;
  
  Adios2(const Grid_& grid, adios2::IO& io)
    : grid_{grid},
      w_ldims_{"grid.ldims", io, MPI_COMM_WORLD}, // FIXME hardcoded comm
      w_domain_gdims_{"grid.domain.gdims", io, MPI_COMM_WORLD},
      w_domain_length_{"grid.domain.length", io, MPI_COMM_WORLD},
      w_domain_corner_{"grid.domain.corner", io, MPI_COMM_WORLD},
      w_domain_np_{"grid.domain.np", io, MPI_COMM_WORLD},
      w_domain_ldims_{"grid.domain.ldims", io, MPI_COMM_WORLD},
      w_domain_dx_{"grid.domain.dx", io, MPI_COMM_WORLD}
  {
    var_dt_ = io.DefineVariable<real_t>("grid.dt");
  }

  void put(adios2::Engine& writer, const Grid_& grid)
  {
    w_ldims_.put(writer, grid.ldims);

    w_domain_gdims_.put(writer, grid.domain.gdims);
    w_domain_length_.put(writer, grid.domain.length);
    w_domain_corner_.put(writer, grid.domain.corner);
    w_domain_np_.put(writer, grid.domain.np);
    w_domain_ldims_.put(writer, grid.domain.ldims);
    w_domain_dx_.put(writer, grid.domain.dx);

    int rank;
    MPI_Comm_rank(grid.comm(), &rank);

    if (rank == 0) {
      writer.Put(var_dt_, grid.dt);
    }
  }
  
private:
  const Grid_& grid_;
  Int3Writer w_ldims_;
  adios2::Variable<real_t> var_dt_;

  Int3Writer w_domain_gdims_;
  Real3Writer w_domain_length_;
  Real3Writer w_domain_corner_;
  Int3Writer w_domain_np_;
  Int3Writer w_domain_ldims_;
  Real3Writer w_domain_dx_;
};

template<typename T>
auto Grid_<T>::checkpoint(adios2::IO& io) -> Adios2
{
  return {*this, io};
}

