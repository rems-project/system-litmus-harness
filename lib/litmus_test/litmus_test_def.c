#include "lib.h"

/** helper file for defining litmus tests */

uint64_t* var_va(litmus_test_run* data, const char* name) {
    return data->va[idx_from_varname(data->ctx, name)];
}

uint64_t var_pa(litmus_test_run* data, const char* name) {
    return data->pa[idx_from_varname(data->ctx, name)];
}

uint64_t* var_pte(litmus_test_run* data, const char* name) {
    return data->pte_descs[3][idx_from_varname(data->ctx, name)];
}

uint64_t* var_pte_level(litmus_test_run* data, const char* name, int level) {
    return data->pte_descs[level][idx_from_varname(data->ctx, name)];
}

uint64_t* var_pte_level0(litmus_test_run* data, const char* name) {
    return var_pte_level(data, name, 0);
}

uint64_t* var_pte_level1(litmus_test_run* data, const char* name) {
    return var_pte_level(data, name, 1);
}

uint64_t* var_pte_level2(litmus_test_run* data, const char* name) {
    return var_pte_level(data, name, 2);
}

uint64_t* var_pte_level3(litmus_test_run* data, const char* name) {
    return var_pte_level(data, name, 3);
}

uint64_t var_desc(litmus_test_run* data, const char* name) {
    return data->desc[idx_from_varname(data->ctx, name)];
}

uint64_t var_page(litmus_test_run* data, const char* name) {
    return PAGE(data->va[idx_from_varname(data->ctx, name)]);
}

uint64_t* out_reg(litmus_test_run* data, const char* name) {
    return data->out_reg[idx_from_regname(data->ctx, name)];
}
