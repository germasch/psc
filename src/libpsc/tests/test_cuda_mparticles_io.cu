
#include <gtest/gtest.h>

#include "test_common.hxx"

#include "psc_particles_double.h"
#include "psc_particles_single.h"
#include "particle_with_id.h"
#include "setup_particles.hxx"
#include "../libpsc/cuda/mparticles_cuda.hxx"
#include "../libpsc/cuda/mparticles_cuda.inl"
#include "../libpsc/cuda/cuda_mparticles.cuh"
#include "../libpsc/cuda/cuda_mparticles.inl"
#include "particles_simple.inl"

template <typename _Mparticles, typename _MakeGrid = MakeTestGrid1>
struct Config
{
  using Mparticles = _Mparticles;
  using MakeGrid = _MakeGrid;
};

using MparticlesTestTypes = ::testing::Types<
  Config<MparticlesCuda<BS144>, MakeTestGridYZ>
  >;

TYPED_TEST_SUITE(MparticlesTest, MparticlesTestTypes);

// ======================================================================
// MparticlesTest

template <typename T>
struct MparticlesTest : ::testing::Test
{
  using Mparticles = typename T::Mparticles;
  using Particle = typename Mparticles::Particle;
  using MakeGrid = typename T::MakeGrid;

  MparticlesTest() : grid_{MakeGrid{}()}
  {
    grid_.kinds.emplace_back(Grid_t::Kind(1., 1., "test_species"));
  }

  Mparticles mk_mprts()
  {
    Mparticles mprts(grid_);
    mprts.define_species("test_species", 1., 1., 100, 10, 10, 0);
    return mprts;
  }

  template <typename _Mparticles>
  void inject_test_particles(_Mparticles& mprts, int n_prts)
  {
    auto inj = mprts.injector();
    for (int p = 0; p < mprts.n_patches(); ++p) {
      auto injector = inj[p];
      auto& patch = mprts.grid().patches[p];
      for (int n = 0; n < n_prts; n++) {
        double nn = double(n) / n_prts;
        auto L = patch.xe - patch.xb;
        psc::particle::Inject prt = {
	  {patch.xb[0] + nn * L[0],
	   patch.xb[1] + nn * L[1],
	   patch.xb[2] + nn * L[2]}, {}, 1., 0};
        injector(prt);
      }
    }
  }

  const Grid_t& grid() { return grid_; }

private:
  Grid_t grid_;
};

#ifdef PSC_HAVE_ADIOS2

// ======================================================================
// MparticlesIOTest

template <typename T>
struct MparticlesIOTest : MparticlesTest<T>
{};

using MparticlesIOTestTypes =
  ::testing::Types<Config<MparticlesCuda<BS144>, MakeTestGridYZ>>;

TYPED_TEST_SUITE(MparticlesIOTest, MparticlesIOTestTypes);

TYPED_TEST(MparticlesIOTest, WriteRead)
{
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  auto mprts = this->mk_mprts();
  this->inject_test_particles(mprts, 4 + rank);

  auto io = kg::io::IOAdios2{};

  cuda_mparticles<BS144>& cmprts = *mprts.cmprts();
  DMparticlesCuda<BS144> d_mprts = cmprts;
  
  {
    auto writer = io.open("test.bp", kg::io::Mode::Write);
    writer.put("d_mprts", d_mprts);
    writer.close();
  }

  DMparticlesCuda<BS144> d_mprts2;
#if 0
  {
    auto reader = io.open("test.bp", kg::io::Mode::Read);
    reader.get("d_mprts", d_mprts2);
    reader.close();
  }

  auto accessor = mprts.accessor();
  auto accessor2 = mprts2.accessor();
  for (int p = 0; p < mprts.n_patches(); ++p) {
    auto prts = accessor[p];
    auto prts2 = accessor2[p];
    ASSERT_EQ(prts.size(), prts2.size());
    for (int n = 0; n < prts.size(); n++) {
      EXPECT_EQ(prts[n].x(), prts2[n].x());
    }
  }
#endif
}

#endif

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);

  ::testing::InitGoogleTest(&argc, argv);
  int rc = RUN_ALL_TESTS();

  MPI_Finalize();
  return rc;
}
