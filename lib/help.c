#include "lib.h"
#include "argdef.h"

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

  *long_desc = '\0';
}

static char *sprint_metavar(char *out, bool for_short_name, const argdef_arg_t *arg) {
  char *metavar = option_metavar(arg);
  char *prefix = for_short_name ? "" : "=";

  if (option_requires_arg(arg)) {
    out = sprintf(out, "%s%s", prefix, metavar);
  } else if (option_takes_arg(arg)) {
    out = sprintf(out, "[%s%s]", prefix, metavar);
  }

  return out;
}

static int line_prefix(char* out, const argdef_arg_t* arg) {
  bool printed_previous;
  char* start = out;

  char* short_name = argdef_arg_shortname(arg);
  char* long_name = argdef_arg_longname(arg);

  out = sprintf(out, "  ");

  if (short_name != NULL) {
    /* print '-s' */
    out = sprintf(out, "%s", short_name);
    out = sprint_metavar(out, true, arg);
    printed_previous = true;
  } else {
    printed_previous = false;
  }

  if (long_name != NULL) {
    if (printed_previous)
      out = sprintf(out, ", ");

    out = sprintf(out, "%s", long_name);
    out = sprint_metavar(out, false, arg);

    if (arg->kind == ARGPARSE_FLAG && !arg->flag->no_negation)
      out = sprintf(out, "/--no-%s", 2+long_name);
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
  if (*arg->flag) {
    printf("on");
  } else {
    printf("off");
  }
}

static void line_postfix(const argdef_arg_t* arg) {
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
}

static void display_line_help_for_arg(const argdef_arg_t* arg, int prefix_pad_to) {
  char prefix[1024];
  /*
   * --help displays one line per arg
   * e.g.:
   *  -h, --help, --help=<CMD>    Display help and quit
   *  -q, --quiet                 be quiet
   *  -n<N>                       number of runs
   *  --hist/--no-hist            histogram
   *
   * ...
   */

  int prefix_len = line_prefix(&prefix[0], arg);
  printf("%s", prefix);
  for (int p = 0; p < prefix_pad_to-prefix_len; p++) {
    printf(" ");
  }
  printf("\t");
  line_postfix(arg);
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

void display_help_and_quit(void) {
  printf("Usage: %s.exe %s\n", THIS_ARGS->exe_name, THIS_ARGS->short_usage);
  printf("%s\n", THIS_ARGS->description);
  printf("\n");

  int no_common_opts = argdef_countargs(&COMMON_ARGS);
  int no_this_opts = argdef_countargs(THIS_ARGS);

  int common_prefix_length = max_prefix_len(&COMMON_ARGS, no_common_opts);
  int this_prefix_length = max_prefix_len(THIS_ARGS, no_this_opts);
  int prefix_length = this_prefix_length < common_prefix_length ? common_prefix_length  : this_prefix_length;

  printf("Common Options:\n");
  for (int i = 0; i < no_common_opts; i++) {
    display_line_help_for_arg(COMMON_ARGS.args[i], prefix_length);
  }

  printf("\n");

  printf("Mode (%s) Options:\n", THIS_ARGS->exe_name);
  for (int i = 0; i < no_this_opts; i++) {
    display_line_help_for_arg(THIS_ARGS->args[i], prefix_length);
  }
  if (no_this_opts == 0) {
    printf("  <none>\n");
  }

  abort();
}

static void display_help_for_arg(const argdef_arg_t* arg) {
  char* desc = argdef_arg_description(arg);
  char short_desc[strlen(desc)];
  char long_desc[strlen(desc)];
  splitdesc(&short_desc[0], &long_desc[0], desc);

  char* short_name = argdef_arg_shortname(arg);
  char* long_name = argdef_arg_longname(arg);
  char *metavar = option_metavar(arg);

  printf("Usage:\n");


  /* -s */
  if (short_name && !option_requires_arg(arg))
    printf(" %s\n", short_name);

  /* --long */
  if (long_name && !option_requires_arg(arg))
    printf(" %s\n", long_name);

  /* --no-long */
  if (long_name && arg->kind == ARGPARSE_FLAG && !arg->flag->no_negation)
    printf(" --no-%s\n", long_name+2);

  /* -sX */
  if (short_name && option_takes_arg(arg))
    printf(" %s%s\n", short_name, metavar);

  /* --long=X */
  if (long_name && option_takes_arg(arg))
    printf(" %s=%s\n", long_name, metavar);

  printf("\n");

  if (arg->kind == ARGPARSE_ENUM) {
    printf("Where %s is one of ", metavar);
    display_enum_options(arg->enumeration);
    printf("\n");
    printf("\n");
  }

  switch (arg->kind) {
    case ARGPARSE_FLAG:
      if (! arg->flag->has_action) {
        printf("Default: ");
        display_flag_default(arg->flag);
        printf("\n");
      }
      break;
    default:
      break;
  }

  if (*long_desc) {
    printf("Description:\n");
    printf(" %s\n", long_desc);
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
}

void display_help_show_tests(void) {
  printf("Tests: \n");
  display_test_help();
  printf("\n");
  abort();
}