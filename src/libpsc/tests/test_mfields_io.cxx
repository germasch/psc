
#include <gtest/gtest.h>

#undef USE_CUDA // FIXME!!!, SetupFields doesn't work

#include "fields3d.hxx"
#include "psc_fields_c.h"
#include "psc_fields_single.h"
#ifdef USE_CUDA
#include "psc_fields_cuda.h"
#endif
#include "setup_fields.hxx"

#undef USE_CUDA // FIXME!!!, SetupFields doesn't work
#ifdef USE_CUDA
#include "../libpsc/cuda/setup_fields_cuda.hxx"
#endif

#include "kg/io.h"

// ======================================================================
// Variable<Mfields>

template <>
class kg::io::Variable<MfieldsC>
{
  using DataType = MfieldsC::real_t;

public:
  using value_type = MfieldsC;

  Variable(const std::string& name, kg::io::Engine& engine) {}

  void put(kg::io::Engine& writer, const value_type& mflds,
           const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    const Grid_t& grid = mflds.grid();

    writer.putAttribute("ib", mflds.ib, launch);
    writer.putAttribute("im", mflds.im, launch);

    auto& gdims = grid.domain.gdims;
    size_t n_comps = mflds.n_comps();
    auto shape = kg::io::Dims{n_comps, static_cast<size_t>(gdims[2]),
                              static_cast<size_t>(gdims[1]),
                              static_cast<size_t>(gdims[0])};
    auto var = writer._defineVariable<DataType>(writer.prefix(), shape);
    for (int p = 0; p < grid.n_patches(); p++) {
      auto& patch = grid.patches[p];
      auto start = kg::io::Dims{0, static_cast<size_t>(patch.off[2]),
                                static_cast<size_t>(patch.off[1]),
                                static_cast<size_t>(patch.off[0])};
      auto count = kg::io::Dims{n_comps, static_cast<size_t>(grid.ldims[2]),
                                static_cast<size_t>(grid.ldims[1]),
                                static_cast<size_t>(grid.ldims[0])};
      auto ib = kg::io::Dims{0, static_cast<size_t>(-mflds.ib[2]),
                             static_cast<size_t>(-mflds.ib[1]),
                             static_cast<size_t>(-mflds.ib[0])};
      auto im = kg::io::Dims{n_comps, static_cast<size_t>(mflds.im[2]),
                             static_cast<size_t>(mflds.im[1]),
                             static_cast<size_t>(mflds.im[0])};
      var.setSelection({start, count});
      var.setMemorySelection({ib, im});
      var.put(writer, mflds.data[p].get());
    }
  }

  void get(kg::io::Engine& reader, value_type& mflds,
           const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    const Grid_t& grid = mflds.grid();

    // FIXME, should just check for consistency? (# ghosts might differ, too)
    // reader.getAttribute("ib", mflds.ib, launch);
    // reader.getAttribute("im", mflds.im, launch);

    auto& gdims = grid.domain.gdims;
    size_t n_comps = mflds.n_comps();
    auto shape = kg::io::Dims{n_comps, static_cast<size_t>(gdims[2]),
                              static_cast<size_t>(gdims[1]),
                              static_cast<size_t>(gdims[0])};
    auto var = reader._defineVariable<DataType>(reader.prefix());
    assert(var.shape() == shape);
    for (int p = 0; p < grid.n_patches(); p++) {
      auto& patch = grid.patches[p];
      auto start = kg::io::Dims{0, static_cast<size_t>(patch.off[2]),
                                static_cast<size_t>(patch.off[1]),
                                static_cast<size_t>(patch.off[0])};
      auto count = kg::io::Dims{n_comps, static_cast<size_t>(grid.ldims[2]),
                                static_cast<size_t>(grid.ldims[1]),
                                static_cast<size_t>(grid.ldims[0])};
      auto ib = kg::io::Dims{0, static_cast<size_t>(-mflds.ib[2]),
                             static_cast<size_t>(-mflds.ib[1]),
                             static_cast<size_t>(-mflds.ib[0])};
      auto im = kg::io::Dims{n_comps, static_cast<size_t>(mflds.im[2]),
                             static_cast<size_t>(mflds.im[1]),
                             static_cast<size_t>(mflds.im[0])};
      var.setSelection({start, count});
      var.setMemorySelection({ib, im});
      var.get(reader, mflds.data[p].get());
    }
  }
};

#include "psc.h" // FIXME, just for EX etc

static Grid_t make_grid()
{
  auto domain =
    Grid_t::Domain{{8, 4, 2}, {80., 40., 20.}, {-40., -20., 0.}, {2, 2, 1}};
  auto bc = GridBc{};
  auto kinds = Grid_t::Kinds{};
  auto norm = Grid_t::Normalization{};
  double dt = .1;
  return Grid_t{domain, bc, kinds, norm, dt};
}

template <typename T>
class MfieldsTest : public ::testing::Test
{};

#ifdef USE_CUDA
using MfieldsTestTypes = ::testing::Types<MfieldsSingle, MfieldsC, MfieldsCuda>;
#else
using MfieldsTestTypes = ::testing::Types</*MfieldsSingle, */ MfieldsC>;
#endif

TYPED_TEST_CASE(MfieldsTest, MfieldsTestTypes);

TYPED_TEST(MfieldsTest, WriteRead)
{
  using Mfields = TypeParam;

  auto grid = make_grid();
  auto mflds = Mfields{grid, NR_FIELDS, {}};

  SetupFields<Mfields>::set(mflds, [](int m, double crd[3]) {
    return m + crd[0] + 100 * crd[1] + 10000 * crd[2];
  });

  auto io = kg::io::IO(MPI_COMM_WORLD);

  {
    auto writer = io.open("test.bp", kg::io::Mode::Write);
    writer.put("mflds", mflds);
    writer.close();
  }

  auto mflds2 = Mfields{grid, NR_FIELDS, {}};
  {
    auto reader = io.open("test.bp", kg::io::Mode::Read);
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
        EXPECT_EQ(mflds[p](m, i, j, k), mflds2[p](m, i, j, k));
      }
    });
  }
}

TYPED_TEST(MfieldsTest, WriteWithGhostsRead)
{
  using Mfields = TypeParam;

  auto grid = make_grid();
  auto mflds = Mfields{grid, NR_FIELDS, {2, 2, 2}};

  SetupFields<Mfields>::set(mflds, [](int m, double crd[3]) {
    return m + crd[0] + 100 * crd[1] + 10000 * crd[2];
  });

  auto io = kg::io::IO(MPI_COMM_WORLD);

  {
    auto writer = io.open("test.bp", kg::io::Mode::Write);
    writer.put("mflds", mflds);
    writer.close();
  }

  auto mflds2 = Mfields{grid, NR_FIELDS, {}};
  {
    auto reader = io.open("test.bp", kg::io::Mode::Read);
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
}

TYPED_TEST(MfieldsTest, WriteReadWithGhosts)
{
  using Mfields = TypeParam;

  auto grid = make_grid();
  auto mflds = Mfields{grid, NR_FIELDS, {}};

  SetupFields<Mfields>::set(mflds, [](int m, double crd[3]) {
    return m + crd[0] + 100 * crd[1] + 10000 * crd[2];
  });

  auto io = kg::io::IO(MPI_COMM_WORLD);

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
	mprintf("[%d, %d, %d] = %06g %06g %06g\n", i, j, k,
		(double) mflds[p](EX, i, j, k), (double) mflds[p](EY, i, j, k), (double) mflds[p](EZ, i, j, k));
#endif
      for (int m = 0; m < NR_FIELDS; m++) {
        EXPECT_EQ(mflds[p](m, i, j, k), mflds2[p](m, i, j, k))
          << " i " << i << " j " << j << " k " << k << " m " << m;
      }
    });
  }
}

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
