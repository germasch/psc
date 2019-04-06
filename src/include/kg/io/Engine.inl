
namespace kg
{
namespace io
{

Engine::Engine(adios2::Engine engine, adios2::IO& io, MPI_Comm comm)
  : engine_{engine}, io_{io}
{
  MPI_Comm_rank(comm, &mpi_rank_);
  MPI_Comm_size(comm, &mpi_size_);
}

template <typename T>
detail::Variable<T> Engine::_defineVariable(const std::string& name,
                                            const Dims& shape,
                                            const Dims& start,
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
Variable<T> Engine::defineVariable(const std::string& name)
{
  return {name, *this};
}

// ----------------------------------------------------------------------
// put for adios2 variables

template <typename T>
void Engine::put(adios2::Variable<T> variable, const T* data, const Mode launch)
{
  engine_.Put(variable, data, launch);
}

template <typename T>
void Engine::put(adios2::Variable<T> variable, const T& datum,
                 const Mode launch)
{
  engine_.Put(variable, datum, launch);
}

template <class T, class... Args>
void Engine::put(T& variable, Args&&... args)
{
  variable.put(*this, std::forward<Args>(args)...);
}

// ----------------------------------------------------------------------
// get for adios2 variables

template <typename T>
void Engine::get(adios2::Variable<T> variable, T& datum, const Mode launch)
{
  engine_.Get(variable, datum, launch);
}

template <typename T>
void Engine::get(adios2::Variable<T> variable, T* data, const Mode launch)
{
  engine_.Get(variable, data, launch);
}

template <typename T>
void Engine::get(adios2::Variable<T> variable, std::vector<T>& data,
                 const Mode launch)
{
  engine_.Get(variable, data, launch);
}

template <class T, class... Args>
void Engine::get(T& variable, Args&&... args)
{
  variable.get(*this, std::forward<Args>(args)...);
}

// ----------------------------------------------------------------------
// performPuts

void Engine::performPuts()
{
  engine_.PerformPuts();
}

// ----------------------------------------------------------------------
// performGets

void Engine::performGets()
{
  engine_.PerformGets();
}

// ----------------------------------------------------------------------
// close

void Engine::close()
{
  engine_.Close();
}

template <typename T>
void Engine::putAttribute(const std::string& name, const T* data, size_t size)
{
  if (mpi_rank_ != 0) { // FIXME, should we do this?
    return;
  }
  auto attr = io_.InquireAttribute<T>(name);
  if (attr) {
    mprintf("attr '%s' already exists -- ignoring it!", name.c_str());
  } else {
    io_.DefineAttribute<T>(name, data, size);
  }
}

template <typename T>
void Engine::putAttribute(const std::string& name, const T& value)
{
  if (mpi_rank_ != 0) { // FIXME, should we do this?
    return;
  }
  auto attr = io_.InquireAttribute<T>(name);
  if (attr) {
    mprintf("attr '%s' already exists -- ignoring it!", name.c_str());
  } else {
    io_.DefineAttribute<T>(name, value);
  }
}

template <typename T>
void Engine::getAttribute(const std::string& name, std::vector<T>& data)
{
  auto attr = io_.InquireAttribute<T>(name);
  assert(attr);
  data = attr.Data();
}

int Engine::mpiRank() const
{
  return mpi_rank_;
}
int Engine::mpiSize() const
{
  return mpi_size_;
}

} // namespace io
} // namespace kg
