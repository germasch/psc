
#ifndef CUDA_BITS_H
#define CUDA_BITS_H

#include "PscConfig.h"

#ifdef PSC_HAVE_RMM
#include <rmm/device_vector.hpp>
#include <rmm/exec_policy.hpp>

namespace psc
{
template <typename T>
using device_vector = rmm::device_vector<T>;
}

#else
#include <thrust/device_vector.h>

namespace psc
{
template <typename T>
using device_vector = thrust::device_vector<T>;
}
#endif

#define cudaCheck(ierr)                                                        \
  do {                                                                         \
    if (ierr != cudaSuccess) {                                                 \
      fprintf(stderr, "IERR = %d (%s) %s:%d\n", ierr, cudaGetErrorName(ierr),  \
              __FILE__, __LINE__);                                             \
    }                                                                          \
    if (ierr != cudaSuccess)                                                   \
      abort();                                                                 \
  } while (0)

static bool CUDA_SYNC = true;

#define cuda_sync_if_enabled()                                                 \
  do {                                                                         \
    cudaError ierr = cudaGetLastError();                                       \
    cudaCheck(ierr);                                                           \
    if (CUDA_SYNC) {                                                           \
      cudaError_t ierr = cudaDeviceSynchronize();                              \
      cudaCheck(ierr);                                                         \
    }                                                                          \
  } while (0)

__host__ __device__ static inline float cuda_int_as_float(int i)
{
  union
  {
    int i;
    float f;
  } u;
  u.i = i;
  return u.f;
};

__host__ __device__ static inline int cuda_float_as_int(float f)
{
  union
  {
    int i;
    float f;
  } u;
  u.f = f;
  return u.i;
};

template <typename V>
std::size_t allocated_bytes(const V& v)
{
  return v.capacity() * sizeof(typename V::value_type);
}

void mem_stats(std::string file, int line);

#endif
