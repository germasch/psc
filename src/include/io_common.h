
#pragma once

#include <kg/io.h>

namespace
{

inline kg::io::Dims makeDims(int m, const Int3& dims)
{
  return kg::io::Dims{size_t(m), size_t(dims[2]), size_t(dims[1]),
                      size_t(dims[0])};
}

} // namespace
