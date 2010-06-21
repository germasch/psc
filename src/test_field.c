
#include "psc.h"
#include "util/profile.h"
#include "util/params.h"

#include <stdio.h>
#include <math.h>
#include <mpi.h>

static void
setup_fields()
{
  for (int iz = psc.ilg[2]; iz < psc.ihg[2]; iz++) {
    for (int iy = psc.ilg[1]; iy < psc.ihg[1]; iy++) {
      for (int ix = psc.ilg[0]; ix < psc.ihg[0]; ix++) {
	f_real xx = 2.*M_PI * ix / psc.domain.itot[0];
	f_real zz = 2.*M_PI * iz / psc.domain.itot[2];
	FF3(JXI, ix,iy,iz) = cos(xx) * sin(zz);
	FF3(JYI, ix,iy,iz) = sin(xx) * sin(zz);
	FF3(JZI, ix,iy,iz) = cos(xx) * cos(zz);
	FF3(EX, ix,iy,iz) = 0.;
	FF3(EY, ix,iy,iz) = 0.;
	FF3(EZ, ix,iy,iz) = 0.;
	FF3(BX, ix,iy,iz) = 0.;
	FF3(BY, ix,iy,iz) = 0.;
	FF3(BZ, ix,iy,iz) = 0.;
      }
    }
  }
}

static void
dump_fld(int m, const char *pfx)
{
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  char fname[100];
  sprintf(fname, "%s-p%d.h5", pfx, rank);
  psc_dump_field(m, fname);
}

int
main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  params_init(argc, argv);

  struct psc_mod_config conf_fortran = {
    .mod_field = "fortran",
  };
  struct psc_mod_config conf_c = {
    .mod_field = "c",
  };

  psc_create_test_xz(&conf_fortran);
  setup_fields();
  dump_fld(EX, "ex0");
  psc_push_field_a();
  dump_fld(EX, "ex1");
  psc_save_fields_ref();
  psc_destroy();

  psc_create_test_xz(&conf_c);
  setup_fields();
  psc_push_field_a();
  dump_fld(EX, "ex2");
  dump_fld(EY, "ey2");
  dump_fld(EZ, "ez2");
  dump_fld(BX, "bx2");
  dump_fld(BY, "by2");
  dump_fld(BZ, "bz2");
  psc_check_fields_ref((int []) { EX, EY, EZ, BX, BY, BZ, -1 }, 1e-7);
  psc_destroy();

  prof_print();

  MPI_Finalize();
}
