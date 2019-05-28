
namespace kg
{
namespace io
{

inline IOAdios::IOAdios(MPI_Comm comm) : comm_{comm}, ad_{comm, adios2::DebugON}
{}

inline Engine IOAdios::open(const std::string& name, const Mode mode)
{
  static int cnt;
  auto io = ad_.DeclareIO("io" + name + "-" + std::to_string(cnt++));
  adios2::Mode adios2_mode;
  if (mode == Mode::Read) {
    adios2_mode = adios2::Mode::Read;
  } else if (mode == Mode::Write) {
    adios2_mode = adios2::Mode::Write;
  } else {
    assert(0);
  }
  File file{new FileAdios2{io.Open(name, adios2_mode), io}};
  return {std::move(file), comm_};
}

} // namespace io
} // namespace kg
