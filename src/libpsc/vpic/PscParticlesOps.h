
#ifndef PSC_PARTICLES_OPS
#define PSC_PARTICLES_OPS

#define HAS_V4_PIPELINE

template<class P, class IA>
struct PscParticlesOps {
  typedef P Particles;
  typedef IA Interpolator;
  
  PscParticlesOps(vpic_simulation *simulation) : simulation_(simulation) { }

  void inject_particle(Particles *vmprts, int patch, const struct psc_particle_inject *prt)
  {
    assert(patch == 0);
    assert(simulation_->accumulator_array);
    species_t *sp = &*vmprts->find_id(prt->kind);
    assert(sp);

    double x = prt->x[0], y = prt->x[1], z = prt->x[2];
    double ux = prt->u[0], uy = prt->u[1], uz = prt->u[2];
    double w = prt->w, age = 0.;
    int update_rhob = 0;

    int ix, iy, iz;

    grid_t *grid = sp->g;
    const double x0 = (double)grid->x0, y0 = (double)grid->y0, z0 = (double)grid->z0;
    const double x1 = (double)grid->x1, y1 = (double)grid->y1, z1 = (double)grid->z1;
    const int    nx = grid->nx,         ny = grid->ny,         nz = grid->nz;
    
    // Do not inject if the particle is strictly outside the local domain
    // or if a far wall of local domain shared with a neighbor
    
    if ((x<x0) | (x>x1) | ( (x==x1) & (grid->bc[BOUNDARY(1,0,0)]>=0))) return;
    if ((y<y0) | (y>y1) | ( (y==y1) & (grid->bc[BOUNDARY(0,1,0)]>=0))) return;
    if ((z<z0) | (z>z1) | ( (z==z1) & (grid->bc[BOUNDARY(0,0,1)]>=0))) return;
    
    // This node should inject the particle
    
    if (sp->np>=sp->max_np) ERROR(( "No room to inject particle" ));
    
    // Compute the injection cell and coordinate in cell coordinate system
    // BJA:  Note the use of double precision here for accurate particle 
    //       placement on large meshes. 
 
    // The ifs allow for injection on the far walls of the local computational
    // domain when necessary
    
    x  = ((double)nx)*((x-x0)/(x1-x0)); // x is rigorously on [0,nx]
    ix = (int)x;                        // ix is rigorously on [0,nx]
    x -= (double)ix;                    // x is rigorously on [0,1)
    x  = (x+x)-1;                       // x is rigorously on [-1,1)
    if( ix==nx ) x = 1;                 // On far wall ... conditional move
    if( ix==nx ) ix = nx-1;             // On far wall ... conditional move
    ix++;                               // Adjust for mesh indexing

    y  = ((double)ny)*((y-y0)/(y1-y0)); // y is rigorously on [0,ny]
    iy = (int)y;                        // iy is rigorously on [0,ny]
    y -= (double)iy;                    // y is rigorously on [0,1)
    y  = (y+y)-1;                       // y is rigorously on [-1,1)
    if( iy==ny ) y = 1;                 // On far wall ... conditional move
    if( iy==ny ) iy = ny-1;             // On far wall ... conditional move
    iy++;                               // Adjust for mesh indexing

    z  = ((double)nz)*((z-z0)/(z1-z0)); // z is rigorously on [0,nz]
    iz = (int)z;                        // iz is rigorously on [0,nz]
    z -= (double)iz;                    // z is rigorously on [0,1)
    z  = (z+z)-1;                       // z is rigorously on [-1,1)
    if( iz==nz ) z = 1;                 // On far wall ... conditional move
    if( iz==nz ) iz = nz-1;             // On far wall ... conditional move
    iz++;                               // Adjust for mesh indexing

    particle_t * p = sp->p + (sp->np++);
    p->dx = (float)x; // Note: Might be rounded to be on [-1,1]
    p->dy = (float)y; // Note: Might be rounded to be on [-1,1]
    p->dz = (float)z; // Note: Might be rounded to be on [-1,1]
    p->i  = VOXEL(ix,iy,iz, nx,ny,nz);
    p->ux = (float)ux;
    p->uy = (float)uy;
    p->uz = (float)uz;
    p->w  = w;

    if (update_rhob) accumulate_rhob( simulation_->field_array->f, p, grid, -sp->q );

    if (age!=0) {
      if( sp->nm >= sp->max_nm )
	WARNING(( "No movers available to age injected  particle" ));
      particle_mover_t * pm = sp->pm + sp->nm;
      age *= grid->cvac*grid->dt/sqrt( ux*ux + uy*uy + uz*uz + 1 );
      pm->dispx = ux*age*grid->rdx;
      pm->dispy = uy*age*grid->rdy;
      pm->dispz = uz*age*grid->rdz;
      pm->i     = sp->np-1;
      sp->nm += move_p( sp->p, pm, simulation_->accumulator_array->a, grid, sp->q );
    }
    
  }
  
  // ----------------------------------------------------------------------
  // advance_p

  typedef struct particle_mover_seg {
    int nm;                             // Number of movers used
    int n_ignored;                      // Number of movers ignored
  } particle_mover_seg_t;

  void
  advance_p_pipeline(/**/  species_t            * RESTRICT sp,
		     accumulator_t * ALIGNED(128) a0,
		     Interpolator& interpolator,
		     particle_mover_seg_t *seg,
		     particle_t * ALIGNED(128) p, int n,
		     particle_mover_t * ALIGNED(16) pm, int max_nm)
  {
    particle_t           * ALIGNED(128) p0 = sp->p;
    const grid_t *                      g  = sp->g;

    const interpolator_t * ALIGNED(16)  f;
    float                * ALIGNED(16)  a;

    const float qdt_2mc  = (sp->q*sp->g->dt)/(2*sp->m*sp->g->cvac);
    const float cdt_dx   = sp->g->cvac*sp->g->dt*sp->g->rdx;
    const float cdt_dy   = sp->g->cvac*sp->g->dt*sp->g->rdy;
    const float cdt_dz   = sp->g->cvac*sp->g->dt*sp->g->rdz;
    const float qsp      = sp->q;

    const float one            = 1.;
    const float one_third      = 1./3.;
    const float two_fifteenths = 2./15.;

    float dx, dy, dz, ux, uy, uz, q;
    float hax, hay, haz, cbx, cby, cbz;
    float v0, v1, v2, v3, v4, v5;

    int ii;
  
    DECLARE_ALIGNED_ARRAY( particle_mover_t, 16, local_pm, 1 );

    int nm = 0;
    int n_ignored = 0;

    // Process particles for this pipeline

    for(;n;n--,p++) {
      dx   = p->dx;                             // Load position
      dy   = p->dy;
      dz   = p->dz;
      ii   = p->i;
      f    = &interpolator[ii];                 // Interpolate E
      hax  = qdt_2mc*(    ( f->ex    + dy*f->dexdy    ) +
			  dz*( f->dexdz + dy*f->d2exdydz ) );
      hay  = qdt_2mc*(    ( f->ey    + dz*f->deydz    ) +
			  dx*( f->deydx + dz*f->d2eydzdx ) );
      haz  = qdt_2mc*(    ( f->ez    + dx*f->dezdx    ) +
			  dy*( f->dezdy + dx*f->d2ezdxdy ) );
      cbx  = f->cbx + dx*f->dcbxdx;             // Interpolate B
      cby  = f->cby + dy*f->dcbydy;
      cbz  = f->cbz + dz*f->dcbzdz;
      ux   = p->ux;                             // Load momentum
      uy   = p->uy;
      uz   = p->uz;
      q    = p->w;
      ux  += hax;                               // Half advance E
      uy  += hay;
      uz  += haz;
      v0   = qdt_2mc/sqrtf(one + (ux*ux + (uy*uy + uz*uz)));
      /**/                                      // Boris - scalars
      v1   = cbx*cbx + (cby*cby + cbz*cbz);
      v2   = (v0*v0)*v1;
      v3   = v0*(one+v2*(one_third+v2*two_fifteenths));
      v4   = v3/(one+v1*(v3*v3));
      v4  += v4;
      v0   = ux + v3*( uy*cbz - uz*cby );       // Boris - uprime
      v1   = uy + v3*( uz*cbx - ux*cbz );
      v2   = uz + v3*( ux*cby - uy*cbx );
      ux  += v4*( v1*cbz - v2*cby );            // Boris - rotation
      uy  += v4*( v2*cbx - v0*cbz );
      uz  += v4*( v0*cby - v1*cbx );
      ux  += hax;                               // Half advance E
      uy  += hay;
      uz  += haz;
      p->ux = ux;                               // Store momentum
      p->uy = uy;
      p->uz = uz;
      v0   = one/sqrtf(one + (ux*ux+ (uy*uy + uz*uz)));
      /**/                                      // Get norm displacement
      ux  *= cdt_dx;
      uy  *= cdt_dy;
      uz  *= cdt_dz;
      ux  *= v0;
      uy  *= v0;
      uz  *= v0;
      v0   = dx + ux;                           // Streak midpoint (inbnds)
      v1   = dy + uy;
      v2   = dz + uz;
      v3   = v0 + ux;                           // New position
      v4   = v1 + uy;
      v5   = v2 + uz;

      // FIXME-KJB: COULD SHORT CIRCUIT ACCUMULATION IN THE CASE WHERE QSP==0!
      if(  v3<=one &&  v4<=one &&  v5<=one &&   // Check if inbnds
	   -v3<=one && -v4<=one && -v5<=one ) {

	// Common case (inbnds).  Note: accumulator values are 4 times
	// the total physical charge that passed through the appropriate
	// current quadrant in a time-step

	q *= qsp;
	p->dx = v3;                             // Store new position
	p->dy = v4;
	p->dz = v5;
	dx = v0;                                // Streak midpoint
	dy = v1;
	dz = v2;
	v5 = q*ux*uy*uz*one_third;              // Compute correction
	a  = (float *)( a0 + ii );              // Get accumulator

#     define ACCUMULATE_J(X,Y,Z,offset)                                 \
	v4  = q*u##X;   /* v2 = q ux                            */	\
	v1  = v4*d##Y;  /* v1 = q ux dy                         */	\
	v0  = v4-v1;    /* v0 = q ux (1-dy)                     */	\
	v1 += v4;       /* v1 = q ux (1+dy)                     */	\
	v4  = one+d##Z; /* v4 = 1+dz                            */	\
	v2  = v0*v4;    /* v2 = q ux (1-dy)(1+dz)               */	\
	v3  = v1*v4;    /* v3 = q ux (1+dy)(1+dz)               */	\
	v4  = one-d##Z; /* v4 = 1-dz                            */	\
	v0 *= v4;       /* v0 = q ux (1-dy)(1-dz)               */	\
	v1 *= v4;       /* v1 = q ux (1+dy)(1-dz)               */	\
	v0 += v5;       /* v0 = q ux [ (1-dy)(1-dz) + uy*uz/3 ] */	\
	v1 -= v5;       /* v1 = q ux [ (1+dy)(1-dz) - uy*uz/3 ] */	\
	v2 -= v5;       /* v2 = q ux [ (1-dy)(1+dz) - uy*uz/3 ] */	\
	v3 += v5;       /* v3 = q ux [ (1+dy)(1+dz) + uy*uz/3 ] */	\
	a[offset+0] += v0;						\
	a[offset+1] += v1;						\
	a[offset+2] += v2;						\
	a[offset+3] += v3

	ACCUMULATE_J( x,y,z, 0 );
	ACCUMULATE_J( y,z,x, 4 );
	ACCUMULATE_J( z,x,y, 8 );

#     undef ACCUMULATE_J

      } else {                                    // Unlikely
	local_pm->dispx = ux;
	local_pm->dispy = uy;
	local_pm->dispz = uz;
	local_pm->i     = p - p0;

	if( move_p( p0, local_pm, a0, g, qsp ) ) { // Unlikely
	  if( nm<max_nm ) {
	    pm[nm++] = local_pm[0];
	  }
	  else {
	    n_ignored++;                 // Unlikely
	  } // if
	} // if
      }

    }

    seg->nm        = nm;
    seg->n_ignored = n_ignored;
  }

#if defined(V4_ACCELERATION) && defined(HAS_V4_PIPELINE)

  void
  advance_p_pipeline_v4(/**/  species_t            * RESTRICT sp,
			accumulator_t * ALIGNED(128) a0,
			Interpolator& interpolator,
			particle_mover_seg_t *seg,
			particle_t * ALIGNED(128) p, int n,
			particle_mover_t * ALIGNED(16) pm, int max_nm)
  {
    using namespace v4;

    particle_t           * ALIGNED(128) p0 = sp->p;
    const grid_t *                      g  = sp->g;

    float                * ALIGNED(16)  vp0;
    float                * ALIGNED(16)  vp1;
    float                * ALIGNED(16)  vp2;
    float                * ALIGNED(16)  vp3;

    const v4float qdt_2mc  = (sp->q*sp->g->dt)/(2*sp->m*sp->g->cvac);
    const v4float cdt_dx   = sp->g->cvac*sp->g->dt*sp->g->rdx;
    const v4float cdt_dy   = sp->g->cvac*sp->g->dt*sp->g->rdy;
    const v4float cdt_dz   = sp->g->cvac*sp->g->dt*sp->g->rdz;
    const v4float qsp      = sp->q;

    const v4float one = 1.;
    const v4float one_third = 1./3.;
    const v4float two_fifteenths = 2./15.;
    const v4float neg_one = -1.;

    const float _qsp = sp->q;

    v4float dx, dy, dz, ux, uy, uz, q;
    v4float hax, hay, haz, cbx, cby, cbz;
    v4float v0, v1, v2, v3, v4, v5;
    v4int   ii, outbnd;

    DECLARE_ALIGNED_ARRAY( particle_mover_t, 16, local_pm, 1 );

    int nq = n >> 2;
  
    int nm   = 0;
    int n_ignored = 0;

    // Process the particle quads for this pipeline

    for( ; nq; nq--, p+=4 ) {
      load_4x4_tr(&p[0].dx,&p[1].dx,&p[2].dx,&p[3].dx,dx,dy,dz,ii);

      // Interpolate fields
      vp0 = (float * ALIGNED(16)) (&interpolator[ii(0)]);
      vp1 = (float * ALIGNED(16)) (&interpolator[ii(1)]);
      vp2 = (float * ALIGNED(16)) (&interpolator[ii(2)]);
      vp3 = (float * ALIGNED(16)) (&interpolator[ii(3)]);
      load_4x4_tr(vp0,  vp1,  vp2,  vp3,  hax,v0,v1,v2); hax = qdt_2mc*fma( fma( v2, dy, v1 ), dz, fma( v0, dy, hax ) );
      load_4x4_tr(vp0+4,vp1+4,vp2+4,vp3+4,hay,v3,v4,v5); hay = qdt_2mc*fma( fma( v5, dz, v4 ), dx, fma( v3, dz, hay ) );
      load_4x4_tr(vp0+8,vp1+8,vp2+8,vp3+8,haz,v0,v1,v2); haz = qdt_2mc*fma( fma( v2, dx, v1 ), dy, fma( v0, dx, haz ) );
      load_4x4_tr(vp0+12,vp1+12,vp2+12,vp3+12,cbx,v3,cby,v4); cbx = fma( v3, dx, cbx );
      /**/                                                    cby = fma( v4, dy, cby );
      load_4x2_tr(vp0+16,vp1+16,vp2+16,vp3+16,cbz,v5);        cbz = fma( v5, dz, cbz );

      // Update momentum
      // If you are willing to eat a 5-10% performance hit,
      // v0 = qdt_2mc/sqrt(blah) is a few ulps more accurate (but still
      // quite in the noise numerically) for cyclotron frequencies
      // approaching the nyquist frequency.

      load_4x4_tr(&p[0].ux,&p[1].ux,&p[2].ux,&p[3].ux,ux,uy,uz,q);
      ux += hax;
      uy += hay;
      uz += haz;
      v0  = qdt_2mc*rsqrt( one + fma( ux,ux, fma( uy,uy, uz*uz ) ) );
      v1  = fma( cbx,cbx, fma( cby,cby, cbz*cbz ) );
      v2  = (v0*v0)*v1;
      v3  = v0*fma( fma( two_fifteenths, v2, one_third ), v2, one );
      v4  = v3*rcp(fma( v3*v3, v1, one ));
      v4 += v4;
      v0  = fma( fms( uy,cbz, uz*cby ), v3, ux );
      v1  = fma( fms( uz,cbx, ux*cbz ), v3, uy );
      v2  = fma( fms( ux,cby, uy*cbx ), v3, uz );
      ux  = fma( fms( v1,cbz, v2*cby ), v4, ux );
      uy  = fma( fms( v2,cbx, v0*cbz ), v4, uy );
      uz  = fma( fms( v0,cby, v1*cbx ), v4, uz );
      ux += hax;
      uy += hay;
      uz += haz;
      store_4x4_tr(ux,uy,uz,q,&p[0].ux,&p[1].ux,&p[2].ux,&p[3].ux);
    
      // Update the position of inbnd particles
      v0  = rsqrt( one + fma( ux,ux, fma( uy,uy, uz*uz ) ) );
      ux *= cdt_dx;
      uy *= cdt_dy;
      uz *= cdt_dz;
      ux *= v0;
      uy *= v0;
      uz *= v0;      // ux,uy,uz are normalized displ (relative to cell size)
      v0  = dx + ux;
      v1  = dy + uy;
      v2  = dz + uz; // New particle midpoint
      v3  = v0 + ux;
      v4  = v1 + uy;
      v5  = v2 + uz; // New particle position
      outbnd = (v3>one) | (v3<neg_one) |
	(v4>one) | (v4<neg_one) |
	(v5>one) | (v5<neg_one);
      v3  = merge(outbnd,dx,v3); // Do not update outbnd particles
      v4  = merge(outbnd,dy,v4);
      v5  = merge(outbnd,dz,v5);
      store_4x4_tr(v3,v4,v5,ii,&p[0].dx,&p[1].dx,&p[2].dx,&p[3].dx);
    
      // Accumulate current of inbnd particles
      // Note: accumulator values are 4 times the total physical charge that
      // passed through the appropriate current quadrant in a time-step
      q  = czero(outbnd,q*qsp);       // Do not accumulate outbnd particles
      dx = v0;                       // Streak midpoint (valid for inbnd only)
      dy = v1;
      dz = v2;
      v5 = q*ux*uy*uz*one_third;     // Charge conservation correction
      vp0 = (float * ALIGNED(16))(a0 + ii(0)); // Accumulator pointers
      vp1 = (float * ALIGNED(16))(a0 + ii(1));
      vp2 = (float * ALIGNED(16))(a0 + ii(2));
      vp3 = (float * ALIGNED(16))(a0 + ii(3));

#   define ACCUMULATE_J(X,Y,Z,offset)					\
      v4  = q*u##X;   /* v4 = q ux                            */	\
      v1  = v4*d##Y;  /* v1 = q ux dy                         */	\
      v0  = v4-v1;    /* v0 = q ux (1-dy)                     */	\
      v1 += v4;       /* v1 = q ux (1+dy)                     */	\
      v4  = one+d##Z; /* v4 = 1+dz                            */	\
      v2  = v0*v4;    /* v2 = q ux (1-dy)(1+dz)               */	\
      v3  = v1*v4;    /* v3 = q ux (1+dy)(1+dz)               */	\
      v4  = one-d##Z; /* v4 = 1-dz                            */	\
      v0 *= v4;       /* v0 = q ux (1-dy)(1-dz)               */	\
      v1 *= v4;       /* v1 = q ux (1+dy)(1-dz)               */	\
      v0 += v5;       /* v0 = q ux [ (1-dy)(1-dz) + uy*uz/3 ] */	\
      v1 -= v5;       /* v1 = q ux [ (1+dy)(1-dz) - uy*uz/3 ] */	\
      v2 -= v5;       /* v2 = q ux [ (1-dy)(1+dz) - uy*uz/3 ] */	\
      v3 += v5;       /* v3 = q ux [ (1+dy)(1+dz) + uy*uz/3 ] */	\
      transpose(v0,v1,v2,v3);						\
      increment_4x1(vp0+offset,v0);					\
      increment_4x1(vp1+offset,v1);					\
      increment_4x1(vp2+offset,v2);					\
      increment_4x1(vp3+offset,v3)

      ACCUMULATE_J( x,y,z, 0 );
      ACCUMULATE_J( y,z,x, 4 );
      ACCUMULATE_J( z,x,y, 8 );

#   undef ACCUMULATE_J

      // Update position and accumulate outbnd

#   define MOVE_OUTBND(N)                                               \
      if( outbnd(N) ) {                       /* Unlikely */		\
      local_pm->dispx = ux(N);                                          \
      local_pm->dispy = uy(N);                                          \
      local_pm->dispz = uz(N);                                          \
      local_pm->i     = (p - p0) + N;                                   \
      if( move_p( p0, local_pm, a0, g, _qsp ) ) { /* Unlikely */        \
      if( nm<max_nm ) copy_4x1( &pm[nm++], local_pm );			\
      else            n_ignored++;             /* Unlikely */		\
    }									\
    }

      MOVE_OUTBND(0);
      MOVE_OUTBND(1);
      MOVE_OUTBND(2);
      MOVE_OUTBND(3);

#   undef MOVE_OUTBND

    }

    seg->nm        = nm;
    seg->n_ignored = n_ignored;
  }

#endif
          
  void
  advance_p(/**/  species_t            * RESTRICT sp,
	    /**/  accumulator_array_t  * RESTRICT aa,
	    Interpolator& interpolator)
  {
    DECLARE_ALIGNED_ARRAY( particle_mover_seg_t, 128, seg, 1 );

    sp->nm = 0;

    particle_t *p = sp->p;
    int n = sp->np & ~15;
#if defined(V4_ACCELERATION) && defined(HAS_V4_PIPELINE)
    advance_p_pipeline_v4(sp, aa->a + aa->stride, interpolator, seg, p, n,
			  sp->pm + sp->nm, sp->max_nm - sp->nm);
#else
    advance_p_pipeline(sp, aa->a + aa->stride, interpolator, seg, p, n,
		       sp->pm + sp->nm, sp->max_nm - sp->nm);
#endif
    sp->nm += seg->nm;

    if (seg->n_ignored)
      WARNING(( "Pipeline %i ran out of storage for %i movers",
		0, seg->n_ignored ));
  
    p += n;
    n = sp->np - n;
    advance_p_pipeline(sp, aa->a, interpolator, seg, p, n,
		       sp->pm + sp->nm, sp->max_nm - sp->nm);
    sp->nm += seg->nm;

    if (seg->n_ignored)
      WARNING(( "Pipeline %i ran out of storage for %i movers",
		1, seg->n_ignored ));
  }


  void advance_p(Particles *vmprts, accumulator_array_t *accumulator_array,
		 Interpolator& interpolator_array)
  {
    for (typename Particles::Iter sp = vmprts->begin(); sp != vmprts->end(); ++sp) {
      TIC advance_p(&*sp, accumulator_array, interpolator_array); TOC(advance_p, 1);
    }
  }
  
private:
  vpic_simulation *simulation_;
};

#endif

