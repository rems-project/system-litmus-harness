#ifndef ARGPARSE_H
#define ARGPARSE_H

#include "lib.h"

/* in the following short_name is either NULL or a string of length 1 */

typedef struct
{
  char* short_name;
  char* long_name;
  u8 has_action;
  union {
    u8* flag;
    void (*action)(void);
  };
  char* desc;
  u8 no_negation; /* disables --no-foo for option --foo */
} argdef_flag_t;

typedef enum {
  OPT_ARG_OPTIONAL,
  OPT_ARG_REQUIRED,
  OPT_ARG_NONE,
} argdef_option_action_kind_t;

typedef struct
{
  char* short_name;
  char* long_name;
  void (*action)(char* value);
  char* desc;
  char* metavar;
  argdef_option_action_kind_t arg;
  u8 show_help_both;
} argdef_option_t;

typedef struct
{
  char* long_name;
  char* enum_var;
  int nopts;
  char** names;
  size_t enum_size;
  char* enum_values;
  char* desc;
} argdef_enum_t;

typedef enum {
  ARGPARSE_FLAG,
  ARGPARSE_OPTION,
  ARGPARSE_ENUM,
} argdef_kind_t;

typedef struct
{
  argdef_kind_t kind;
  union {
    const argdef_flag_t* flag;
    const argdef_option_t* option;
    const argdef_enum_t* enumeration;
  };
} argdef_arg_t;

typedef struct
{
  const char* exe_name;
  const char* short_usage;
  const char* description;
  const argdef_arg_t** args;
} argdef_t;

extern argdef_t COMMON_ARGS;
extern argdef_t LITMUS_ARGS;
extern argdef_t UNITTEST_ARGS;
extern argdef_t* THIS_ARGS; /* either LITMUS_ARGS or UNITTEST_ARGS */

/* helper functions over argdef_t objects */
int argdef_countargs(argdef_t* args);

char* argdef_arg_shortname(const argdef_arg_t* arg);
char* argdef_arg_longname(const argdef_arg_t* arg);
char* argdef_arg_description(const argdef_arg_t* arg);

/* macros to help make the above */
/* clang-format off */
#define OPT(short, long, action_fn, full_desc, ...) \
  &(argdef_arg_t){                                  \
    .kind=ARGPARSE_OPTION,                          \
    .option=&(argdef_option_t){                     \
      .short_name=short,                            \
      .long_name=long,                              \
      .action=action_fn,                            \
      .desc=full_desc,                              \
      __VA_ARGS__                                   \
    }                                               \
  }

#define FLAG(short, long, flagvar, full_desc, ...) \
  &(argdef_arg_t){                                 \
    .kind=ARGPARSE_FLAG,                           \
    .flag=&(argdef_flag_t){                        \
      .short_name=short,                           \
      .long_name=long,                             \
      .flag=&flagvar,                              \
      .desc=full_desc,                             \
      __VA_ARGS__                                  \
    }                                              \
  }

#define FLAG_ACTION(short, long, action_fn, full_desc, ...) \
  &(argdef_arg_t){                                          \
    .kind=ARGPARSE_FLAG,                                    \
    .flag=&(argdef_flag_t){                                 \
      .short_name=short,                                    \
      .long_name=long,                                      \
      .has_action=1,                                        \
      .action=action_fn,                                    \
      .desc=full_desc,                                      \
      __VA_ARGS__                                           \
    }                                                       \
  }

#define ARR(...) __VA_ARGS__

#define ENUMERATE(long, enumvar, enumty, no_of_opts, optnames, enums, full_desc, ...) \
  &(argdef_arg_t){                                                                    \
    .kind=ARGPARSE_ENUM,                                                              \
    .enumeration=&(argdef_enum_t){                                                    \
      .long_name=long,                                                                \
      .desc=full_desc,                                                                \
      .enum_var=(char*)&enumvar,                                                      \
      .enum_size=sizeof(enumty),                                                      \
      .nopts=no_of_opts,                                                              \
      .names=(char**)(optnames),                                                      \
      .enum_values=(char*)(enums),                                                    \
      __VA_ARGS__                                                                     \
    }                                                                                 \
  }
/* clang-format on */

void argparse_read_args(int argc, char** argv);

static inline bool option_requires_arg(const argdef_arg_t* arg) {
  return ((arg->kind == ARGPARSE_ENUM) || (arg->kind == ARGPARSE_OPTION && arg->option->arg == OPT_ARG_REQUIRED));
}

static inline bool option_takes_arg(const argdef_arg_t* arg) {
  return ((arg->kind == ARGPARSE_ENUM) || (arg->kind == ARGPARSE_OPTION && arg->option->arg != OPT_ARG_NONE));
}

static inline char* option_metavar(const argdef_arg_t* arg) {
  if (arg->kind == ARGPARSE_OPTION && arg->option->metavar)
    return arg->option->metavar;

  return "X";
}

#endif /* ARGPARSE_H */