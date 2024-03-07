#include "lib.h"

#include "argdef.h"

int argparse_check_name_exact(char* argv, char* short_name, char* long_name) {
  char* s = argv + 1;
  int flipped = 0;

  if (strstartswith(s, "-no-")) {
    flipped = 1;
    s += 3;
  }

  if (short_name && strcmp(s, 1 + short_name)) {
    return 1;
  } else {
    switch (*s) {
      case '-':
        s++;
        if (long_name && strcmp(s, 2 + long_name)) {
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

int argdef_try_split(char* lhs, char* rhs, char* argv, char* short_name, char* long_name, argdef_option_action_kind_t opt_arg) {
  /* check for -short_nameNNNN or --long_name= */
  char* s = argv + 1;
  switch (*s) {
    case '-':
      s++;
      if (long_name && strstartswith(s, 2 + long_name)) {
        bool split = strpartition(lhs, rhs, argv, '=');

        if (!split && opt_arg == OPT_ARG_REQUIRED) {
          *rhs = '\0';
          fail("argument mismatch: %s expected argument.  See Usage or --help=%s for more info.\n", lhs, lhs);
        }

        if (split && opt_arg == OPT_ARG_NONE) {
          fail("argument mismatch: %s expected no argument.  See Usage or --help=%s for more info.\n", lhs, lhs);
        }

        return 1;
      }
      break;
    default:
      if (short_name && (*s == *(1 + short_name))) {
        *lhs = *s;
        valloc_memcpy(rhs, s+1, sizeof(char)*(1+strlen(s+1)));
        return 1;
      }
      break;
  }

  return 0;
}

int argparse_check_flag_arg(argdef_t* argdefr, char* argv, const argdef_flag_t* arg) {
  int match = argparse_check_name_exact(argv, arg->short_name, arg->long_name);
  if (match == 0) {
    return 0;
  } else if (match == 1) {
    if (arg->has_action) {
      arg->action();
    } else {
      *arg->flag = 1;
    }
    return 1;
  } else if (match == 2) {
    if (! arg->has_action) {
      /* if --foo has action, then --no-foo is no-op. */
      *arg->flag = 0;
    }
    return 1;
  }

  return 0;
}

int argparse_check_option_arg(argdef_t* argdefr, char* argv, const argdef_option_t* arg) {
  char lhs[100] = {0};
  char rhs[100] = {0};
  int match = argdef_try_split((char*)&lhs[0], (char*)&rhs, argv, arg->short_name, arg->long_name, arg->arg);

  if (match == 0) {
    return 0;
  } else {
    if (arg->arg == OPT_ARG_NONE && rhs[0]) {
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
  char lhs[100] = {0};
  char rhs[100] = {0};
  int match = argdef_try_split((char*)&lhs[0], (char*)&rhs, argv, NULL, arg->long_name, OPT_ARG_REQUIRED);

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

bool argparse_read_arg(argdef_t* argdefr, char* argv) {
  int no_defr_args = argdef_countargs(argdefr);
  for (int i = 0; i < no_defr_args; i++) {
    if (argparse_check_arg(argdefr, argv, argdefr->args[i])) {
      return true;
    }
  }

  return false;
}

void argparse_read_args(int argc, char** argv) {
  for (int i = 0; i < argc; i++) {
    switch (*argv[i]) {
      case '-':
        if (  !argparse_read_arg(THIS_ARGS, argv[i])
           && !argparse_read_arg(&COMMON_ARGS, argv[i])
        )
          fail("unknown argument '%s'\n", argv[i]);
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