
#pragma once

namespace kg
{

namespace detail
{
template <typename T>
struct Variable;
}

template <typename T>
struct Variable;

template <typename T, typename Enable = void>
struct Attribute;

// ======================================================================
// IO

struct IO
{
  IO(adios2::ADIOS& ad, const char* name) : io_{ad.DeclareIO(name)} {}

  Engine open(const std::string& name, const adios2::Mode mode)
  {
    // FIXME, assumes that the ADIOS2 object underlying io_ was created on
    // MPI_COMM_WORLD
    auto comm = MPI_COMM_WORLD;

    return {io_.Open(name, mode), io_, comm};
  }

  template <typename T>
  detail::Variable<T> _defineVariable(const std::string& name,
                                      const Dims& shape = Dims(),
                                      const Dims& start = Dims(),
                                      const Dims& count = Dims(),
                                      const bool constantDims = false)
  {
    auto var = io_.InquireVariable<T>(name);
    if (var) {
      return var;
    } else {
      return io_.DefineVariable<T>(name, shape, start, count, constantDims);
    }
  }

  template <typename T>
  Variable<T> defineVariable(const std::string& name)
  {
    return {name, *this};
  }

  template <class T>
  Variable<T> inquireVariable(const std::string& name)
  {
    return io_.InquireVariable<T>(name);
  }

private:
  adios2::IO io_;
};

} // namespace kg
