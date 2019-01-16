
#ifndef CUDA_BITS_H
#define CUDA_BITS_H

#define cudaCheck(ierr) do {						\
    if (ierr != cudaSuccess)						\
      fprintf(stderr, "IERR = %d (%s)\n", ierr, cudaGetErrorName(ierr)); \
    assert(ierr == cudaSuccess);					\
  } while(0)

static bool CUDA_SYNC = true;

#define cuda_sync_if_enabled() do {					\
    cudaError ierr = cudaGetLastError(); cudaCheck(ierr);		\
    if (CUDA_SYNC) {							\
      cudaError_t ierr = cudaDeviceSynchronize(); cudaCheck(ierr);	\
    }									\
  } while(0)

__host__ __device__
static inline float cuda_int_as_float(int i)
{
  union { int i; float f; } u;
  u.i = i;
  return u.f;
};

__host__ __device__
static inline int cuda_float_as_int(float f)
{
  union { int i; float f; } u;
  u.f = f;
  return u.i;
};

void cuda_mem_status(const char *s);

extern size_t mem_cuda_mparticles;
extern size_t mem_cuda_mparticles_alt;
extern size_t mem_cuda_mfields;
extern size_t mem_cuda_heating_curand;
extern size_t mem_cuda_collision_curand;
extern size_t mem_cuda_bnd_maps;
extern size_t mem_cuda_bndp;

#endif
