
#include "Attribute.h"
#include "Variable.h"

namespace kg
{
namespace io
{

inline Engine::Engine(adios2::Engine engine, adios2::IO& io, MPI_Comm comm)
  : engine_{engine}, io_{io}
{
  MPI_Comm_rank(comm, &mpi_rank_);
  MPI_Comm_size(comm, &mpi_size_);
}

template <typename T>
inline detail::Variable<T> Engine::makeVariable(const Dims& shape)
{
  auto var = io_.InquireVariable<T>(prefix());
  if (var) {
    return var;
  } else {
    return io_.DefineVariable<T>(prefix(), shape);
  }
}

// ----------------------------------------------------------------------
// put

template <class T, class... Args>
inline void Engine::put(const std::string& pfx, const T& datum, Args&&... args)
{
  put<Descr>(pfx, datum, std::forward<Args>(args)...);
}

template <class T, class... Args>
inline void Engine::putAttribute(const std::string& pfx, const T& datum,
                                 Args&&... args)
{
  put<Attribute>(pfx, datum, std::forward<Args>(args)...);
}

  template <class T>
inline void Engine::putLocal(const std::string& pfx, const T& datum, Mode launch)
{
  prefixes_.push_back(pfx);
  auto var = makeVariable<T>({adios2::LocalValueDim});
  var.put(*this, datum, launch);
  prefixes_.pop_back();
}

template <template <typename...> class Var, class T, class... Args>
inline void Engine::put(const std::string& pfx, const T& datum, Args&&... args)
{
  prefixes_.push_back(pfx);
  // mprintf("put<Var> pfx %s -- %s\n", pfx.c_str(), prefix().c_str());
  Var<T> var{prefix(), *this};
  var.put(*this, datum, std::forward<Args>(args)...);
  prefixes_.pop_back();
}

// ----------------------------------------------------------------------
// get

template <class T, class... Args>
inline void Engine::getAttribute(const std::string& pfx, T& datum,
                                 Args&&... args)
{
  get<Attribute>(pfx, datum, std::forward<Args>(args)...);
}

template <class T, class... Args>
inline void Engine::getLocal(const std::string& pfx, T& datum, Args&&... args)
{
  prefixes_.push_back(pfx);
  auto var = makeVariable<T>();
  auto shape = var.shape();
  assert(shape.size() == 1);
  auto dim0 = shape[0];
  assert(dim0 == mpiSize());

  // FIXME, setSelection doesn't work, so read the whole thing
  std::vector<T> vals(shape[0]);
  var.get(*this, vals.data(), std::forward<Args>(args)...);
  datum = vals[mpiRank()];
  prefixes_.pop_back();
}

template <class T, class... Args>
inline void Engine::get(const std::string& pfx, T& datum, Args&&... args)
{
  get<Descr>(pfx, datum, std::forward<Args>(args)...);
}

template <template <typename...> class Var, class T, class... Args>
inline void Engine::get(const std::string& pfx, T& datum, Args&&... args)
{
  prefixes_.push_back(pfx);
  // mprintf("get<Var> pfx %s -- %s\n", pfx.c_str(), prefix().c_str());
  Var<T> var{prefix(), *this};
  var.get(*this, datum, std::forward<Args>(args)...);
  prefixes_.pop_back();
}

// ----------------------------------------------------------------------
// performPuts

inline void Engine::performPuts()
{
  engine_.PerformPuts();
}

// ----------------------------------------------------------------------
// performGets

inline void Engine::performGets()
{
  engine_.PerformGets();
}

// ----------------------------------------------------------------------
// close

inline void Engine::close()
{
  engine_.Close();
}

inline int Engine::mpiRank() const
{
  return mpi_rank_;
}
inline int Engine::mpiSize() const
{
  return mpi_size_;
}

} // namespace io
} // namespace kg
