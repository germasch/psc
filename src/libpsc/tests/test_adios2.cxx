
#include <adios2.h>

void test1()
{
  using T = char;

  adios2::ADIOS ad(MPI_COMM_WORLD, adios2::DebugON);

  {
    adios2::IO io_writer = ad.DeclareIO("writer");
    auto var = io_writer.DefineVariable<T>("var");
    auto writer = io_writer.Open("test.bp", adios2::Mode::Write);
    
    T test = 99;
    writer.Put(var, test);
    writer.Close();
  }

  {
    adios2::IO io_reader = ad.DeclareIO("reader");
    auto reader = io_reader.Open("test.bp", adios2::Mode::Read);
    auto var = io_reader.InquireVariable<T>("var");
    assert(static_cast<bool>(var));
    
    T test;
    reader.Get(var, test);
    reader.Close();

    assert(test == 99);
  }
}

template<typename T>
void print_type(const char* name)
{
  printf("%s: %s %s\n", name, typeid(T).name(), typeid(typename adios2::TypeInfo<T>::IOType).name());
}

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  auto comm = MPI_COMM_WORLD;
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  print_type<char>("char");
  print_type<signed char>("signed char");
  print_type<unsigned char>("unsigned char");
  
  print_type<unsigned long>("unsigned long");
  print_type<unsigned long long>("unsigned long long");
  print_type<size_t>("size_t");
  print_type<uint64_t>("uint64_t");
  test1();
  return 0;

  //auto configfile = "adios2.xml";
  adios2::ADIOS ad(comm, adios2::DebugON);

  adios2::IO bpio = ad.DeclareIO("writer");

  const size_t n = 10;
  const size_t N = n * size;
  double T[n];
  for (int i = 0; i < n; i++) {
    T[i] = i + n * rank;
  }

  auto varSize = bpio.DefineVariable<int>("Size");
  auto varRank = bpio.DefineVariable<int>("Rank");
  auto varT = bpio.DefineVariable<double>(
        "T",
        // Global dimensions
        {N},
        // starting offset of the local array in the global space
        {n*rank},
        // local size, could be defined later using SetSelection()
        {n});

  // Promise that we are not going to change the variable sizes nor add new
  // variables
  //bpio.LockDefinitions();
  
  auto bpWriter = bpio.Open("test.bp", adios2::Mode::Write, comm);

  if (rank == 0) {
    bpWriter.Put(varSize, size);
  }

  bpWriter.Put(varRank, rank);

#if 0 // FIXME, crashes bpls -D
  auto varRank2 = bpio.DefineVariable<int>("Rank2", {adios2::LocalValueDim});
  bpWriter.Put(varRank2, rank);
#endif

#if 0 // local array, doesn't seem to work right, doesn't show in bpls unless we skip "T" below
  auto varTlocal = bpio.DefineVariable<double>("Tlocal", {}, {}, {n});
  double Tlocal[n] = {0, -1, -2, -3, -4, -5, -6, -7, -8, -9};
  bpWriter.Put(varTlocal, Tlocal);
#endif
  
#if 1
  for (int s = 0; s < 3; s++) {
    bpWriter.BeginStep();
    bpWriter.Put(varT, T);
    bpWriter.EndStep();
  }
#endif
  
  bpWriter.Close();  

  MPI_Finalize();
  return 0;
}
