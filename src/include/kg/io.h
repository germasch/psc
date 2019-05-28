
#pragma once

#include <vector>

namespace kg
{
namespace io
{

enum class Mode
{
  Write,
  Read,
  Blocking,
  NonBlocking,
};

using size_t = std::size_t;

using Dims = std::vector<size_t>;

struct Extents
{
  Dims start;
  Dims count;
};

} // namespace io
} // namespace kg

#include "io/Descr.h"
#include "io/Engine.h"
#include "io/IO.h"
