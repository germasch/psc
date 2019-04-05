
#pragma once

#include <adios2.h>

namespace kg
{

namespace detail
{
// ======================================================================
// Variable
//
// This general version handles T being one of the base adios2 types (only!)

template <typename T>
struct Variable
{
  using value_type = T;
  using is_adios_variable = std::true_type;

  Variable(adios2::Variable<T> var) : var_{var} {}

  void put(Engine& writer, const T datum, const Mode launch = Mode::Deferred)
  {
    writer.put(var_, datum, launch);
  }

  void put(Engine& writer, const T* data, const Mode launch = Mode::Deferred)
  {
    writer.put(var_, data, launch);
  }

  void get(Engine& reader, T& datum, const Mode launch = Mode::Deferred)
  {
    reader.get(var_, datum, launch);
  }

  void get(Engine& reader, T* data, const Mode launch = Mode::Deferred)
  {
    reader.get(var_, data, launch);
  }

  void get(Engine& reader, std::vector<T>& data,
           const Mode launch = Mode::Deferred)
  {
    reader.get(var_, data, launch);
  }

  void setSelection(const Box<Dims>& selection)
  {
    var_.SetSelection(selection);
  }

  void setShape(const Dims& shape) { var_.SetShape(shape); }

  Dims shape() const { return var_.Shape(); }

private:
  adios2::Variable<T> var_;
};
} // namespace detail

// ======================================================================
// VariableGlobalSingleValue

template <typename T>
struct VariableGlobalSingleValue
{
  using value_type = T;
  using is_adios_variable = std::false_type;

  VariableGlobalSingleValue(const std::string& name, IO& io)
    : var_{io.defineVariable<T>(name)}
  {}

  void put(Engine& writer, const T datum, const Mode launch = Mode::Deferred)
  {
    if (writer.mpiRank() == 0) {
      writer.put(var_, datum, launch);
    }
  }

  void get(Engine& reader, T& val, const Mode launch = Mode::Deferred)
  {
    reader.get(var_, val, launch);
  }

  explicit operator bool() const { return static_cast<bool>(var_); }

private:
  Variable<T> var_;
};

template <typename T>
struct VariableGlobalSingleValue<Vec3<T>>
{
  using value_type = Vec3<T>;

  VariableGlobalSingleValue(const std::string& name, IO& io)
    : var_{io.defineVariable<T>(name, {3}, {0}, {0})}
  // adios2 FIXME {3} {} {} gives no error, but problems
  {}

  void put(Engine& writer, const Vec3<T>& vec3,
           const Mode launch = Mode::Deferred)
  {
    if (writer.mpiRank() == 0) {
      var_.setSelection(
        {{0}, {3}}); // adios2 FIXME, would be nice to specify {}, {3}
      writer.put(var_, vec3.data(), launch);
    }
  };

  void get(Engine& reader, Vec3<T>& vec3, const Mode launch = Mode::Deferred)
  {
    var_.setSelection(
      {{0}, {3}}); // adios2 FIXME, would be nice to specify {}, {3}
    reader.get(var_, vec3.data(), launch);
  };

  explicit operator bool() const { return static_cast<bool>(var_); }

private:
  Variable<T> var_;
};

// ======================================================================
// VariableGlobalSingleArray

template <typename T>
struct VariableGlobalSingleArray
{
  using value_type = T;
  using is_adios_variable = std::false_type;

  VariableGlobalSingleArray(const std::string& name, IO& io)
    : var_{io.defineVariable<T>(name, {1}, {0}, {1})}
  {}

  void put(Engine& writer, const T* data, const Dims& shape,
           const Mode launch = Mode::Deferred)
  {
    var_.setShape(shape);
    if (writer.mpiRank() == 0) {
      var_.setSelection({Dims(shape.size()), shape});
      writer.put(var_, data, launch);
    }
  }

  void put(Engine& writer, const std::vector<T>& vec,
           const Mode launch = Mode::Deferred)
  {
    put(writer, vec.data(), {vec.size()});
  }

  void get(Engine& reader, T* data, const Mode launch = Mode::Deferred)
  {
    // FIXME, without a setSelection, is it guaranteed that the default
    // selection is {{}, shape}?
    reader.get(var_, data, launch);
  }

  void get(Engine& reader, std::vector<T>& data,
           const Mode launch = Mode::Deferred)
  {
    // FIXME, without a setSelection, is it guaranteed that the default
    // selection is {{}, shape}?
    reader.get(var_, data, launch);
  }

  Dims shape() const { return var_.shape(); }

  explicit operator bool() const { return static_cast<bool>(var_); }

private:
  Variable<T> var_;
};

// ======================================================================
// VariableLocalSingleValue

template <typename T>
struct VariableLocalSingleValue
{
  using value_type = T;
  using is_adios_variable = std::false_type;

  VariableLocalSingleValue(const std::string& name, IO& io)
    : var_{io._defineVariable<T>(name, {adios2::LocalValueDim})}
  {}

  void put(Engine& writer, const T datum, const Mode launch = Mode::Deferred)
  {
    writer.put(var_, datum, launch);
  }

  void get(Engine& reader, T& val, const Mode launch = Mode::Deferred)
  {
    auto shape = var_.shape();
    assert(shape.size() == 1);
    auto dim0 = shape[0];
    assert(dim0 == reader.mpiSize());

    // FIXME, setSelection doesn't work, so read the whole thing
    std::vector<T> vals(shape[0]);
    reader.get(var_, vals.data(), launch);
    // for (auto val : vals) mprintf("val %d\n", val);
    val = vals[reader.mpiRank()];
  }

  Dims shape() const { return var_.shape(); }

  explicit operator bool() const { return static_cast<bool>(var_); }

private:
  detail::Variable<T> var_;
};

} // namespace kg
