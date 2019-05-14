
#include "Attribute.h"
#include "Variable.h"

namespace kg
{
namespace io
{

// ======================================================================
// FileAdios

inline FileAdios::FileAdios(adios2::Engine engine, adios2::IO io)
  : engine_{engine}, io_{io}
{}

inline void FileAdios::close()
{
  engine_.Close();
}

inline void FileAdios::performPuts()
{
  engine_.PerformPuts();
}

inline void FileAdios::performGets()
{
  engine_.PerformGets();
}

template <typename T>
inline void FileAdios::put(adios2::Variable<T>& var, const T* data, const Mode launch)
{
  engine_.Put(var, data, launch);
}

template <typename T>
inline void FileAdios::get(adios2::Variable<T>& var, T* data, const Mode launch)
{
  engine_.Get(var, data, launch);
}

// ======================================================================
// Engine

inline Engine::Engine(adios2::Engine engine, adios2::IO io, MPI_Comm comm)
  : file_{engine, io}
{
  MPI_Comm_rank(comm, &mpi_rank_);
  MPI_Comm_size(comm, &mpi_size_);
}

template <typename T>
inline detail::Variable<T> Engine::makeVariable()
{
  return {prefix(), file_.io_};
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
inline void Engine::putLocal(const std::string& pfx, const T& datum,
                             Mode launch)
{
  prefixes_.push_back(pfx);
  auto var = makeVariable<T>();
  var.setShape({adios2::LocalValueDim});
  var.put(*this, datum, launch);
  prefixes_.pop_back();
}

template <template <typename...> class Var, class T, class... Args>
inline void Engine::put(const std::string& pfx, const T& datum, Args&&... args)
{
  prefixes_.push_back(pfx);
  // mprintf("put<Var> pfx %s -- %s\n", pfx.c_str(), prefix().c_str());
  Var<T> var;
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
  Var<T> var;
  var.get(*this, datum, std::forward<Args>(args)...);
  prefixes_.pop_back();
}

// ----------------------------------------------------------------------
// performPuts

inline void Engine::performPuts()
{
  file_.performPuts();
}

// ----------------------------------------------------------------------
// performGets

inline void Engine::performGets()
{
  file_.performGets();
}

// ----------------------------------------------------------------------
// close

inline void Engine::close()
{
  file_.close();
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
