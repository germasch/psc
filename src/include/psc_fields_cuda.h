
#ifndef PSC_FIELDS_CUDA_H
#define PSC_FIELDS_CUDA_H

#include <mpi.h>
#include "fields3d.hxx"
#include "fields_traits.hxx"

#include "psc_fields_single.h"

#include "mrc_json.h"

// ======================================================================
// MfieldsCuda

struct MfieldsCuda;

template <>
struct MfieldsCRTPInnerTypes<MfieldsCuda>
{
  using Storage = gt::gtensor<float, 5, gt::space::device>;
};

struct MfieldsCuda
  : MfieldsBase
  , MfieldsCRTP<MfieldsCuda>
{
  using real_t = float;
  using Real = real_t;
  using Base = MfieldsCRTP<MfieldsCuda>;
  using Storage = typename Base::Storage;
  using space = gt::space::device;

  MfieldsCuda(const Grid_t& grid, int n_fields, Int3 ibn)
    : MfieldsBase{grid, n_fields, ibn},
      Base(n_fields, {-ibn, grid.ldims + 2 * ibn}, grid.n_patches()),
      storage_(gt::shape(Base::box().im(0), Base::box().im(1),
                         Base::box().im(2), n_fields, Base::n_patches()))
  {
    thrust::fill(storage_.data(), storage_.data() + storage_.size(), real_t{});
  }

  const Grid_t& grid() const { return _grid(); }

  void reset(const Grid_t& new_grid) override
  {
    *this = MfieldsCuda(new_grid, n_comps(), ibn());
  }

  auto gt()
  {
    return gt::adapt_device(storage_.data().get(), storage_.shape());
  }

  auto gt() const
  {
    return gt::adapt_device(storage_.data().get(), storage_.shape());
  }

  static const Convert convert_to_, convert_from_;
  const Convert& convert_to() override { return convert_to_; }
  const Convert& convert_from() override { return convert_from_; }

private:
  Storage storage_;

  Storage& storageImpl() { return storage_; }
  const Storage& storageImpl() const { return storage_; }

  friend class MfieldsCRTP<MfieldsCuda>;
};

// ======================================================================
// MfieldsStateCuda

struct MfieldsStateCuda : MfieldsStateBase
{
  using real_t = MfieldsCuda::real_t;
  using space = gt::space::device;

  MfieldsStateCuda(const Grid_t& grid)
    : MfieldsStateBase{grid, NR_FIELDS, grid.ibn},
      mflds_{grid, NR_FIELDS, grid.ibn}
  {}

  int n_patches() const { return mflds_.n_patches(); }
  int n_comps() const { return mflds_.n_comps(); }

  const Grid_t& grid() const { return *grid_; }

  static const Convert convert_to_, convert_from_;
  const Convert& convert_to() override { return convert_to_; }
  const Convert& convert_from() override { return convert_from_; }

  MfieldsCuda& mflds() { return mflds_; }
  const MfieldsCuda& mflds() const { return mflds_; }

  auto gt() { return mflds_.gt(); }
  auto& storage() { return mflds_.storage(); }
  auto& storage() const { return mflds_.storage(); }

private:
  MfieldsCuda mflds_;
};

template <>
struct Mfields_traits<MfieldsCuda>
{
  static constexpr const char* name = "cuda";
};

namespace detail
{
template <typename T>
struct Mfields_from_type_space<T, gt::space::device>
{
  static_assert(std::is_same<T, float>::value, "CUDA only supports float");
  using type = MfieldsCuda;
};
} // namespace detail

// ----------------------------------------------------------------------
// FIXME hacky workaround for lack of gt::view on device

template <typename E>
GT_INLINE auto view_patch(E&& gt, int p)
{
  return gt::adapt_device<4>(
    (&gt(0, 0, 0, 0, p)).get(),
    {gt.shape(0), gt.shape(1), gt.shape(2), gt.shape(3)});
}

#endif
