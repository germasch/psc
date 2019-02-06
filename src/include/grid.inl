
template<typename T>
struct Grid_<T>::Adios2
{
  Adios2(const Grid_& grid, adios2::IO& io)
    : grid_{grid}
  {
    var_ldims_ = io.DefineVariable<int>("grid.ldims", {3}, {0}, {0}); // adios2 FIXME {3} {} {} gives no error, but probs
    var_dt_ = io.DefineVariable<real_t>("grid.dt");

    var_domain_gdims_ = io.DefineVariable<int>("grid.domain.gdims", {3}, {0}, {0});
    var_domain_length_ = io.DefineVariable<real_t>("grid.domain.length", {3}, {0}, {0});
    var_domain_corner_ = io.DefineVariable<real_t>("grid.domain.corner", {3}, {0}, {0});
    var_domain_np_ = io.DefineVariable<int>("grid.domain.np", {3}, {0}, {0});
    var_domain_ldims_ = io.DefineVariable<int>("grid.domain.ldims", {3}, {0}, {0});
    var_domain_dx_ = io.DefineVariable<real_t>("grid.domain.dx", {3}, {0}, {0});
  }

  void put(adios2::Engine& writer, const Grid_& grid)
  {
    int rank;
    MPI_Comm_rank(grid.comm(), &rank);

    if (rank == 0) {
      var_ldims_.SetSelection({{0}, {3}}); // adios2 FIXME, would be nice to specify {}, {3}
      writer.Put(var_ldims_, grid.ldims.data());
      writer.Put(var_dt_, grid.dt);

      var_domain_gdims_.SetSelection({{0}, {3}});
      writer.Put(var_domain_gdims_, grid.domain.gdims.data());
      var_domain_length_.SetSelection({{0}, {3}});
      writer.Put(var_domain_length_, grid.domain.length.data());
      var_domain_corner_.SetSelection({{0}, {3}});
      writer.Put(var_domain_corner_, grid.domain.corner.data());
      var_domain_np_.SetSelection({{0}, {3}});
      writer.Put(var_domain_np_, grid.domain.np.data());
      var_domain_ldims_.SetSelection({{0}, {3}});
      writer.Put(var_domain_ldims_, grid.domain.ldims.data());
      var_domain_dx_.SetSelection({{0}, {3}});
      writer.Put(var_domain_dx_, grid.domain.dx.data());
    }
  }
  
private:
  const Grid_& grid_;
  adios2::Variable<int> var_ldims_;
  adios2::Variable<real_t> var_dt_;

  adios2::Variable<int> var_domain_gdims_;
  adios2::Variable<real_t> var_domain_length_;
  adios2::Variable<real_t> var_domain_corner_;
  adios2::Variable<int> var_domain_np_;
  adios2::Variable<int> var_domain_ldims_;
  adios2::Variable<real_t> var_domain_dx_;
};

template<typename T>
auto Grid_<T>::checkpoint(adios2::IO& io) -> Adios2
{
  return {*this, io};
}

