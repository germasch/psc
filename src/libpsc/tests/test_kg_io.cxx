
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

TEST(KgIo, WriteReadAttr)
{
  auto io = kg::io::IO(MPI_COMM_WORLD);

  {
    auto writer = io.open("test.bp", kg::io::Mode::Write);
    auto attr_double = kg::io::Attribute<double>{"attr_double", writer};
    writer.put(attr_double, 99.);
    writer.close();
  }

  {
    auto reader = io.open("test.bp", kg::io::Mode::Read);
    auto attr_double = kg::io::Attribute<double>{"attr_double", reader};
    double dbl;
    reader.get(attr_double, dbl);
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
  Variable(const std::string& name, kg::io::Engine& engine)
    : attr_i_{name + ".i", engine}, var_d_{name + ".d", engine}
  {}

  void put(kg::io::Engine& writer, const Custom& c,
           const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    writer.put(attr_i_, c.i, launch);
    writer.put(var_d_, c.d, launch);
  }

  void get(kg::io::Engine& reader, Custom& c,
           const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    reader.get(attr_i_, c.i, launch);
    reader.get(var_d_, c.d, launch);
  }

private:
  kg::io::Attribute<int> attr_i_;
  kg::io::VariableLocalSingleValue<double> var_d_;
};

TEST(KgIo, WriteReadCustom)
{
  auto io = kg::io::IO(MPI_COMM_WORLD);

  {
    auto writer = io.open("test.bp", kg::io::Mode::Write);
    auto var_custom = kg::io::Variable<Custom>{"var_custom", writer};
    auto c = Custom{3, 99.};
    writer.put(var_custom, c);
    writer.close();
  }

  {
    auto reader = io.open("test.bp", kg::io::Mode::Read);
    auto var_custom = kg::io::Variable<Custom>{"var_custom", reader};
    auto c = Custom{};
    reader.get(var_custom, c);
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
