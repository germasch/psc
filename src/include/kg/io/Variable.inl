
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
  writer.engine_.Put(var_, datum, launch);
}

template <typename T>
void Variable<T>::put(Engine& writer, const T* data, const Mode launch)
{
  writer.engine_.Put(var_, data, launch);
}

template <typename T>
void Variable<T>::get(Engine& reader, T& datum, const Mode launch)
{
  reader.engine_.Get(var_, datum, launch);
}

template <typename T>
void Variable<T>::get(Engine& reader, T* data, const Mode launch)
{
  reader.engine_.Get(var_, data, launch);
}

template <typename T>
void Variable<T>::setSelection(const Box<Dims>& selection)
{
  var_.SetSelection(selection);
}

template <typename T>
void Variable<T>::setMemorySelection(const Box<Dims>& selection)
{
  var_.SetMemorySelection(selection);
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
{}

template <typename T>
void VariableGlobalSingleValue<T>::put(Engine& writer, const T datum,
                                       const Mode launch)
{
  auto var = writer._defineVariable<T>();
  if (writer.mpiRank() == 0) {
    writer.put(var, datum, launch);
  }
}

template <typename T>
void VariableGlobalSingleValue<T>::get(Engine& reader, T& val,
                                       const Mode launch)
{
  auto var = reader._defineVariable<T>();
  reader.get(var, val, launch);
}

// ----------------------------------------------------------------------
// VariableGlobalSingleArray

template <typename T>
VariableGlobalSingleArray<T>::VariableGlobalSingleArray(const std::string& name,
                                                        Engine& engine)
{}

template <typename T>
void VariableGlobalSingleArray<T>::put(Engine& writer, const T* data,
                                       const Dims& shape, const Mode launch)
{
  auto var = writer._defineVariable<T>();
  var.setShape(shape);
  if (writer.mpiRank() == 0) {
    var.setSelection({Dims(shape.size()), shape});
    writer.put(var, data, launch);
  }
}

template <typename T>
void VariableGlobalSingleArray<T>::get(Engine& reader, T* data,
                                       const Mode launch)
{
  // FIXME, without a setSelection, is it guaranteed that the default
  // selection is {{}, shape}?
  auto var = reader._defineVariable<T>();
  reader.get(var, data, launch);
}

// ======================================================================
// VariableLocalSingleValue

template <typename T>
VariableLocalSingleValue<T>::VariableLocalSingleValue(const std::string& name,
                                                      Engine& engine)
{}

template <typename T>
void VariableLocalSingleValue<T>::put(Engine& writer, const T& datum,
                                      const Mode launch)
{
  auto var = writer._defineVariable<T>();
  var.setShape({adios2::LocalValueDim});
  var.put(writer, datum, launch);
}

template <typename T>
void VariableLocalSingleValue<T>::get(Engine& reader, T& val, const Mode launch)
{
  auto var = reader._defineVariable<T>();
  auto shape = var.shape();
  assert(shape.size() == 1);
  auto dim0 = shape[0];
  assert(dim0 == reader.mpiSize());

  // FIXME, setSelection doesn't work, so read the whole thing
  std::vector<T> vals(shape[0]);
  var.get(reader, vals.data(), launch);
  val = vals[reader.mpiRank()];
}

} // namespace io
} // namespace kg
