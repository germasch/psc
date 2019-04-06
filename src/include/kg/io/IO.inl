
namespace kg
{
namespace io
{

IO::IO(MPI_Comm comm) : comm_{comm}, ad_{comm, adios2::DebugON} {}

Engine IO::open(const std::string& name, const adios2::Mode mode)
{
  static int cnt;
  // FIXME, assumes that the ADIOS2 object underlying io_ was created on
  // MPI_COMM_WORLD
  auto io = ad_.DeclareIO("io" + name + "-" + std::to_string(cnt++));
  return {io.Open(name, mode), io, comm_};
}

} // namespace io
} // namespace kg
