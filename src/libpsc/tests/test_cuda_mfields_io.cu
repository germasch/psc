
#include <gtest/gtest.h>

#include "fields3d.hxx"
#include "psc_fields_cuda.h"
#include "psc_fields_cuda.inl"
#include "setup_fields.hxx"

#include "../libpsc/cuda/setup_fields_cuda.hxx"

#include "kg/io.h"
#include "fields3d.inl"

#include "psc.h" // FIXME, just for EX etc

#include "cuda_mfields.h"
#include "cuda_mfields.inl"

static Grid_t make_grid()
{
  auto domain =
    Grid_t::Domain{{8, 4, 2}, {80., 40., 20.}, {-40., -20., 0.}, {2, 1, 1}};
  auto bc = psc::grid::BC{};
  auto kinds = Grid_t::Kinds{};
  auto norm = Grid_t::Normalization{};
  double dt = .1;
  return Grid_t{domain, bc, kinds, norm, dt};
}

template <typename T>
class CudaMfieldsTest : public ::testing::Test
{};

using CudaMfieldsTestTypes = ::testing::Types<MfieldsCuda>;

TYPED_TEST_SUITE(CudaMfieldsTest, CudaMfieldsTestTypes);

TYPED_TEST(CudaMfieldsTest, WriteRead)
{
  using Mfields = TypeParam;

  auto grid = make_grid();
  auto mflds = Mfields{grid, NR_FIELDS, {}};

  setupFields(mflds, [](int m, double crd[3]) {
    return m + crd[0] + 100 * crd[1] + 10000 * crd[2];
  });

  DMFields d_mflds = *mflds.cmflds();
  
  auto io = kg::io::IOAdios2{};

  {
    auto writer = io.open("test.bp", kg::io::Mode::Write);
    writer.put("d_mflds", d_mflds);
    writer.close();
  }

  auto box = d_mflds.box();
  auto n_comps = d_mflds.n_comps();
  auto n_patches = d_mflds.n_patches();
  thrust::device_vector<float> d_data(box.size() * n_comps * n_patches);
  auto d_mflds2 = DMFields{box, n_comps, n_patches, d_data.data().get(), d_data.size()};

  {
    auto reader = io.open("test.bp", kg::io::Mode::Read);
    reader.get("d_mflds", d_mflds2);
    reader.close();
  }

#if 0
  for (int p = 0; p < mflds.n_patches(); ++p) {
    grid.Foreach_3d(0, 0, [&](int i, int j, int k) {
#if 0
	mprintf("[%d, %d, %d] = %06g %06g %06g\n", i, j, k,
		(double) mflds[p](EX, i, j, k), (double) mflds[p](EY, i, j, k), (double) mflds[p](EZ, i, j, k));
#endif
      for (int m = 0; m < NR_FIELDS; m++) {
        EXPECT_EQ(mflds[p](m, i, j, k), mflds2[p](m, i, j, k));
      }
    });
  }
#endif
}

TYPED_TEST(CudaMfieldsTest, WriteWithGhostsRead)
{
  using Mfields = TypeParam;

  auto grid = make_grid();
  auto mflds = Mfields{grid, NR_FIELDS, {2, 2, 2}};

  setupFields(mflds, [](int m, double crd[3]) {
    return m + crd[0] + 100 * crd[1] + 10000 * crd[2];
  });

  DMFields d_mflds = *mflds.cmflds();
  
  auto io = kg::io::IOAdios2{};

  {
    auto writer = io.open("test1.bp", kg::io::Mode::Write);
    writer.put("d_mflds", d_mflds);
    writer.close();
  }

#if 0
  auto mflds2 = Mfields{grid, NR_FIELDS, {}};
  {
    auto reader = io.open("test1.bp", kg::io::Mode::Read);
    reader.get("mflds", mflds2);
    reader.close();
  }

  for (int p = 0; p < mflds.n_patches(); ++p) {
    grid.Foreach_3d(0, 0, [&](int i, int j, int k) {
#if 0
	mprintf("[%d, %d, %d] = %06g %06g %06g\n", i, j, k,
		(double) mflds[p](EX, i, j, k), (double) mflds[p](EY, i, j, k), (double) mflds[p](EZ, i, j, k));
#endif
      for (int m = 0; m < NR_FIELDS; m++) {
        EXPECT_EQ(mflds[p](m, i, j, k), mflds2[p](m, i, j, k))
          << " i " << i << " j " << j << " k " << k << " m " << m;
      }
    });
  }
#endif
}

#if 0
TYPED_TEST(CudaMfieldsTest, WriteReadWithGhosts)
{
  using Mfields = TypeParam;

  auto grid = make_grid();
  auto mflds = Mfields{grid, NR_FIELDS, {}};

  setupFields(mflds, [](int m, double crd[3]) {
    return m + crd[0] + 100 * crd[1] + 10000 * crd[2];
  });

  auto io = kg::io::IOAdios2{};

  {
    auto writer = io.open("test.bp", kg::io::Mode::Write);
    writer.put("mflds", mflds);
    writer.close();
  }

  auto mflds2 = Mfields{grid, NR_FIELDS, {2, 2, 2}};
  {
    auto reader = io.open("test.bp", kg::io::Mode::Read);
    reader.get("mflds", mflds2);
    reader.close();
  }

  for (int p = 0; p < mflds.n_patches(); ++p) {
    grid.Foreach_3d(0, 0, [&](int i, int j, int k) {
#if 0
	mprintf("p%d [%d, %d, %d] = %06g %06g %06g\n", p, i, j, k,
		(double) mflds2[p](EX, i, j, k), (double) mflds2[p](EY, i, j, k), (double) mflds2[p](EZ, i, j, k));
#endif
#if 1
      for (int m = 0; m < NR_FIELDS; m++) {
        EXPECT_EQ(mflds[p](m, i, j, k), mflds2[p](m, i, j, k))
          << " i " << i << " j " << j << " k " << k << " m " << m;
      }
#endif
    });
  }
}
#endif

// ======================================================================
// main

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  ::testing::InitGoogleTest(&argc, argv);
  int rc = RUN_ALL_TESTS();
  MPI_Finalize();
  return rc;
}
