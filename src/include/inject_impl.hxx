

#include "../libpsc/psc_output_fields/fields_item_moments_1st.hxx"
#include <bnd.hxx>
#include <fields.hxx>
#include <fields_item.hxx>
#include <inject.hxx>

#include <stdlib.h>
#include <string>

// ======================================================================
// Inject_

template <typename _Mparticles, typename _Mfields, typename Target_t,
          typename _ItemMoment>
struct Inject_ : InjectBase
{
  using Mfields = _Mfields;
  using Mparticles = _Mparticles;
  using real_t = typename Mparticles::real_t;
  using ItemMoment_t = _ItemMoment;
  using SetupParticles = ::SetupParticles<Mparticles>;

  // ----------------------------------------------------------------------
  // ctor

  Inject_(const Grid_t& grid, int interval, int tau, int kind_n,
          Target_t target, SetupParticles& setup_particles)
    : InjectBase{interval, tau, kind_n},
      target_{target},
      setup_particles_{setup_particles}
  {}

  // ----------------------------------------------------------------------
  // operator()

  void operator()(Mparticles& mprts)
  {
    static int pr, pr_A, pr_B, pr_C;
    if (!pr) {
      pr = prof_register("Inject", 1., 0, 0);
      pr_A = prof_register("Inject moment", 1., 0, 0);
      pr_B = prof_register("Inject get_as", 1., 0, 0);
      pr_C = prof_register("Inject prts", 1., 0, 0);
    }

    prof_start(pr);
    const auto& grid = mprts.grid();

    prof_start(pr_A);
    ItemMoment_t moment_n(mprts);
    auto mres = evalMfields(moment_n);
    prof_stop(pr_A);
    prof_start(pr_B);
    auto& mf_n = mres.template get_as<Mfields>(kind_n, kind_n + 1);
    prof_start(pr_B);

    prof_start(pr_C);
    real_t fac = (interval * grid.dt / tau) / (1. + interval * grid.dt / tau);

    auto lf_init_npt = [&](int kind, Double3 pos, int p, Int3 idx,
                           psc_particle_npt& npt) {
      if (target_.is_inside(pos)) {
        target_.init_npt(kind, pos, npt);
        npt.n -= mf_n[p](kind_n, idx[0], idx[1], idx[2]);
        if (npt.n < 0) {
          npt.n = 0;
        }
        npt.n *= fac;
      }
    };

    setup_particles_.setupParticles(mprts, lf_init_npt);
    prof_stop(pr_C);

    mres.put_as(mf_n, 0, 0);
    prof_stop(pr);
  }

private:
  Target_t target_;
  SetupParticles setup_particles_;
};

// ======================================================================
// InjectSelector
//
// FIXME, should go away eventually

template <typename Mparticles, typename InjectShape, typename Dim,
          typename Enable = void>
struct InjectSelector
{
  using Inject =
    Inject_<Mparticles, MfieldsC, InjectShape,
            Moment_n_1st<Mparticles, MfieldsC>>; // FIXME, shouldn't
                                                 // always use MfieldsC
};

#ifdef USE_CUDA

#include "../libpsc/cuda/fields_item_moments_1st_cuda.hxx"
#include <psc_fields_single.h>

// This not particularly pretty template arg specializes InjectSelector for all
// CUDA Mparticles types
template <typename Mparticles, typename InjectShape, typename Dim>
struct InjectSelector<Mparticles, InjectShape, Dim,
                      typename std::enable_if<Mparticles::is_cuda::value>::type>
{
  using Inject = Inject_<Mparticles, MfieldsSingle, InjectShape,
                         Moment_n_1st_cuda<Mparticles, Dim>>;
};

#endif
