
#include "psc_config.hxx"
#include <psc.h>
#include <psc.hxx>
#include <psc_fields_single.h>
#include <psc_particles_single.h>

#include "balance.hxx"
#include "bnd_particles.hxx"
#include "checks.hxx"
#include "collision.hxx"
#include "marder.hxx"
#include "push_fields.hxx"
#include "push_particles.hxx"
#include "sort.hxx"

#include "setup_fields.hxx"
#include "setup_particles.hxx"

#include "../src/libpsc/psc_output_fields/fields_item_moments_1st.hxx"

#include <mrc_params.h>

#include <math.h>

// ======================================================================
// PscBubbleParams

struct PscBubbleParams
{
  double BB;
  double nnb;
  double nn0;
  double MMach;
  double LLn;
  double LLB;
  double LLz;
  double LLy;
  double TTe;
  double TTi;
  double MMi;
};

// ======================================================================
// Global parameters

namespace
{

// Parameters specific to this run. They don't really need to be collected in a
// struct, but maybe it's nice that they are
PscBubbleParams g;

// This is a set of generic PSC params (see include/psc.hxx),
// like number of steps to run, etc, which also should be set by the case
PscParams psc_params;

} // namespace

// ======================================================================
// PSC configuration
//
// This sets up compile-time configuration for the code, in particular
// what data structures and algorithms to use
//
// EDIT to change order / floating point type / cuda / 2d/3d

using Dim = dim_yz;
using PscConfig = PscConfig1vbecSingle<Dim>;

// ----------------------------------------------------------------------

using MfieldsState = PscConfig::MfieldsState;
using Mparticles = PscConfig::Mparticles;
using Balance = PscConfig::Balance_t;
using Collision = PscConfig::Collision_t;
using Checks = PscConfig::Checks_t;
using Marder = PscConfig::Marder_t;
using OutputParticles = PscConfig::OutputParticles;

// ======================================================================
// setupGrid
//
// This helper function is responsible for setting up the "Grid",
// which is really more than just the domain and its decomposition, it
// also encompasses PC normalization parameters, information about the
// particle kinds, etc.

Grid_t* setupGrid()
{
  auto domain = Grid_t::Domain{{1, 128, 512},
                               {g.LLn, g.LLy, g.LLz},
                               {0., -.5 * g.LLy, -.5 * g.LLz},
                               {1, 1, 4}};

  auto bc =
    psc::grid::BC{{BND_FLD_PERIODIC, BND_FLD_PERIODIC, BND_FLD_PERIODIC},
                  {BND_FLD_PERIODIC, BND_FLD_PERIODIC, BND_FLD_PERIODIC},
                  {BND_PRT_PERIODIC, BND_PRT_PERIODIC, BND_PRT_PERIODIC},
                  {BND_PRT_PERIODIC, BND_PRT_PERIODIC, BND_PRT_PERIODIC}};

  auto kinds = Grid_t::Kinds(NR_KINDS);
  kinds[KIND_ELECTRON] = {-1., 1., "e"};
  kinds[KIND_ION] = {1., 100., "i"};

  mpi_printf(MPI_COMM_WORLD, "lambda_D = %g\n", sqrt(g.TTe));

  // --- generic setup
  auto norm_params = Grid_t::NormalizationParams::dimensionless();
  norm_params.nicell = 100;

  double dt = psc_params.cfl * courant_length(domain);
  Grid_t::Normalization norm{norm_params};

  Int3 ibn = {2, 2, 2};
  if (Dim::InvarX::value) {
    ibn[0] = 0;
  }
  if (Dim::InvarY::value) {
    ibn[1] = 0;
  }
  if (Dim::InvarZ::value) {
    ibn[2] = 0;
  }

  return new Grid_t{domain, bc, kinds, norm, dt, -1, ibn};
}

// ======================================================================
// PscBubble

struct PscBubble : Psc<PscConfig>
{
  using Base = Psc<PscConfig>;

  // ----------------------------------------------------------------------
  // ctor

  PscBubble(const PscParams& params, Grid_t& grid, MfieldsState& mflds,
            Mparticles& mprts, Balance& balance, Collision& collision,
            Checks& checks, Marder& marder, OutputFieldsC& outf,
	    OutputParticles& outp)
  {
    auto comm = grid.comm();

    Base::p_ = params;

    Base::define_grid(grid);
    Base::define_field_array(mflds);
    Base::define_particles(mprts);

    Base::balance_.reset(&balance);
    Base::collision_.reset(&collision);
    Base::checks_.reset(&checks);
    Base::marder_.reset(&marder);

    Base::outf_.reset(&outf);
    Base::outp_.reset(&outp);

    // --- partition particles and initial balancing
    mpi_printf(comm, "**** Partitioning...\n");
    auto n_prts_by_patch = setup_initial_partition();
    balance_->initial(grid_, n_prts_by_patch);
    // balance::initial does not rebalance particles, because the old way of
    // doing this does't even have the particle data structure created yet --
    // FIXME?
    mprts_->reset(grid);

    mpi_printf(comm, "**** Setting up particles...\n");
    setup_initial_particles(*mprts_, n_prts_by_patch);

    mpi_printf(comm, "**** Setting up fields...\n");
    setup_initial_fields(*mflds_);

    Base::init();
    Base::initialize();
  }

  void init_npt(int kind, double crd[3], psc_particle_npt& npt)
  {
    double V0 = g.MMach * sqrt(g.TTe / g.MMi);

    double r1 = sqrt(sqr(crd[2]) + sqr(crd[1] + .5 * g.LLy));
    double r2 = sqrt(sqr(crd[2]) + sqr(crd[1] - .5 * g.LLy));

    npt.n = g.nnb;
    if (r1 < g.LLn) {
      npt.n += (g.nn0 - g.nnb) * sqr(cos(M_PI / 2. * r1 / g.LLn));
      if (r1 > 0.0) {
        npt.p[2] += V0 * sin(M_PI * r1 / g.LLn) * crd[2] / r1;
        npt.p[1] += V0 * sin(M_PI * r1 / g.LLn) * (crd[1] + .5 * g.LLy) / r1;
      }
    }
    if (r2 < g.LLn) {
      npt.n += (g.nn0 - g.nnb) * sqr(cos(M_PI / 2. * r2 / g.LLn));
      if (r2 > 0.0) {
        npt.p[2] += V0 * sin(M_PI * r2 / g.LLn) * crd[2] / r2;
        npt.p[1] += V0 * sin(M_PI * r2 / g.LLn) * (crd[1] - .5 * g.LLy) / r2;
      }
    }

    switch (kind) {
      case KIND_ELECTRON:
        // electron drift consistent with initial current
        if ((r1 <= g.LLn) && (r1 >= g.LLn - 2. * g.LLB)) {
          npt.p[0] = -g.BB * M_PI / (2. * g.LLB) *
                     cos(M_PI * (g.LLn - r1) / (2. * g.LLB)) / npt.n;
        }
        if ((r2 <= g.LLn) && (r2 >= g.LLn - 2. * g.LLB)) {
          npt.p[0] = -g.BB * M_PI / (2. * g.LLB) *
                     cos(M_PI * (g.LLn - r2) / (2. * g.LLB)) / npt.n;
        }

        npt.T[0] = g.TTe;
        npt.T[1] = g.TTe;
        npt.T[2] = g.TTe;
        break;
      case KIND_ION:
        npt.T[0] = g.TTi;
        npt.T[1] = g.TTi;
        npt.T[2] = g.TTi;
        break;
      default:
        assert(0);
    }
  }

  // ----------------------------------------------------------------------
  // setup_initial_partition

  std::vector<uint> setup_initial_partition()
  {
    SetupParticles<Mparticles> setup_particles;
    return setup_particles.setup_partition(
      grid(), [&](int kind, double crd[3], psc_particle_npt& npt) {
        this->init_npt(kind, crd, npt);
      });
  }

  // ----------------------------------------------------------------------
  // setup_initial_particles

  void setup_initial_particles(Mparticles& mprts,
                               std::vector<uint>& n_prts_by_patch)
  {
    SetupParticles<Mparticles> setup_particles;
    setup_particles.setup_particles(
      mprts, n_prts_by_patch,
      [&](int kind, double crd[3], psc_particle_npt& npt) {
        this->init_npt(kind, crd, npt);
      });
  }

  // ----------------------------------------------------------------------
  // setup_initial_fields

  void setup_initial_fields(MfieldsState& mflds)
  {
    setupFields(grid(), mflds, [&](int m, double crd[3]) {
      double z1 = crd[2];
      double y1 = crd[1] + .5 * g.LLy;
      double r1 = sqrt(sqr(z1) + sqr(y1));
      double z2 = crd[2];
      double y2 = crd[1] - .5 * g.LLy;
      double r2 = sqrt(sqr(z2) + sqr(y2));

      double rv = 0.;
      switch (m) {
        case HZ:
          if ((r1 < g.LLn) && (r1 > g.LLn - 2 * g.LLB)) {
            rv += -g.BB * sin(M_PI * (g.LLn - r1) / (2. * g.LLB)) * y1 / r1;
          }
          if ((r2 < g.LLn) && (r2 > g.LLn - 2 * g.LLB)) {
            rv += -g.BB * sin(M_PI * (g.LLn - r2) / (2. * g.LLB)) * y2 / r2;
          }
          return rv;

        case HY:
          if ((r1 < g.LLn) && (r1 > g.LLn - 2 * g.LLB)) {
            rv += g.BB * sin(M_PI * (g.LLn - r1) / (2. * g.LLB)) * z1 / r1;
          }
          if ((r2 < g.LLn) && (r2 > g.LLn - 2 * g.LLB)) {
            rv += g.BB * sin(M_PI * (g.LLn - r2) / (2. * g.LLB)) * z2 / r2;
          }
          return rv;

        case EX:
          if ((r1 < g.LLn) && (r1 > g.LLn - 2 * g.LLB)) {
            rv += g.MMach * sqrt(g.TTe / g.MMi) * g.BB *
                  sin(M_PI * (g.LLn - r1) / (2. * g.LLB)) *
                  sin(M_PI * r1 / g.LLn);
          }
          if ((r2 < g.LLn) && (r2 > g.LLn - 2 * g.LLB)) {
            rv += g.MMach * sqrt(g.TTe / g.MMi) * g.BB *
                  sin(M_PI * (g.LLn - r2) / (2. * g.LLB)) *
                  sin(M_PI * r2 / g.LLn);
          }
          return rv;

          // FIXME, JXI isn't really needed anymore (?)
        case JXI:
          if ((r1 < g.LLn) && (r1 > g.LLn - 2 * g.LLB)) {
            rv += g.BB * M_PI / (2. * g.LLB) *
                  cos(M_PI * (g.LLn - r1) / (2. * g.LLB));
          }
          if ((r2 < g.LLn) && (r2 > g.LLn - 2 * g.LLB)) {
            rv += g.BB * M_PI / (2. * g.LLB) *
                  cos(M_PI * (g.LLn - r2) / (2. * g.LLB));
          }
          return rv;

        default:
          return 0.;
      }
    });
  }
};

// ======================================================================
// run

static void run()
{
  mpi_printf(MPI_COMM_WORLD, "*** Setting up...\n");

  // ----------------------------------------------------------------------
  // Set up a bunch of parameters

  // -- set some generic PSC parameters
  psc_params.nmax = 1000; // 32000;
  psc_params.stats_every = 100;

  // -- start from checkpoint:
  //
  // Uncomment when wanting to start from a checkpoint, ie.,
  // instead of setting up grid, particles and state fields here,
  // they'll be read from a file
  // FIXME: This parameter would be a good candidate to be provided
  // on the command line, rather than requiring recompilation when change.

  // read_checkpoint_filename = "checkpoint_500.bp";

  // -- Set some parameters specific to this case
  g.BB = .07;
  g.nnb = .1;
  g.nn0 = 1.;
  g.MMach = 3.;
  g.LLn = 200.;
  g.LLB = 200. / 6.;
  g.TTe = .02;
  g.TTi = .02;
  g.MMi = 100.;

  g.LLy = 2. * g.LLn;
  g.LLz = 3. * g.LLn;

  // ----------------------------------------------------------------------
  // Set up grid, state fields, particles

  auto grid_ptr = setupGrid();
  auto& grid = *grid_ptr;
  auto& mflds = *new MfieldsState{grid};
  auto& mprts = *new Mparticles{grid};

  // ----------------------------------------------------------------------
  // Set up various objects needed to run this case

  // -- Balance
  psc_params.balance_interval = 0;
  auto& balance = *new Balance{psc_params.balance_interval, .1};

  // -- Sort
  psc_params.sort_interval = 10;

  // -- Collision
  int collision_interval = 10;
  double collision_nu = .1;
  auto& collision = *new Collision{grid, collision_interval, collision_nu};

  // -- Checks
  ChecksParams checks_params{};
  auto& checks = *new Checks{grid, MPI_COMM_WORLD, checks_params};

  // -- Marder correction
  double marder_diffusion = 0.9;
  int marder_loop = 3;
  bool marder_dump = false;
  psc_params.marder_interval = 0; // 5
  auto& marder = *new Marder(grid, marder_diffusion, marder_loop, marder_dump);

  // ----------------------------------------------------------------------
  // Set up output
  //
  // FIXME, this really is too complicated and not very flexible

  // -- output fields
  OutputFieldsCParams outf_params{};
  outf_params.pfield_step = 10;
  std::vector<std::unique_ptr<FieldsItemBase>> outf_items;
  outf_items.emplace_back(new FieldsItem_E_cc(grid));
  outf_items.emplace_back(new FieldsItem_H_cc(grid));
  outf_items.emplace_back(new FieldsItem_J_cc(grid));
  outf_items.emplace_back(new FieldsItem_n_1st_cc<Mparticles>(grid));
  outf_items.emplace_back(new FieldsItem_v_1st_cc<Mparticles>(grid));
  outf_items.emplace_back(new FieldsItem_T_1st_cc<Mparticles>(grid));
  auto& outf = *new OutputFieldsC{grid, outf_params, std::move(outf_items)};

  // -- output particles
  OutputParticlesParams outp_params{};
  outp_params.every_step = 0;
  outp_params.data_dir = ".";
  outp_params.basename = "prt";
  auto& outp = *new OutputParticles{grid, outp_params};

  PscBubble psc(psc_params, grid, mflds, mprts, balance, collision, checks,
                marder, outf, outp);

  psc.integrate();
}

// ======================================================================
// main

int main(int argc, char** argv)
{
  psc_init(argc, argv);

  run();

  libmrc_params_finalize();
  MPI_Finalize();

  return 0;
}
