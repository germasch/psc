
#pragma once

#include <adios2.h>

namespace kg
{
namespace io
{

class Engine;

namespace detail
{
// ======================================================================
// Variable
//
// This general version handles T being one of the base adios2 types (only!)

template <typename T>
class Variable
{
public:
  using value_type = T;

  Variable(const std::string& name, adios2::IO io);

  void put(Engine& writer, const T& datum, const Mode launch = Mode::Deferred);
  void put(Engine& writer, const T* data, const Mode launch = Mode::Deferred);

  void get(Engine& reader, T& datum, const Mode launch = Mode::Deferred);
  void get(Engine& reader, T* data, const Mode launch = Mode::Deferred);

  void setShape(const Dims& shape);
  void setSelection(const Box<Dims>& selection);
  void setMemorySelection(const Box<Dims>& selection);

  std::string name() const;
  Dims shape() const;
  Box<Dims> selection() const;
  Box<Dims> memorySelection() const;

  adios2::Variable<T> makeVariable() const;
  
private:
  std::string name_;
  Dims shape_;
  Box<Dims> selection_;
  Box<Dims> memory_selection_;
  adios2::IO io_;
};
} // namespace detail

} // namespace io
} // namespace kg

#include "Variable.inl"
