
#pragma once

#include "Engine.h"

namespace kg
{
namespace io
{

// ======================================================================
// IOAdios

class IOAdios
{
public:
  IOAdios(MPI_Comm comm);

  Engine open(const std::string& name, const adios2::Mode mode);

private:
  MPI_Comm comm_; // FIXME, should probably be MPI_Comm_dup'd, but then one
                  // needs to be careful with ad_ destruction
  adios2::ADIOS ad_;
};

} // namespace io
} // namespace kg

#include "IO.inl"
