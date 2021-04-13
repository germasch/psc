
#pragma once

#include "../libpsc/psc_output_fields/fields_item_fields.hxx"
#include "../libpsc/psc_output_fields/fields_item_moments_1st.hxx"
#ifdef USE_CUDA
#include "../libpsc/cuda/fields_item_moments_1st_cuda.hxx"
#endif
#include "fields_item.hxx"
#include "psc_particles_double.h"

#include <memory>

namespace detail
{

template <typename MfieldsState>
struct tfd_Item_jeh
{
  using type = Mfields<typename MfieldsState::Real>;
};

#ifdef USE_CUDA
template <>
struct tfd_Item_jeh<MfieldsStateCuda>
{
  using type = MfieldsCuda;
};
#endif

} // namespace detail

template <typename MfieldsState>
using tfd_Item_jeh_t = typename detail::tfd_Item_jeh<MfieldsState>::type;

namespace detail
{
template <typename Mparticles, typename Dim, typename Enable = void>
struct moment_selector
{
  using type = Moments_1st<Mparticles>;
  using Mfields = Mfields<typename Mparticles::real_t>;
};

#ifdef USE_CUDA
template <typename Mparticles, typename Dim>
struct moment_selector<
  Mparticles, Dim, typename std::enable_if<Mparticles::is_cuda::value>::type>
{
  using type = Moment_1st_cuda<Mparticles, Dim>;
  using Mfields = MfieldsCuda;
};
#endif
} // namespace detail

template <typename Mparticles>
using FieldsItem_Moments_1st_cc =
  typename detail::moment_selector<Mparticles, dim_yz>::type;

template <typename Mparticles>
using tfd_Item_moments_t =
  typename detail::moment_selector<Mparticles, dim_yz>::Mfields;

// ======================================================================
// OutputFieldsItemParams

struct OutputFieldsItemParams
{
  std::string data_dir = ".";
  int pfield_interval = 0;
  int pfield_first = 0;
  int tfield_interval = 0;
  int tfield_first = 0;
  int tfield_average_length = 1000000;
  int tfield_average_every = 1;
  Int3 rn = {};
  Int3 rx = {1000000, 1000000, 100000};
};

// ======================================================================
// OutputFieldsItem

template <typename Mfields, typename Writer>
class OutputFieldsItem : public OutputFieldsItemParams
{
public:
  OutputFieldsItem(const Grid_t& grid, const OutputFieldsItemParams& prm,
                   int n_comps, std::string sfx)
    : OutputFieldsItemParams{prm},
      pfield_next_{prm.pfield_first},
      tfield_next_{prm.tfield_first},
      tfd_{grid, n_comps, {}}
  {
    if (pfield_interval > 0) {
      io_pfd_.open("pfd" + sfx, prm.data_dir);
    }
    if (tfield_interval > 0) {
      io_tfd_.open("tfd" + sfx, prm.data_dir);
    }
  }

  template <typename F>
  void operator()(int timestep, F&& get_item)
  {
    if (first_time_) {
      first_time_ = false;
      if (timestep != 0) {
        pfield_next_ = timestep + pfield_interval;
        tfield_next_ = timestep + tfield_interval;
      }
    }

    bool do_pfield = pfield_interval > 0 && timestep >= pfield_next_;
    bool do_tfield = tfield_interval > 0 && timestep >= tfield_next_;
    bool doaccum_tfield =
      tfield_interval > 0 &&
      (((timestep >= (tfield_next_ - tfield_average_length + 1)) &&
        timestep % tfield_average_every == 0) ||
       timestep == 0);

    if (do_pfield || doaccum_tfield) {
      auto&& item = get_item();
      auto&& pfd = item.gt();

      if (do_pfield) {
        mpi_printf(item.grid().comm(), "***** Writing PFD output for '%s'\n",
                   item.name());
        pfield_next_ += pfield_interval;
        io_pfd_.begin_step(item.grid());
        io_pfd_.set_subset(item.grid(), rn, rx);
        io_pfd_.write(pfd, item.grid(), item.name(), item.comp_names());
        io_pfd_.end_step();
      }

      if (doaccum_tfield) {
        // tfd += pfd
        tfd_.gt() = tfd_.gt() + pfd;
        naccum_++;
      }

      if (do_tfield) {
        mpi_printf(item.grid().comm(), "***** Writing TFD output for '%s'\n",
                   item.name());
        tfield_next_ += tfield_interval;

        // convert accumulated values to correct temporal mean
        tfd_.gt() = (1. / naccum_) * tfd_.gt();

        io_tfd_.begin_step(item.grid());
        io_tfd_.set_subset(item.grid(), rn, rx);
        io_tfd_.write(tfd_.gt(), item.grid(), item.name(), item.comp_names());
        io_tfd_.end_step();
        naccum_ = 0;

        tfd_.gt().view() = 0;
      }
    }
  }

private:
  int pfield_next_;
  int tfield_next_;
  Writer io_pfd_;
  Writer io_tfd_;
  Mfields tfd_;
  int naccum_ = 0;
  bool first_time_ =
    true; // to keep track so we can skip first output on restart
};

// ======================================================================
// OutputFieldsParams

struct OutputFieldsParams
{
  OutputFieldsItemParams fields;
  OutputFieldsItemParams moments;
};

// ======================================================================
// OutputFieldsDefault

template <typename MfieldsState, typename Mparticles, typename Writer>
class OutputFieldsDefault
{
public:
  // ----------------------------------------------------------------------
  // ctor

  OutputFieldsDefault(const Grid_t& grid, const OutputFieldsParams& prm)
    : fields{grid, prm.fields, Item_jeh<MfieldsState>::n_comps(), ""},
      moments{grid, prm.moments,
              FieldsItem_Moments_1st_cc<Mparticles>::n_comps(grid), "_moments"}
  {}

  // ----------------------------------------------------------------------
  // operator()

  void operator()(MfieldsState& mflds, Mparticles& mprts)
  {
    const auto& grid = mflds._grid();

    static int pr, pr_fields, pr_moments;
    if (!pr) {
      pr = prof_register("outf", 1., 0, 0);
      pr_fields = prof_register("outf_fields", 1., 0, 0);
      pr_moments = prof_register("outf_moments", 1., 0, 0);
    }

    auto timestep = grid.timestep();
    prof_start(pr);

    prof_start(pr_fields);
    fields(timestep, [&]() { return Item_jeh<MfieldsState>(mflds); });
    prof_stop(pr_fields);

    prof_start(pr_moments);
    moments(timestep,
            [&]() { return FieldsItem_Moments_1st_cc<Mparticles>(mprts); });
    prof_stop(pr_moments);

    prof_stop(pr);
  };

public:
  OutputFieldsItem<tfd_Item_jeh_t<MfieldsState>, Writer> fields;
  OutputFieldsItem<tfd_Item_moments_t<Mparticles>, Writer> moments;
};

#ifdef xPSC_HAVE_ADIOS2

#include "writer_adios2.hxx"
template <typename MfieldsState, typename Mparticles>
using OutputFields =
  OutputFieldsDefault<MfieldsState, Mparticles, WriterADIOS2>;

#else

#include "writer_mrc.hxx"
template <typename MfieldsState, typename Mparticles>
using OutputFields = OutputFieldsDefault<MfieldsState, Mparticles, WriterMRC>;

#endif
