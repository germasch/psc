
#include <kg/io.h>

#include <gtest/gtest.h>

TEST(KgIo, WriteRead)
{
  auto io = kg::io::IO(MPI_COMM_WORLD);

  {
    auto writer = io.open("test.bp", kg::io::Mode::Write);
    auto var_double = writer._defineVariable<double>("var_double");
    writer.put(var_double, 99.);
    writer.close();
  }

  {
    auto reader = io.open("test.bp", kg::io::Mode::Read);
    auto var_double = reader._defineVariable<double>("var_double");
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
