
#ifndef KG_STORAGE_H
#define KG_STORAGE_H

#include <gtensor/gtensor.h>

namespace kg
{

// ======================================================================
// StorageUniquePtr
//
// Mostly, std::vector would be an equivalent choice, though this
// Storagle class is move-only, preventing accidental copies

template <typename T>
class StorageUniquePtr
{
public:
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;

  StorageUniquePtr(size_t size) : data_(gt::shape(size)) {}

  const_reference operator[](int offset) const { return data_(offset); }
  reference operator[](int offset) { return data_(offset); }

  // FIXME access to underlying storage might better be avoided?
  // use of this makes assumption that storage is contiguous
  const_pointer data() const { return data_.data(); }
  pointer data() { return data_.data(); }

private:
  gt::gtensor<T, 1> data_;
};

// ======================================================================
// StorageNoOwnership

template <typename T>
class StorageNoOwnership
{
public:
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;

  KG_INLINE StorageNoOwnership(pointer data, std::size_t size)
    : data_(data, {size}, {1})
  {}

  KG_INLINE const_reference operator[](int offset) const
  {
    return data_(offset);
  }
  KG_INLINE reference operator[](int offset) { return data_(offset); }

  // FIXME access to underlying storage might better be avoided?
  // use of this makes assumption that storage is contiguous
  const_pointer data() const { return data_.data(); }
  pointer data() { return data_.data(); }

private:
  gt::gtensor_span<T, 1> data_;
};

} // namespace kg

#endif
