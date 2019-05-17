
#pragma once

#include <vec3.hxx>

namespace kg
{
namespace io
{

class Engine;

namespace detail
{

// ======================================================================
// detail::Attribute
//
// Handles T being one of the base adios2 types (or arrays thereof)

template <typename T>
struct Attribute
{};

}; // namespace detail

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

// ======================================================================
// Attribute<T[N]>

template <class T, size_t N>
class Attribute<T[N]> // typename
                      // std::enable_if<std::is_adios2_type<T>::value>::type>
{
public:
  using value_type = T[N];

  void put(Engine& writer, const value_type& arr, Mode launch = Mode::Deferred);
  void get(Engine& reader, value_type& arr, Mode launch = Mode::Deferred);
};

} // namespace io
} // namespace kg

#include "Attribute.inl"
