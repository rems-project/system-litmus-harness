#include "lib.h"
#include "testlib.h"

#define __ASSERT_HASH_SHA1(D, MESSAGE, EXPECTED_HEXDIGEST) \
  char s[41];                                              \
  hexdigest(&D, s);                                        \
  ASSERT(                                                  \
    strcmp(s, EXPECTED_HEXDIGEST) == 1,                    \
    "b'%s' expected to hash to '%s' but hashed to '%s'.",  \
    MESSAGE,                                               \
    EXPECTED_HEXDIGEST,                                    \
    s                                                      \
  );

#define ASSERT_HASH_SHA1(MESSAGE, EXPECTED_HEXDIGEST)   \
  ({                                                    \
    digest d = hash_sha1(MESSAGE, sizeof(MESSAGE) - 1); \
    __ASSERT_HASH_SHA1(d, MESSAGE, EXPECTED_HEXDIGEST); \
  })

UNIT_TEST(test_sha1_NIST_examples)
void test_sha1_NIST_examples(void) {
  ASSERT_HASH_SHA1("abc", "a9993e364706816aba3e25717850c26c9cd0d89d");
  ASSERT_HASH_SHA1(
    "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "84983e441c3bd26ebaae4aa1f95129e5e54670f1"
  );
}

UNIT_TEST(test_sha1_empty)
void test_sha1_empty(void) {
  ASSERT_HASH_SHA1("", "da39a3ee5e6b4b0d3255bfef95601890afd80709");
  ASSERT_HASH_SHA1("\0", "5ba93c9db0cff93f52b521d7420e43f6eda2784f");
}

static digest hash_sha1_b64(const char* msg, u64 len) {
  char data[len * 8];
  u64 data_len = b64decode(msg, len, data);
  return hash_sha1(data, data_len);
}

#define ASSERT_HASH_SHA1_BASE64(MESSAGE, EXPECTED_HEXDIGEST) \
  ({                                                         \
    digest d = hash_sha1_b64(MESSAGE, sizeof(MESSAGE) - 1);  \
    __ASSERT_HASH_SHA1(d, MESSAGE, EXPECTED_HEXDIGEST);      \
  })

UNIT_TEST(test_sha1_random_shortmsg)
void test_sha1_random_shortmsg(void) {
  ASSERT_HASH_SHA1_BASE64(
    "4AOcJe3kMvI3Ow6fh51U++Iop5/eWdEBdkxCioh6qmgBtoIu+lWW69iTNHg2"
    "Vn1vZWJNowp0HQ64kWMeD69thQ==",
    "53beceef668d7d44c96a30f3a0c9f3431a2d64d5"
  );

  ASSERT_HASH_SHA1_BASE64(
    "gnLguJ7B+ewpLwx4AAVXApo1b3d1zKWw0CcBlzpSFgfSwTuWsSpzpJkVJWTd"
    "oWpkVwkezk1KPoVVu9IQwYpjzI0Ft7+XFUSTPEByGwlDa1rtTasu/O2K2svZ"
    "wtf4zYahDCwPnPa3xOi6v/LX1CNcMl4ZZZE2Rw0uAD62HPhRTg0=",
    "5bf5f7acdf84e92e4a8018981cd13d50d9f674e1"
  );

  ASSERT_HASH_SHA1_BASE64(
    "YgZmG+fApAlMSulKSwXg+VEHEy/iPOUQLOnBDS1F45rwbfEA62RIWA/iIF7i"
    "kHz6MEQSFUzWDJh2AmvDf50gcM3Znlndw2oU5aRQTHHpK39qMCv/dPIfTD3q"
    "KO21hz+dLT9IX/knygnySxmA9pyOTZWvIvcm81Fuq9k83R3gM3MGl4jowDjt"
    "J7RipEAgopIf0QvCzMxZ7jSAGfMg+k+RO2apgv/a1KvD/28AIJMH/yyccyGz"
    "i7tuPLwVzDhi9/A4",
    "b0f7c6413f7f2a23b8abd832feb7fba5dacd30e2"
  );

  ASSERT_HASH_SHA1_BASE64(
    "YYctfVlOQ+SweDWvLm53CnylLh57giS8wxEn7Loy5RrdesjlWeuyAL886TUv"
    "lQiSyEehUNbejKiiJ2GhySaA9B+URweCdb0tTwAAPVYymQBha5uMyGQ3XONY"
    "U/KtjouFtAQ/pVORfflpqB/Nx+eBY/x7xs3L/xInRzzrpdn+2MpNJ0qXt1IA"
    "/L40H1Kus73DcCy5bdRDFRCWdSvCiT9jwdVkaNdWDityiDrr4hAPSL6GyuZC"
    "VPntBAAtK6UQtIW2aG7gE4cRnqbNmWl9AigCt61yACHvjpBIcyD1quCkqh5F"
    "GUHQS2D7EKu8o/FoGZCjB/AahQYXhEcIpL6ZDvKgXQ==",
    "a0a19b290bd050b2b9e71f323dedf63ebc105106"
  );

  ASSERT_HASH_SHA1_BASE64(
    "nMPZMCvO4zDDn48Jvlc8SnqpOFNdrCYiQ3o4uQLhPNpo0vIni5jAf5MIJ6Qr"
    "zZeMhZVi+REJ5THh63jFzLu/x0zH0xQGFV9zyIVPkkiksyl8/fEZOf4REJ3y"
    "YY6OpqvLR5BphYM9z1/WjrU4u1uhcCzPaH9yzzy0wEGuPpRr0qYyqMwuL0k6"
    "cloUFEtRLa3KYVsBvgm6qfBJrJLO5fjlDbJigUOjmemkaRdGTFzbGORkoP+3"
    "ZBw4NoCFqJR6LsUoaXQulMKbtsu4/zeLeeaDKOLWZEsMBr1j39HOJtSk5qqE"
    "qbZ2VmqNNyY0p/AAYQ7p9CZKB8QCLSjXsWtIfW08kpSmAXU8BxMgaTdu0c9i"
    "1iTZSYfn4CFfZozNrYdsNOTz4VRvzbZjh+O9wvGmQPcDakpPg7eefePUM68k"
    "Z6y03jo=",
    "1614af233c8ee0740539c19daab241eed998955b"
  );

  ASSERT_HASH_SHA1_BASE64(
    "uiwjvh4Pt2VPhTUDF3YQSX9Q5dDtz3C51/LoxPEezWFl8REZqasymsp/5LoQ"
    "bkamOEjwZRdVuxd5HcXDTie7bZImHGKgWhFjNSIRKCh0kpVBfIueTjSJrgfl"
    "+/FWzedvHx9oP8t168KIRa2ceK8YFaVfVp1UMlIt6YdYFZFymhyPZzywcQmH"
    "lpAnOr6uLjdfEevBjh82i/ZUSGUYHZj0qdD1sjUGNgXUZUEntjOCN3jdQKZL"
    "nThuyVh8XTxQvufp43kzLHQBRl1HNYqgTC2KupvluAJAPtgcYVcAtL0WiQyg"
    "Y+cisGbDQMO4GOxBBED8/jujmgidG8lbY9EWvYWhUcPSf6xPCRPtLFs3cPMv"
    "O/Ud14GqrQoZ6HzMAGkVCAAFRlVO/nqckz1LZdPBhss8mEnq6k6g4XT3Zg/2"
    "xrlfCftPLyqt1R+79uIGY6dxwmHTRFkN+I6jP5Czz2uy2EZ/GD6QqAsMItEW"
    "uvbv7D9+5x4BJ9GefmBcFRI3enkOVmM3",
    "977bb04cb0f9990d11f05cb821c1e0f71ad3b8b5"
  );

  ASSERT_HASH_SHA1_BASE64(
    "fwSw1E+HeRKEnVnyPEz9l1YdBPY3O8+Raat+4rXT+7UQ1nRrsxK+EcvoZ3NU"
    "IVIPeOjUzcpp91NRjUGNXIU8M9TI/5lTPyUpD8O7AJntONnPF1EVoNnfNRy6"
    "CZdDyUxC6WCLI/dyC9tb93nscWMidjHr434/82eBSa5rTAj9Mvyj5wD3MlWQ"
    "ZofFvD6xuMtzGYs6/56wGYG2MDfr3XyKT1rwaACyn2m6a7BTee3/9SzkUpte"
    "9E/L8fU3C+Fg2r40qhrRxGkEhzCusNOWASgK7rYViBvGy4xwDG/2emUgj2n1"
    "mxNgObCkjmXPERCmD6TuyFk3kyfdJ/l/0KfDU8L031WpYhG+a5WiWnHQsGQu"
    "5Ye4wQeUFOK1ctkXMiI6/GMPpm1KF5tVhAaGM4r/3rmq8TLSrVruwuSd9H2E"
    "WsxCD31/DCEZNzWS4J3LHglZDdujD4uEG89r/WOgAmVyWU6ZvqUBQ89y3tyw"
    "5OFYqjYul0qqrsuzSuNQif/zudA08uayf4GUEVYcQRsntYvA+PUm+1DB876n"
    "8yQM7AB3JKeBdJPHzNphLH5Ft9SULupSRelCBAs3DoOhyMzNfVN8qshiHg==",
    "bd34ad374985681e5eaaeecad004f27520f7ca18"
  );
}