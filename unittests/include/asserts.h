#ifndef ASSERTS_H
#define ASSERTS_H

/** if check is false, then raise failure as msg
 */
#define ASSERT2(check, fmt, ...)                           \
  do {                                                     \
    if (!(check)) {                                        \
      if (fmt != NULL) {                                   \
        STREAM* out = NEW_BUFFER(current_test->msg, 1024); \
        sprintf(out, fmt, ##__VA_ARGS__);                  \
      }                                                    \
      current_test->result = 0;                            \
      return;                                              \
    } else {                                               \
      current_test->result = 1;                            \
    }                                                      \
  } while (0);

#define ASSERT(check, ...) ASSERT2(check, ##__VA_ARGS__, NULL)

#endif /* ASSERTS_H */