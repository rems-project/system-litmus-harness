#include "lib.h"

#include "argdef.h"

int argparse_check_name_exact(char* argv, char* short_name, char* long_name) {
  char* s = argv + 1;
  int flipped = 0;

  if (strstartswith(s, "-no-")) {
    flipped = 1;
    s += 3;
  }

  if (short_name && strcmp(s, short_name)) {
    return 1;
  } else {
    switch (*s) {
      case '-':
        s++;
        if (long_name && strcmp(s, long_name)) {
          if (flipped)
            return 2;
          else
            return 1;
        }
        break;
      default:
        break;
    }
  }

  return 0;
}

int argdef_try_split(char* lhs, char* rhs, char* argv, char* short_name, char* long_name) {
  /* check for -short_nameNNNN or --long_name= */
  char* s = argv + 1;
  switch (*s) {
    case '-':
      s++;
      if (long_name && strstartswith(s, long_name)) {
        if (! strpartition(lhs, rhs, argv, '=')) {
          *rhs = '\0';
          // fail("argument mismatch: %s expected argument.  See Usage or --help=%s for more info.\n", lhs, lhs);
        }
        return 1;
      }
      break;
    default:
      if (short_name && (*s == *short_name)) {
        *lhs = *s;
        valloc_memcpy(rhs, s+1, sizeof(char)*(1+strlen(s+1)));
        return 1;
      }
      break;
  }

  return 0;
}

int argparse_check_flag_arg(argdef_t* argdefr, char* argv, const argdef_flag_t* arg) {
  int match = argparse_check_name_exact(argv, 1+arg->short_name, 2+arg->long_name);
  if (match == 0) {
    return 0;
  } else if (match == 1) {
    *arg->flag = 1;
    return 1;
  } else if (match == 2) {
    *arg->flag = 0;
    return 1;
  }

  return 0;
}

int argparse_check_option_arg(argdef_t* argdefr, char* argv, const argdef_option_t* arg) {
  char lhs[100];
  char rhs[100];
  int match = argdef_try_split((char*)&lhs[0], (char*)&rhs, argv, 1+arg->short_name, 2+arg->long_name);

  if (match == 0) {
    return 0;
  } else {
    if (arg->only_action && rhs[0]) {
      if (arg->short_name && arg->long_name)
        fail("%s/%s did not expect an argument.\n", arg->short_name, arg->long_name);
      else if (arg->short_name)
        fail("%s did not expect an argument.\n", arg->short_name);
      else
        fail("%s did not expect an argument.\n", arg->long_name);
    }

    arg->action((char*)&rhs[0]);
    return 1;
  }
}

int argparse_check_enum_arg(argdef_t* argdefr, char* argv, const argdef_enum_t* arg) {
  char lhs[100];
  char rhs[100];
  int match = argdef_try_split((char*)&lhs[0], (char*)&rhs, argv, NULL, 2+arg->long_name);

  if (match == 0) {
    return 0;
  } else {
    for (int o = 0; o < arg->nopts; o++) {
      if (strcmp(arg->names[o], (char*)&rhs[0])) {
        valloc_memcpy(arg->enum_var, &arg->enum_values[o*arg->enum_size], arg->enum_size);
        return 1;
      }
    }

    fail("for option %s, got unexpected value '%s'\n", arg->long_name, (char*)&rhs[0]);
    return 0;
  }

  return 0;
}

int argparse_check_arg(argdef_t* argdefr, char* argv, const argdef_arg_t* arg) {
  switch (arg->kind) {
    case ARGPARSE_FLAG:
      return argparse_check_flag_arg(argdefr, argv, arg->flag);
    case ARGPARSE_OPTION:
      return argparse_check_option_arg(argdefr, argv, arg->option);
    case ARGPARSE_ENUM:
      return argparse_check_enum_arg(argdefr, argv, arg->enumeration);
  }

  return 0;
}

void argparse_read_arg(argdef_t* argdefr, char* argv) {
  int no_args = argdef_countargs(argdefr);

  for (int i = 0; i < no_args; i++) {
    if (argparse_check_arg(argdefr, argv, argdefr->args[i])) {
      return;
    }
  }

  fail("unknown argument '%s'\n", argv);
}

void argparse_read_args(argdef_t* args, int argc, char** argv) {
  for (int i = 0; i < argc; i++) {
    switch (*argv[i]) {
      case '-':
        argparse_read_arg(args, argv[i]);
        break;
      default:
        collected_tests[collected_tests_count++] = argv[i];
        break;
    }
  }
}

/* helper funcs */
int argdef_countargs(argdef_t* args) {
  int i = 0;
  while (1) {
    if (args->args[i++] == NULL) {
      return i - 1;
    }
  }
}

char* argdef_arg_shortname(const argdef_arg_t* arg) {
  switch (arg->kind) {
    case ARGPARSE_FLAG:
      return arg->flag->short_name;
    case ARGPARSE_OPTION:
      return arg->option->short_name;
    case ARGPARSE_ENUM:
      return NULL;
  }

  return NULL;
}

char* argdef_arg_longname(const argdef_arg_t* arg) {
  switch (arg->kind) {
    case ARGPARSE_FLAG:
      return arg->flag->long_name;
    case ARGPARSE_OPTION:
      return arg->option->long_name;
    case ARGPARSE_ENUM:
      return arg->enumeration->long_name;
  }

  return NULL;
}

char* argdef_arg_description(const argdef_arg_t* arg) {
  switch (arg->kind) {
    case ARGPARSE_FLAG:
      return arg->flag->desc;
    case ARGPARSE_OPTION:
      return arg->option->desc;
    case ARGPARSE_ENUM:
      return arg->enumeration->desc;
  }

  return "";
}