#include "lib.h"

/** helper file for defining litmus tests */

/* addresses */

u64* var_va(litmus_test_run* data, const char* name) {
  return data->va[idx_from_varname(data->ctx, name)];
}

u64 var_pa(litmus_test_run* data, const char* name) {
  return data->pa[idx_from_varname(data->ctx, name)];
}

u64 var_page(litmus_test_run* data, const char* name) {
  return PAGE(data->va[idx_from_varname(data->ctx, name)]);
}

/* pointers to table entries */

u64* var_pte_level(litmus_test_run* data, const char* name, int level) {
  return data->tt_entries[idx_from_varname(data->ctx, name)][level];
}

u64* var_pgd(litmus_test_run* data, const char* name) {
  return var_pte_level(data, name, 0);
}

u64* var_pud(litmus_test_run* data, const char* name) {
  return var_pte_level(data, name, 1);
}

u64* var_pmd(litmus_test_run* data, const char* name) {
  return var_pte_level(data, name, 2);
}

u64* var_pte(litmus_test_run* data, const char* name) {
  return var_pte_level(data, name, 3);
}

/* initial table entry descriptors */

u64 var_desc_level(litmus_test_run* data, const char* name, int level) {
  return data->tt_descs[idx_from_varname(data->ctx, name)][level];
}

u64 var_desc(litmus_test_run* data, const char* name) {
  return var_ptedesc(data, name);
}

u64 var_ptedesc(litmus_test_run* data, const char* name) {
  return var_desc_level(data, name, 3);
}

u64 var_pmddesc(litmus_test_run* data, const char* name) {
  return var_desc_level(data, name, 2);
}

u64 var_puddesc(litmus_test_run* data, const char* name) {
  return var_desc_level(data, name, 1);
}

u64 var_pgddesc(litmus_test_run* data, const char* name) {
  return var_desc_level(data, name, 0);
}

/* output registers */

u64* out_reg(litmus_test_run* data, const char* name) {
  return data->out_reg[idx_from_regname(data->ctx, name)];
}

/** different levels */
char* pin_level_to_str(pin_level_t lvl) {
  switch (lvl) {
  case REGION_SAME_VAR_OFFSET:
    return "REGION_SAME_VAR";
  case REGION_SAME_CACHE_LINE_OFFSET:
    return "REGION_SAME_CACHE_LINE";
  case REGION_SAME_PAGE_OFFSET:
    return "REGION_SAME_PAGE";
  case REGION_SAME_PMD_OFFSET:
    return "REGION_SAME_PMD";
  case REGION_SAME_PUD_OFFSET:
    return "REGION_SAME_PUD";
  case REGION_SAME_PGD_OFFSET:
    return "REGION_SAME_PGD";
  default:
    return "<unknown pin_level_t>";
  }
}

char* init_type_to_str(init_type_t type) {
  switch (type) {
    case TYPE_HEAP:
      return "TYPE_HEAP";
    case TYPE_PTE:
      return "TYPE_PTE";
    case TYPE_FIX:
      return "TYPE_FIX";
    case TYPE_UNMAPPED:
      return "TYPE_UNMAPPED";
    case TYPE_IDENTITY_MAP:
      return "TYPE_IDENTITY_MAP";
    case TYPE_ALIAS:
      return "TYPE_ALIAS";
    case TYPE_ATTRS:
      return "TYPE_ATTRS";
    case TYPE_REGION_OWN:
      return "TYPE_REGION_OWN";
    case TYPE_REGION_PIN:
      return "TYPE_REGION_PIN";
    case TYPE_REGION_OFFSET:
      return "TYPE_REGION_OFFSET";
    case TYPE_MAIR:
      return "TYPE_MAIR";
    default:
      return "<unknown init_type_t>";
  }
}

char* prot_type_to_str(prot_type_t prot) {
  switch (prot) {
  case PROT_AP:
    return "AP";
  case PROT_ATTRIDX:
    return "AttrIdx";
  default:
    return "<unknown prot_type_t>";
  }
}

char* rel_offset_to_str(rel_offset_t lvl) {
  switch (lvl) {
  case REGION_SAME_VAR_OFFSET:
    return "REGION_SAME_VAR_OFFSET";
  case REGION_SAME_CACHE_LINE_OFFSET:
    return "REGION_SAME_CACHE_LINE_OFFSET";
  case REGION_SAME_PAGE_OFFSET:
    return "REGION_SAME_PAGE_OFFSET";
  case REGION_SAME_PMD_OFFSET:
    return "REGION_SAME_PMD_OFFSET";
  case REGION_SAME_PUD_OFFSET:
    return "REGION_SAME_PUD_OFFSET";
  case REGION_SAME_PGD_OFFSET:
    return "REGION_SAME_PGD_OFFSET";
  default:
    return "<unknown rel_offset_t>";
  }
}

char* own_level_to_str(own_level_t lvl) {
  switch (lvl) {
  case REGION_OWN_VAR:
    return "REGION_OWN_VAR";
  case REGION_OWN_CACHE_LINE:
    return "REGION_OWN_CACHE_LINE";
  case REGION_OWN_PAGE:
    return "REGION_OWN_PAGE";
  case REGION_OWN_PMD:
    return "REGION_OWN_PMD";
  case REGION_OWN_PUD:
    return "REGION_OWN_PUD";
  case REGION_OWN_PGD:
    return "REGION_OWN_PGD";
  default: {
    char* s = alloc(1024);
    sprintf(s, "<unknown own_level_t: %d>", lvl);
    return s;
  }
  }
}