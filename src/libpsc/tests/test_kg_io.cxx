
#include <kg/io.h>

#include <gtest/gtest.h>

TEST(KgIo, WriteRead)
{
  auto ad = adios2::ADIOS(MPI_COMM_WORLD, adios2::DebugON);

  {
    auto io_writer = kg::IO(ad, "io_writer");
    auto writer = io_writer.open("test.bp", kg::Mode::Write);
    auto var_double = io_writer._defineVariable<double>("var_double");
    writer.put(var_double, 99.);
    writer.close();
  }

  {
    auto io_reader = kg::IO(ad, "io_reader");
    auto reader = io_reader.open("test.bp", kg::Mode::Read);
    auto var_double = io_reader._defineVariable<double>("var_double");
    double dbl;
    reader.get(var_double, dbl);
    reader.close();
    EXPECT_EQ(dbl, 99.);
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
