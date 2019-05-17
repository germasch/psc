
#include "Engine.h"

namespace kg
{
namespace io
{

namespace detail
{

template <typename T>
void Attribute<T>::put(Engine& writer, const T* data, size_t size)
{
  writer.put(*this, data, size);
}

template <typename T>
void Attribute<T>::put(Engine& writer, const T& datum)
{
  writer.put(*this, datum);
}

template <typename T>
void Attribute<T>::get(Engine& reader, std::vector<T>& data)
{
  reader.get(*this, data);
}

template <typename T>
void Attribute<T>::get(Engine& reader, T& datum)
{
  reader.get(*this, datum);
}

} // namespace detail

// ======================================================================
// Attribute<T>

template <class T, class Enable>
inline void Attribute<T, Enable>::put(Engine& writer, const T& value,
                                      Mode launch)
{
  detail::Attribute<T> attr;
  attr.put(writer, value);
}

template <class T, class Enable>
inline void Attribute<T, Enable>::get(Engine& reader, T& value, Mode launch)
{
  detail::Attribute<T> attr;
  attr.get(reader, value);
}

// ======================================================================
// Attribute<std::vector<T>>

template <class T>
inline void Attribute<std::vector<T>>::put(Engine& writer,
                                           const std::vector<T>& vec,
                                           Mode launch)
{
  detail::Attribute<T> attr;
  attr.put(writer, vec.data(), vec.size());
}

template <class T>
inline void Attribute<std::vector<T>>::get(Engine& reader, std::vector<T>& vec,
                                           Mode launch)
{
  detail::Attribute<T> attr;
  attr.get(reader, vec);
}

// ======================================================================
// Attribute<Vec3<T>>

template <class T>
inline void Attribute<Vec3<T>>::put(Engine& writer, const Vec3<T>& vec,
                                    Mode launch)
{
  detail::Attribute<T> attr;
  attr.put(writer, vec.data(), 3);
}

template <class T>
inline void Attribute<Vec3<T>>::get(Engine& reader, Vec3<T>& vec, Mode launch)
{
  std::vector<T> vals;
  detail::Attribute<T> attr;
  attr.get(reader, vals);
  assert(vals.size() == 3);
  vec = {vals[0], vals[1], vals[2]};
}

// ======================================================================
// Attribute<T[N]>

template <class T, size_t N>
inline void Attribute<T[N]>::put(Engine& writer,
                                 const Attribute<T[N]>::value_type& arr,
                                 Mode launch)
{
  detail::Attribute<T> attr;
  attr.put(writer, arr, N);
}

template <class T, size_t N>
inline void Attribute<T[N]>::get(Engine& reader,
                                 Attribute<T[N]>::value_type& arr,
                                 Mode launch)
{
  std::vector<T> vals;
  detail::Attribute<T> attr;
  attr.get(reader, vals);
  assert(vals.size() == N);
  std::copy(vals.begin(), vals.end(), arr);
}

} // namespace io
} // namespace kg
