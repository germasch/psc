
#include <psc.hxx>
#include <psc_bits.h>
#include <setup_fields.hxx>
#include <setup_particles.hxx>

#include "DiagnosticsDefault.h"
#include "OutputFieldsDefault.h"
#include "psc_config.hxx"

#ifdef USE_CUDA
#include "cuda_bits.h"
#endif
#define PRINT(c)                                                               \
  {                                                                            \
    std::cout << "JOHN " << c << std::endl;                                    \
  }
// ======================================================================
// Particle kinds
//
// Particle kinds can be used to define different species, or different
// populations of the same species
//
// Here, we only enumerate the types, the actual information gets set up later.
// The last kind (MY_ELECTRON) will be used as "neutralizing kind", ie, in the
// initial setup, the code will add as many electrons as there are ions in a
// cell, at the same position, to ensure the initial plasma is neutral
// (so that Gauss's Law is satisfied).
enum
{
  MY_ELECTRON,
  MY_ION,
  N_MY_KINDS,
};

// ======================================================================
// PscHarrisParams

struct PscHarrisParams
{
  double Lx_di, Ly_di, Lz_di; // Size of box in d_i

  double L_di; // Sheet thickness / ion inertial length

  double BB;
  double Zi;
  double mass_ratio;
  double Ti_Te;
  double lambda0;

  // double background_n;
  // double background_Te;
  // double background_Ti;

  double bg; // guide field as fraction of B0
  double theta;
  double dbz_b0;
  double Lpert_Ly;

  // The following parameters are calculated from the above / and other
  // information

  double d_i;
  double b0; // B0
  // double n0;
  double L; // sheet width in d_e
  double TTi, TTe;
  double wpe_wce;

  double wpe, wpi, wce, wci;
  double Lx, Ly, Lz; // size of box
  double Lpert;      // wavelength of perturbation
  double dby;        // Perturbation in Bz relative to Bo (Only change here)
  double dbz;        // Set Bx perturbation so that div(B) = 0
};

// ======================================================================
// Global parameters
//
// I'm not a big fan of global parameters, but they're only for
// this particular case and they help make things simpler.

// An "anonymous namespace" makes these variables visible in this source file
// only
namespace
{

// Parameters specific to this case. They don't really need to be collected in a
// struct, but maybe it's nice that they are

PscHarrisParams g;

std::string read_checkpoint_filename;

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

#ifdef USE_CUDA
using PscConfig = PscConfig1vbecCuda<Dim>;
#else
using PscConfig = PscConfig1vbecSingle<Dim>;
#endif

using Writer = WriterDefault; // can choose WriterMrc, WriterAdios2

// ----------------------------------------------------------------------

using MfieldsState = PscConfig::MfieldsState;
using Mparticles = PscConfig::Mparticles;
using Balance = PscConfig::Balance;
using Collision = PscConfig::Collision;
using Checks = PscConfig::Checks;
using Marder = PscConfig::Marder;
using OutputParticles = PscConfig::OutputParticles;

// ======================================================================
// setupParameters

void setupParameters()
{
  // -- set some generic PSC parameters
  psc_params.nmax = 10000001; // 5001;
  psc_params.cfl = 0.75;
  psc_params.write_checkpoint_every_step = 1000;
  psc_params.stats_every = 1;

  // -- start from checkpoint:
  //
  // Uncomment when wanting to start from a checkpoint, ie.,
  // instead of setting up grid, particles and state fields here,
  // they'll be read from a file
  // FIXME: This parameter would be a good candidate to be provided
  // on the command line, rather than requiring recompilation when change.

  // read_checkpoint_filename = "checkpoint_500.bp";

  // -- Set some parameters specific to this case

  // PIC units, only used in this scope
  // double eps0 = 1;
  double me = 1;
  double ec = 1;
  double c = 1; // Speed of light
  double eps0 = 1;

  g.Lx_di = 1.;
  g.Ly_di = 40.;
  g.Lz_di = 20.;
  g.L_di = .5;
  g.Lpert_Ly = 1.;

  g.BB = 0.;
  g.Zi = 1.;
  g.mass_ratio = 16.;
  g.Ti_Te = 5.;
  g.lambda0 = 20.;
  g.bg = 0.;
  g.theta = 0;
  g.dbz_b0 = .03;

  g.wpe_wce = 2.;
  g.TTe = me * sqr(c) / (2. * eps0 * sqr(g.wpe_wce) * (1. + g.Ti_Te));
  g.TTi = g.TTe * g.Ti_Te;

  g.wci = 1. / (g.mass_ratio * g.wpe_wce); // Ion cyclotron frequency
  g.wce = g.wci * g.mass_ratio;            // Electron cyclotron freqeuncy
  g.wpe = g.wce * g.wpe_wce;               // electron plasma frequency
  g.wpi = g.wpe / sqrt(g.mass_ratio);      // ion plasma frequency

  g.d_i = c / g.wpi; // ion inertial length
  g.L = g.L_di * g.d_i;
  g.Lx = g.Lx_di * g.d_i; // size of box in x dimension
  g.Ly = g.Ly_di * g.d_i; // size of box in y dimension
  g.Lz = g.Lz_di * g.d_i; // size of box in z dimension

  g.b0 = me * c * g.wce / ec; // Asymptotic magnetic field strength
  // g.n0 = me * eps0 * wpe * wpe / (ec * ec); // Peak electron (ion) density
  g.Lpert = g.Lpert_Ly * g.Ly; // wavelength of perturbation
  g.dbz =
    g.dbz_b0 * g.b0; // Perturbation in Bz relative to Bo (Only change here)
  g.dby =
    -g.dbz * g.Lpert / (2. * g.Lz); // Set Bx perturbation so that div(B) = 0
}

// ======================================================================
// setupGrid
//
// This helper function is responsible for setting up the "Grid",
// which is really more than just the domain and its decomposition, it
// also encompasses PC normalization parameters, information about the
// particle kinds, etc.

Grid_t* setupGrid()
{
  // -- setup particle kinds
  Grid_t::Kinds kinds(N_MY_KINDS);
  kinds[MY_ION] = {g.Zi, g.mass_ratio * g.Zi, "i"};
  kinds[MY_ELECTRON] = {-1., 1., "e"};

  mpi_printf(MPI_COMM_WORLD, "d_e = %g, d_i = %g\n", 1., g.d_i);
  mpi_printf(MPI_COMM_WORLD, "lambda_De (background) = %g\n", sqrt(g.TTe));

  // --- setup domain
  Grid_t::Real3 LL = {g.Lx_di * g.d_i, g.Ly_di * g.d_i,
                      g.Lz_di * g.d_i}; // domain size (in d_e)
  // Int3 gdims = {1, 512, 128};
  // Int3 np = {1, 4, 1};
  Int3 gdims = {1, 640, 320};
  Int3 np = {1, 20, 10};

  Grid_t::Domain domain{gdims, LL, -.5 * LL, np};

  psc::grid::BC bc{{BND_FLD_PERIODIC, BND_FLD_PERIODIC, BND_FLD_PERIODIC},
                   {BND_FLD_PERIODIC, BND_FLD_PERIODIC, BND_FLD_PERIODIC},
                   {BND_PRT_PERIODIC, BND_PRT_PERIODIC, BND_PRT_PERIODIC},
                   {BND_PRT_PERIODIC, BND_PRT_PERIODIC, BND_PRT_PERIODIC}};

  // -- setup normalization
  auto norm_params = Grid_t::NormalizationParams::dimensionless();
  norm_params.nicell = 10000;

  double dt = psc_params.cfl * courant_length(domain);
  Grid_t::Normalization norm{norm_params};

  mpi_printf(MPI_COMM_WORLD, "dt = %g\n", dt);

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
// initializeParticles

void initializeParticles(SetupParticles<Mparticles>& setup_particles,
                         Balance& balance, Grid_t*& grid_ptr, Mparticles& mprts)
{
  // -- set particle initial condition
  partitionAndSetupParticles(setup_particles, balance, grid_ptr, mprts,
                             [&](int kind, Double3 crd, psc_particle_npt& npt) {
                               // struct psc_harris* harris =
                               // to_psc_harris(psc);

                               // double BB = harris->B0;
                               // double LLL = harris->LLL;
                               // double nnb = harris->nb;
                               // double TTi = harris->Ti;
                               // double TTe = harris->Te;

                               switch (kind) {
                                 case MY_ION: // ion drifting
                                   npt.n = 1. / sqr(cosh(crd[2] / g.L));
                                   npt.p[0] = 2. * g.TTi / g.b0 / g.L;
                                   npt.T[0] = g.TTi;
                                   npt.T[1] = g.TTi;
                                   npt.T[2] = g.TTi;
                                   npt.kind = MY_ION;
                                   break;
                                 // case 1: // ion bg
                                 //   npt->n = nnb;
                                 //   npt->q = 1.;
                                 //   npt->m = harris->mi_over_me;
                                 //   npt->T[0] = TTi;
                                 //   npt->T[1] = TTi;
                                 //   npt->T[2] = TTi;
                                 //   npt->kind = KIND_ION;
                                 //   break;
                                 case MY_ELECTRON: // electron drifting
                                   npt.n = 1. / sqr(cosh(crd[2] / g.L));
                                   npt.p[0] = -2. * g.TTe / g.b0 / g.L;
                                   npt.T[0] = g.TTe;
                                   npt.T[1] = g.TTe;
                                   npt.T[2] = g.TTe;
                                   npt.kind = MY_ELECTRON;
                                   break;
                                 // case 3: // electron bg
                                 //   npt->n = nnb;
                                 //   npt->q = -1.;
                                 //   npt->m = 1.;
                                 //   npt->T[0] = TTe;
                                 //   npt->T[1] = TTe;
                                 //   npt->T[2] = TTe;
                                 //   npt->kind = KIND_ELECTRON;
                                 //   break;
                                 default: assert(0);
                               }
                             });
}

// ======================================================================
// initializeFields

void initializeFields(MfieldsState& mflds)
{
  double b0 = g.b0, dby = g.dby, dbz = g.dbz;
  double L = g.L, Ly = g.Ly, Lz = g.Lz, Lpert = g.Lpert;
  double cs = cos(g.theta), sn = sin(g.theta);

  setupFields(mflds, [&](int m, double crd[3]) {
    double y = crd[1], z = crd[2];

    switch (m) {
      case HX: return -sn * b0 * tanh(z / L) + b0 * g.bg;

      case HY:
        return cs * b0 * tanh(z / L) +
               dby * cos(2. * M_PI * (y - .5 * Ly) / Lpert) *
                 sin(M_PI * z / Lz);

      case HZ:
        return dbz * cos(M_PI * z / Lz) *
               sin(2.0 * M_PI * (y - 0.5 * Ly) / Lpert);

      case JXI: return 0.; // FIXME

      default: return 0.;
    }
  });
}

// ======================================================================
// run
//
// This is basically the main function of this run,
// which sets up everything and then uses PscIntegrator to run the
// simulation

void run()
{
  mpi_printf(MPI_COMM_WORLD, "*** Setting up...\n");

  // ----------------------------------------------------------------------
  // setup various parameters first

  setupParameters();

  // ----------------------------------------------------------------------
  // Set up grid, state fields, particles

  auto grid_ptr = setupGrid();
  auto& grid = *grid_ptr;

  Mparticles mprts(grid);
  MfieldsState mflds(grid);
  if (!read_checkpoint_filename.empty()) {
    read_checkpoint(read_checkpoint_filename, grid, mprts, mflds);
  }

  // ----------------------------------------------------------------------
  // Set up various objects needed to run this case

  // -- Balance
  psc_params.balance_interval = 500;
  Balance balance{3};

  // -- Sort
  psc_params.sort_interval = 10;

  // -- Collision
  int collision_interval = 0;
  double collision_nu = 1e-10;
  Collision collision{grid, collision_interval, collision_nu};

  // -- Checks
  ChecksParams checks_params{};
  checks_params.continuity_every_step = 0;
  checks_params.continuity_dump_always = false;
  checks_params.continuity_threshold = 1e-4;
  checks_params.continuity_verbose = true;

  checks_params.gauss_every_step = 100;
  checks_params.gauss_dump_always = false;
  checks_params.gauss_threshold = 1e-4;
  checks_params.gauss_verbose = true;

  Checks checks{grid, MPI_COMM_WORLD, checks_params};

  // -- Marder correction
  double marder_diffusion = 0.9;
  int marder_loop = 3;
  bool marder_dump = false;
  psc_params.marder_interval = 100;
  Marder marder(grid, marder_diffusion, marder_loop, marder_dump);

  // ----------------------------------------------------------------------
  // Set up output
  //
  // FIXME, this really is too complicated and not very flexible

  // -- output fields
  OutputFieldsItemParams outf_item_params{};
  OutputFieldsParams outf_params{};
  outf_item_params.pfield_interval = 4;
  outf_item_params.tfield_interval = -4;
  outf_item_params.tfield_average_every = 50;

  outf_params.fields = outf_item_params;
  outf_params.moments = outf_item_params;
  OutputFields<MfieldsState, Mparticles, Dim, Writer> outf{grid, outf_params};

  // -- output particles
  OutputParticlesParams outp_params{};
  outp_params.every_step = -4;
  outp_params.data_dir = ".";
  outp_params.basename = "prt";
  OutputParticles outp{grid, outp_params};

  int oute_interval = -100;
  DiagEnergies oute{grid.comm(), oute_interval};

  auto diagnostics = makeDiagnosticsDefault(outf, outp, oute);

  // ----------------------------------------------------------------------
  // Set up objects specific to the Harris case

  SetupParticles<Mparticles> setup_particles(grid);
  setup_particles.fractional_n_particles_per_cell = true;
  setup_particles.neutralizing_population = MY_ION;

  // ----------------------------------------------------------------------
  // setup initial conditions

  if (read_checkpoint_filename.empty()) {
    initializeParticles(setup_particles, balance, grid_ptr, mprts);
    initializeFields(mflds);
  }

  // ----------------------------------------------------------------------
  // hand off to PscIntegrator to run the simulation

  auto psc =
    makePscIntegrator<PscConfig>(psc_params, *grid_ptr, mflds, mprts, balance,
                                 collision, checks, marder, diagnostics);

  MEM_STATS();
  psc.integrate();
  MEM_STATS();
}

// ======================================================================
// main

int main(int argc, char** argv)
{
  psc_init(argc, argv);

  run();

  MEM_STATS();

  psc_finalize();
  return 0;
}
