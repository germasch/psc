
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
  // put

  template <class T, class... Args>
  void put(T& variable, Args&&... args);

  // ----------------------------------------------------------------------
  // get

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
