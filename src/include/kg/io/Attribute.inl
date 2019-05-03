
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
  if (writer.mpiRank() != 0) { // FIXME, should we do this?
    return;
  }
  auto attr = writer.io_.InquireAttribute<T>(writer.prefix());
  if (attr) {
    mprintf("attr '%s' already exists -- ignoring it!\n", writer.prefix().c_str());
  } else {
    writer.io_.DefineAttribute<T>(writer.prefix(), data, size);
  }
}

template <typename T>
void Attribute<T>::put(Engine& writer, const T& datum)
{
  if (writer.mpiRank() != 0) { // FIXME, should we do this?
    return;
  }
  auto attr = writer.io_.InquireAttribute<T>(writer.prefix());
  if (attr) {
    mprintf("attr '%s' already exists -- ignoring it!\n", writer.prefix().c_str());
  } else {
    writer.io_.DefineAttribute<T>(writer.prefix(), datum);
  }
}

template <typename T>
void Attribute<T>::get(Engine& reader, std::vector<T>& data)
{
  auto attr = reader.io_.InquireAttribute<T>(reader.prefix());
  assert(attr);
  data = attr.Data();
}

template <typename T>
void Attribute<T>::get(Engine& reader, T& datum)
{
  auto attr = reader.io_.InquireAttribute<T>(reader.prefix());
  assert(attr);
  auto data = attr.Data();
  assert(data.size() == 1);
  datum = data[0];
}

} // namespace detail

} // namespace io
} // namespace kg
