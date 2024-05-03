#include "lib.h"

void reset_var_info(var_info_t* vinfo, var_idx_t idx, const char* varname);
void read_init_unmapped(const litmus_test_t* cfg, var_info_t* infos, const char* varname);
void read_init_region_pin(
  const litmus_test_t* cfg, var_info_t* infos, const char* varname, const char* pinned_var_name, pin_level_t pin_level
);
void read_init_region_own(const litmus_test_t* cfg, var_info_t* infos, const char* varname, own_level_t region);
void read_init_region_offs(
  const litmus_test_t* cfg, var_info_t* infos, const char* varname, const char* offsvarname, rel_offset_t offs
);
void read_init_alias(const litmus_test_t* cfg, var_info_t* infos, const char* varname, const char* aliasname);
void read_init_ap(const litmus_test_t* cfg, var_info_t* infos, const char* varname, prot_type_t prot_type, u64 attr);
void read_init_pte(const litmus_test_t* cfg, var_info_t* infos, const char* varname, u64 pte);
void read_init_fix(const litmus_test_t* cfg, var_info_t* infos, const char* varname, u64 pa);
void read_init_heap(const litmus_test_t* cfg, var_info_t* infos, const char* varname, u64 value);
void read_init_mair(init_system_state_t* st, u64 mair_attr7);
void read_init_idmap(const litmus_test_t* cfg, var_info_t* infos, const char* varname);

void read_var_infos(const litmus_test_t* cfg, init_system_state_t* sys_st, var_info_t* infos, int no_runs) {
  for (var_idx_t v = 0; v < cfg->no_heap_vars; v++) {
    reset_var_info(&infos[v], v, cfg->heap_var_names[v]);
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
    case (TYPE_FIX):
      read_init_fix(cfg, infos, name, var->value);
      break;
    case (TYPE_UNMAPPED):
      read_init_unmapped(cfg, infos, name);
      break;
    case (TYPE_IDENTITY_MAP):
      read_init_idmap(cfg, infos, name);
      break;
    case (TYPE_ALIAS):
      read_init_alias(cfg, infos, name, var->aliasname);
      break;
    case (TYPE_ATTRS):
      read_init_ap(cfg, infos, name, var->prot_type, var->attr_value);
      break;
    case (TYPE_MAIR):
      read_init_mair(sys_st, var->value);
      break;
    case (TYPE_REGION_PIN):
      read_init_region_pin(cfg, infos, name, var->pinned_var_name, var->pinned_level);
      break;
    case (TYPE_REGION_OWN):
      read_init_region_own(cfg, infos, name, var->ownership_level);
      break;
    case (TYPE_REGION_OFFSET):
      read_init_region_offs(cfg, infos, name, var->offset_var_name, var->offset_level);
      break;
    }
  }

  /* check that at least one of the symbolic variables owns some memory
   * otherwise the constraints are just cyclic and we can't concretize
   */
  var_info_t* first = NULL;
  for (var_idx_t v = 0; v < cfg->no_heap_vars; v++) {
    var_info_t* vinfo = &infos[v];
    if (
         (var_owns_region(vinfo) && !vinfo->init_attrs.has_region_offset)
      /* we allow NOTSET, purely for unit testing purposes.
       * Trying to run a real litmus test with a NOTSET variable will cause an error. */
      || (vinfo->ty == VAR_NOTSET && !vinfo->init_attrs.has_region_offset)
    ) {
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
      fail("! read_var_infos: default state for test '%s' has no owned variables to root to.\n", cfg->name);
    }
  }

  /* for all variables without an explicit offset,
   * and which aren't aliased or pinned to another,
   * make them offset from the primary var
   * at the same page offset
   */
  for (var_idx_t v = 0; v < cfg->no_heap_vars; v++) {
    var_info_t* vinfo = &infos[v];

    if (vinfo->varidx == first->varidx)
      continue;

    if (!var_owns_region(vinfo))
      continue;

    if (vinfo->ty == VAR_ALIAS)
      continue;

    if (vinfo->init_attrs.has_region_offset)
      continue;

    vinfo->init_attrs.has_region_offset = 1;
    vinfo->init_attrs.region_offset.offset_var = first->varidx;
    vinfo->init_attrs.region_offset.offset_level = REGION_SAME_PAGE_OFFSET;
  }

  /* all aliased variables must be at the same offset as the parent */
  for (var_idx_t v = 0; v < cfg->no_heap_vars; v++) {
    var_info_t* vinfo = &infos[v];

    if (vinfo->varidx == first->varidx)
      continue;

    if (vinfo->ty != VAR_ALIAS)
      continue;

    fail_on(vinfo->init_attrs.has_region_offset, "alias variable already has an offset");

    vinfo->init_attrs.has_region_offset = 1;
    vinfo->init_attrs.region_offset.offset_var = vinfo->alias.aliased_with;
    /* TODO: BS: for now alias vars are only aliased in the first page */
    vinfo->init_attrs.region_offset.offset_level = REGION_SAME_PAGE_OFFSET;
  }

  /* for debugging, print all of the vars out */
  for (var_idx_t v = 0; v < cfg->no_heap_vars; v++) {
    var_info_t* vinfo = &infos[v];
    debug("varinfo[%d] = ", v);
    debug_print_var_info(vinfo, infos);
  }
}

void reset_var_info(var_info_t* vinfo, var_idx_t idx, const char* name) {
  vinfo->varidx = idx;
  vinfo->name = name;

  /* generic attrs */
  vinfo->init_attrs.has_ap = 0;
  vinfo->init_attrs.has_attridx = 0;
  vinfo->init_attrs.has_region_offset = 0;

  /* type = UNSET */
  vinfo->ty = VAR_NOTSET;
}

void read_init_mair(init_system_state_t* st, u64 mair_attr7) {
  st->enable_mair = 1;
  st->mair_attr7 = mair_attr7;
}

void read_init_unmapped(const litmus_test_t* cfg, var_info_t* infos, const char* varname) {
  var_idx_t idx = idx_from_varname_infos(cfg, infos, varname);
  var_info_t* vinfo = &infos[idx];

  fail_on(
    vinfo->ty != VAR_NOTSET,
    "cannot unmap \"%s\": it already set (%d:%s)\n",
    varname,
    vinfo->ty,
    var_info_type_to_str(vinfo->ty)
  );
  infos[idx].ty = VAR_UNMAPPED;
}

void read_init_idmap(const litmus_test_t* cfg, var_info_t* infos, const char* varname) {
  var_idx_t idx = idx_from_varname_infos(cfg, infos, varname);
  var_info_t* vinfo = &infos[idx];

  fail_on(!is_backed_var(vinfo), "can only idmap a heap or pin variable.");

  var_info_backing_t* back = var_backing(vinfo);
  back->is_identity_mapped = 1;
}

void read_init_region_offs(
  const litmus_test_t* cfg, var_info_t* infos, const char* varname, const char* offsvarname, rel_offset_t offs
) {
  var_idx_t idx = idx_from_varname_infos(cfg, infos, varname);
  var_info_t* vinfo = &infos[idx];

  var_idx_t offsvaridx = idx_from_varname_infos(cfg, infos, offsvarname);
  var_info_t* offsvinfo = &infos[offsvaridx];

  fail_on(vinfo->ty == VAR_NOTSET, "cannot modify a variable that is unset");
  fail_on(!is_backed_var(offsvinfo), "can only be offset from heap or pin variable.");

  var_info_attrs_t* attr = &vinfo->init_attrs;

  attr->has_region_offset = 1;
  attr->region_offset.offset_var = offsvaridx;
  attr->region_offset.offset_level = offs;
}

void read_init_region_own(const litmus_test_t* cfg, var_info_t* infos, const char* varname, own_level_t region) {
  var_idx_t idx = idx_from_varname_infos(cfg, infos, varname);
  var_info_t* vinfo = &infos[idx];

  fail_on(infos[idx].ty == VAR_NOTSET, "cannot modify a variable that is unset");
  fail_on(vinfo->ty != VAR_HEAP, "can only set owned region for a heap variable.");

  vinfo->heap.owned_region_size = region;
}

void read_init_region_pin(
  const litmus_test_t* cfg, var_info_t* infos, const char* varname, const char* pinned_var_name, pin_level_t pin_level
) {
  var_idx_t idx = idx_from_varname_infos(cfg, infos, varname);
  var_info_t* vinfo = &infos[idx];

  var_idx_t pinnedidx = idx_from_varname_infos(cfg, infos, pinned_var_name);
  var_info_t* pinnedvinfo = &infos[pinnedidx];

  fail_on(vinfo->ty == VAR_NOTSET, "cannot modify a variable that is unset");
  fail_on(vinfo->ty != VAR_HEAP, "can only set pinned region for a heap variable.");
  fail_on(pinnedvinfo->ty != VAR_HEAP, "can only pinned to a heap variable."); // TODO: or another pin variable?

  vinfo->ty = VAR_PINNED;
  vinfo->pin.back = vinfo->heap.back; // TODO: is this valid C?
  vinfo->pin.pin_region_var = pinnedidx;
  vinfo->pin.pin_region_level = pin_level;
}

void read_init_alias(const litmus_test_t* cfg, var_info_t* infos, const char* varname, const char* aliasname) {
  var_idx_t idx = idx_from_varname_infos(cfg, infos, varname);
  var_info_t* vinfo = &infos[idx];

  var_idx_t aliasidx = idx_from_varname_infos(cfg, infos, aliasname);
  var_info_t* aliasvinfo = &infos[aliasidx];

  fail_on(vinfo->ty != VAR_NOTSET, "can only alias an unset variable");
  fail_on(!is_backed_var(aliasvinfo), "can only alias to a heap variable.");

  vinfo->ty = VAR_ALIAS;
  vinfo->alias.aliased_with = aliasidx;
}

void read_init_ap(const litmus_test_t* cfg, var_info_t* infos, const char* varname, prot_type_t prot_type, u64 attr) {
  var_idx_t idx = idx_from_varname_infos(cfg, infos, varname);
  var_info_t* vinfo = &infos[idx];
  var_info_attrs_t* init_attr = &vinfo->init_attrs;

  switch (prot_type) {
  case PROT_AP:
    init_attr->has_ap = 1;
    init_attr->ap = attr;
    break;
  case PROT_ATTRIDX:
    init_attr->has_attridx = 1;
    init_attr->attridx = attr;
    break;
  }
}

void read_init_pte(const litmus_test_t* cfg, var_info_t* infos, const char* varname, u64 desc) {
  fail("INIT_PTE unsupported, use INIT_UNMAPPED or INIT_AP\n");
}

void read_init_fix(const litmus_test_t* cfg, var_info_t* infos, const char* varname, uint64_t pa) {
  var_idx_t idx = idx_from_varname_infos(cfg, infos, varname);
  var_info_t* vinfo = &infos[idx];

  fail_on(vinfo->ty != VAR_NOTSET, "can only fix an unset variable");

  vinfo->ty = VAR_FIXED;
  vinfo->fixed.phys = pa;
}

void read_init_heap(const litmus_test_t* cfg, var_info_t* infos, const char* varname, u64 value) {
  var_idx_t idx = idx_from_varname_infos(cfg, infos, varname);
  var_info_t* vinfo = &infos[idx];

  fail_on(vinfo->ty != VAR_NOTSET, "can only set an unset variable");

  vinfo->ty = VAR_HEAP;

  vinfo->heap.back.val = value;
  vinfo->heap.back.is_identity_mapped = 0;
  vinfo->heap.owned_region_size = REGION_OWN_PAGE;
}

#define ID(N) [(N)] = #N
static const char* var_type_names[] = {
  ID(VAR_NOTSET), ID(VAR_HEAP), ID(VAR_ALIAS), ID(VAR_PINNED), ID(VAR_FIXED), ID(VAR_UNMAPPED),
};
const char* var_info_type_to_str(var_info_type_t ty) {
  return var_type_names[ty];
}

static void debug_region_offset(var_info_attrs_t* attr, var_info_t* infos) {
  debug(
    "{"
    ".offset_var=%s,"
    " "
    ".offset_level=\"%s\","
    "}",
    infos[attr->region_offset.offset_var].name,
    rel_offset_to_str(attr->region_offset.offset_level)
  );
}

static void debug_backing(var_info_backing_t* back, var_info_t* infos) {
  debug("{");

  debug(".init_val=0x%lx", back->val);

  if (back->is_identity_mapped) {
    debug(", ");
    debug(".is_identity_mapped=true");
  }

  debug("}");
}

void debug_print_var_info(var_info_t* vinfo, var_info_t* infos) {
  debug("{\n");
  debug(" .name=\"%s\"\n", vinfo->name);
  debug(" .varidx=%ld\n", vinfo->varidx);

  var_info_attrs_t* attr = &vinfo->init_attrs;
  if (attr->has_ap || attr->has_attridx || attr->has_region_offset) {
    debug("  .init_attrs={\n");
    if (attr->has_ap) {
      debug("    .ap=0x%lx\n", attr->ap);
    }
    if (attr->has_attridx) {
      debug("    .attridx=%ld\n", attr->attridx);
    }
    if (attr->has_region_offset) {
      debug("    .region_offset=");
      debug_region_offset(attr, infos);
      debug("\n");
    }
    debug("  }\n");
  }

  debug("  .kind=%s\n", var_type_names[vinfo->ty]);

  switch (vinfo->ty) {
  case VAR_NOTSET:
    break;
  case VAR_HEAP:
    debug("  .back=");
    debug_backing(&vinfo->heap.back, infos);
    debug("\n");
    debug("  .owned_region_size=%s\n", own_level_to_str(vinfo->heap.owned_region_size));
    break;
  case VAR_ALIAS:
    debug("  .aliased_with=\"%s\"\n", infos[vinfo->alias.aliased_with].name);
    break;
  case VAR_PINNED:
    debug("  .back=");
    debug_backing(&vinfo->pin.back, infos);
    debug("\n");
    debug("  .pin_region_var=\"%s\"\n", infos[vinfo->pin.pin_region_var].name);
    debug("  .pin_region_level=%s\n", pin_level_to_str(vinfo->pin.pin_region_level));
    break;
  case VAR_FIXED:
    debug("  .phys=%p\n", vinfo->fixed.phys);
    break;
  case VAR_UNMAPPED:
    break;
  default:
    fail("unreachable");
  }
  debug("}\n");
}