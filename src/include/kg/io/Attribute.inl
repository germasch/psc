
#include "Engine.h"

namespace kg
{
namespace io
{

// ======================================================================
// Attribute<T>

template <class T, class Enable>
inline void Attribute<T, Enable>::put(Engine& writer, const T& value,
                                      Mode launch)
{
  detail::Attribute<T> attr;
  writer.put(attr, value);
}

template <class T, class Enable>
inline void Attribute<T, Enable>::get(Engine& reader, T& value, Mode launch)
{
  detail::Attribute<T> attr;
  reader.get(attr, value);
}

// ======================================================================
// Attribute<std::vector<T>>

template <class T>
inline void Attribute<std::vector<T>>::put(Engine& writer,
                                           const std::vector<T>& vec,
                                           Mode launch)
{
  detail::Attribute<T> attr;
  writer.put(attr, vec.data(), vec.size());
}

template <class T>
inline void Attribute<std::vector<T>>::get(Engine& reader, std::vector<T>& vec,
                                           Mode launch)
{
  detail::Attribute<T> attr;
  reader.get(attr, vec);
}

// ======================================================================
// Attribute<Vec3<T>>

template <class T>
inline void Attribute<Vec3<T>>::put(Engine& writer, const Vec3<T>& vec,
                                    Mode launch)
{
  detail::Attribute<T> attr;
  writer.put(attr, vec.data(), 3);
}

template <class T>
inline void Attribute<Vec3<T>>::get(Engine& reader, Vec3<T>& vec, Mode launch)
{
  std::vector<T> vals;
  detail::Attribute<T> attr;
  reader.get(attr, vals);
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
  writer.put(attr, arr, N);
}

template <class T, size_t N>
inline void Attribute<T[N]>::get(Engine& reader,
                                 Attribute<T[N]>::value_type& arr,
                                 Mode launch)
{
  std::vector<T> vals;
  detail::Attribute<T> attr;
  reader.get(attr, vals);
  assert(vals.size() == N);
  std::copy(vals.begin(), vals.end(), arr);
}

} // namespace io
} // namespace kg
