
#pragma once

#include "Engine.h"

namespace kg
{

namespace detail
{
template <typename T>
struct Variable;
}

namespace io
{
struct Manager;
}

template <typename T>
struct Variable;

template <typename T, typename Enable = void>
struct Attribute;

// ======================================================================
// IO

struct IO
{
  IO(io::Manager& mgr, const char* name);

  Engine open(const std::string& name, const adios2::Mode mode);

  template <typename T>
  detail::Variable<T> _defineVariable(const std::string& name,
                                      const Dims& shape = Dims(),
                                      const Dims& start = Dims(),
                                      const Dims& count = Dims(),
                                      const bool constantDims = false);

  template <typename T>
  Variable<T> defineVariable(const std::string& name);

  template <class T>
  Variable<T> inquireVariable(const std::string& name);

  // private:
  adios2::IO io_;
};

} // namespace kg

#include "IO.inl"
