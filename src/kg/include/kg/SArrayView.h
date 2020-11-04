
#ifndef KG_SARRAY_VIEW_H
#define KG_SARRAY_VIEW_H

#include <kg/SArrayContainer.h>
#include <kg/Storage.h>

namespace kg
{

// ======================================================================
// SArrayView

template <typename T, typename L = kg::LayoutSOA, bool PRINT = false>
struct SArrayView;

template <typename T, typename L, bool PRINT>
struct SArrayContainerInnerTypes<SArrayView<T, L, PRINT>>
{
  using Layout = L;
  using Storage = StorageNoOwnership<T>;
};

template <typename T, typename L, bool PRINT>
struct SArrayView : kg::SArrayContainer<SArrayView<T, L, PRINT>>
{
  using Base = kg::SArrayContainer<SArrayView<T, L, PRINT>>;
  using Storage = typename Base::Storage;
  using real_t = typename Base::value_type;

  KG_INLINE SArrayView(const Box3& box, int n_comps, real_t* data)
    : Base{box, n_comps}, storage_{data}
  {
#ifndef __CUDA_ARCH__
    //printf("SArrayView nc %d\n", n_comps);
#endif
  }

  KG_INLINE typename Base::const_reference operator()(int m, int i, int j,
                                                      int k) const
  {
    static_assert(std::is_same<typename Base::Layout, LayoutSOA>::value,
                  "only works for SOA");
    return storage_[((m * this->im(2) + k) * this->im(1) + j) * this->im(0) +
                    i + this->off_];
  }

  KG_INLINE typename Base::reference operator()(int m, int i, int j, int k)
  {
    static_assert(std::is_same<typename Base::Layout, LayoutSOA>::value,
                  "only works for SOA");
    return storage_[((m * this->im(2) + k) * this->im(1) + j) * this->im(0) +
                    i + this->off_];
  }

private:
  Storage storage_;

  KG_INLINE Storage& storageImpl() { return storage_; }
  KG_INLINE const Storage& storageImpl() const { return storage_; }

  friend class kg::SArrayContainer<SArrayView<T, L, PRINT>>;
};

} // namespace kg

#endif
