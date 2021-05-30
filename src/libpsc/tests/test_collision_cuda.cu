
#include "gtest/gtest.h"

#include "../libpsc/cuda/cuda_mparticles.cuh"
#include "../libpsc/cuda/cuda_mparticles_sort.cuh"

#include "../libpsc/cuda/collision_cuda_impl.hxx"
#include "../libpsc/cuda/cuda_collision.cuh"
#include "psc_fields_single.h"
#include "psc_particles_single.h"
#include "testing.hxx"

using dim = dim_yz;
using Mparticles = MparticlesCuda<BS144>;

// ======================================================================
// RngStateCuda

TEST(RngStateCuda, ctor_dtor)
{
  RngStateCuda rng_state;
  RngStateCuda rng_state2(128);
}

TEST(RngStateCuda, resize)
{
  RngStateCuda rng_state;
  rng_state.resize(256);
  EXPECT_EQ(rng_state.size(), 256);
}

__global__ static void kernel_random(RngStateCuda::Device rng_state,
                                     thrust::device_ptr<float> x)
{
  int n = threadIdx.x + blockDim.x * blockIdx.x;

  auto rng = rng_state[n];
  x[n] = rng.uniform();
  rng_state[n] = rng;
}

TEST(RngStateCuda, access)
{
  dim3 dim_grid(2);
  RngStateCuda rng_state(dim_grid.x * THREADS_PER_BLOCK);

  ASSERT_EQ(THREADS_PER_BLOCK, 128);
  psc::device_vector<float> x(dim_grid.x * THREADS_PER_BLOCK);
  kernel_random<<<dim_grid, THREADS_PER_BLOCK>>>(rng_state, x.data());

  float sum = thrust::reduce(x.begin(), x.end(), 0.f, thrust::plus<float>());
  float avg = sum / x.size();
  EXPECT_NEAR(avg, .5, .05);

  // repeat to make sure numbers don't repeat
  kernel_random<<<dim_grid, THREADS_PER_BLOCK>>>(rng_state, x.data());

  float sum2 = thrust::reduce(x.begin(), x.end(), 0.f, thrust::plus<float>());
  float avg2 = sum2 / x.size();
  EXPECT_NEAR(avg2, .5, .05);
  EXPECT_NE(avg, avg2);
}

// ======================================================================
// make_psc
//
// FIXME, duplicated in various testing environments

template <typename dim>
static Grid_t& make_psc(const Grid_t::Kinds& kinds)
{
  Int3 gdims = {16, 16, 16};
  Int3 ibn = {2, 2, 2};
  Int3 np = {2, 2, 2};
  Vec3<double> length = {160., 160., 160.};
  if (dim::InvarX::value) {
    gdims[0] = 1;
    ibn[0] = 0;
    np[0] = 1;
  }
  if (dim::InvarY::value) {
    gdims[1] = 1;
    ibn[1] = 0;
    np[1] = 1;
  }
  if (dim::InvarZ::value) {
    gdims[2] = 1;
    ibn[2] = 0;
    np[2] = 1;
  }

  auto grid_domain = Grid_t::Domain{gdims, length, {}, np};
  auto grid_bc =
    psc::grid::BC{{BND_FLD_PERIODIC, BND_FLD_PERIODIC, BND_FLD_PERIODIC},
                  {BND_FLD_PERIODIC, BND_FLD_PERIODIC, BND_FLD_PERIODIC},
                  {BND_PRT_PERIODIC, BND_PRT_PERIODIC, BND_PRT_PERIODIC},
                  {BND_PRT_PERIODIC, BND_PRT_PERIODIC, BND_PRT_PERIODIC}};

  auto norm_params = Grid_t::NormalizationParams::dimensionless();
  norm_params.nicell = 200;
  auto coeff = Grid_t::Normalization{norm_params};
  return *new Grid_t{grid_domain, grid_bc, kinds, coeff, 1.};
}

static Mparticles make_mparticles(const Grid_t& grid)
{
  Mparticles mprts{grid};
  auto inj = mprts.injector();
  auto injector = inj[0];
  injector({{5., 5., 5.}, {1., 0., 0.}, 1., 0});
  injector({{5., 5., 5.}, {0., 0., 0.}, 1., 0});
  injector({{5., 15., 15.}, {.6, 0., 0.}, 1., 0});
  injector({{5., 15., 15.}, {.7, 0., 0.}, 1., 0});
  injector({{5., 15., 15.}, {.8, 0., 0.}, 1., 0});
  injector({{5., 15., 5.}, {.1, 0., 0.}, 1., 0});
  injector({{5., 15., 5.}, {.2, 0., 0.}, 1., 0});
  injector({{5., 15., 5.}, {.3, 0., 0.}, 1., 0});
  injector({{5., 15., 5.}, {.4, 0., 0.}, 1., 0});
  MHERE;
  return mprts;
}

TEST(cuda_mparticles_sort, sort)
{
  auto kinds = Grid_t::Kinds{Grid_t::Kind(1., 1., "test_species")};
  const auto& grid = make_psc<dim>(kinds);
  std::cout << "n_patches " << grid.n_patches() << "\n";

  // init particles
  MHERE;
  auto mprts = make_mparticles(grid);
  MHERE;

  auto& cmprts = *mprts.cmprts();
  auto sort = cuda_mparticles_sort(cmprts.n_cells());

  sort.find_indices_ids(cmprts);
  EXPECT_EQ(sort.d_idx, (std::vector<int>{0, 0, 9, 9, 9, 1, 1, 1, 1}));
  EXPECT_EQ(sort.d_id, (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8}));

  sort.stable_sort_cidx();
  EXPECT_EQ(sort.d_idx, (std::vector<int>{0, 0, 1, 1, 1, 1, 9, 9, 9}));
  EXPECT_EQ(sort.d_id, (std::vector<int>{0, 1, 5, 6, 7, 8, 2, 3, 4}));

  sort.find_offsets();
  std::vector<int> off(cmprts.n_cells() + 1);
  off[0] = 0;
  off[1] = 2;
  for (int i = 2; i < 10; i++) {
    off[i] = 6;
  }
  for (int i = 10; i <= 256; i++) {
    off[i] = 9;
  }
  EXPECT_EQ(sort.d_off, off);
}

TEST(cuda_mparticles_randomize_sort, sort)
{
  auto kinds = Grid_t::Kinds{Grid_t::Kind(1., 1., "test_species")};
  const auto& grid = make_psc<dim>(kinds);

  // init particles
  auto mprts = make_mparticles(grid);

  auto& cmprts = *mprts.cmprts();
  cuda_mparticles_randomize_sort sort;

  sort.find_indices_ids(cmprts);
  EXPECT_EQ(sort.d_id, (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8}));

  sort.sort();
  // EXPECT_EQ(sort.d_id, (std::vector<int>{0, 1, 5, 8, 7, 6, 2, 3, 4}));
  // EXPECT_EQ(sort.d_id, (std::vector<int>{1, 0, 8, 7, 5, 6, 4, 2, 3}));
  EXPECT_EQ(sort.d_id, (std::vector<int>{0, 1, 7, 5, 8, 6, 2, 4, 3}));

  float last = sort.d_random_idx[0];
  for (int i = 1; i < cmprts.size(); i++) {
    EXPECT_GE(sort.d_random_idx[i], last);
    last = sort.d_random_idx[i];
  }
  // for (int i = 0; i < cmprts.size(); i++) {
  //   mprintf("i %d r_idx %g id %d\n", i, (float)sort.d_random_idx[i],
  //           (int)sort.d_id[i]);
  // }

  sort.find_offsets();
  std::vector<int> off(cmprts.n_cells() + 1);
  off[0] = 0;
  off[1] = 2;
  for (int i = 2; i < 10; i++) {
    off[i] = 6;
  }
  for (int i = 10; i <= 256; i++) {
    off[i] = 9;
  }
  EXPECT_EQ(sort.d_off, off);

#if 1
  // do over, get different permutation
  sort.find_indices_ids(cmprts);
  sort.sort();
  // for (int i = 0; i < cmprts.size(); i++) {
  //   mprintf("i %d r_idx %g id %d\n", i, (float)sort.d_random_idx[i],
  //           (int)sort.d_id[i]);
  // }
  EXPECT_NE(sort.d_id, (std::vector<int>{0, 1, 7, 5, 8, 6, 2, 4, 3}));
#endif
}

TEST(CollisionTest, Test2)
{
  using Collision = CollisionCuda<MparticlesCuda<BS144>, RngStateFake>;
  const typename Mparticles::real_t eps = 1e-5;

  auto kinds = Grid_t::Kinds{Grid_t::Kind(1., 1., "test_species")};
  const auto& grid = make_psc<dim>(kinds);

  // init particles
  auto mprts = make_mparticles(grid);

  auto collision = Collision(grid, 1, 1.);

  auto& cmprts = *mprts.cmprts();
  auto sort_by_cell = cuda_mparticles_sort(cmprts.n_cells());
  sort_by_cell.find_indices_ids(cmprts);
  EXPECT_EQ(sort_by_cell.d_idx, (std::vector<int>{0, 0, 9, 9, 9, 1, 1, 1, 1}));
  EXPECT_EQ(sort_by_cell.d_id, (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8}));
  sort_by_cell.stable_sort_cidx();
  EXPECT_EQ(sort_by_cell.d_idx, (std::vector<int>{0, 0, 1, 1, 1, 1, 9, 9, 9}));
  EXPECT_EQ(sort_by_cell.d_id, (std::vector<int>{0, 1, 5, 6, 7, 8, 2, 3, 4}));
  sort_by_cell.find_offsets();
  // for (int i = 0; i < cmprts.size(); i++) {
  //   mprintf("i %d idx %d id %d\n", i, (int)sort_by_cell.d_idx[i],
  //   (int)sort_by_cell.d_id[i]);

  collision(mprts);

  auto accessor = mprts.accessor();
  auto it = accessor[0].begin();
  auto prtf0 = *it++;
  auto prtf1 = *it++;
  EXPECT_NEAR(prtf0.u()[0] + prtf1.u()[0], 1., eps);
  EXPECT_NEAR(prtf0.u()[1] + prtf1.u()[1], 0., eps);
  EXPECT_NEAR(prtf0.u()[2] + prtf1.u()[2], 0., eps);

#if 0
  // depends on random numbers, but for RngFake, we know
  EXPECT_NEAR(prtf0.u()[0],  0.96226911, eps);
  EXPECT_NEAR(prtf0.u()[1],  0.        , eps);
  EXPECT_NEAR(prtf0.u()[2], -0.17342988, eps);
  EXPECT_NEAR(prtf1.u()[0],  0.03773088, eps);
  EXPECT_NEAR(prtf1.u()[1], -0.        , eps);
  EXPECT_NEAR(prtf1.u()[2],  0.17342988, eps);
#endif
}

// ======================================================================
// main

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  ::testing::InitGoogleTest(&argc, argv);
  cuda_base_init();
  int rc = RUN_ALL_TESTS();
  MPI_Finalize();
  return rc;
}
