
#pragma once

#include <adios2.h>

namespace kg
{

using Mode = adios2::Mode;
using Dims = adios2::Dims;
template <typename T>
using Box = adios2::Box<T>;

} // namespace kg

#include "io/Attribute.h"
#include "io/Engine.h"
#include "io/IO.h"
#include "io/Manager.h"
#include "io/Variable.h"
