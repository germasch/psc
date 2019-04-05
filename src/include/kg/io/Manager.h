
#pragma once

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

private:
  adios2::ADIOS ad_;

  friend class kg::IO;
};

} // namespace io
} // namespace kg
