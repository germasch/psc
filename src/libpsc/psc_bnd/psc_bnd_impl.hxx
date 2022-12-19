
#pragma once

#include "psc.h"
#include "fields.hxx"
#include "bnd.hxx"
#include "balance.hxx"

#include <mrc_profile.h>
#include <mrc_ddc.h>

namespace psc
{
namespace bnd
{

template <typename S>
struct bnd_context
{
  using storage_type = S;

  storage_type& mflds_gt;
  const Int3& ib;
};

template <typename MF>
class internal
{
public:
  using real_t = typename MF::real_t;
  using storage_type = typename MF::Storage;
  using bnd_context_type = bnd_context<storage_type>;

  // ----------------------------------------------------------------------
  // ctor

  internal(const Grid_t& grid, const int ibn[3])
    : balance_generation_cnt_{psc_balance_generation_cnt}
  {
    static struct mrc_ddc_funcs ddc_funcs = {
      .copy_to_buf = copy_to_buf,
      .copy_from_buf = copy_from_buf,
      .add_from_buf = add_from_buf,
    };

    ddc_ = grid.create_ddc();
    mrc_ddc_set_funcs(ddc_, &ddc_funcs);
    mrc_ddc_set_param_int3(ddc_, "ibn", ibn);
    mrc_ddc_set_param_int(ddc_, "max_n_fields", 24);
    mrc_ddc_set_param_int(ddc_, "size_of_type", sizeof(real_t));
    assert(ibn[0] > 0 || ibn[1] > 0 || ibn[2] > 0);
    mrc_ddc_setup(ddc_);
  }

  // ----------------------------------------------------------------------
  // dtor

  ~internal() { mrc_ddc_destroy(ddc_); }

  // ----------------------------------------------------------------------
  // reset

  void reset(const Grid_t& grid)
  {
    // FIXME, not really a pretty way of doing this
    this->~internal();
    new (this) internal(grid, grid.ibn);
  }

  // ----------------------------------------------------------------------
  // add_ghosts

  void add_ghosts(const Grid_t& grid, storage_type& mflds_gt, const Int3& ib,
                  int mb, int me)
  {
    if (psc_balance_generation_cnt != balance_generation_cnt_) {
      balance_generation_cnt_ = psc_balance_generation_cnt;
      reset(grid);
    }

    // FIXME
    // I don't think we need as many points, and only stencil star
    // rather then box
    auto&& h_mflds_gt = gt::host_mirror(mflds_gt);
    gt::copy(mflds_gt, h_mflds_gt);
    bnd_context_type ctx{h_mflds_gt, ib};
    mrc_ddc_add_ghosts(ddc_, mb, me, &ctx);
    gt::copy(h_mflds_gt, mflds_gt);
  }

  // ----------------------------------------------------------------------
  // fill_ghosts

  void fill_ghosts(const Grid_t& grid, storage_type& mflds_gt, const Int3& ib,
                   int mb, int me)
  {
    if (psc_balance_generation_cnt != balance_generation_cnt_) {
      balance_generation_cnt_ = psc_balance_generation_cnt;
      reset(grid);
    }

    // FIXME
    // I don't think we need as many points, and only stencil star
    // rather then box
    auto&& h_mflds_gt = gt::host_mirror(mflds_gt);
    gt::copy(mflds_gt, h_mflds_gt);
    bnd_context_type ctx{h_mflds_gt, ib};
    mrc_ddc_fill_ghosts(ddc_, mb, me, &ctx);
    gt::copy(h_mflds_gt, mflds_gt);
  }

  // ----------------------------------------------------------------------
  // copy_to_buf

  static void copy_to_buf(int mb, int me, int p, int ilo[3], int ihi[3],
                          void* _buf, void* _ctx)
  {
    bnd_context_type* ctx = static_cast<bnd_context_type*>(_ctx);
    real_t* buf = static_cast<real_t*>(_buf);
    const Int3& ib = ctx->ib;

    for (int m = mb; m < me; m++) {
      for (int iz = ilo[2]; iz < ihi[2]; iz++) {
        for (int iy = ilo[1]; iy < ihi[1]; iy++) {
          for (int ix = ilo[0]; ix < ihi[0]; ix++) {
            MRC_DDC_BUF3(buf, m - mb, ix, iy, iz) =
              ctx->mflds_gt(ix - ib[0], iy - ib[1], iz - ib[2], m, p);
          }
        }
      }
    }
  }

  static void add_from_buf(int mb, int me, int p, int ilo[3], int ihi[3],
                           void* _buf, void* _ctx)
  {
    bnd_context_type* ctx = static_cast<bnd_context_type*>(_ctx);
    real_t* buf = static_cast<real_t*>(_buf);
    const Int3& ib = ctx->ib;

    for (int m = mb; m < me; m++) {
      for (int iz = ilo[2]; iz < ihi[2]; iz++) {
        for (int iy = ilo[1]; iy < ihi[1]; iy++) {
          for (int ix = ilo[0]; ix < ihi[0]; ix++) {
            ctx->mflds_gt(ix - ib[0], iy - ib[1], iz - ib[2], m, p) +=
              MRC_DDC_BUF3(buf, m - mb, ix, iy, iz);
          }
        }
      }
    }
  }

  static void copy_from_buf(int mb, int me, int p, int ilo[3], int ihi[3],
                            void* _buf, void* _ctx)
  {
    bnd_context_type* ctx = static_cast<bnd_context_type*>(_ctx);
    real_t* buf = static_cast<real_t*>(_buf);
    const Int3& ib = ctx->ib;

    for (int m = mb; m < me; m++) {
      for (int iz = ilo[2]; iz < ihi[2]; iz++) {
        for (int iy = ilo[1]; iy < ihi[1]; iy++) {
          for (int ix = ilo[0]; ix < ihi[0]; ix++) {
            ctx->mflds_gt(ix - ib[0], iy - ib[1], iz - ib[2], m, p) =
              MRC_DDC_BUF3(buf, m - mb, ix, iy, iz);
          }
        }
      }
    }
  }

private:
  mrc_ddc* ddc_;
  int balance_generation_cnt_;
};

} // namespace bnd
} // namespace psc

template <typename MF>
struct Bnd_ : BndBase
{
  using Mfields = MF;
  using real_t = typename Mfields::real_t;
  using storage_type = typename Mfields::Storage;

  // ----------------------------------------------------------------------
  // ctor

  Bnd_(const Grid_t& grid, const int ibn[3])
    : bnd_{grid, ibn}, balance_generation_cnt_{psc_balance_generation_cnt}
  {}

  // ----------------------------------------------------------------------
  // reset

  void reset(const Grid_t& grid)
  {
    // FIXME, not really a pretty way of doing this
    this->~Bnd_();
    new (this) Bnd_(grid, grid.ibn);
  }

  // ----------------------------------------------------------------------
  // add_ghosts

  void add_ghosts(const Grid_t& grid, storage_type& mflds_gt, const Int3& ib,
                  int mb, int me)
  {
    bnd_.add_ghosts(grid, mflds_gt, ib, mb, me);
  }

  void add_ghosts(Mfields& mflds, int mb, int me)
  {
    bnd_.add_ghosts(mflds.grid(), mflds.storage(), mflds.ib(), mb, me);
  }

  // ----------------------------------------------------------------------
  // fill_ghosts

  void fill_ghosts(const Grid_t& grid, storage_type& mflds_gt, const Int3& ib,
                   int mb, int me)
  {
    bnd_.fill_ghosts(grid, mflds_gt, ib, mb, me);
  }

  void fill_ghosts(Mfields& mflds, int mb, int me)
  {
    bnd_.fill_ghosts(mflds.grid(), mflds.storage(), mflds.ib(), mb, me);
  }

private:
  psc::bnd::internal<Mfields> bnd_;
  int balance_generation_cnt_;
};
