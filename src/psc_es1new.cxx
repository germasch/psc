
#include <psc.hxx>
#include <setup_fields.hxx>
#include <setup_particles.hxx>

#include "DiagnosticsDefault.h"
#include "OutputFieldsDefault.h"
#include "psc_config.hxx"

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

// ----------------------------------------------------------------------

using MfieldsState = PscConfig::MfieldsState;
using Mparticles = PscConfig::Mparticles;
using Balance = PscConfig::Balance;
using Collision = PscConfig::Collision;
using Checks = PscConfig::Checks;
using Marder = PscConfig::Marder;
using OutputParticles = PscConfig::OutputParticles;

enum
{
  KIND_E1,
  KIND_E2,
  N_KINDS,
};

// ======================================================================
// PscES1Params

struct PscES1Species
{
  double vt1 = 0.;
  double vt2 = 0.;
  int nlg = 1;
  double mode = 1.;
  double v0 = 0.;
  double x1 = 0.;
  double v1 = 0.;
  double thetax = 0.;
  double thetav = 0.;
};

struct PscES1Params
{
  PscES1Species s[2];
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
PscES1Params g;

std::string read_checkpoint_filename;

// This is a set of generic PSC params (see include/psc.hxx),
// like number of steps to run, etc, which also should be set by the case
PscParams psc_params;

} // namespace

// ======================================================================
// setupParameters

void setupParameters()
{
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

  g.s[0].nlg = 1;
  g.s[0].v0 = .1;
  g.s[0].mode = 1.;
  g.s[0].x1 = .001;
  g.s[0].v1 = 0.;

  g.s[1].nlg = 1;
  g.s[1].v0 = -.1;
  g.s[1].mode = 1.;
  g.s[1].x1 = .001;
  g.s[1].v1 = 0.;
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
  auto domain =
    Grid_t::Domain{{1, 2, 32}, {1., 1., 2. * M_PI}, {0., 0., 0.}, {1, 1, 1}};

  auto bc =
    psc::grid::BC{{BND_FLD_PERIODIC, BND_FLD_PERIODIC, BND_FLD_PERIODIC},
                  {BND_FLD_PERIODIC, BND_FLD_PERIODIC, BND_FLD_PERIODIC},
                  {BND_PRT_PERIODIC, BND_PRT_PERIODIC, BND_PRT_PERIODIC},
                  {BND_PRT_PERIODIC, BND_PRT_PERIODIC, BND_PRT_PERIODIC}};

  auto kinds = Grid_t::Kinds(N_KINDS);
  kinds[KIND_E1] = {-1., 1., "e1"};
  kinds[KIND_E2] = {-1., 1., "e2"};

  // --- generic setup
  auto norm_params = Grid_t::NormalizationParams::dimensionless();
  norm_params.nicell = 32;

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

// ----------------------------------------------------------------------
// ranf
//
// return random number between 0 and 1

static double ranf()
{
  return (double)random() / RAND_MAX;
}

// ----------------------------------------------------------------------
// psc_es1_init_species

void psc_es1_init_species(int kind, PscES1Species* s, Mparticles& mprts, int p,
                          int* p_il1)
{
  const auto& grid = mprts.grid();
  auto ldims = grid.ldims;
  int n = ldims[0] * ldims[1] * ldims[2] / grid.norm.cori; // FIXME
  double l = grid.domain.length[2];

  // load first of nlg groups of particles.
  int ngr = n / s->nlg;
  double lg = l / s->nlg;
  double ddx = l / n;

  double* x = (double*)calloc(n, sizeof(*x));
  double* vx = (double*)calloc(n, sizeof(*vx));

  // load evenly spaced, with drift.
  // also does cold case.
  for (int i = 0; i < ngr; i++) {
    double x0 = (i + .5) * ddx;
    int i1 = i; // + il1;
    x[i1] = x0;
    vx[i1] = s->v0;
  }

  // load order velocities in vx ("quiet start", or at least subdued).
  // is set up for maxwellian*v*nv2, but can do any smooth distribution.
  // hereafter, ngr is prferably a power of 2
  if (s->vt2 != 0.) {
    // first store indefinite integral of distribution function in x array.
    // use midpoint rule -simple and quite accurate.
    double vmax = 5. * s->vt2;
    double dv = 2. * vmax / (n - 1);
    double vvnv2 = 1.;
    x[0] = 0.;
    for (int i = 2; i <= n; i++) {
      double vv = ((i - 1.5) * dv - vmax) / s->vt2;
#if 0
      if (s->nv2 != 0.) {
	vvnv2 = pow(vv, s->nv2);
      }
#endif
      double fv = vvnv2 * exp(-.5 * sqr(vv));
      int i1 = i - 1;
      x[i1] = x[i1 - 1] + fmax(fv, 0.);
      printf("i1 %d x %g\n", i1, x[i1]);
    }

    // for evenly spaced (half-integer multiples) valus of the integral,
    // find corresponding velocities by inverse linear interpolation
    double df = x[n - 1] / ngr;
    int i1 = 0;
    int j = 0;
    for (int i = 1; i <= ngr; i++) {
      double fv = (i - .5) * df;
      while (fv >= x[j + 1]) {
        j++;
        assert(j < n - 1);
      }
      double vv = dv * (j + (fv - x[j]) / (x[j + 1] - x[j])) - vmax;
      vx[i1] += vv;
      i1++;
    }

    // for ordered velocities, scramble positions to reduce correlations
    float xs = 0.0;
    for (int i = 1; i <= ngr; i++) {
      i1 = i - 1;
      x[i1] = xs * lg + 0.5 * ddx;
      float xsi = 1.0;
      do {
        xsi *= 0.5;
        xs -= xsi;
      } while (xs >= 0.0);
      xs += 2.0 * xsi;
    }
  }

  // if magnetized, rotate (vx, 0) into (vx, vy)
#if 0
  if (s->wc != 0.) {
    assert(0);
  }
#endif

  // copy first group into rest of groups.
  if (s->nlg > 1) {
    int xs = 0.;
    for (int i = ngr + 1; i <= n; i += ngr) {
      xs += lg;
      for (int j = 1; j <= ngr; j++) {
        int i1 = j - 1 - 1;
        int i2 = i1 + i - 1 - 1;
        x[i2] = x[i1] + xs;
        vx[i2] = vx[i1];
#if 0
	if (s->wc != 0.) {
	  vy[i2] = vy[i1];
	}
#endif
      }
    }
  }

  // add random maxwellian.
  if (s->vt1 != 0.) {
    for (int i = 1; i < n; i++) {
      int i1 = i - 1;
      for (int j = 1; j < 12.; j++) {
#if 0
	if (s->wc != 0.) {
	}
#endif
        vx[i1] += s->vt1 * (ranf() - .5);
      }
    }
  }

  // add perturbation.p5
  for (int i = 0; i < n; i++) {
    double theta = 2. * M_PI * s->mode * x[i] / l;
    x[i] += s->x1 * cos(theta + s->thetax);
    vx[i] += s->v1 * sin(theta + s->thetav);
  }

  // copy to PSC data structure
  auto injector = mprts.injector();
  for (int i = 0; i < n; i++) {
    if (x[i] < 0.)
      x[i] += l;
    if (x[i] >= l)
      x[i] -= l;

    psc::particle::Inject prt = {
      {0., 0., x[i]}, {0., 0., vx[i] / grid.norm.cc}, 1., kind};
    injector[p](prt);
  }

  free(x);
  free(vx);
}

// ======================================================================
// initializeParticles

void initializeParticles(Balance& balance, Grid_t*& grid_ptr, Mparticles& mprts)
{
  const auto& kinds = mprts.grid().kinds;
  for (int p = 0; p < mprts.n_patches(); p++) {
    int il1 = 0;
    for (int kind = 0; kind < kinds.size(); kind++) {
      psc_es1_init_species(kind, &g.s[kind], mprts, p, &il1);
    }
  }
}

// ======================================================================
// initializeFields

void initializeFields(MfieldsState& mflds)
{
  const auto& grid = mflds.grid();
  const auto& kinds = grid.kinds;

  setupFields(mflds, [&](int m, double crd[3]) {
    switch (m) {
      case EZ: {
        double ez = 0;
        for (int kind = 0; kind < N_KINDS; kind++) {
          auto& s = g.s[kind];
          double theta = 2. * M_PI * s.mode / grid.domain.length[2] * crd[2];
          ez -= kinds[kind].q * s.x1 * cos(theta + s.thetax);
        }
        return ez;
      }
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

static void run()
{
  mpi_printf(MPI_COMM_WORLD, "*** Setting up...\n");

  // ----------------------------------------------------------------------
  // setup various parameters first

  setupParameters();

  // ----------------------------------------------------------------------
  // Set up grid, state fields, particles

  auto grid_ptr = setupGrid();
  auto& grid = *grid_ptr;
  MfieldsState mflds{grid};
  Mparticles mprts{grid};

  // ----------------------------------------------------------------------
  // Set up various objects needed to run this case

  // -- Balance
  psc_params.balance_interval = 0;
  Balance balance{psc_params.balance_interval, .1};

  // -- Sort
  psc_params.sort_interval = 10;

  // -- Collision
  int collision_interval = 0;
  double collision_nu = .1;
  Collision collision{grid, collision_interval, collision_nu};

  // -- Checks
  ChecksParams checks_params{};
  Checks checks{grid, MPI_COMM_WORLD, checks_params};

  // -- Marder correction
  double marder_diffusion = 0.9;
  int marder_loop = 3;
  bool marder_dump = false;
  psc_params.marder_interval = 0; // 5
  Marder marder(grid, marder_diffusion, marder_loop, marder_dump);

  // ----------------------------------------------------------------------
  // Set up output
  //
  // FIXME, this really is too complicated and not very flexible

  // -- output fields
  OutputFieldsParams outf_params{};
  outf_params.pfield_step = 200;
  OutputFields outf{grid, outf_params};

  // -- output particles
  OutputParticlesParams outp_params{};
  outp_params.every_step = 10;
  outp_params.data_dir = ".";
  outp_params.basename = "prt";
  OutputParticles outp{grid, outp_params};

  int oute_interval = 100;
  DiagEnergies oute{grid.comm(), oute_interval};

  auto diagnostics = makeDiagnosticsDefault(outf, outp, oute);

  // ----------------------------------------------------------------------
  // setup initial conditions

  if (read_checkpoint_filename.empty()) {
    initializeParticles(balance, grid_ptr, mprts);
    initializeFields(mflds);
  } else {
    read_checkpoint(read_checkpoint_filename, *grid_ptr, mprts, mflds);
  }

  // ----------------------------------------------------------------------
  // hand off to PscIntegrator to run the simulation

  auto psc =
    makePscIntegrator<PscConfig>(psc_params, *grid_ptr, mflds, mprts, balance,
                                 collision, checks, marder, diagnostics);

  psc.integrate();
}

// ======================================================================
// main

int main(int argc, char** argv)
{
  psc_init(argc, argv);

  run();

  psc_finalize();
  return 0;
}
