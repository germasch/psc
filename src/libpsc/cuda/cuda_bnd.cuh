
#pragma once

#include "psc_fields_cuda.h"
#include "fields.hxx"
#include "cuda_bits.h"

#include "mrc_ddc_private.h"

#include <unordered_map>

#include <thrust/gather.h>
#include <thrust/scatter.h>
#include <thrust/sort.h>

// ======================================================================
// gtensor scatter etc

namespace psc
{
namespace bnd
{

template <typename T, typename TI, typename R>
void gather(const gt::gtensor<TI, 1>& map, R& buf, gt::gtensor<T, 1>& result)
{
  int n = buf.size();
  assert(map.size() == n);
  for (int i = 0; i < n; i++) {
    result[i] = buf[map[i]];
  }
}

#ifdef USE_CUDA

template <typename T, typename TI, typename R>
void gather(const gt::gtensor_device<TI, 1>& map, R& buf,
            gt::gtensor_device<T, 1>& result)
{
  thrust::gather(map.data(), map.data() + map.size(),
                 &buf[0], // FIXME, buf.data() or something would be nicer
                 result.data());
}

#endif

template <typename T, typename TI, typename R>
void scatter(const gt::gtensor<TI, 1>& map, const gt::gtensor<T, 1>& buf,
             R& result)
{
  int n = buf.size();
  assert(map.size() == n);
  for (int i = 0; i < n; i++) {
    result[map[i]] = buf[i];
  }
}

#ifdef USE_CUDA

template <typename T, typename TI, typename R>
void scatter(const gt::gtensor_device<TI, 1>& map,
             const gt::gtensor_device<T, 1>& buf, R& result)
{
  thrust::scatter(map.data(), map.data() + map.size(), buf.data(), &result[0]);
}

#endif

template <typename T, typename TI, typename R>
void scatter_add(const gt::gtensor<TI, 1>& map, const gt::gtensor<T, 1>& buf,
                 R& result)
{
  int n = buf.size();
  assert(map.size() == n);
  for (int i = 0; i < n; i++) {
    result[map[i]] += buf[i];
  }
}

#ifdef USE_CUDA

template <typename real_t>
__global__ static void k_scatter_add(const real_t* buf, const uint* map,
                                     real_t* flds, unsigned int size)
{
  int i = threadIdx.x + blockIdx.x * blockDim.x;
  if (i < size) {
    atomicAdd(&flds[map[i]], buf[i]);
  }
}

template <typename T, typename TI, typename R>
void scatter_add(const gt::gtensor_device<TI, 1>& map,
                 const gt::gtensor_device<T, 1>& buf, R& result)
{
  if (map.size() == 0)
    return;

  const int THREADS_PER_BLOCK = 256;
  dim3 dimGrid((buf.size() + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK);
  k_scatter_add<<<dimGrid, THREADS_PER_BLOCK>>>(
    buf.data().get(), map.data().get(), (&result[0]).get(), buf.size());
  cuda_sync_if_enabled();
}

#endif

} // namespace bnd
} // namespace psc

// ======================================================================
// Maps

template <typename S>
struct Maps
{
  using space_type = S;

  template <typename GT>
  Maps(mrc_ddc* ddc, mrc_ddc_pattern2* patt2, int mb, int me, Int3 ib,
       const GT& gt)
    : patt{patt2}, mb{mb}, me{me}
  {
    auto h_send = gt::host_mirror(d_send);
    auto h_recv = gt::host_mirror(d_recv);
    setup_remote_maps(h_send, h_recv, ddc, patt2, mb, me, ib, gt);
    d_send.resize(h_send.size());
    gt::copy(h_send, d_send);
    d_recv.resize(h_recv.size());
    gt::copy(h_recv, d_recv);
    // mem_bnd += allocated_bytes(d_send);
    // mem_bnd += allocated_bytes(d_recv);

    auto h_local_send = gt::host_mirror(d_local_send);
    auto h_local_recv = gt::host_mirror(d_local_recv);
    setup_local_maps(h_local_send, h_local_recv, ddc, patt2, mb, me, ib, gt);
    d_local_send.resize(h_local_send.size());
    gt::copy(h_local_send, d_local_send);
    d_local_recv.resize(h_local_recv.size());
    gt::copy(h_local_recv, d_local_recv);
    // mem_bnd += allocated_bytes(d_local_send);
    // mem_bnd += allocated_bytes(d_local_recv);
  }

  Maps(const Maps&) = delete;
  Maps(Maps&&) = default;

  ~Maps()
  {
    // mem_bnd -= allocated_bytes(d_send);
    // mem_bnd -= allocated_bytes(d_recv);
    // mem_bnd -= allocated_bytes(d_local_send);
    // mem_bnd -= allocated_bytes(d_local_recv);
  }

  // ----------------------------------------------------------------------
  // setup_remote_maps

  template <typename GT>
  static void setup_remote_maps(gt::gtensor<uint, 1>& map_send,
                                gt::gtensor<uint, 1>& map_recv, mrc_ddc* ddc,
                                struct mrc_ddc_pattern2* patt2, int mb, int me,
                                Int3 ib, const GT& gt)
  {
    struct mrc_ddc_multi* sub = mrc_ddc_multi(ddc);
    struct mrc_ddc_rank_info* ri = patt2->ri;

    map_send.resize(patt2->n_send * (me - mb));
    map_recv.resize(patt2->n_recv * (me - mb));

    uint off_send = 0, off_recv = 0;
    for (int r = 0; r < sub->mpi_size; r++) {
      if (r == sub->mpi_rank) {
        continue;
      }

      for (int i = 0; i < ri[r].n_send_entries; i++) {
        struct mrc_ddc_sendrecv_entry* se = &ri[r].send_entry[i];
        map_setup(map_send, off_send, mb, me, se->patch, se->ilo, se->ihi, ib,
                  gt);
        off_send += se->len * (me - mb);
      }
      for (int i = 0; i < ri[r].n_recv_entries; i++) {
        struct mrc_ddc_sendrecv_entry* re = &ri[r].recv_entry[i];
        map_setup(map_recv, off_recv, mb, me, re->patch, re->ilo, re->ihi, ib,
                  gt);
        off_recv += re->len * (me - mb);
      }
    }
  }

  // ----------------------------------------------------------------------
  // setup_local_maps

  template <typename GT>
  static void setup_local_maps(gt::gtensor<uint, 1>& map_send,
                               gt::gtensor<uint, 1>& map_recv, mrc_ddc* ddc,
                               struct mrc_ddc_pattern2* patt2, int mb, int me,
                               Int3 ib, const GT& gt)
  {
    struct mrc_ddc_multi* sub = mrc_ddc_multi(ddc);
    struct mrc_ddc_rank_info* ri = patt2->ri;

    uint buf_size = 0;
    for (int i = 0; i < ri[sub->mpi_rank].n_send_entries; i++) {
      struct mrc_ddc_sendrecv_entry* se = &ri[sub->mpi_rank].send_entry[i];
      if (se->ilo[0] == se->ihi[0] || se->ilo[1] == se->ihi[1] ||
          se->ilo[2] == se->ihi[2]) { // FIXME, we shouldn't even create these
        continue;
      }
      buf_size += se->len * (me - mb);
    }

    map_send.resize(buf_size);
    map_recv.resize(buf_size);

    uint off = 0;
    for (int i = 0; i < ri[sub->mpi_rank].n_send_entries; i++) {
      struct mrc_ddc_sendrecv_entry* se = &ri[sub->mpi_rank].send_entry[i];
      struct mrc_ddc_sendrecv_entry* re = &ri[sub->mpi_rank].recv_entry[i];
      if (se->ilo[0] == se->ihi[0] || se->ilo[1] == se->ihi[1] ||
          se->ilo[2] == se->ihi[2]) { // FIXME, we shouldn't even create these
        continue;
      }
      uint size = se->len * (me - mb);
      map_setup(map_send, off, mb, me, se->patch, se->ilo, se->ihi, ib, gt);
      map_setup(map_recv, off, mb, me, re->patch, re->ilo, re->ihi, ib, gt);
      off += size;
    }
  }

  template <typename GT>
  static void map_setup(gt::gtensor<uint, 1>& map, uint off, int mb, int me,
                        int p, int ilo[3], int ihi[3], Int3 ib, const GT& gt)
  {
    auto cur = &map[off];
    for (int m = mb; m < me; m++) {
      for (int iz = ilo[2]; iz < ihi[2]; iz++) {
        for (int iy = ilo[1]; iy < ihi[1]; iy++) {
          for (int ix = ilo[0]; ix < ihi[0]; ix++) {
            *cur++ = &gt(ix - ib[0], iy - ib[1], iz - ib[2], m, p) - gt.data();
          }
        }
      }
    }
  }

public:
  gt::gtensor<uint, 1, space_type> d_recv, d_send;
  gt::gtensor<uint, 1, space_type> d_local_recv, d_local_send;

  mrc_ddc_pattern2* patt;
  int mb, me;
};

extern std::size_t mem_bnd;

#define mrc_ddc_multi(ddc) mrc_to_subobj(ddc, struct mrc_ddc_multi)

// ======================================================================
// CudaBnd

template <typename MF>
struct CudaBnd
{
  using Mfields = MF;
  using real_t = typename Mfields::real_t;
  using space_type = gt::space::device;

  // ======================================================================
  // Scatter

  struct ScatterAdd
  {
    template <typename R>
    void operator()(const gt::gtensor<uint, 1>& map,
                    const gt::gtensor<real_t, 1>& buf, R& h_flds)
    {
      psc::bnd::scatter_add(map, buf, h_flds);
    }

    template <typename R>
    void operator()(const gt::gtensor_device<uint, 1>& map,
                    const gt::gtensor_device<real_t, 1>& buf, R& d_flds)
    {
      psc::bnd::scatter_add(map, buf, d_flds);
    }
  };

  struct Scatter
  {
    template <typename R>
    void operator()(const gt::gtensor<uint, 1>& map,
                    const gt::gtensor<real_t, 1>& buf, R& h_flds)
    {
      psc::bnd::scatter(map, buf, h_flds);
    }

    template <typename R>
    void operator()(const gt::gtensor_device<uint, 1>& map,
                    const gt::gtensor_device<real_t, 1>& buf, R& d_flds)
    {
      psc::bnd::scatter(map, buf, d_flds);
    }
  };

  // ----------------------------------------------------------------------
  // ctor

  CudaBnd(const Grid_t& grid, Int3 ibn)
  {
    static int pr;
    if (!pr) {
      pr = prof_register("CudaBnd_ctor", 1, 0, 0);
    }

    prof_start(pr);
    static struct mrc_ddc_funcs ddc_funcs;

    ddc_ = grid.mrc_domain().create_ddc();
    mrc_ddc_set_funcs(ddc_, &ddc_funcs);
    mrc_ddc_set_param_int3(ddc_, "ibn", ibn);
    mrc_ddc_set_param_int(ddc_, "max_n_fields", 24);
    mrc_ddc_set_param_int(ddc_, "size_of_type", sizeof(real_t));
    mrc_ddc_setup(ddc_);
    prof_stop(pr);
  }

  CudaBnd(const CudaBnd& bnd) = delete;

  CudaBnd& operator=(const CudaBnd& bnd) = delete;

  // ----------------------------------------------------------------------
  // dtor

  ~CudaBnd() { mrc_ddc_destroy(ddc_); }

  // ----------------------------------------------------------------------
  // run

  template <typename F>
  void run(Mfields& mflds, int mb, int me, mrc_ddc_pattern2* patt2,
           std::unordered_map<int, Maps<space_type>>& maps, F&& scatter)
  {
    // static int pr_ddc_run, pr_ddc_sync1, pr_ddc_sync2;
    // if (!pr_ddc_run) {
    //   pr_ddc_run = prof_register("ddc_run", 1., 0, 0);
    //   pr_ddc_sync1 = prof_register("ddc_sync1", 1., 0, 0);
    //   pr_ddc_sync2 = prof_register("ddc_sync2", 1., 0, 0);
    // }

#if 0
    prof_start(pr_ddc_sync1);
    MPI_Barrier(MPI_COMM_WORLD);
    prof_stop(pr_ddc_sync1);
#endif

    int key = mb + 100 * me;
    auto map = maps.find(key);
    if (map == maps.cend()) {
      auto pair = maps.emplace(std::make_pair(
        key, Maps<space_type>{ddc_, patt2, mb, me, -mflds.ibn(), mflds.gt()}));
      map = pair.first;
    }

    // prof_start(pr_ddc_run);
    ddc_run(map->second, patt2, mb, me, mflds, std::forward<F>(scatter));
    // prof_stop(pr_ddc_run);

#if 0
    prof_start(pr_ddc_sync2);
    MPI_Barrier(MPI_COMM_WORLD);
    prof_stop(pr_ddc_sync2);
#endif
  }

  // ----------------------------------------------------------------------
  // add_ghosts

  void add_ghosts(Mfields& mflds, int mb, int me)
  {
    mrc_ddc_multi* sub = mrc_ddc_multi(ddc_);

    run(mflds, mb, me, &sub->add_ghosts2, maps_add_, ScatterAdd{});
  }

  // ----------------------------------------------------------------------
  // fill_ghosts

  void fill_ghosts(Mfields& mflds, int mb, int me)
  {
    // FIXME
    // I don't think we need as many points, and only stencil star
    // rather then box
    mrc_ddc_multi* sub = mrc_ddc_multi(ddc_);

    run(mflds, mb, me, &sub->fill_ghosts2, maps_fill_, Scatter{});
  }

  // ----------------------------------------------------------------------
  // ddc_run

  template <typename F>
  void ddc_run(Maps<space_type>& maps, mrc_ddc_pattern2* patt2, int mb, int me,
               Mfields& mflds, F&& scatter)
  {
    // static int pr_ddc0, pr_ddc1, pr_ddc2, pr_ddc3, pr_ddc4, pr_ddc5;
    // static int pr_ddc6, pr_ddc7, pr_ddc8, pr_ddc9, pr_ddc10;
    // if (!pr_ddc1) {
    //   pr_ddc0 = prof_register("ddc0", 1., 0, 0);
    //   pr_ddc1 = prof_register("ddc1", 1., 0, 0);
    //   pr_ddc2 = prof_register("ddc2", 1., 0, 0);
    //   pr_ddc3 = prof_register("ddc3", 1., 0, 0);
    //   pr_ddc4 = prof_register("ddc4", 1., 0, 0);
    //   pr_ddc5 = prof_register("ddc5", 1., 0, 0);
    //   pr_ddc6 = prof_register("ddc6", 1., 0, 0);
    //   pr_ddc7 = prof_register("ddc7", 1., 0, 0);
    //   pr_ddc8 = prof_register("ddc8", 1., 0, 0);
    //   pr_ddc9 = prof_register("ddc9", 1., 0, 0);
    //   pr_ddc10 = prof_register("ddc10", 1., 0, 0);
    // }

#if 0
    thrust::device_ptr<real_t> d_flds{mflds.gt().data()};
    thrust::host_vector<real_t> h_flds{d_flds, d_flds + cmflds.n_fields * cmflds.n_cells};

    postReceives(maps);
    psc::bnd::gather(maps.send, h_flds, maps.send_buf.data());
    postSends(maps);

    MPI_Waitall(maps.patt->recv_cnt, maps.patt->recv_req, MPI_STATUSES_IGNORE);
    scatter(maps.recv, maps.recv_buf, h_flds);
    MPI_Waitall(maps.patt->send_cnt, maps.patt->send_req, MPI_STATUSES_IGNORE);

    // local part
    psc::bnd::gather(maps.local_send, h_flds, maps.local_buf.data());
    scatter(maps.local_recv, maps.local_buf, h_flds);
    thrust::copy(h_flds.begin(), h_flds.end(), d_flds);
#else
    auto d_flds = gt::flatten(mflds.gt());
    prof_barrier("ddc_run");

    gt::gtensor<real_t, 1, space_type> d_send_buf(maps.d_send.size());
    gt::gtensor<real_t, 1, space_type> d_recv_buf(maps.d_recv.size());
    auto h_send_buf = gt::host_mirror(d_send_buf);
    auto h_recv_buf = gt::host_mirror(d_recv_buf);

    // prof_start(pr_ddc1);
    postReceives(maps, h_recv_buf);
    // prof_stop(pr_ddc1);

    {
      // prof_start(pr_ddc2);
      psc::bnd::gather(maps.d_send, d_flds, d_send_buf);
      // prof_stop(pr_ddc2);

      // prof_start(pr_ddc3);
      gt::copy(d_send_buf, h_send_buf);
      // prof_stop(pr_ddc3);

      // prof_start(pr_ddc4);
      postSends(maps, h_send_buf);
      // prof_stop(pr_ddc4);
    }

    // local part
    {
      gt::gtensor<real_t, 1, space_type> d_local_buf(maps.d_local_send.size());
      // prof_start(pr_ddc5);
      psc::bnd::gather(maps.d_local_send, d_flds, d_local_buf);
      // prof_stop(pr_ddc5);

      // prof_start(pr_ddc6);
      scatter(maps.d_local_recv, d_local_buf, d_flds);
      // prof_stop(pr_ddc6);
    }

    {
      // prof_start(pr_ddc7);
      MPI_Waitall(maps.patt->recv_cnt, maps.patt->recv_req,
                  MPI_STATUSES_IGNORE);
      // prof_stop(pr_ddc7);

      // prof_start(pr_ddc8);
      gt::copy(h_recv_buf, d_recv_buf);
      // prof_stop(pr_ddc8);

      // prof_start(pr_ddc9);
      scatter(maps.d_recv, d_recv_buf, d_flds);
      // prof_stop(pr_ddc9);
    }

    // prof_start(pr_ddc10);
    MPI_Waitall(maps.patt->send_cnt, maps.patt->send_req, MPI_STATUSES_IGNORE);
    // prof_stop(pr_ddc10);
#endif
  }

  // ----------------------------------------------------------------------
  // postReceives

  void postReceives(Maps<space_type>& maps, gt::gtensor<real_t, 1>& recv_buf)
  {
    struct mrc_ddc_multi* sub = mrc_ddc_multi(ddc_);
    struct mrc_ddc_rank_info* ri = maps.patt->ri;
    MPI_Datatype mpi_dtype =
      MPI_FLOAT; // FIXME Mfields_traits<Mfields>::mpi_dtype();
    int mm = maps.me - maps.mb;

    maps.patt->recv_cnt = 0;
    auto p_recv = recv_buf.data();
    for (int r = 0; r < sub->mpi_size; r++) {
      if (r != sub->mpi_rank && ri[r].n_recv_entries) {
        MPI_Irecv(p_recv, ri[r].n_recv * mm, mpi_dtype, r, 0, ddc_->obj.comm,
                  &maps.patt->recv_req[maps.patt->recv_cnt++]);
        p_recv += ri[r].n_recv * mm;
      }
    }
    assert(p_recv == recv_buf.data() + recv_buf.size());
  }

  // ----------------------------------------------------------------------
  // postSends

  void postSends(Maps<space_type>& maps, gt::gtensor<real_t, 1>& send_buf)
  {
    struct mrc_ddc_multi* sub = mrc_ddc_multi(ddc_);
    struct mrc_ddc_rank_info* ri = maps.patt->ri;
    MPI_Datatype mpi_dtype =
      MPI_FLOAT; // FIXME Mfields_traits<Mfields>::mpi_dtype();
    int mm = maps.me - maps.mb;

    maps.patt->send_cnt = 0;
    auto p_send = send_buf.data();
    for (int r = 0; r < sub->mpi_size; r++) {
      if (r != sub->mpi_rank && ri[r].n_send_entries) {
        MPI_Isend(p_send, ri[r].n_send * mm, mpi_dtype, r, 0, ddc_->obj.comm,
                  &maps.patt->send_req[maps.patt->send_cnt++]);
        p_send += ri[r].n_send * mm;
      }
    }
    assert(p_send == send_buf.data() + send_buf.size());
  }

  void clear()
  {
    maps_add_.clear();
    maps_fill_.clear();
  }

private:
  mrc_ddc* ddc_;
  std::unordered_map<int, Maps<space_type>> maps_add_;
  std::unordered_map<int, Maps<space_type>> maps_fill_;
};
