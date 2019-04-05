
#include "Manager.h"

namespace kg
{

IO::IO(io::Manager& mgr, const char* name) : io_{mgr.ad_.DeclareIO(name)} {}

template <typename T>
detail::Variable<T> IO::_defineVariable(const std::string& name,
                                        const Dims& shape, const Dims& start,
                                        const Dims& count,
                                        const bool constantDims)
{
  auto var = io_.InquireVariable<T>(name);
  if (var) {
    return var;
  } else {
    return io_.DefineVariable<T>(name, shape, start, count, constantDims);
  }
}

template <typename T>
Variable<T> IO::defineVariable(const std::string& name)
{
  return {name, *this};
}

template <class T>
Variable<T> IO::inquireVariable(const std::string& name)
{
  return io_.InquireVariable<T>(name);
}

} // namespace kg
