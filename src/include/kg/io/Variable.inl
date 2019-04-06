
#pragma once

#include "Engine.h"

namespace kg
{
namespace io
{

namespace detail
{

// ----------------------------------------------------------------------
// detail::Variable

template <typename T>
Variable<T>::Variable(adios2::Variable<T> var) : var_{var}
{}

template <typename T>
void Variable<T>::put(Engine& writer, const T& datum, const Mode launch)
{
  writer.put(var_, datum, launch);
}

template <typename T>
void Variable<T>::put(Engine& writer, const T* data, const Mode launch)
{
  writer.put(var_, data, launch);
}

template <typename T>
void Variable<T>::get(Engine& reader, T& datum, const Mode launch)
{
  reader.get(var_, datum, launch);
}

template <typename T>
void Variable<T>::get(Engine& reader, T* data, const Mode launch)
{
  reader.get(var_, data, launch);
}

template <typename T>
void Variable<T>::get(Engine& reader, std::vector<T>& data, const Mode launch)
{
  reader.get(var_, data, launch);
}

template <typename T>
void Variable<T>::setSelection(const Box<Dims>& selection)
{
  var_.SetSelection(selection);
}

template <typename T>
void Variable<T>::setShape(const Dims& shape)
{
  var_.SetShape(shape);
}

template <typename T>
Dims Variable<T>::shape() const
{
  return var_.Shape();
}

} // namespace detail

// ----------------------------------------------------------------------
// VariableGlobalSingleValue

template <typename T>
VariableGlobalSingleValue<T>::VariableGlobalSingleValue(const std::string& name,
                                                        Engine& engine)
  : var_{engine._defineVariable<T>(name)}
{}

template <typename T>
void VariableGlobalSingleValue<T>::put(Engine& writer, const T datum,
                                       const Mode launch)
{
  if (writer.mpiRank() == 0) {
    writer.put(var_, datum, launch);
  }
}

template <typename T>
void VariableGlobalSingleValue<T>::get(Engine& reader, T& val,
                                       const Mode launch)
{
  reader.get(var_, val, launch);
}

// ----------------------------------------------------------------------
// VariableGlobalSingleArray

template <typename T>
VariableGlobalSingleArray<T>::VariableGlobalSingleArray(const std::string& name,
                                                        Engine& engine)
  : var_{engine._defineVariable<T>(name, {1}, {0}, {1})} // FIXME?
{}

template <typename T>
void VariableGlobalSingleArray<T>::put(Engine& writer, const T* data,
                                       const Dims& shape, const Mode launch)
{
  var_.setShape(shape);
  if (writer.mpiRank() == 0) {
    var_.setSelection({Dims(shape.size()), shape});
    writer.put(var_, data, launch);
  }
}

template <typename T>
void VariableGlobalSingleArray<T>::put(Engine& writer,
                                       const std::vector<T>& vec,
                                       const Mode launch)
{
  put(writer, vec.data(), {vec.size()});
}

template <typename T>
void VariableGlobalSingleArray<T>::get(Engine& reader, T* data,
                                       const Mode launch)
{
  // FIXME, without a setSelection, is it guaranteed that the default
  // selection is {{}, shape}?
  reader.get(var_, data, launch);
}

template <typename T>
void VariableGlobalSingleArray<T>::get(Engine& reader, std::vector<T>& data,
                                       const Mode launch)
{
  // FIXME, without a setSelection, is it guaranteed that the default
  // selection is {{}, shape}?
  reader.get(var_, data, launch);
}

template <typename T>
Dims VariableGlobalSingleArray<T>::shape() const
{
  return var_.shape();
}

// ======================================================================
// VariableLocalSingleValue

template <typename T>
VariableLocalSingleValue<T>::VariableLocalSingleValue(const std::string& name,
                                                      Engine& engine)
  : var_{engine._defineVariable<T>(name, {adios2::LocalValueDim})}
{}

template <typename T>
void VariableLocalSingleValue<T>::put(Engine& writer, const T& datum,
                                      const Mode launch)
{
  writer.put(var_, datum, launch);
}

template <typename T>
void VariableLocalSingleValue<T>::get(Engine& reader, T& val, const Mode launch)
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

template <typename T>
Dims VariableLocalSingleValue<T>::shape() const
{
  return var_.shape();
}

} // namespace io
} // namespace kg
