
#ifndef GGCM_MHD_COMMU_H
#define GGCM_MHD_COMMU_H

#include <mrc_obj.h>

MRC_CLASS_DECLARE(ggcm_mhd_commu, struct ggcm_mhd_commu);

void ggcm_mhd_commu_run(struct ggcm_mhd_commu *ggcm_mhd_commu, int mb, int me);

#endif
