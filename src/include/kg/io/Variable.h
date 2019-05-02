
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

  Variable(adios2::Variable<T> var);

  void put(Engine& writer, const T& datum, const Mode launch = Mode::Deferred);
  void put(Engine& writer, const T* data, const Mode launch = Mode::Deferred);

  void get(Engine& reader, T& datum, const Mode launch = Mode::Deferred);
  void get(Engine& reader, T* data, const Mode launch = Mode::Deferred);

  void setSelection(const Box<Dims>& selection);
  void setMemorySelection(const Box<Dims>& selection);
  void setShape(const Dims& shape);

  Dims shape() const;
  std::string name() const { return var_.Name(); }

private:
  adios2::Variable<T> var_;
};
} // namespace detail

} // namespace io
} // namespace kg

#include "Variable.inl"
