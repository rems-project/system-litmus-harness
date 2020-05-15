#include "lib.h"

/** helper file for defining litmus tests */

uint64_t* var_va(litmus_test_run* data, const char* name) {
    return data->va[idx_from_varname(data->ctx, name)];
}

uint64_t var_pa(litmus_test_run* data, const char* name) {
    return data->pa[idx_from_varname(data->ctx, name)];
}

uint64_t* var_pte(litmus_test_run* data, const char* name) {
    return data->pte[idx_from_varname(data->ctx, name)];
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
