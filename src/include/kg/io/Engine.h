
#pragma once

#include <mrc_common.h>

#include <deque>
#include <iostream>

#include "Descr.h"
#include "FileAdios2.h"

namespace kg
{
namespace io
{

// ======================================================================
// Variable

template <typename T>
class Variable;

// ======================================================================
// Engine

class Engine
{
public:
  Engine(adios2::Engine engine, adios2::IO io, MPI_Comm comm);

  // ----------------------------------------------------------------------
  // put

  template <class T, class... Args>
  void put(const std::string& pfx, const T& datum, Args&&... args);

  template <class T, class... Args>
  void putAttribute(const std::string& pfx, const T& datum, Args&&... args);

  template <class T>
  void putLocal(const std::string& pfx, const T& datum,
                Mode launch = Mode::Deferred);

  template <template <typename...> class Var, class T, class... Args>
  void put(const std::string& pfx, const T& datum, Args&&... args);

  // ----------------------------------------------------------------------
  // get

  template <class T, class... Args>
  void get(const std::string& pfx, T& datum, Args&&... args);

  template <class T, class... Args>
  void getAttribute(const std::string& pfx, T& datum, Args&&... args);

  template <class T>
  void getLocal(const std::string& pfx, T& datum, Mode launch = Mode::Deferred);

  template <template <typename...> class Var, class T, class... Args>
  void get(const std::string& pfx, T& datum, Args&&... args);

  // ----------------------------------------------------------------------
  // performPuts

  void performPuts();

  // ----------------------------------------------------------------------
  // performGets

  void performGets();

  // ----------------------------------------------------------------------
  // variableShape

  template <typename T>
  Dims variableShape();

  // ----------------------------------------------------------------------
  // internal

  template <typename T>
  void putVariable(const T* data, const Mode launch, const Dims& shape,
                   const Box<Dims>& selection = {},
                   const Box<Dims>& memory_selection = {});

  template <typename T>
  void getVariable(T* data, const Mode launch, const Box<Dims>& selection = {},
                   const Box<Dims>& memory_selection = {});

  template <typename T>
  void writeAttribute(const T& datum);

  template <typename T>
  void writeAttribute(const T* data, size_t size);

  template <typename T>
  void getAttribute(T& datum);

  template <typename T>
  void getAttribute(std::vector<T>& data);

  // ----------------------------------------------------------------------
  // close

  void close();

  int mpiRank() const;
  int mpiSize() const;

  std::string prefix() const
  {
    std::string s;
    bool first = true;
    for (auto& pfx : prefixes_) {
      if (!first) {
        s += "::";
      }
      s += pfx;
      first = false;
    }
    return s;
  }

private:
  FileAdios2 file_;
  std::deque<std::string> prefixes_;
  int mpi_rank_;
  int mpi_size_;
};

} // namespace io
} // namespace kg

#include "Engine.inl"
