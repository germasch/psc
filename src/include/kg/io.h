
#pragma once

#include <adios2.h>

namespace kg
{
namespace io
{

using Mode = adios2::Mode;
using Dims = adios2::Dims;
template <typename T>
using Box = adios2::Box<T>;

} // namespace io
} // namespace kg

#include "io/Attribute.h"
#include "io/Descr.h"
#include "io/Engine.h"
#include "io/IO.h"
