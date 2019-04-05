
#include <kg/io.h>

#include <gtest/gtest.h>

TEST(KgIo, WriteRead)
{
  auto mgr = kg::io::Manager(MPI_COMM_WORLD);

  {
    auto writer = mgr.open("test.bp", kg::Mode::Write);
    auto var_double = writer._defineVariable<double>("var_double");
    writer.put(var_double, 99.);
    writer.close();
  }

  {
    auto reader = mgr.open("test.bp", kg::Mode::Read);
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
