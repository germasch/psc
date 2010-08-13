
#include "psc_sse2.h"

#include <stdlib.h>
#include <assert.h>

/////////
/// Fills the global constant numerical vectors used by sse2. 
///
/// **Must** be called before they are used
void 
init_vec_numbers(void) {		
  ones.r = pv_set1_real(1.0);			
  half.r = pv_set1_real(.5);			
  onepfive.r = pv_set1_real(1.5);		
  threefourths.r = pv_set1_real(.75);		
  third.r = pv_set1_real(1./3.);		
  ione.r = pv_set1_int(1);			
}

////////////
/// Multiply two vectors of 32 bit unsigned integers and return
/// another vector of 32 bit unsigned integers.
///
/// Note that both the single and double precison version of the code use
/// integer vectors of 4 32 bit integers, so this works for both. 
/// The double precision code only uses the first two elements of the vector.
/// If simd_wrap is expanded to use altivec, this function will need to defined
/// differently ( as the __m128i data type is SSE exclusive)

__m128i 
func_mul_epu32(__m128i a, __m128i b){
  // Multiply elements 0 and 2, and bput the 64 bit results into a vector.
  __m128i tmp02 = _mm_mul_epu32(a, b);
  // Shift the vectors by one word to the right, making 3->2, and 1->0, and 
  // then multiply into double word vector.
  __m128i tmp13 = _mm_mul_epu32( _mm_srli_si128(a, 4), _mm_srli_si128(b,4));
  // Shuffle the vectors to place the lower 32 bits of each results in the
  // lower two words. I have some concerns about endianness and portability
  // related to this function. 
  __m128i tmpres02 = _mm_shuffle_epi32(tmp02, _MM_SHUFFLE(0,0,2,0));
  __m128i tmpres13 = _mm_shuffle_epi32(tmp13, _MM_SHUFFLE(0,0,2,0));
  // upcack the shuffled vectors into a return value
  return _mm_unpacklo_epi32(tmpres02, tmpres13);
}
				      
/// Pointers to functions optimized for SSE2

struct psc_ops psc_ops_sse2 = {
  .name = "sse2",
  .push_part_yz_a         = sse2_push_part_yz_a,
  .push_part_yz_b         = sse2_push_part_yz_b,
  .push_part_yz           = sse2_push_part_yz,
  .push_part_xz           = sse2_push_part_xz,
}; 

/// \file psc_sse2.c Backend functions for SSE2 implementation.
