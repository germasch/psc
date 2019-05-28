
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
  Sync,
  Deferred,
};

using size_t = std::size_t;

using Dims = std::vector<size_t>;

using Extents = std::pair<Dims, Dims>;

} // namespace io
} // namespace kg

#include "io/Descr.h"
#include "io/Engine.h"
#include "io/IO.h"
