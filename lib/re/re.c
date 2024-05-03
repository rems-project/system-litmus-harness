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
    r->no_states = 0;
    r->start = NULL;

    if (strlen(s) == 0)
        return r;

    char c;
    c = *s++;
    re_st_t _initial = _mk_st(c, NULL);
    r->sts[idx++] = _initial;
    r->no_states++;
    re_st_t* prev = &r->sts[0];
    r->start = prev;

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
    /** position in the stream */
    int idx;

    /** position in the regex */
    re_st_t* current;
} re_ctx_pos_t;

typedef struct {
    re_ctx_pos_t ctx_pos;
    int valid_to_idx;
} re_ctx_backtrack_t;

typedef struct {
    re_ctx_backtrack_t backtrack_stack[100];
    int stack_depth;
} re_ctx_backtrack_stack_t;

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
            .current = re->start,
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

void _chomp_wildcard(re_ctx_t* ctx, re_ctx_backtrack_stack_t *stack) {
    /* chomp up to the next token
     * saving the state onto the backtrack stack
     */

    int valid_up_to = ctx->pos.idx;

    re_st_t* next_st = ctx->pos.current;
    re_tok_t next_tok;

    /* swallow wildcards up until end-of-regex or first non-wildcard-token in regex */
    do {
        next_st = next_st->next;

        if (next_st == NULL) {
            /* wildcard at end of regex
             * just advance idx to end and let the next go round the loop catch it.
             */
            valid_up_to = strlen(ctx->input);
            goto next_token;
        }

        /* otherwise, we're looking for this token */
        next_tok = next_st->accepter;

        /* if another wildcard, eat it and go round again */
        if (!next_tok.is_char)
            continue;

    } while (false);

    /* eat stream until we find the token we're looking for
     * stopping if we reach the end prematurely
     */
    while (true) {
        /* if at end, but haven't found the acceptor
         * reject input and go back
         */
        if (valid_up_to == strlen(ctx->input)) {
            goto next_token;
        }

        /* run from valid_up_to until we find a match for next_tok,
        * or end of stream */
        if (_would_accept_char(ctx, next_st, valid_up_to)) {
            break;
        }

        valid_up_to++;
    }

    /* place onto the backtracking stack:
     *  - the current token = the wildcard
     *  - the current pos = where we will now accept
     */
    stack->backtrack_stack[stack->stack_depth++] = (re_ctx_backtrack_t) {
        .valid_to_idx = valid_up_to + 1,
        .ctx_pos = ctx->pos
    };

    /* step up to there */
next_token:
    ctx->pos.current = next_st;
    ctx->pos.idx = valid_up_to;
}

void _backtrack(re_ctx_t* ctx, re_ctx_backtrack_stack_t *stack) {
    re_ctx_backtrack_t bt = stack->backtrack_stack[--stack->stack_depth];
    ctx->pos = bt.ctx_pos;
    ctx->pos.idx = bt.valid_to_idx;
}

int re_matches(re_t* re, const char* input) {
    re_ctx_t ctx = _build_ctx(re, input);

    re_ctx_backtrack_stack_t backtrack_stack = {
        .stack_depth = 0,
    };

    while (1) {
        if (_at_end_of_stream(&ctx) || _no_more_regex(&ctx)) {
            if (_at_end_of_stream(&ctx) && _no_more_regex(&ctx)) {
                return 1;
            } else if (backtrack_stack.stack_depth > 0) {
                _backtrack(&ctx, &backtrack_stack);
            } else {
                return 0;
            }
        } else if (_accepts_next_char(&ctx)) {
            _accept_and_step(&ctx);
        } else if (_is_wildcard(&ctx)) {
            _chomp_wildcard(&ctx, &backtrack_stack);
        } else if (backtrack_stack.stack_depth > 0) {
            _backtrack(&ctx, &backtrack_stack);
            continue;
        } else {
            return 0;
        }
    }
}