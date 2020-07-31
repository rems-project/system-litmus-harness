#ifndef ARGPARSE_H
#define ARGPARSE_H

#include "lib.h"

/* in the following short_name is either NULL or a string of length 1 */

typedef struct {
    char* short_name;
    char* long_name;
    uint8_t* flag;
    char* desc;
    uint8_t no_negation;  /* disables --no-foo for option --foo */
} argdef_flag_t;

typedef struct {
    char* short_name;
    char* long_name;
    void (*action)(char* value);
    char* desc;
    uint8_t only_action;
    uint8_t show_help_both;
} argdef_option_t;

typedef struct {
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

typedef struct {
    argdef_kind_t kind;
    union {
        const argdef_flag_t* flag;
        const argdef_option_t* option;
        const argdef_enum_t* enumeration;
    };
} argdef_arg_t;

typedef struct {
    const char* version;
    const argdef_arg_t** args;
} argdef_t;

/* globally defined argdef_t object
 * as set by DEFINE_ARGS */
extern argdef_t ARGS;

/* helper functions over argdef_t objects */
int argdef_countargs(argdef_t* args);

char* argdef_arg_shortname(const argdef_arg_t* arg);
char* argdef_arg_longname(const argdef_arg_t* arg);
char* argdef_arg_description(const argdef_arg_t* arg);

/* macros to help make the above */

#define OPT(short, long, action_fn, full_desc, ...) \
  &(argdef_arg_t){ \
    .kind=ARGPARSE_OPTION, \
    .option=&(argdef_option_t){ \
      .short_name=short, \
      .long_name=long, \
      .action=action_fn, \
      .desc=full_desc, \
      __VA_ARGS__ \
    } \
  }

#define FLAG(short, long, flagvar, full_desc, ...) \
  &(argdef_arg_t){ \
    .kind=ARGPARSE_FLAG, \
    .flag=&(argdef_flag_t){ \
      .short_name=short, \
      .long_name=long, \
      .flag=&flagvar, \
      .desc=full_desc, \
      __VA_ARGS__ \
    } \
  }

#define ARR(...) __VA_ARGS__

#define ENUMERATE(long, enumvar, enumty, no_of_opts, optnames, enums, full_desc, ...) \
  &(argdef_arg_t){ \
    .kind=ARGPARSE_ENUM, \
    .enumeration=&(argdef_enum_t){ \
      .long_name=long, \
      .desc=full_desc, \
      .enum_var=(char*)&enumvar, \
      .enum_size=sizeof(enumty), \
      .nopts=no_of_opts, \
      .names=(char**)(optnames), \
      .enum_values=(char*)(enums), \
      __VA_ARGS__ \
    } \
  }

#define DEFINE_ARGS(...) \
  argdef_t ARGS = (argdef_t){ \
    .version=VERSION, \
    .args=(const argdef_arg_t*[]){ \
      __VA_ARGS__ NULL \
    } \
  };

void argparse_read_args(argdef_t* def, int argc, char** argv);

#endif /* ARGPARSE_H */