
#include <gtest/gtest.h>
#include <adios2.h>

#include "test_common.hxx"
#include "grid.inl"

int mpi_rank, mpi_size;

// ======================================================================
// MPI test (to be run on 2 procs)

TEST(Grid, CtorComplete)
{
  EXPECT_EQ(mpi_size, 2);
  
  auto domain = Grid_t::Domain{{8, 4, 2},
			       {80.,  40., 20.}, {-40., -20., 0.},
			       {2, 2, 1}};
  auto bc = GridBc{};
  auto kinds = Grid_t::Kinds{};
  auto norm = Grid_t::Normalization{};
  double dt = .1;
  int n_patches = -1;
  auto grid = Grid_t{domain, bc, kinds, norm, dt, n_patches};

  EXPECT_EQ(grid.domain.gdims, Int3({ 8, 4, 2 }));
  EXPECT_EQ(grid.domain.dx, Grid_t::Real3({ 10., 10., 10. }));

  EXPECT_EQ(grid.ldims, Int3({ 4, 2, 2 }));
  EXPECT_EQ(grid.n_patches(), 2);
  if (mpi_rank == 0) {
    EXPECT_EQ(grid.patches[0].off, Int3({ 0, 0, 0 }));
    EXPECT_EQ(grid.patches[0].xb, Grid_t::Real3({ -40., -20.,  0. }));
    EXPECT_EQ(grid.patches[0].xe, Grid_t::Real3({   0.,   0., 20. }));
    EXPECT_EQ(grid.patches[1].off, Int3({ 4, 0, 0 }));
    EXPECT_EQ(grid.patches[1].xb, Grid_t::Real3({   0., -20.,  0. }));
    EXPECT_EQ(grid.patches[1].xe, Grid_t::Real3({  40.,   0., 20. }));
  } else {
    EXPECT_EQ(grid.patches[0].off, Int3({ 0, 2, 0 }));
    EXPECT_EQ(grid.patches[0].xb, Grid_t::Real3({ -40.,   0.,  0. }));
    EXPECT_EQ(grid.patches[0].xe, Grid_t::Real3({   0.,  20., 20. }));
    EXPECT_EQ(grid.patches[1].off, Int3({ 4, 2, 0 }));
    EXPECT_EQ(grid.patches[1].xb, Grid_t::Real3({   0.,   0.,  0. }));
    EXPECT_EQ(grid.patches[1].xe, Grid_t::Real3({  40.,  20., 20. }));
  }
}

TEST(Grid, MoveCtor)
{
  auto domain = Grid_t::Domain{{8, 4, 2},
			       {80.,  40., 20.}, {-40., -20., 0.},
			       {2, 2, 1}};
  auto offs = std::vector<Int3>{{0, 0, 0}, {4, 0, 0}};
  auto bc = GridBc{};
  auto kinds = Grid_t::Kinds{};
  auto norm = Grid_t::Normalization{};
  double dt = .1;
  int n_patches = -1;

  auto grid = Grid_t{domain, bc, kinds, norm, dt, n_patches};
  auto grid2 = std::move(grid);
}
  
TEST(Grid, MoveAssign)
{
  auto domain = Grid_t::Domain{{8, 4, 2},
			       {80.,  40., 20.}, {-40., -20., 0.},
			       {2, 2, 1}};
  auto offs = std::vector<Int3>{{0, 0, 0}, {4, 0, 0}};
  auto bc = GridBc{};
  auto kinds = Grid_t::Kinds{};
  auto norm = Grid_t::Normalization{};
  double dt = .1;
  int n_patches = -1;

  auto grid = Grid_t{domain, bc, kinds, norm, dt, n_patches};
  auto grid2 = Grid_t{domain, bc, kinds, norm, dt, n_patches};

  grid2 = std::move(grid);
}
  
TEST(Grid, Kinds)
{
  auto kinds = Grid_t::Kinds{};
  kinds.emplace_back(Grid_t::Kind(1., 1., "test_species"));
  EXPECT_EQ(kinds.size(), 1);
}

TEST(Grid, adios2_write)
{
  auto ad = adios2::ADIOS(MPI_COMM_WORLD, adios2::DebugON);

  {
    auto domain = Grid_t::Domain{{8, 4, 2},
				 {80.,  40., 20.}, {-40., -20., 0.},
				 {2, 2, 1}};
    auto offs = std::vector<Int3>{{0, 0, 0}, {4, 0, 0}};
    auto bc = GridBc{};
    auto kinds = Grid_t::Kinds{};
    auto norm = Grid_t::Normalization{};
    double dt = .1;
    int n_patches = -1;
    auto grid = Grid_t{domain, bc, kinds, norm, dt, n_patches};

    auto io_writer = kg::IO(ad, "io_writer");
    auto var_grid = io_writer.defineVariable<Grid_t>("grid");
    
    auto writer = io_writer.open("test.bp", kg::Mode::Write);
    writer.put(var_grid, grid);
    writer.close();
  }

  {
    double dt;

    auto io_reader = kg::IO(ad, "io_reader");
    auto reader = io_reader.open("test.bp", kg::Mode::Read);
    auto var_dt = io_reader.inquireVariable<double>("grid.dt");
    assert(bool(var_dt));
    reader.get(var_dt, dt);
    reader.close();

    EXPECT_EQ(dt, .1);
  }
}

// ======================================================================
// main

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  ::testing::InitGoogleTest(&argc, argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  int rc = RUN_ALL_TESTS();

  MPI_Finalize();
  return rc;
}
