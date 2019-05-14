
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
Variable<T>::Variable(const std::string &name, const Dims& shape, adios2::IO io)
{
  var_ = io.InquireVariable<T>(name);
  if (!var_) {
    var_ = io.DefineVariable<T>(name, shape);
  }
}
  
template <typename T>
void Variable<T>::put(Engine& writer, const T& datum, const Mode launch)
{
  writer.file_.put(var_, &datum, launch);
}

template <typename T>
void Variable<T>::put(Engine& writer, const T* data, const Mode launch)
{
  writer.file_.put(var_, data, launch);
}

template <typename T>
void Variable<T>::get(Engine& reader, T& datum, const Mode launch)
{
  reader.file_.get(var_, datum, launch);
}

template <typename T>
void Variable<T>::get(Engine& reader, T* data, const Mode launch)
{
  reader.file_.get(var_, data, launch);
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
  shape_ = shape;
  var_.SetShape(shape);
}

template <typename T>
Dims Variable<T>::shape() const
{
  return var_.Shape();
}

} // namespace detail

} // namespace io
} // namespace kg
