
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
Variable<T>::Variable(const std::string& name)
  : name_{name}
{}

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

} // namespace detail

} // namespace io
} // namespace kg
