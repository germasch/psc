
#include "psc_push_fields_private.h"
#include "psc_fields_as_single.h"

#include "psc_push_fields_common.c"

// ======================================================================
// psc_push_fields: subclass "single"

struct psc_push_fields_ops psc_push_fields_single_ops = {
  .name                  = "single",
  .push_mflds_E          = psc_push_fields_sub_push_mflds_E,
  .push_mflds_H          = psc_push_fields_sub_push_mflds_H,
};
