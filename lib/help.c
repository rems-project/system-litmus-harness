#include "lib.h"
#include "argdef.h"

const char* DESCRIPTION =
  "Run EL1/EL0 litmus tests\n"
  ;

static void splitdesc(char* short_desc, char* long_desc, char* full) {
  while (*full != '\n') {
    *short_desc = *full;

    if (*short_desc == '\0') {
      return;
    }

    short_desc++;
    full++;
  }

  *short_desc = '\0';

  while (*full == '\n') {
    full++;
  }

  while (*full != '\0') {
    *long_desc = *full;
    long_desc++;
    full++;
  }
}

static int line_prefix(char* out, const argdef_arg_t* arg) {
  char* start = out;

  char* short_name = argdef_arg_shortname(arg);
  char* long_name = argdef_arg_longname(arg);

  if (short_name != NULL) {
    out = sprintf(out, "%s", short_name);

    if (arg->kind == ARGPARSE_OPTION) {
      if (!arg->option->only_action && !arg->option->show_help_both) {
        out = sprintf(out, "X");
      }
    }

    if (long_name != NULL) {
      out = sprintf(out, ", ");
    }
  }

  if (long_name != NULL) {
    out = sprintf(out, "%s", long_name);

    if ((arg->kind == ARGPARSE_OPTION && !arg->option->only_action) || arg->kind == ARGPARSE_ENUM) {
      if (arg->option->show_help_both) {
        out = sprintf(out, ", %s", long_name);
      }
      out = sprintf(out, "=X");
    }

    if (arg->kind == ARGPARSE_FLAG) {
      out = sprintf(out, "/--no-%s", 2+long_name);
    }
  }

  return strlen(start);
}

static void display_enum_options(const argdef_enum_t* arg) {
  printf("{");
  for (int i = 0; i < arg->nopts - 1; i++) {
    printf("%s,", arg->names[i]);
  }
  printf("%s}", arg->names[arg->nopts - 1]);
}

static void display_flag_default(const argdef_flag_t* arg) {
  printf("%d", *arg->flag);
}

static void display_line_help_for_arg(const argdef_arg_t* arg, int prefix_pad_to) {
  char prefix[1024];
  int prefix_len = line_prefix(&prefix[0], arg);

  printf("%s", prefix);
  for (int p = 0; p < prefix_pad_to-prefix_len; p++) {
    printf(" ");
  }

  printf("\t");

  char* desc = argdef_arg_description(arg);
  if (*desc) {
    char short_desc[strlen(desc)];
    char long_desc[strlen(desc)];
    splitdesc(&short_desc[0], &long_desc[0], desc);
    printf("%s ", short_desc);
  }

  if (arg->kind == ARGPARSE_FLAG) {
    printf("(default: ");
    display_flag_default(arg->flag);
    printf(")");
  } else if (arg->kind == ARGPARSE_ENUM) {
    printf(" (options: ");
    display_enum_options(arg->enumeration);
    printf(")");
  }

  printf("\n");
}

static int max_prefix_len(argdef_t* args, int no_opts) {
  int maxl = 0;

  for (int i = 0; i < no_opts; i++) {
    char prefix[100];
    int prefix_len = line_prefix(&prefix[0], args->args[i]);
    if (prefix_len > maxl) {
      maxl = prefix_len;
    }
  }

  return maxl;
}

void display_help_and_quit(argdef_t* args) {
  printf("Usage: litmus.exe [OPTION]... [TEST]...\n");
  printf(DESCRIPTION);
  printf("\n");
  printf("Options: \n");

  int no_opts = argdef_countargs(args);
  int prefix_length = max_prefix_len(args, no_opts);

  for (int i = 0; i < no_opts; i++) {
    display_line_help_for_arg(args->args[i], prefix_length);
  }

  abort();
}

static void display_help_for_arg(const argdef_arg_t* arg) {
  char* desc = argdef_arg_description(arg);
  char short_desc[strlen(desc)];
  char long_desc[strlen(desc)];
  splitdesc(&short_desc[0], &long_desc[0], desc);

  printf("%s\n", short_desc);
  printf("Usage:\n");

  char* short_name = argdef_arg_shortname(arg);
  char* long_name = argdef_arg_longname(arg);
  if (short_name) {
    printf(" %s", short_name);

    switch (arg->kind) {
      case ARGPARSE_OPTION:
        if (arg->option->show_help_both) {
          printf("\n");

          if (! arg->option->only_action)
            printf(" %s", short_name);
        }

        if (! arg->option->only_action)
          printf("X");
        break;
      default:
        break;
    }

    printf("\n");
  }

  if (long_name) {
    printf(" %s", long_name);

    switch (arg->kind) {
      case ARGPARSE_OPTION:
        if (arg->option->show_help_both) {
          printf("\n");

          if (! arg->option->only_action)
            printf(" %s", long_name);
        }

        if (! arg->option->only_action)
          printf("=X");
        break;
      case ARGPARSE_ENUM:
        printf("=");
        display_enum_options(arg->enumeration);
        break;
      default:
        break;
    }

    printf("\n");
  }

  switch (arg->kind) {
    case ARGPARSE_FLAG:
      printf("Default: ");
      display_flag_default(arg->flag);
      printf("\n");
      break;
    default:
      break;
  }

  if (*long_desc) {
    printf("\n");

    printf("%s\n", long_desc);
  }
}

void display_help_for_and_quit(argdef_t* args, char* opt) {
  int no_args = argdef_countargs(args);

  for (int i = 0; i < no_args; i++) {
    char* short_name = argdef_arg_shortname(args->args[i]);
    char* long_name = argdef_arg_longname(args->args[i]);

    if ((short_name && strcmp(1+short_name, opt))
      || (long_name && strcmp(2+long_name, opt))) {
        display_help_for_arg(args->args[i]);
        abort();
    }
  }

  fail("unknown argument '%s'\n", opt);
}

void display_help_show_tests(void) {
  printf("Tests: \n");
  display_test_help();
  printf("\n");
  abort();
}