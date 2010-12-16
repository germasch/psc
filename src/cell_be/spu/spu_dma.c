#include "../psc_cbe_common.h"
// #include "../../psc_particles_cbe.h"
#include "spu_particles.h"
#ifdef __SPU__
#include <spu_mfcio.h>
#else
#include "spu_mfcio_c.h"
#endif

#include <assert.h>


const vector unsigned char uplo_pat = 
  {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, // first word from first vec
   0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17}; // first word from second vec

const vector unsigned char uphi_pat = 
  {0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, // second word from first vec
   0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f}; // second word from second vec

const vector signed long long element_assign[2] = {{-1ll, 0}, {0, -1ll}};


enum {
  ST_NULL, 
  ST_LOADING, 
  ST_LOADED, 
  ST_STORING, 
  ST_STORED,
};

struct bl_info {
  int state; 
  int tagid; 
};

const unsigned int tag_pget = 1;
const unsigned int tag_pput = 2;

struct part_buffs buff; 

// Is this a hardware limit?
#define MAX_TAGID (32)


struct mfc_dma_el {
  unsigned int size;
  unsigned long eal;
};

static unsigned int tagmask;

// returns the first available tag, and sets
// that bit to 1 in the tagmask 
static int
get_tagid(void)
{
  for (int i = 0; i < MAX_TAGID; i++) {
    if (!(tagmask & (1 << i))) {
      tagmask |= (1 << i);
      return i;
    }
  }
  return -1;
}

// sets the tagid-th bit of tagmask 
// to 0
static void
put_tagid(int tagid)
{
  assert(tagid >= 0 && tagid < MAX_TAGID);
  assert(tagmask & (1 << tagid));
  tagmask &= ~(1 << tagid);
}

	

static void
wait_tagid(int tagid)
{
  assert(tagid >= 0 && tagid < MAX_TAGID);
  assert(tagmask & (1 << tagid));
  // Assign what tagid to look for 
  mfc_write_tag_mask(1 << tagid);
  // Wait until the previously indicated tagid
  // is done transfering. 
  unsigned int mask = mfc_read_tag_status_any();

  // the openggcm implementation has 
  // some other stuff here. I'm not 
  // sure it's needed for psc. 

}

void
spu_dma_get(volatile void *ls, unsigned long long ea, unsigned long size)
{
  // Check that we're on 16B boundaries, and 
  // the size of the struct we're bringing in is 
  // a multiple of 16B 
  //  fprintf(stderr, "size %d\n", size);
  assert(((unsigned long)ls & 15) == 0);
  assert((ea & 15) == 0);
  assert((size & 15) == 0);
  //fflush(stdout);
  //  fprintf(stderr,"dma_get %p %#llx %#lx\n", ls, ea, size);

  int tagid = get_tagid();
  assert(tagid >= 0);
  
  //  fprintf(stderr, " size %d \n", size);
  mfc_get(ls, ea, size, tagid, 0, 0);
  wait_tagid(tagid);
  put_tagid(tagid);
}


// I'm not quite sure how to integrate these functions, 
// of if they even need to be seperate functions. 
// They're here, but for now I'm leaving them commented out.

/*
void
part_dma_get(struct part_buffs *buff, unsigned long long np_ea){
  mfc_get(buff->lb1, np_ea, sizeof(2 * particle_cbe_t), 
	  tagp_get, 0, 0);
    // Breaking moved to end of loop
}

unsigned long long
part_dma_put(struct part_buffs *buff, unsigned long long cp_ea){
  unsigned long long np_ea = cp_ea + 2 * sizeof(struct particle_cbe_t);
  end = psc_block.part_end; 
  // rotate the buffers 
  unsigned long btmp1, btmp2;
  btmp1 = buff->sb1;
  btmp2 = buff->sb2;
  buff->sb1 = buff->lb1;
  buff->sb2 = buff->lb2;
  buff->lb1 = buff->plb1;
  buff->lb2 = buff->plb2;
  buff->plb1 = btmp1;
  buff->plb2 = btmp2;
  
  if(__builtin_expect((np_ea >= end),0)) {
    mfc_put(buff->sb1, cp_ea, (unsigned size_t) (end - cp_ea),
	    tagp_put, 0, 0);
  }
  else {
    mfc_put(buff->sb1, cp_ea, 2 * sizeof(struct particle_cbe_t),
	    tagp_put, 0, 0);
  }

  return np_ea;
}
*/
