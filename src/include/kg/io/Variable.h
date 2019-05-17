
#pragma once

namespace kg
{
namespace io
{

namespace detail
{
// ======================================================================
// detail::Variable

template <typename T>
class Variable
{
public:
  using value_type = T;

  void setShape(const Dims& shape);
  void setSelection(const Box<Dims>& selection);
  void setMemorySelection(const Box<Dims>& selection);

  Dims shape() const;
  Box<Dims> selection() const;
  Box<Dims> memorySelection() const;

private:
  Dims shape_;
  Box<Dims> selection_;
  Box<Dims> memory_selection_;
};
} // namespace detail

} // namespace io
} // namespace kg

#include "Variable.inl"
