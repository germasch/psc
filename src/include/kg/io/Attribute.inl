
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

} // namespace io
} // namespace kg
