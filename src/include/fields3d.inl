
#include "kg/io.h"

#include "psc_fields_c.h"

// ======================================================================
// Variable<Mfields>

template <>
class kg::io::Descr<MfieldsC>
{
  using DataType = MfieldsC::real_t;

public:
  using value_type = MfieldsC;

  void put(kg::io::Engine& writer, const value_type& mflds,
           const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    const Grid_t& grid = mflds.grid();

    writer.putAttribute("ib", mflds.ib, launch);
    writer.putAttribute("im", mflds.im, launch);

    auto& gdims = grid.domain.gdims;
    size_t n_comps = mflds.n_comps();
    auto shape = kg::io::Dims{n_comps, static_cast<size_t>(gdims[2]),
                              static_cast<size_t>(gdims[1]),
                              static_cast<size_t>(gdims[0])};
    auto var = kg::io::detail::Variable<DataType>{};
    var.setShape(shape);
    for (int p = 0; p < grid.n_patches(); p++) {
      auto& patch = grid.patches[p];
      auto start = kg::io::Dims{0, static_cast<size_t>(patch.off[2]),
                                static_cast<size_t>(patch.off[1]),
                                static_cast<size_t>(patch.off[0])};
      auto count = kg::io::Dims{n_comps, static_cast<size_t>(grid.ldims[2]),
                                static_cast<size_t>(grid.ldims[1]),
                                static_cast<size_t>(grid.ldims[0])};
      auto ib = kg::io::Dims{0, static_cast<size_t>(-mflds.ib[2]),
                             static_cast<size_t>(-mflds.ib[1]),
                             static_cast<size_t>(-mflds.ib[0])};
      auto im = kg::io::Dims{n_comps, static_cast<size_t>(mflds.im[2]),
                             static_cast<size_t>(mflds.im[1]),
                             static_cast<size_t>(mflds.im[0])};
      var.setSelection({start, count});
      var.setMemorySelection({ib, im});
      writer.putVariable(var, mflds.data[p].get());
    }
  }

  void get(kg::io::Engine& reader, value_type& mflds,
           const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    const Grid_t& grid = mflds.grid();

    // FIXME, should just check for consistency? (# ghosts might differ, too)
    // reader.getAttribute("ib", mflds.ib, launch);
    // reader.getAttribute("im", mflds.im, launch);

    auto& gdims = grid.domain.gdims;
    size_t n_comps = mflds.n_comps();
    auto shape = kg::io::Dims{n_comps, static_cast<size_t>(gdims[2]),
                              static_cast<size_t>(gdims[1]),
                              static_cast<size_t>(gdims[0])};
    assert(reader.variableShape<DataType>() == shape);
    auto var = kg::io::detail::Variable<DataType>{};
    for (int p = 0; p < grid.n_patches(); p++) {
      auto& patch = grid.patches[p];
      auto start = kg::io::Dims{0, static_cast<size_t>(patch.off[2]),
                                static_cast<size_t>(patch.off[1]),
                                static_cast<size_t>(patch.off[0])};
      auto count = kg::io::Dims{n_comps, static_cast<size_t>(grid.ldims[2]),
                                static_cast<size_t>(grid.ldims[1]),
                                static_cast<size_t>(grid.ldims[0])};
      auto ib = kg::io::Dims{0, static_cast<size_t>(-mflds.ib[2]),
                             static_cast<size_t>(-mflds.ib[1]),
                             static_cast<size_t>(-mflds.ib[0])};
      auto im = kg::io::Dims{n_comps, static_cast<size_t>(mflds.im[2]),
                             static_cast<size_t>(mflds.im[1]),
                             static_cast<size_t>(mflds.im[0])};
      var.setSelection({start, count});
      var.setMemorySelection({ib, im});
      reader.getVariable(var, mflds.data[p].get());
    }
  }
};

