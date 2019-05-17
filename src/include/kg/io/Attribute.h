
#pragma once

#include <vec3.hxx>

namespace kg
{
namespace io
{

class Engine;

// ======================================================================
// Attribute<T>
//
// single value

template <typename T, typename Enable = void>
class Attribute
{
public:
  void put(Engine& writer, const T& value, Mode launch = Mode::Deferred);
  void get(Engine& reader, T& value, Mode launch = Mode::Deferred);
};

// ======================================================================
// Attribute<std::vector>

template <class T>
class Attribute<std::vector<T>>
{
public:
  using value_type = std::vector<T>;

  void put(Engine& writer, const value_type& vec, Mode launch = Mode::Deferred);
  void get(Engine& reader, value_type& vec, Mode launch = Mode::Deferred);
};

// ======================================================================
// Attribute<Vec3>

template <class T>
class Attribute<Vec3<T>>
{
public:
  using value_type = Vec3<T>;

  void put(Engine& writer, const value_type& vec, Mode launch = Mode::Deferred);
  void get(Engine& reader, value_type& data, Mode launch = Mode::Deferred);
};

} // namespace io
} // namespace kg

#include "Attribute.inl"
