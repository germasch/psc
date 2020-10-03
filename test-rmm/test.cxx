
#include <thrust/version.h>

#include <rmm/mr/device/cuda_memory_resource.hpp>
#include <rmm/mr/device/logging_resource_adaptor.hpp>
#include <rmm/mr/device/per_device_resource.hpp>
#include <rmm/thrust_rmm_allocator.h>

#include <iostream>
#include <vector>

extern void v();

void print(const std::vector<int>& vec)
{
  for (auto i : vec) {
    std::cout << " " << i;
  }
  std::cout << "\n";
}

void test_tracking()
{
  static rmm::mr::cuda_memory_resource pool_mr;
  auto log_mr = rmm::mr::make_logging_adaptor(&pool_mr);
  rmm::mr::set_current_device_resource(&log_mr);
  
  thrust::device_vector<double> x;
  x.resize(10);
}

int main()
{
  v();
  std::cout << "cxx " << THRUST_VERSION << "\n";

  std::vector<int> vec = {1, 2, 3};

  print(vec);

  test_tracking();
}
