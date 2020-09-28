#include "lib.h"

void read_init_unmapped(const litmus_test_t* cfg, var_info_t* infos, const char* varname);
void read_init_region(const litmus_test_t* cfg, var_info_t* infos, const char* varname, const char* pinned_var_name, pin_level_t pin_level);
void read_init_region_own(const litmus_test_t* cfg, var_info_t* infos, const char* varname, own_level_t region);
void read_init_region_offs(const litmus_test_t* cfg, var_info_t* infos, const char* varname, const char* offsvarname, rel_offset_t offs);
void read_init_alias(const litmus_test_t* cfg, var_info_t* infos, const char* varname, const char* aliasname);
void read_init_ap(const litmus_test_t* cfg, var_info_t* infos, const char* varname, uint64_t ap);
void read_init_pte(const litmus_test_t* cfg, var_info_t* infos, const char* varname, uint64_t pte);
void read_init_heap(const litmus_test_t* cfg, var_info_t* infos, const char* varname, uint64_t value);

void read_var_infos(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int no_runs) {
  for (var_idx_t v = 0; v < cfg->no_heap_vars; v++) {
    infos[v].varidx = v;
    infos[v].name = cfg->heap_var_names[v];
  }

  for (int i = 0; i < cfg->no_init_states; i++) {
    init_varstate_t* var = cfg->init_states[i];
    const char* name = var->varname;
    switch (var->type) {
      case (TYPE_HEAP):
        read_init_heap(cfg, infos, name, var->value);
        break;
      case (TYPE_PTE):
        read_init_pte(cfg, infos, name, var->value);
        break;
      case (TYPE_UNMAPPED):
        read_init_unmapped(cfg, infos, name);
        break;
      case (TYPE_ALIAS):
        read_init_alias(cfg, infos, name, var->aliasname);
        break;
      case (TYPE_AP):
        read_init_ap(cfg, infos, name, var->value);
        break;
      case (TYPE_REGION_PIN):
        read_init_region(cfg, infos, name, var->pinned_var_name, var->pinned_level);
        break;
      case (TYPE_REGION_OWN):
        read_init_region_own(cfg, infos, name, var->ownership_level);
        break;
      case (TYPE_REGION_OFFSET):
        read_init_region_offs(cfg, infos, name, var->offset_var_name, var->offset_level);
        break;
    }
  }

  /* fill defaults:
  * if a var does not specify a pinned offset from another var
   * then assume they're pinned to the page offset of the first var
   */
  var_info_t* first = NULL;
  for (var_idx_t v = 0; v < cfg->no_heap_vars; v++) {
    var_info_t* vinfo = &infos[v];
    if (vinfo->init_owns_region && !vinfo->init_pinned_region) {
      first = vinfo;
      break;
    } else if (! vinfo->init_pinned_region && ! vinfo->init_owns_region) {
      vinfo->init_owns_region = 1;
      vinfo->init_owned_region_size = REGION_OWN_PAGE;
      first = vinfo;
      break;
    }
  }

  if (first == NULL) {
    if (cfg->no_heap_vars == 0) {
      /* it's OK for a test to have no heap vars
       * e.g. for tests about relaxed registers and the like
       */
      return;
    } else {
      /* but if it has variables there must be at least 1 to be the 'root'
       * to attach others' PAGE bits to.
       */
      fail("! read_var_infos: default state for test '%s' has no owned variable to root to.\n", cfg->name);
    }
  }

  for (var_idx_t v = 0; v < cfg->no_heap_vars; v++) {
    var_info_t* vinfo = &infos[v];

    if (vinfo->varidx == first->varidx)
      continue;

    if ((! vinfo->init_pinned_region) && (! vinfo->init_owns_region)) {
      vinfo->init_owns_region = 1;
      vinfo->init_pinned_region = 0;
      vinfo->init_owned_region_size = REGION_OWN_PAGE;

      /* if the var DID have a INIT_REGION_OFFSET then keep that
       */
      if (! vinfo->init_region_offset) {
        vinfo->init_region_offset = 1;
        vinfo->offset_var = first->varidx;
        vinfo->offset_level = REGION_SAME_PAGE_OFFSET;
      }
    }
  }

  for (var_idx_t v = 0; v < cfg->no_heap_vars; v++) {
    var_info_t* vinfo = &infos[v];
    debug("varinfo[%d] = {\n", v);
    debug(" .name=\"%s\"\n", vinfo->name);
    debug(" .init_value=%p\n", vinfo->init_value);
    debug(" .init_ap=%p\n", vinfo->init_ap);
    debug(" .init_unmapped=%p\n", vinfo->init_unmapped);
    debug(" .init_owns_region=%d\n", vinfo->init_owns_region);

    if (vinfo->init_owns_region) {
      debug(" .init_owned_region_size=%s\n", own_level_to_str(vinfo->init_owned_region_size));
      debug(" .init_pinned_region=%d\n", vinfo->init_pinned_region);
    }

    if (vinfo->init_pinned_region) {
      debug(" .pin_region_var=\"%s\"\n", infos[vinfo->pin_region_var].name);
      debug(" .pin_region_level=%s\n", pin_level_to_str(vinfo->pin_region_level));
    }

    debug(" .init_region_offset=%d\n", vinfo->init_region_offset);
    if (vinfo->init_region_offset) {
      debug(" .offset_var=%s\n", infos[vinfo->offset_var].name);
      debug(" .offset_level=\"%s\"\n", rel_offset_to_str(vinfo->offset_level));
    }
    debug(" .alias=\"%s\"\n", infos[vinfo->alias].name);
    debug("}\n");
  }
}

void read_init_unmapped(const litmus_test_t* cfg, var_info_t* infos, const char* varname) {
  var_idx_t idx = idx_from_varname_infos(cfg, infos, varname);
  infos[idx].init_unmapped = 1;
}

void read_init_region_offs(const litmus_test_t* cfg, var_info_t* infos, const char* varname, const char* offsvarname, rel_offset_t offs) {
  var_idx_t idx = idx_from_varname_infos(cfg, infos, varname);
  var_idx_t offsvaridx = idx_from_varname_infos(cfg, infos, offsvarname);

  infos[idx].init_region_offset = 1;
  infos[idx].offset_var = offsvaridx;
  infos[idx].offset_level = offs;
}

void read_init_region_own(const litmus_test_t* cfg, var_info_t* infos, const char* varname, own_level_t region) {
  var_idx_t idx = idx_from_varname_infos(cfg, infos, varname);
  infos[idx].init_owns_region = 1;
  infos[idx].init_owned_region_size = region;
}

void read_init_region(const litmus_test_t* cfg, var_info_t* infos, const char* varname, const char* pinned_var_name, pin_level_t pin_level) {
  var_idx_t idx = idx_from_varname_infos(cfg, infos, varname);

  infos[idx].init_pinned_region = 1;
  infos[idx].pin_region_var = idx_from_varname_infos(cfg, infos, pinned_var_name);
  infos[idx].pin_region_level = pin_level;
}

void read_init_alias(const litmus_test_t* cfg, var_info_t* infos, const char* varname, const char* aliasname) {
  var_idx_t idx = idx_from_varname_infos(cfg, infos, varname);
  infos[idx].is_alias = 1;
  infos[idx].alias = idx_from_varname_infos(cfg, infos, aliasname);
}

void read_init_ap(const litmus_test_t* cfg, var_info_t* infos, const char* varname, uint64_t ap) {
  var_idx_t idx = idx_from_varname_infos(cfg, infos, varname);
  infos[idx].init_ap = ap;
}

void read_init_pte(const litmus_test_t* cfg, var_info_t* infos, const char* varname, uint64_t desc) {
  fail("INIT_PTE unsupported, use INIT_UNMAPPED or INIT_AP\n");
}

void read_init_heap(const litmus_test_t* cfg, var_info_t* infos, const char* varname, uint64_t value) {
  var_idx_t idx = idx_from_varname_infos(cfg, infos, varname);
  infos[idx].init_value = value;
}