
#include "io_common.h"
#include "kg/io.h"

template <>
class kg::io::Descr<DMFields>
{
public:
  void put(kg::io::Engine& writer, const DMFields& d_mflds,
           const kg::io::Mode launch = kg::io::Mode::NonBlocking)
  {
    writer.put("ib", d_mflds.box().ib(), launch);
    writer.put("im", d_mflds.box().im(), launch);
    writer.put("n_comps", d_mflds.n_comps(), launch);
    writer.put("n_patches", d_mflds.n_patches(), launch);

    // raw storage
    thrust::host_vector<float> h_data(thrust::device_pointer_cast(d_mflds.begin()),
				      thrust::device_pointer_cast(d_mflds.end()));
    writer.putVariable(h_data.data(), launch, {d_mflds.size()}, {{0}, {d_mflds.size()}},
		       {{0}, {d_mflds.size()}});
  }

  void get(kg::io::Engine& reader, DMFields& d_mflds,
           const kg::io::Mode launch = kg::io::Mode::NonBlocking)
  {
    // FIXME, should just check for consistency? (# ghosts might differ, too)
    Int3 ib, im;
    int n_comps, n_patches;
    reader.get("ib", ib, kg::io::Mode::Blocking);
    reader.get("im", im, kg::io::Mode::Blocking);
    reader.get("n_comps", n_comps, kg::io::Mode::Blocking);
    reader.get("n_patches", n_patches, kg::io::Mode::Blocking);
    assert(ib == d_mflds.box().ib());
    assert(im == d_mflds.box().im());
    assert(n_comps == d_mflds.n_comps());
    assert(n_patches == d_mflds.n_patches());

    // raw storage
    thrust::host_vector<float> h_data(d_mflds.size());
    reader.getVariable(h_data.data(), kg::io::Mode::Blocking,
		       {{0}, {d_mflds.size()}}, {{0}, {d_mflds.size()}});
    thrust::copy(h_data.begin(), h_data.end(), thrust::device_pointer_cast(d_mflds.begin()));
  }
};

