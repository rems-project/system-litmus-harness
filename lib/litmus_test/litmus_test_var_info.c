#include "lib.h"

void read_var_infos(test_ctx_t* ctx, const litmus_test_t* cfg, var_info_t* infos, int no_runs) {
  for (int v = 0; v < cfg->no_heap_vars; v++) {
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
      case (TYPE_ALIAS):
        read_init_alias(cfg, infos, name, var->aliasname);
        break;
      case (TYPE_AP):
        read_init_ap(cfg, infos, name, var->value);
        break;
      case (TYPE_REGION):
        read_init_region(cfg, infos, name, var->pinned_var_name, var->pinned_level);
        break;
    }
  }
}

void read_init_region(const litmus_test_t* cfg, var_info_t* infos, const char* varname, const char* pinned_var_name, pin_level_t pin_level) {
  uint64_t idx = idx_from_varname_infos(cfg, infos, varname);

  if (pin_level >= NR_REGIONS) {
    fail("! error: in test %s, variable '%s' specified an invalid region.\n",
      cfg->name,
      varname
    );
  }

  infos[idx].init_region_pinned = 1;
  infos[idx].pin_region_var = pinned_var_name;
  infos[idx].pin_region_level = pin_level;
}

void read_init_alias(const litmus_test_t* cfg, var_info_t* infos, const char* varname, const char* aliasname) {
  uint64_t idx = idx_from_varname_infos(cfg, infos, varname);
  infos[idx].alias = aliasname;
}

void read_init_ap(const litmus_test_t* cfg, var_info_t* infos, const char* varname, uint64_t ap) {
  uint64_t idx = idx_from_varname_infos(cfg, infos, varname);
  infos[idx].init_ap = ap;
}

void read_init_pte(const litmus_test_t* cfg, var_info_t* infos, const char* varname, uint64_t desc) {
  fail("INIT_PTE unsupported, use INIT_UNMAPPED or INIT_AP\n");
}

void read_init_heap(const litmus_test_t* cfg, var_info_t* infos, const char* varname, uint64_t value) {
  uint64_t idx = idx_from_varname_infos(cfg, infos, varname);
  infos[idx].init_value = value;
}