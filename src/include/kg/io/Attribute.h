
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
{
  void put(Engine& writer, const T* data, size_t size);
  void put(Engine& writer, const T& datum);

  void get(Engine& reader, std::vector<T>& data);
  void get(Engine& reader, T& datum);
};

}; // namespace detail

// ======================================================================
// Attribute<T>
//
// single value

template <typename T, typename Enable = void>
class Attribute
{
public:
  void put(Engine& writer, const T& value, const Mode launch = Mode::Deferred)
  {
    detail::Attribute<T> attr;
    attr.put(writer, value);
  }

  void get(Engine& reader, T& value, const Mode launch = Mode::Deferred)
  {
    detail::Attribute<T> attr;
    attr.get(reader, value);
  }
};

// ======================================================================
// Attribute<std::vector>

template <class T>
class Attribute<std::vector<T>>
{
public:
  using value_type = std::vector<T>;

  void put(Engine& writer, const value_type& vec,
           const Mode launch = Mode::Deferred)
  {
    detail::Attribute<T> attr;
    attr.put(writer, vec.data(), vec.size());
  }

  void get(Engine& reader, value_type& vec, const Mode launch = Mode::Deferred)
  {
    detail::Attribute<T> attr;
    attr.get(reader, vec);
  }
};

// ======================================================================
// Attribute<Vec3>

template <class T>
class Attribute<Vec3<T>>
{
public:
  using value_type = Vec3<T>;

  void put(Engine& writer, const value_type& vec,
           const Mode launch = Mode::Deferred)
  {
    detail::Attribute<T> attr;
    attr.put(writer, vec.data(), 3);
  }

  void get(Engine& reader, value_type& data, const Mode launch = Mode::Deferred)
  {
    std::vector<T> vals;
    detail::Attribute<T> attr;
    attr.get(reader, vals);
    assert(vals.size() == 3);
    data = {vals[0], vals[1], vals[2]};
  }
};

// ======================================================================
// Attribute<T[N]>

template <class T, std::size_t N>
class Attribute<T[N]> // typename
                      // std::enable_if<std::is_adios2_type<T>::value>::type>
{
public:
  using value_type = T[N];

  void put(Engine& writer, const value_type& arr,
           const Mode launch = Mode::Deferred)
  {
    detail::Attribute<T> attr;
    attr.put(writer, arr, N);
  }

  void get(Engine& reader, value_type& arr, const Mode launch = Mode::Deferred)
  {
    std::vector<T> vals;
    detail::Attribute<T> attr;
    attr.get(reader, vals);
    assert(vals.size() == N);
    std::copy(vals.begin(), vals.end(), arr);
  }
};

} // namespace io
} // namespace kg

#include "Attribute.inl"
