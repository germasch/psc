
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

// ======================================================================
// VariableGlobalSingleValue

template <typename T>
class VariableGlobalSingleValue
{
public:
  using value_type = T;

  VariableGlobalSingleValue(const std::string& name, Engine& engine);

  void put(Engine& writer, const T datum, const Mode launch = Mode::Deferred);

  void get(Engine& reader, T& val, const Mode launch = Mode::Deferred);

  std::string Name() const { return var_.Name(); }

private:
  detail::Variable<T> var_;
};

// ======================================================================
// VariableGlobalSingleArray

template <typename T>
class VariableGlobalSingleArray
{
public:
  using value_type = T;

  VariableGlobalSingleArray(const std::string& name, Engine& engine);

  void put(Engine& writer, const T* data, const Dims& shape,
           const Mode launch = Mode::Deferred);
  void put(Engine& writer, const std::vector<T>& vec,
           const Mode launch = Mode::Deferred);

  void get(Engine& reader, T* data, const Mode launch = Mode::Deferred);
  void get(Engine& reader, std::vector<T>& data,
           const Mode launch = Mode::Deferred);

  Dims shape() const;

private:
  detail::Variable<T> var_;
};

// ======================================================================
// VariableLocalSingleValue

template <typename T>
class VariableLocalSingleValue
{
public:
  using value_type = T;

  VariableLocalSingleValue(const std::string& name, Engine& engine);

  void put(Engine& writer, const T& datum, const Mode launch = Mode::Deferred);

  void get(Engine& reader, T& val, const Mode launch = Mode::Deferred);

  Dims shape() const;

  std::string name() const { return var_.name(); }

private:
  detail::Variable<T> var_;
};

} // namespace io
} // namespace kg

#include "Variable.inl"
