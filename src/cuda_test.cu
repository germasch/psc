
#include <rmm/thrust_rmm_allocator.h>
#include <rmm/mr/device/cnmem_memory_resource.hpp>

int main()
{
  rmm::mr::cnmem_memory_resource pool_mr{};
  //rmm::mr::set_default_resource(&pool_mr);

  for (int i = 0; i < 5; i++) {
    rmm::device_vector<float> x(1000000);
  }
}
