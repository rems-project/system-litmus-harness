/* this file is a single compilation unit that
 * defines a single function that outputs a version string
 * that can be used elsewhere
 */

#include "lib.h"

#define BUILD __BUILD_STR__
#define VERSION __VERSION_STR__
#define BUILD_STRING BUILD "/" COMMITHASH

const char* version_string(void) {
  return VERSION;
}

const char* build_string(void) {
  return BUILD_STRING;
}