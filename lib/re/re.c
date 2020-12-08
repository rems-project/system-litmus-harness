#include "lib.h"

#include "re_types.h"

re_tok_t _tokenize(char c) {
    if (c == '*' || c == '?') {
        return (re_tok_t){.accepts='\0', .is_char=0};
    } else {
        return (re_tok_t){.accepts=c, .is_char=1};
    }
}

re_st_t _mk_st(char c, re_st_t* prev) {
    re_tok_t tok = _tokenize(c);

    re_st_t st;
    st.accepter = tok;
    st.next = NULL;
    st.prev = prev;
    return st;
}

re_t* re_compile(char* s) {
    int idx = 0;
    re_t* r = alloc(sizeof(re_t) + sizeof(re_st_t)*strlen(s));
    r->original_expr = s;

    char c;
    c = *s++;
    re_st_t _initial = _mk_st(c, NULL);
    r->sts[idx++] = _initial;
    r->no_states++;

    re_st_t* prev = &r->sts[0];

    while (*s) {
        c = *s++;
        re_st_t _next = _mk_st(c, prev);
        r->sts[idx++] = _next;
        prev = &r->sts[idx-1];
        prev->prev->next = prev;
        r->no_states++;
    }

    return r;
}

void re_free(re_t* re) {
    FREE(re);
}

typedef struct {
    int idx;
    re_st_t* current;
} re_ctx_pos_t;

typedef struct {
    re_t* re;
    const char* input;
    re_ctx_pos_t pos;
} re_ctx_t;

re_ctx_t _build_ctx(re_t* re, const char* input) {
    return (re_ctx_t) {
        .re = re,
        .input = input,
        .pos = (re_ctx_pos_t){
            .idx = 0,
            .current = &re->sts[0],
        },
    };
}

int _no_more_regex(re_ctx_t* ctx) {
  return (ctx->pos.current == NULL);
}

int _at_end_of_stream(re_ctx_t* ctx) {
  return (ctx->pos.idx == strlen(ctx->input));
}

int _would_accept_char(re_ctx_t* ctx, re_st_t* st, int idx) {
    char c = ctx->input[idx];
    if (st->accepter.is_char) {
        if (st->accepter.accepts == c) {
            return 1;
        }
    }

    return 0;
}

int _accepts_next_char(re_ctx_t* ctx) {
    return _would_accept_char(ctx, ctx->pos.current, ctx->pos.idx);
}

int _is_wildcard(re_ctx_t* ctx) {
    return ! ctx->pos.current->accepter.is_char;
}

void _accept_and_step(re_ctx_t* ctx) {
    ctx->pos.idx += 1;
    ctx->pos.current = ctx->pos.current->next;
}

typedef struct {
    re_ctx_pos_t ctx_pos;
    int valid_to_idx;
} re_ctx_backtrack_t;

int re_matches(re_t* re, const char* input) {
    re_ctx_t ctx = _build_ctx(re, input);

    int stack_depth=0;
    re_ctx_backtrack_t backtrack_stack[100];

    while (1) {
        if (_at_end_of_stream(&ctx) || _no_more_regex(&ctx)) {
            return _at_end_of_stream(&ctx) && _no_more_regex(&ctx);
        } else if (_accepts_next_char(&ctx)) {
            _accept_and_step(&ctx);
        } else if (_is_wildcard(&ctx)) {
            if (ctx.pos.current->next == NULL) {
                /* wildcard at end */
                return 1;
            }

            int valid_up_to = ctx.pos.idx;
            re_st_t* next_st = ctx.pos.current->next;
            re_tok_t next_tok = next_st->accepter;

            if (! next_tok.is_char) {
                /* two wildcards */
                _accept_and_step(&ctx);
                continue;
            }

            while (1) {
                /* run from valid_up_to until we find a match for next_tok */
                if (! _would_accept_char(&ctx, next_st, valid_up_to)) {
                    valid_up_to++;
                } else {
                    break;
                }
            }

            backtrack_stack[stack_depth++] = (re_ctx_backtrack_t) {
                .valid_to_idx = valid_up_to,
                .ctx_pos = ctx.pos,
            };

            /* step up to there */
            ctx.pos.current = next_st;
            ctx.pos.idx = valid_up_to;
        } else if (stack_depth > 0) {
            /* backtrack */
            re_ctx_backtrack_t bt = backtrack_stack[stack_depth--];
            ctx.pos = bt.ctx_pos;
            ctx.pos.idx = bt.valid_to_idx;
            continue;
        } else {
            return 0;
        }
    }
}