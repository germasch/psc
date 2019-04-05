
#include "Manager.h"

namespace kg
{

IO::IO(io::Manager& mgr, const char* name) : io_{mgr.ad_.DeclareIO(name)} {}

Engine IO::open(const std::string& name, const adios2::Mode mode)
{
  // FIXME, assumes that the ADIOS2 object underlying io_ was created on
  // MPI_COMM_WORLD
  auto comm = MPI_COMM_WORLD;

  return {io_.Open(name, mode), io_, comm};
}

template <typename T>
detail::Variable<T> IO::_defineVariable(const std::string& name,
                                        const Dims& shape, const Dims& start,
                                        const Dims& count,
                                        const bool constantDims)
{
  auto var = io_.InquireVariable<T>(name);
  if (var) {
    return var;
  } else {
    return io_.DefineVariable<T>(name, shape, start, count, constantDims);
  }
}

template <typename T>
Variable<T> IO::defineVariable(const std::string& name)
{
  return {name, *this};
}

template <class T>
Variable<T> IO::inquireVariable(const std::string& name)
{
  return io_.InquireVariable<T>(name);
}

} // namespace kg
