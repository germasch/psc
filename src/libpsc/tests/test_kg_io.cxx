
#include <kg/io.h>

#include <gtest/gtest.h>

TEST(KgIo, WriteRead)
{
  auto io = kg::io::IO(MPI_COMM_WORLD);

  {
    auto writer = io.open("test.bp", kg::io::Mode::Write);
    auto var_double =
      kg::io::VariableLocalSingleValue<double>{"var_double", writer};
    writer.put(var_double, "var_double", 99.);
    writer.close();
  }

  {
    auto reader = io.open("test.bp", kg::io::Mode::Read);
    auto var_double =
      kg::io::VariableLocalSingleValue<double>{"var_double", reader};
    double dbl;
    reader.get(var_double, "var_double", dbl);
    reader.close();
    EXPECT_EQ(dbl, 99.);
  }
}

TEST(KgIo, WriteReadAttr)
{
  auto io = kg::io::IO(MPI_COMM_WORLD);

  {
    auto writer = io.open("test.bp", kg::io::Mode::Write);
    writer.putAttribute("attr_double", 99.);
    writer.close();
  }

  {
    auto reader = io.open("test.bp", kg::io::Mode::Read);
    double dbl;
    reader.getAttribute("attr_double", dbl);
    reader.close();
    EXPECT_EQ(dbl, 99.);
  }
}

struct Custom
{
  int i;
  double d;
};

template <>
class kg::io::Variable<Custom>
{
public:
  Variable(const std::string& pfx, Engine& engine) {}

  static void put(kg::io::Engine& writer, const Custom& c,
                  const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    writer.putAttribute("i", c.i, launch);
    writer.putLocal("d", c.d, launch);
  }

  static void get(kg::io::Engine& reader, Custom& c,
                  const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    reader.getAttribute("i", c.i, launch);
    reader.getLocal("d", c.d, launch);
  }
};

TEST(KgIo, WriteReadCustom)
{
  auto io = kg::io::IO(MPI_COMM_WORLD);

  {
    auto writer = io.open("test.bp", kg::io::Mode::Write);
    auto c = Custom{3, 99.};
    writer.putVar("var_custom", c);
    writer.close();
  }

  {
    auto reader = io.open("test.bp", kg::io::Mode::Read);
    auto c = Custom{};
    reader.getVar("var_custom", c);
    reader.close();
    EXPECT_EQ(c.i, 3);
    EXPECT_EQ(c.d, 99.);
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
