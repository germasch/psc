
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
inline void FileAdios::putVariable(const std::string& name,
                                   detail::Variable<T>& var, const T* data,
                                   const Mode launch, const Dims& shape)
{
  auto v = io_.InquireVariable<T>(name);
  if (!v) {
    v = io_.DefineVariable<T>(name, shape);
  }
  if (!var.selection().first.empty()) {
    v.SetSelection(var.selection());
  }
  if (!var.memorySelection().first.empty()) {
    v.SetMemorySelection(var.memorySelection());
  }
  engine_.Put(v, data, launch);
}

template <typename T>
inline void FileAdios::getVariable(const std::string& name,
                                   detail::Variable<T>& var, T* data,
                                   const Mode launch)
{
  auto& io = const_cast<adios2::IO&>(io_); // FIXME
  auto v = io.InquireVariable<T>(name);
  if (!var.selection().first.empty()) {
    v.SetSelection(var.selection());
  }
  if (!var.memorySelection().first.empty()) {
    v.SetMemorySelection(var.memorySelection());
  }
  engine_.Get(v, data, launch);
}

template <typename T>
inline Dims FileAdios::shape(const std::string& name) const
{
  auto& io = const_cast<adios2::IO&>(io_); // FIXME
  auto v = io.InquireVariable<T>(name);
  return v.Shape();
}

template <typename T>
inline void FileAdios::getAttribute(const std::string& name,
                                    std::vector<T>& data)
{
  auto attr = io_.InquireAttribute<T>(name);
  assert(attr);
  data = attr.Data();
}

template <typename T>
inline void FileAdios::putAttribute(const std::string& name, const T* data,
                                    size_t size)
{
  // if (mpiRank() != 0) { // FIXME, should we do this?
  //   return;
  // }
  auto attr = io_.InquireAttribute<T>(name);
  if (attr) {
    mprintf("attr '%s' already exists -- ignoring it!\n", name.c_str());
  } else {
    io_.DefineAttribute<T>(name, data, size);
  }
}

template <typename T>
inline void FileAdios::putAttribute(const std::string& name, const T& datum)
{
  // if (mpiRank() != 0) { // FIXME, should we do this?
  //   return;
  // }
  auto attr = io_.InquireAttribute<T>(name);
  if (attr) {
    mprintf("attr '%s' already exists -- ignoring it!\n", name.c_str());
  } else {
    io_.DefineAttribute<T>(name, datum);
  }
}

// ======================================================================
// Engine

inline Engine::Engine(adios2::Engine engine, adios2::IO io, MPI_Comm comm)
  : file_{engine, io}
{
  MPI_Comm_rank(comm, &mpi_rank_);
  MPI_Comm_size(comm, &mpi_size_);
}

// ----------------------------------------------------------------------
// put

template <typename T>
inline void Engine::putVariable(detail::Variable<T>& var, const T* data,
                                const Mode launch)
{
  file_.putVariable(prefix(), var, data, launch, var.shape());
}

template <typename T>
inline void Engine::writeAttribute(const T& datum)
{
  file_.putAttribute(prefix(), datum);
}

template <typename T>
inline void Engine::writeAttribute(const T* data, size_t size)
{
  file_.putAttribute(prefix(), data, size);
}

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
  auto var = detail::Variable<T>{};
  var.setShape({adios2::LocalValueDim});
  file_.putVariable(prefix(), var, &datum, launch, var.shape());
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

template <typename T>
inline void Engine::getVariable(detail::Variable<T>& var, T* data,
                                const Mode launch)
{
  file_.getVariable(prefix(), var, data, launch);
}

template <typename T>
inline void Engine::getAttribute(T& datum)
{
  auto data = std::vector<T>{};
  file_.getAttribute(prefix(), data);
  assert(data.size() == 1);
  datum = data[0];
}

template <typename T>
inline void Engine::getAttribute(std::vector<T>& data)
{
  file_.getAttribute(prefix(), data);
}

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
  auto shape = file_.shape<T>(prefix());
  assert(shape == Dims{static_cast<size_t>(mpiSize())});

  // FIXME, setSelection doesn't work, so read the whole thing
  auto var = detail::Variable<T>{};
  std::vector<T> vals(shape[0]);
  file_.getVariable(prefix(), var, vals.data(), std::forward<Args>(args)...);
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
// getShape

template <typename T>
inline Dims Engine::variableShape()
{
  return file_.shape<T>(prefix());
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
