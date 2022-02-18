
#ifndef MRC_PERFETTO_TRACE_H
#define MRC_PERFETTO_TRACE_H

#include <perfetto.h>

PERFETTO_DEFINE_CATEGORIES(
  perfetto::Category("psc").SetDescription("Events from PSC"),
  perfetto::Category("mpi").SetDescription("MPI Events"));

#endif
