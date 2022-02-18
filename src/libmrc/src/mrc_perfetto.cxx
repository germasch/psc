
#include "mrc_perfetto.h"
#include "mrc_perfetto_trace.h"

#include <perfetto.h>
#include <mpi.h>
#include <fstream>

PERFETTO_TRACK_EVENT_STATIC_STORAGE();

#define PERFETTO_IN_PROCESS_BACKEND

static std::unique_ptr<perfetto::TracingSession> tracing_session;

void perfetto_initialize()
{
  perfetto::TracingInitArgs args;
#ifdef PERFETTO_IN_PROCESS_BACKEND
  args.backends |= perfetto::kInProcessBackend;
#else
  args.backends |= perfetto::kSystemBackend;
#endif

  perfetto::Tracing::Initialize(args);
  perfetto::TrackEvent::Register();

#ifdef PERFETTO_IN_PROCESS_BACKEND
  // perfetto::protos::gen::TrackEventConfig track_event_cfg;
  //   track_event_cfg.add_disabled_categories("*");
  //   track_event_cfg.add_enabled_categories("gene");

  perfetto::TraceConfig cfg;
  cfg.add_buffers()->set_size_kb(1024); // Record up to 1 MiB.
  auto* ds_cfg = cfg.add_data_sources()->mutable_config();
  ds_cfg->set_name("track_event");
  // ds_cfg->set_track_event_config_raw(track_event_cfg.SerializeAsString());

  tracing_session = perfetto::Tracing::NewTrace();
  tracing_session->Setup(cfg);
  tracing_session->StartBlocking();
#endif
}

void perfetto_finalize()
{
  perfetto::TrackEvent::Flush();

#ifdef PERFETTO_IN_PROCESS_BACKEND
  tracing_session->StopBlocking();
  std::vector<char> trace_data(tracing_session->ReadTraceBlocking());

  // Write the trace into a file.
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  std::ofstream output;
  output.open("atrace-" + std::to_string(rank) + ".perfetto",
              std::ios::out | std::ios::binary);
  output.write(trace_data.data(), trace_data.size());
  output.close();
#endif
}

void perfetto_event_begin(const char* s)
{
  TRACE_EVENT_BEGIN("psc", nullptr, [&](perfetto::EventContext ctx) {
    ctx.event()->set_name(s);
  });
}

void perfetto_event_end()
{
  TRACE_EVENT_END("psc");
}

// int main(int argv, char** argc)
// {
//   perfetto_init();

//   perfetto_perfon("abc");
//   //  TRACE_EVENT("gene", "LayerTreeHost::DoUpdateLayers");

//   for (int i = 0; i < 3; i++) {
//     perfetto_perfon("def");
//     //    TRACE_EVENT("gene", "PictureLayer::Update");
//     usleep(1000);
//     perfetto_perfoff();
//   }
//   perfetto_perfoff();

//   perfetto::TrackEvent::Flush();

//   perfetto_finalize();
// }
