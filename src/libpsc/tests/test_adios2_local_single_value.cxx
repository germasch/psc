
#include <adios2.h>

#define mprintf(fmt...) do { int __rank; MPI_Comm_rank(MPI_COMM_WORLD, &__rank); { printf("[%d] ", __rank); printf(fmt); } } while(0)

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  auto comm = MPI_COMM_WORLD;
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  
  auto ad = adios2::ADIOS(comm, adios2::DebugON);

  {
    auto io_witer = ad.DeclareIO("io_writer");
    auto varSize = io_witer.DefineVariable<int>("Size");
    auto varRankLocal = io_witer.DefineVariable<int>("RankLocal", {adios2::LocalValueDim});
    
    // --- writing
    auto writer = io_witer.Open("test.bp", adios2::Mode::Write);
    
    // write single global value
    if (rank == 0) {
      writer.Put(varSize, size);
    }
    
    // write local single value
    writer.Put(varRankLocal, rank);
  
    writer.Close();
  }

  {
    auto io_reader = ad.DeclareIO("io_reader");
    
    // --- reading back
    auto reader = io_reader.Open("test.bp", adios2::Mode::Read);

    auto varSize = io_reader.InquireVariable<int>("Size");
    auto varRankLocal = io_reader.InquireVariable<int>("RankLocal");
    // auto shape_id = varRankLocal.ShapeID();
    // mprintf("shapeId %d LocalArray %d LocalValue %d GlobalArray %d\n", shape_id, adios2::ShapeID::LocalArray,
    // 	    adios2::ShapeID::LocalValue, adios2::ShapeID::GlobalArray);

    auto shape = varRankLocal.Shape();
    //mprintf("shape (%ld) %ld\n", shape.size(), shape[0]);
    // varRankLocal.SetSelection({{0}, {2}});

    assert(varSize);
    assert(varRankLocal);
     
    // read single global value
    int r_size;
    reader.Get(varSize, r_size);

    // read local single value
    std::vector<int> r_rank(size);
    reader.Get(varRankLocal, r_rank.data());

    reader.Close();
    mprintf("B r_size %d\n", r_size);
    mprintf("B r_rank (%zu)", r_rank.size());
    for (auto val: r_rank) { printf(" %d", val); } printf("\n");
  }
  
  MPI_Finalize();
  return 0;
}
