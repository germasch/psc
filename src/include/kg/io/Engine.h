
#pragma once

#include <mrc_common.h>

namespace kg
{
namespace io
{

namespace detail
{
template <typename T>
class Variable;

template <typename T>
class Attribute;
}

template <typename T>
class Variable;

// ======================================================================
// Engine

class Engine
{
public:
  Engine(adios2::Engine engine, adios2::IO& io, MPI_Comm comm);

  template <typename T>
  detail::Variable<T> _defineVariable(const std::string& name,
                                      const Dims& shape = Dims(),
                                      const Dims& start = Dims(),
                                      const Dims& count = Dims(),
                                      const bool constantDims = false);

  template <typename T>
  Variable<T> defineVariable(const std::string& name);

  // ----------------------------------------------------------------------
  // put for adios2 variables

  template <typename T>
  void put(adios2::Variable<T> variable, const T* data,
           const Mode launch = Mode::Deferred);

  template <typename T>
  void put(adios2::Variable<T> variable, const T& datum,
           const Mode launch = Mode::Deferred);

  // ----------------------------------------------------------------------
  // put in general

  template <class T, class... Args>
  void put(T& variable, Args&&... args);

  // ----------------------------------------------------------------------
  // get for adios2 variables

  template <typename T>
  void get(adios2::Variable<T> variable, T& datum,
           const Mode launch = Mode::Deferred);

  template <typename T>
  void get(adios2::Variable<T> variable, T* data,
           const Mode launch = Mode::Deferred);

  template <typename T>
  void get(adios2::Variable<T> variable, std::vector<T>& data,
           const Mode launch = Mode::Deferred);

  // ----------------------------------------------------------------------
  // get in general

  template <class T, class... Args>
  void get(T& variable, Args&&... args);

  // ----------------------------------------------------------------------
  // performPuts

  void performPuts();

  // ----------------------------------------------------------------------
  // performGets

  void performGets();

  // ----------------------------------------------------------------------
  // close

  void close();

  int mpiRank() const;
  int mpiSize() const;

private:
  adios2::Engine engine_;
  adios2::IO io_;
  int mpi_rank_;
  int mpi_size_;

  template <typename T>
  friend class detail::Attribute;
  template <typename T>
  friend class detail::Variable;
};

} // namespace io
} // namespace kg

#include "Engine.inl"
