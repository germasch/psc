
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
Variable<T>::Variable(const std::string& name, adios2::IO io)
  : name_{name}, io_{io}
{}

template <typename T>
void Variable<T>::put(Engine& writer, const T& datum, const Mode launch)
{
  writer.file_.put(*this, &datum, launch);
}

template <typename T>
void Variable<T>::put(Engine& writer, const T* data, const Mode launch)
{
  writer.file_.put(*this, data, launch);
}

template <typename T>
void Variable<T>::get(Engine& reader, T& datum, const Mode launch)
{
  reader.file_.get(*this, datum, launch);
}

template <typename T>
void Variable<T>::get(Engine& reader, T* data, const Mode launch)
{
  reader.file_.get(*this, data, launch);
}

template <typename T>
void Variable<T>::setSelection(const Box<Dims>& selection)
{
  selection_ = selection;
}

template <typename T>
void Variable<T>::setMemorySelection(const Box<Dims>& selection)
{
  memory_selection_ = selection;
}

template <typename T>
void Variable<T>::setShape(const Dims& shape)
{
  shape_ = shape;
}

template <typename T>
std::string Variable<T>::name() const
{
  return name_;
}

template <typename T>
Dims Variable<T>::shape() const
{
  return shape_;
}

template <typename T>
Box<Dims> Variable<T>::selection() const
{
  return selection_;
}

template <typename T>
Box<Dims> Variable<T>::memorySelection() const
{
  return memory_selection_;
}

template <typename T>
adios2::Variable<T> Variable<T>::makeVariable() const
{
  auto& io = const_cast<adios2::IO&>(io_);
  auto var = io.InquireVariable<T>(name_);
  if (!var) {
    var = io.DefineVariable<T>(name_, shape_);
  }
  if (selection_.first.size() != 0) {
    var.SetSelection(selection_);
  }
  if (memory_selection_.first.size() != 0) {
    var.SetMemorySelection(memory_selection_);
  }

  return var;
}

} // namespace detail

} // namespace io
} // namespace kg
