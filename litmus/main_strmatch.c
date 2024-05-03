#include "lib.h"
#include "frontend.h"

#define LOWER(c) (((c) < 90 && (c) > 65 ? (c)+32 : (c)))

/** estimate a numeric difference between two strings
 * horribly inefficient but who cares
 *
 * This is optimized for litmus test names, which are often the same jumble of letters but in slightly different orders
 */
static int strdiff(const char* w1, const char* w2) {
  int sw1 = strlen(w1);
  int sw2 = strlen(w2);
  int diff = 0;
  for (int i = 0; i < sw1; i++) {
    char c, d, e;

    if (w2[i] == '\0') {
      return diff + 10*(sw1 - sw2);
    }

    c = d  = e = w2[i];
    if (0 < (i - 1) && (i - 1) < sw2) {
      c = w2[i - 1];
    }

    if ((i + 1) < sw2) {
      e = w2[i + 1];
    }

    #define CMPCHR(a,b) \
        (a==b ? 0 : \
          (LOWER(a)==LOWER(b) ? 1 : \
            ((a == '+' || b == '+') ? 2 : \
              (MAX(LOWER(a),LOWER(b))-MIN(LOWER(a),LOWER(b))))))

    d = w2[i];
    char x = w1[i];
    diff += MIN(CMPCHR(x,d), MIN(1+CMPCHR(x,c), 1+CMPCHR(x,e)));
  }

  return diff + sw2 - sw1;
}

/** if a test cannot be found
 * find a nearby one and say that instead
 */
static const char* __find_closest_str(const litmus_test_group* grp, const char* arg) {
  char const* smallest = NULL;
  int small_diff = 0;

  for (int i = 0; i < grp_num_tests(grp); i++) {
    char* name = (char*)grp->tests[i]->name;
    int diff = strdiff(arg, name);
    if (0 < diff) {
      if (smallest == NULL || diff < small_diff) {
        smallest = name;
        small_diff = diff;
      }
    }
  }

  if (grp->name) {
    int diff = strdiff(arg, (char*)grp->name);
    if (0 < diff) {
      if (smallest == NULL || diff < small_diff) {
        smallest = grp->name;
        small_diff = diff;
      }
    }
  }

  for (int i = 0; i < grp_num_groups(grp); i++) {
    const char* _sub_smallest = __find_closest_str(grp->groups[i], arg);
    int diff = strdiff(arg, (char*)_sub_smallest);
    if (0 < diff) {
      if (smallest == NULL || diff < small_diff) {
        smallest = _sub_smallest;
        small_diff = diff;
      }
    }
  }

  return smallest;
}

void print_closest(const litmus_test_group* grp, re_t* arg) {
  const char* smallest = __find_closest_str(grp, arg->original_expr);

  if (smallest != NULL) {
    printf("Did you mean \"%s\" ?\n", smallest);
  }
}
