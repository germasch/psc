
namespace kg
{
namespace io
{

inline IOAdios::IOAdios(MPI_Comm comm) : comm_{comm}, ad_{comm, adios2::DebugON}
{}

inline Engine IOAdios::open(const std::string& name, const adios2::Mode mode)
{
  static int cnt;
  auto io = ad_.DeclareIO("io" + name + "-" + std::to_string(cnt++));
  return {io.Open(name, mode), io, comm_};
}

} // namespace io
} // namespace kg
