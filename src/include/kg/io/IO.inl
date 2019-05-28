
namespace kg
{
namespace io
{

inline IOAdios::IOAdios(MPI_Comm comm) : comm_{comm}, ad_{comm, adios2::DebugON}
{}

inline Engine IOAdios::open(const std::string& name, const Mode mode)
{
  return {new FileAdios2{ad_, name, mode}, comm_};
}

} // namespace io
} // namespace kg
