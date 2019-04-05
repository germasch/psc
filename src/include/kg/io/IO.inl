
#include "Manager.h"

namespace kg
{

IO::IO(io::Manager& mgr, const std::string& name) : io_{mgr.ad_.DeclareIO(name)} {}

} // namespace kg
