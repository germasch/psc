
#pragma once

#include "rng_state.cuh"

// ----------------------------------------------------------------------
// cuda_base

void cuda_base_init(void);

std::size_t mem_cuda_allocated();

RngStateCuda& get_rng_state();
