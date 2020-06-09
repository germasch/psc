
#pragma once

// ----------------------------------------------------------------------

template<typename _Order, typename _Dim, typename _fields_t>
struct CurrentNone;

// ----------------------------------------------------------------------

template <typename Curr, typename real_t = typename Curr::real_t>
__host__ __device__
inline void currx(Curr& curr, const int i[3], real_t fnqx, const real_t dx[3], const real_t xa[3],
		  real_t h, const int off[3])
{
  curr.add(0, i[0], i[1]  , i[2]  , fnqx * (dx[0] * (1.f - xa[1]) * (1.f - xa[2]) + h), off);
  curr.add(0, i[0], i[1]+1, i[2]  , fnqx * (dx[0] * (      xa[1]) * (1.f - xa[2]) - h), off);
  curr.add(0, i[0], i[1]  , i[2]+1, fnqx * (dx[0] * (1.f - xa[1]) * (      xa[2]) - h), off);
  curr.add(0, i[0], i[1]+1, i[2]+1, fnqx * (dx[0] * (      xa[1]) * (      xa[2]) + h), off);
}

template <typename Curr, typename real_t = typename Curr::real_t>
__host__ __device__
inline void curry(Curr& curr, const int i[3], real_t fnqy, const real_t dx[3], const real_t xa[3],
		  real_t h, const int off[3])
{
  curr.add(1, i[0]  ,i[1]  ,i[2]  , fnqy * (dx[1] * (1.f - xa[0]) * (1.f - xa[2]) + h), off);
  curr.add(1, i[0]+1,i[1]  ,i[2]  , fnqy * (dx[1] * (      xa[0]) * (1.f - xa[2]) - h), off);
  curr.add(1, i[0]  ,i[1]  ,i[2]+1, fnqy * (dx[1] * (1.f - xa[0]) * (      xa[2]) - h), off);
  curr.add(1, i[0]+1,i[1]  ,i[2]+1, fnqy * (dx[1] * (      xa[0]) * (      xa[2]) + h), off);
}
   
template <typename Curr, typename real_t = typename Curr::real_t>
__host__ __device__
inline void currz(Curr& curr, const int i[3], real_t fnqz, const real_t dx[3], const real_t xa[3],
		  real_t h, const int off[3])
{
  curr.add(2, i[0]  ,i[1]  ,i[2]  , fnqz * (dx[2] * (1.f - xa[0]) * (1.f - xa[1]) + h), off);
  curr.add(2, i[0]+1,i[1]  ,i[2]  , fnqz * (dx[2] * (      xa[0]) * (1.f - xa[1]) - h), off);
  curr.add(2, i[0]  ,i[1]+1,i[2]  , fnqz * (dx[2] * (1.f - xa[0]) * (      xa[1]) - h), off);
  curr.add(2, i[0]+1,i[1]+1,i[2]  , fnqz * (dx[2] * (      xa[0]) * (      xa[1]) + h), off);
}

#include "inc_curr_1vb_split.c"
#include "inc_curr_1vb_var1.c"
#include "inc_curr_1vb_2d.c"

template<typename _Order, typename _Dim, typename _fields_t>
struct Current1vb;


