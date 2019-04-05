
#pragma once

#include "Engine.h"

#include <adios2.h>

namespace kg
{

class IO;

namespace io
{

class Manager
{
public:
  Manager(MPI_Comm comm) : ad_{comm, adios2::DebugON} {}

  Engine open(const std::string& name, const adios2::Mode mode)
  {
    static int cnt;
    // FIXME, assumes that the ADIOS2 object underlying io_ was created on
    // MPI_COMM_WORLD
    auto comm = MPI_COMM_WORLD;
    auto io = IO(*this, "io" + name + "-" + std::to_string(cnt++));
    return {io.io_.Open(name, mode), io, comm};
  }

private:
  adios2::ADIOS ad_;

  friend class kg::IO;
};

} // namespace io
} // namespace kg
