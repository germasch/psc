// clang-format off

#include <mrc_perfetto_trace.h>

#include <iostream>

static void perfetto_mpi_tracing_init()
{
  std::cout << "Initializing Perfetto MPI tracing" << std::endl;

//   perfetto::TracingInitArgs args;
//   args.backends |= perfetto::kSystemBackend;
// //  args.backends |= perfetto::kInProcessBackend;

//   perfetto::Tracing::Initialize(args);
  perfetto::TrackEvent::Register();
}

// Initialize MPI and perfetto
{{fn name MPI_Init}}
  perfetto_mpi_tracing_init();
  {
    TRACE_EVENT("mpi", "{{name}}");
    {{callfn}}
  }
{{endfn}}

{{fn name MPI_Init_thread}}
  perfetto_mpi_tracing_init();
  {
    TRACE_EVENT("mpi", "{{name}}");
    {{callfn}}
  }
{{endfn}}

// Finalize MPI and perfetto
{{fn name MPI_Finalize}}
  {
    TRACE_EVENT("mpi", "{{name}}");
    {{callfn}}
  }

  perfetto::TrackEvent::Flush();
{{endfn}}

extern void* mpi_fortran_in_place_;

void* mpi_in_place_f2c(const void* ptr)
{
  if (ptr == &mpi_fortran_in_place_) {
    return MPI_IN_PLACE;
  } else {
    return (void*)ptr;
  }
}

{{fn name MPI_Send MPI_Recv MPI_Allreduce MPI_Reduce MPI_Wait MPI_Waitany
MPI_Waitall MPI_Waitsome MPI_Gather MPI_Gatherv MPI_Scatter MPI_Scatterv
MPI_Allgather MPI_Allgatherv MPI_Alltoall MPI_Alltoallv MPI_Alltoallw
MPI_Bcast MPI_Sendrecv MPI_Barrier MPI_Isend MPI_Irecv}}
  {
    TRACE_EVENT("mpi", "{{name}}");
    {{callfn}}
  }
{{endfn}}
