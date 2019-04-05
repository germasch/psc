
#pragma once

namespace kg
{

using Mode = adios2::Mode;
using Dims = adios2::Dims;
template <typename T>
using Box = adios2::Box<T>;

// ======================================================================
// Engine

struct Engine
{
  Engine(adios2::Engine engine, adios2::IO io, MPI_Comm comm)
    : engine_{engine}, io_{io}
  {
    MPI_Comm_rank(comm, &mpi_rank_);
    MPI_Comm_size(comm, &mpi_size_);
  }

  // ----------------------------------------------------------------------
  // put for adios2 variables

  template <typename T>
  void put(adios2::Variable<T> variable, const T* data,
           const Mode launch = Mode::Deferred)
  {
    engine_.Put(variable, data, launch);
  }

  template <typename T>
  void put(adios2::Variable<T> variable, const T& datum,
           const Mode launch = Mode::Deferred)
  {
    engine_.Put(variable, datum, launch);
  }

  // ----------------------------------------------------------------------
  // put in general

  template <class T, class... Args>
  void put(T& variable, Args&&... args)
  {
    variable.put(*this, std::forward<Args>(args)...);
  }

  // ----------------------------------------------------------------------
  // get for adios2 variables

  template <typename T>
  void get(adios2::Variable<T> variable, T& datum,
           const Mode launch = Mode::Deferred)
  {
    engine_.Get(variable, datum, launch);
  }

  template <typename T>
  void get(adios2::Variable<T> variable, T* data,
           const Mode launch = Mode::Deferred)
  {
    engine_.Get(variable, data, launch);
  }

  template <typename T>
  void get(adios2::Variable<T> variable, std::vector<T>& data,
           const Mode launch = Mode::Deferred)
  {
    engine_.Get(variable, data, launch);
  }

  // ----------------------------------------------------------------------
  // get in general

  template <class T, class... Args>
  void get(T& variable, Args&&... args)
  {
    variable.get(*this, std::forward<Args>(args)...);
  }

  // ----------------------------------------------------------------------
  // performPuts

  void performPuts() { engine_.PerformPuts(); }

  // ----------------------------------------------------------------------
  // performGets

  void performGets() { engine_.PerformGets(); }

  // ----------------------------------------------------------------------
  // close

  void close() { engine_.Close(); }

  template <typename T>
  void putAttribute(const std::string& name, const T* data, size_t size)
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
  void putAttribute(const std::string& name, const T& value)
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
  void getAttribute(const std::string& name, std::vector<T>& data)
  {
    auto attr = io_.InquireAttribute<T>(name);
    assert(attr);
    data = attr.Data();
  }

  int mpiRank() const { return mpi_rank_; }
  int mpiSize() const { return mpi_size_; }

private:
  adios2::Engine engine_;
  adios2::IO io_;
  int mpi_rank_;
  int mpi_size_;
};

} // namespace kg
