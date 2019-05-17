
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

    writer.put("ib", mflds.ib, launch);
    writer.put("im", mflds.im, launch);

    auto& gdims = grid.domain.gdims;
    size_t n_comps = mflds.n_comps();
    auto shape = kg::io::Dims{n_comps, static_cast<size_t>(gdims[2]),
                              static_cast<size_t>(gdims[1]),
                              static_cast<size_t>(gdims[0])};
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
      writer.putVariable(mflds.data[p].get(), launch, shape, {start, count},
                         {ib, im});
    }
  }

  void get(kg::io::Engine& reader, value_type& mflds,
           const kg::io::Mode launch = kg::io::Mode::Deferred)
  {
    const Grid_t& grid = mflds.grid();

    // FIXME, should just check for consistency? (# ghosts might differ, too)
    // reader.get("ib", mflds.ib, launch);
    // reader.get("im", mflds.im, launch);

    auto& gdims = grid.domain.gdims;
    size_t n_comps = mflds.n_comps();
    auto shape = kg::io::Dims{n_comps, static_cast<size_t>(gdims[2]),
                              static_cast<size_t>(gdims[1]),
                              static_cast<size_t>(gdims[0])};
    assert(reader.variableShape<DataType>() == shape);
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
      reader.getVariable(mflds.data[p].get(), launch, {start, count}, {ib, im});
    }
  }
};
