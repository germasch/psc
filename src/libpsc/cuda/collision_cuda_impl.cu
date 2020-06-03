
#include "collision_cuda_impl.hxx"

#include "balance.hxx"
#include "cuda_collision.cuh"
#include "mparticles_cuda.hxx"
#include "../libpsc/psc_sort/psc_sort_impl.hxx"

// ======================================================================
// CollisionCuda

template <typename Mparticles, typename RngState>
CollisionCuda<Mparticles, RngState>::CollisionCuda(const Grid_t& grid,
                                                   int interval, double nu)
  : fwd_{new CudaCollision<typename Mparticles::CudaMparticles, RngState>{
    interval, nu, int(1. / grid.norm.cori + .5),
      grid.dt}}, // FIXME nicell hack
    c_(grid, interval, nu)
{}

template <typename Mparticles, typename RngState>
void CollisionCuda<Mparticles, RngState>::operator()(Mparticles& mprts)
{
#if 0
  fwd_->sort_by_cell(*mprts.cmprts());
  auto& h_mprts = mprts.template get_as<MparticlesSingle>();
  //SortCountsort2<MparticlesSingle> sort;
  //sort(h_mprts);
  c_(h_mprts);
  mprts.put_as(h_mprts);
#else
  fwd_->sort_by_cell(*mprts.cmprts());
  //mprts.cmprts()->check_ordered();
  (*fwd_)(*mprts.cmprts());
#endif
}

template <typename Mparticles, typename RngState>
int CollisionCuda<Mparticles, RngState>::interval() const
{
  return fwd_->interval();
}

template struct CollisionCuda<MparticlesCuda<BS144>>;
template struct CollisionCuda<MparticlesCuda<BS444>>;

template struct CollisionCuda<MparticlesCuda<BS144>, RngStateFake>;
