
#pragma once

#include "Engine.h"

#include <adios2.h>

namespace kg
{
namespace io
{

class Manager
{
public:
  Manager(MPI_Comm comm) : comm_{comm}, ad_{comm, adios2::DebugON} {}

  Engine open(const std::string& name, const adios2::Mode mode)
  {
    static int cnt;
    // FIXME, assumes that the ADIOS2 object underlying io_ was created on
    // MPI_COMM_WORLD
    auto io = ad_.DeclareIO("io" + name + "-" + std::to_string(cnt++));
    return {io.Open(name, mode), io, comm_};
  }

private:
  MPI_Comm comm_; // FIXME, should probably be MPI_Comm_dup'd, but then one
                  // needs to be careful with ad_ destruction
  adios2::ADIOS ad_;
};

} // namespace io
} // namespace kg
