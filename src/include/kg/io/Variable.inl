
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
Variable<T>::Variable(const std::string& name, const Dims& shape, adios2::IO io)
  : name_{name}, shape_{shape}, io_{io}
{}

template <typename T>
void Variable<T>::put(Engine& writer, const T& datum, const Mode launch)
{
  auto var = makeVariable();
  writer.file_.put(var, &datum, launch);
}

template <typename T>
void Variable<T>::put(Engine& writer, const T* data, const Mode launch)
{
  auto var = makeVariable();
  writer.file_.put(var, data, launch);
}

template <typename T>
void Variable<T>::get(Engine& reader, T& datum, const Mode launch)
{
  auto var = makeVariable();
  reader.file_.get(var, datum, launch);
}

template <typename T>
void Variable<T>::get(Engine& reader, T* data, const Mode launch)
{
  auto var = makeVariable();
  reader.file_.get(var, data, launch);
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
Dims Variable<T>::shape() const
{
  auto var = makeVariable();
  return var.Shape();
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
