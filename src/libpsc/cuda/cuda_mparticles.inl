
#include <kg/io.h>

template <>
class kg::io::Descr<DMparticlesCuda<BS144>>
{
  using DMparticles = DMparticlesCuda<BS144>;
public:
  void put(kg::io::Engine& writer, const DMparticles& d_mprts,
           const kg::io::Mode launch = kg::io::Mode::NonBlocking)
  {
    auto ldims = Vec3<uint>::fromPointer(d_mprts.ldims_);
    writer.put("ldims", ldims);
    auto b_mx = Vec3<uint>::fromPointer(d_mprts.b_mx_);
    writer.put("b_mx", b_mx);
    writer.put("n_blocks", d_mprts.n_blocks_, launch);
    auto dxi = Vec3<float>::fromPointer(d_mprts.dxi_);
    writer.put("dxi", dxi);
    
    writer.put("dt", d_mprts.dt_, launch);
    writer.put("fnqs", d_mprts.fnqs_, launch);
    writer.put("fnqxs", d_mprts.fnqxs_, launch);
    writer.put("fnqys", d_mprts.fnqys_, launch);
    writer.put("fnqzs", d_mprts.fnqzs_, launch);
    writer.put("dqs", d_mprts.dqs_, launch);
    std::vector<float> dq(d_mprts.dq_, d_mprts.dq_ + 4);
    writer.put("dq", dq, kg::io::Mode::Blocking);
    std::vector<float> q_inv(d_mprts.q_inv_, d_mprts.q_inv_ + 4);
    writer.put("q_inv", q_inv, kg::io::Mode::Blocking);
    std::vector<float> q(d_mprts.q_, d_mprts.q_ + 4);
    writer.put("q", q, kg::io::Mode::Blocking);
    std::vector<float> m(d_mprts.m_, d_mprts.m_ + 4);
    writer.put("m", m, kg::io::Mode::Blocking);

    // raw storage
    size_t size = d_mprts.size_;
    {
      writer.pushPrefix("storage");
      writer.pushPrefix("xi4");
      thrust::host_vector<float4> h_xi4(thrust::device_pointer_cast(d_mprts.storage.xi4),
					thrust::device_pointer_cast(d_mprts.storage.xi4 + size));
      writer.putVariable((float*)h_xi4.data(), launch, {4*size}, {{0ul}, {4*size}});
      writer.popPrefix();
      writer.pushPrefix("pxi4");
      thrust::host_vector<float4> h_pxi4(thrust::device_pointer_cast(d_mprts.storage.pxi4),
					 thrust::device_pointer_cast(d_mprts.storage.pxi4 + size));
      writer.putVariable((float*)h_pxi4.data(), launch, {4*size}, {{0ul}, {4*size}});
      writer.popPrefix();
      writer.popPrefix();
      writer.performPuts();
    }

    {
      writer.pushPrefix("alt_storage");
      writer.pushPrefix("xi4");
      thrust::host_vector<float4> h_xi4(thrust::device_pointer_cast(d_mprts.alt_storage.xi4),
					thrust::device_pointer_cast(d_mprts.alt_storage.xi4 + size));
      writer.putVariable((float*)h_xi4.data(), launch, {4*size}, {{0ul}, {4*size}});
      writer.popPrefix();
      writer.pushPrefix("pxi4");
      thrust::host_vector<float4> h_pxi4(thrust::device_pointer_cast(d_mprts.alt_storage.pxi4),
					 thrust::device_pointer_cast(d_mprts.alt_storage.pxi4 + size));
      writer.putVariable((float*)h_pxi4.data(), launch, {4*size}, {{0ul}, {4*size}});
      writer.popPrefix();
      writer.popPrefix();
      writer.performPuts();
    }
    {
      writer.pushPrefix("id");
      thrust::host_vector<uint> h_id(thrust::device_pointer_cast(d_mprts.id_),
				     thrust::device_pointer_cast(d_mprts.id_ + size));
      writer.putVariable(h_id.data(), launch, {size}, {{0}, {size}});
      writer.popPrefix();
      writer.pushPrefix("bidx");
      thrust::host_vector<uint> h_bidx(thrust::device_pointer_cast(d_mprts.bidx_),
				       thrust::device_pointer_cast(d_mprts.bidx_ + size));
      writer.putVariable(h_bidx.data(), launch, {size}, {{0}, {size}});
      writer.popPrefix();
      writer.performPuts();
    }
    {
      auto n_blocks = d_mprts.n_blocks_;
      writer.pushPrefix("off");
      thrust::host_vector<uint> h_off(thrust::device_pointer_cast(d_mprts.off_),
				      thrust::device_pointer_cast(d_mprts.off_ + n_blocks + 1));
      writer.putVariable(h_off.data(), launch, {n_blocks + 1}, {{0}, {n_blocks + 1}});
      writer.popPrefix();
      writer.performPuts();
    }
  }

  void get(kg::io::Engine& reader, DMparticles& d_mprts,
           const kg::io::Mode launch = kg::io::Mode::NonBlocking)
  {
    Vec3<uint> ldims, b_mx;
    Vec3<float> dxi;
    reader.get("ldims", ldims, launch);
    reader.get("b_mx", b_mx, launch);
    reader.get("dxi", dxi, launch);
    reader.get("n_blocks", d_mprts.n_blocks_, launch);
    reader.performGets();
    for (int d = 0; d < 3; d++) {
      d_mprts.ldims_[d] = ldims[d];
      d_mprts.b_mx_[d] = b_mx[d];
      d_mprts.dxi_[d] = dxi[d];
    }
    
    reader.get("dt", d_mprts.dt_, launch);
    reader.get("fnqs", d_mprts.fnqs_, launch);
    reader.get("fnqxs", d_mprts.fnqxs_, launch);
    reader.get("fnqys", d_mprts.fnqys_, launch);
    reader.get("fnqzs", d_mprts.fnqzs_, launch);
    reader.get("dqs", d_mprts.dqs_, launch);

    std::vector<float> dq(4);
    reader.get("dq", dq, kg::io::Mode::Blocking);
    std::copy(dq.begin(), dq.end(), d_mprts.dq_);
    std::vector<float> q_inv(4);
    reader.get("q_inv", q_inv, kg::io::Mode::Blocking);
    std::copy(q_inv.begin(), q_inv.end(), d_mprts.q_inv_);
    std::vector<float> q(4);
    reader.get("q", q, kg::io::Mode::Blocking);
    std::copy(q.begin(), q.end(), d_mprts.q_);
    std::vector<float> m(4);
    reader.get("m", m, kg::io::Mode::Blocking);
    std::copy(m.begin(), m.end(), d_mprts.m_);

    // raw storage
    reader.pushPrefix("storage");
    reader.pushPrefix("xi4");
    auto shape = reader.variableShape<float>();
    reader.popPrefix();
    reader.popPrefix();
    //std::cout << "shape" << shape << "\n";
    size_t size = shape[0] / 4;
    d_mprts.size_ = size;
    printf("n_blocks %d size %zu\n", d_mprts.n_blocks_, size);
    {
      reader.pushPrefix("storage");
      reader.pushPrefix("xi4");
      thrust::host_vector<float4> h_xi4(size);
      reader.getVariable((float*)h_xi4.data(), launch, {{0ul}, {4*size}});
      reader.popPrefix();
      reader.pushPrefix("pxi4");
      thrust::host_vector<float4> h_pxi4(size);
      reader.getVariable((float*)h_pxi4.data(), launch, {{0ul}, {4*size}});
      reader.popPrefix();
      reader.popPrefix();
      reader.performGets();
      auto d_xi4 = new thrust::device_vector<float4>(h_xi4);
      auto d_pxi4 = new thrust::device_vector<float4>(h_pxi4);
      d_mprts.storage.xi4 = d_xi4->data().get();
      d_mprts.storage.pxi4 = d_pxi4->data().get();
    }

    {
      reader.pushPrefix("alt_storage");
      reader.pushPrefix("xi4");
      thrust::host_vector<float4> h_xi4(size);
      reader.getVariable((float*)h_xi4.data(), launch, {{0ul}, {4*size}});
      reader.popPrefix();
      reader.pushPrefix("pxi4");
      thrust::host_vector<float4> h_pxi4(size);
      reader.getVariable((float*)h_pxi4.data(), launch, {{0ul}, {4*size}});
      reader.popPrefix();
      reader.popPrefix();
      reader.performGets();
      auto d_xi4 = new thrust::device_vector<float4>(h_xi4);
      auto d_pxi4 = new thrust::device_vector<float4>(h_pxi4);
      d_mprts.alt_storage.xi4 = d_xi4->data().get();
      d_mprts.alt_storage.pxi4 = d_pxi4->data().get();
    }
    {
      reader.pushPrefix("id");
      thrust::host_vector<uint> h_id(size);
      reader.getVariable(h_id.data(), launch, {{0}, {size}});
      reader.popPrefix();
      reader.pushPrefix("bidx");
      thrust::host_vector<uint> h_bidx(size);
      reader.getVariable(h_bidx.data(), launch, {{0}, {size}});
      reader.popPrefix();
      reader.performGets();
      auto d_id = new thrust::device_vector<uint>(h_id);
      auto d_bidx = new thrust::device_vector<uint>(h_bidx);
      d_mprts.id_ = d_id->data().get();
      d_mprts.bidx_ = d_bidx->data().get();
    }
    {
      auto n_blocks = d_mprts.n_blocks_;
      reader.pushPrefix("off");
      thrust::host_vector<uint> h_off(n_blocks + 1);
      reader.getVariable(h_off.data(), launch, {{0}, {n_blocks + 1}});
      reader.popPrefix();

      reader.performGets();
      auto d_off = new thrust::device_vector<uint>(h_off);
      d_mprts.off_ = d_off->data().get();
    }
  }
};

