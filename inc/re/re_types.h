#ifndef RE_TYPES_H
#define RE_TYPES_H

typedef struct {
    int is_char;
    char accepts;
} re_tok_t;

typedef struct st {
    re_tok_t accepter;
    struct st* next;
    struct st* prev;
} re_st_t;

typedef struct {
    char* original_expr;
    re_st_t* start;

    int no_states;
    re_st_t sts[];
} re_t;

#endif /* RE_TYPES_H */