
#pragma once

#include <kg/io.h>

#include "fields3d.inl"

#include <cstdio>

#include "io_common.h"
#include "kg/io.h"

#define PSC_USE_IO_THREADS

#ifdef PSC_USE_IO_THREADS

#include <thread>

static std::mutex writer_mutex;

void WriterThread(std::reference_wrapper<const Grid_t> grid_ref,
                  std::reference_wrapper<kg::io::IOAdios2> io, std::string pfx,
                  std::string dir, int step, double time,
                  gt::gtensor<double, 5>&& h_expr, std::string name,
                  std::vector<std::string> comp_names)
{
  static int pr;
  if (!pr) {
    pr = prof_register("writer thread", 1., 0, 0);
  }

  prof_start(pr);
  MPI_Comm comm;
  MPI_Comm_dup(MPI_COMM_WORLD, &comm);

  const Grid_t& g = grid_ref;
  Grid_t grid(g.domain, g.bc, g.kinds, g.norm, g.dt, g.n_patches(), g.ibn);

  Mfields<double> h_mflds(grid, h_expr.shape(3), {});
  h_mflds.gt() = h_expr;

  char filename[dir.size() + pfx.size() + 20];
  sprintf(filename, "%s/%s.%09d.bp", dir.c_str(), pfx.c_str(), step);
  {
    std::lock_guard<std::mutex> guard(writer_mutex);
    auto file = io.get().open(filename, kg::io::Mode::Write, comm, pfx);

    file.beginStep(kg::io::StepMode::Append);
    file.put("step", step);
    file.put("time", time);

    file.put(name, h_mflds);
    file.performPuts();
    file.endStep();
    file.close();
  }
  MPI_Comm_free(&comm);
  prof_stop(pr);
}

#endif

class WriterADIOS2
{
public:
  explicit operator bool() const { return pfx_.size() != 0; }

  ~WriterADIOS2()
  {
    if (writer_thread_.joinable()) {
      writer_thread_.join();
    }
  }

  void open(const std::string& pfx, const std::string& dir = ".")
  {
    assert(pfx_.size() == 0);
    pfx_ = pfx;
    dir_ = dir;
  }

  void close()
  {
    assert(pfx_.size() != 0);
    pfx_.clear();
    dir_.clear();
  }

  void begin_step(const Grid_t& grid)
  {
    begin_step(grid.timestep(), grid.timestep() * grid.dt);
  }

  void begin_step(int step, double time)
  {
    char filename[dir_.size() + pfx_.size() + 20];
    sprintf(filename, "%s/%s.%09d.bp", dir_.c_str(), pfx_.c_str(), step);
    file_ = io__.open(filename, kg::io::Mode::Write, MPI_COMM_WORLD, pfx_);
    file_.beginStep(kg::io::StepMode::Append);
    file_.put("step", step);
    file_.put("time", time);
    file_.performPuts();
  }

  void end_step() { file_.close(); }

  void set_subset(const Grid_t& grid, Int3 rn, Int3 rx) {}

  template <typename E>
  void write(const E& expr, const Grid_t& grid, const std::string& name,
             const std::vector<std::string>& comp_names)
  {
    static int pr_write, pr_eval;
    if (!pr_write) {
      pr_write = prof_register("adios2_write", 1., 0, 0);
      pr_eval = prof_register("adios2_eval", 1., 0, 0);
    }

    prof_start(pr_eval);
    auto h_expr = gt::host_mirror(expr);
    gt::copy(gt::eval(expr), h_expr);
    Mfields<gt::expr_value_type<E>> h_mflds(grid, h_expr.shape(3), {});
    h_mflds.gt() = h_expr;
    prof_stop(pr_eval);

    prof_start(pr_write);
    file_.put(name, h_mflds);
    file_.performPuts();
    prof_stop(pr_write);
  }

  template <typename E>
  void write_step(const Grid_t& grid, const Int3& rn, const Int3& rx,
                  const E& expr, const std::string& name,
                  const std::vector<std::string>& comp_names)
  {

    static int pr, pr_copy, pr_wait, pr_write;
    if (!pr) {
      pr = prof_register("write_step", 1., 0, 0);
      pr_copy = prof_register("ws copy", 1., 0, 0);
      pr_wait = prof_register("ws wait", 1., 0, 0);
      pr_write = prof_register("ws write", 1., 0, 0);
    }

    prof_start(pr);

    // FIXME? rn, rx ignored
    prof_start(pr_copy);
    auto h_expr = gt::host_mirror(expr);
    gt::copy(gt::eval(expr), h_expr);
    prof_stop(pr_copy);

#ifdef PSC_USE_IO_THREADS
    if (writer_thread_.joinable()) {
      // make sure previous I/O is finished
      prof_start(pr_wait);
      writer_thread_.join();
      prof_stop(pr_wait);
    }
    prof_start(pr_write);
    int step = grid.timestep();
    double time = step * grid.dt;
    writer_thread_ = std::thread{
      WriterThread, std::cref(grid),   std::ref(io__), pfx_,      dir_, step,
      time,         std::move(h_expr), name,           comp_names};
    prof_stop(pr_write);
#else
    prof_start(pr_write);
    begin_step(grid);
    set_subset(grid, rn, rx);
    write(expr, grid, name, comp_names);
    end_step();
    prof_stop(pr_wite);
#endif
    prof_stop(pr);
  }

private:
  kg::io::IOAdios2 io__;
  kg::io::Engine file_;
  std::string pfx_;
  std::string dir_;
#ifdef PSC_USE_IO_THREADS
  std::thread writer_thread_;
#endif
};
