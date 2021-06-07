
#pragma once

#include <rmm/mr/device/thrust_allocator_adaptor.hpp>

#define GTENSOR_DEFAULT_DEVICE_ALLOCATOR(T) rmm::mr::thrust_allocator<T>

#include <gtensor/gtensor.h>
