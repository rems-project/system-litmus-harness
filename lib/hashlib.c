#include "lib.h"

struct sha1_state
{
  unsigned int h[5];
  u32* W;
  u32* K;
};

struct sha1_state INITIAL_STATE = {
  .h = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0 },
};

/**
 * Return a new stream, where buf has been split into N 512-bit blocks
 * each with appropriate padding
 */
u64 preprocess(const char* buf, u64 bufsize, char** out) {
  unsigned long l = bufsize * 8;
  unsigned long new_l = ALIGN_UP(l + 64 + 1, 9);
  unsigned long new_size = new_l >> 3;
  char* new_buf = ALLOC_SIZED(new_size);

  unsigned int b = 0;
  for (; b < bufsize; b++) {
    new_buf[b] = buf[b];
  }

  /* write 1 */
  new_buf[b++] = 0x80;

  /* write remaining zeros */
  for (; b < new_size - 8; b++) {
    new_buf[b] = 0;
  }

  /* write l (big-endian required) */
  write_be64(&new_buf[b], l);

  *out = new_buf;
  return (new_l >> 9);
}

#define M(MSG, T, I) ((u32*)&(MSG)[(I)*64])[T]

#define ROTLONE(X) ((X) << 1) | ((X) >> 31)

u32 rotl(int n, u32 x) {
  for (int i = 0; i < n; i++)
    x = ROTLONE(x);
  return x;
}

void prepare_initial_state(struct sha1_state* st) {
  for (int i = 0; i < 5; i++) {
    st->h[i] = INITIAL_STATE.h[i];
  }

  st->W = ALLOC_SIZED(80 * sizeof(u32));
  st->K = ALLOC_SIZED(80 * sizeof(u32));
}

void prepare_message_schedule_W(char* message, u32 W[80], unsigned int i) {
  int t = 0;
  for (; t < 16; t++) {
    W[t] = read_be((char*)&M(message, t, i));
  }

  for (; t < 80; t++) {
    W[t] = rotl(1, W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16]);
  }
}

void prepare_constants_K(u32 K[80]) {
  int t = 0;

  for (; t < 20; t++)
    K[t] = 0x5a827999;

  for (; t < 40; t++)
    K[t] = 0x6ed9eba1;

  for (; t < 60; t++)
    K[t] = 0x8f1bbcdc;

  for (; t < 80; t++)
    K[t] = 0xca62c1d6;
}

u32 Ch(u32 x, u32 y, u32 z) {
  return (x & y) ^ ((~x) & z);
}

u32 Parity(u32 x, u32 y, u32 z) {
  return x ^ y ^ z;
}

u32 Maj(u32 x, u32 y, u32 z) {
  return (x & y) ^ (x & z) ^ (y & z);
}

u32 f(int t, u32 x, u32 y, u32 z) {
  if (0 <= t && t <= 19)
    return Ch(x, y, z);
  else if (20 <= t && t <= 39)
    return Parity(x, y, z);
  else if (40 <= t && t <= 59)
    return Maj(x, y, z);
  else if (60 <= t && t <= 79)
    return Parity(x, y, z);
  else
    unreachable();
}

void write_digest(digest* d, struct sha1_state st) {
  char* p = (char*)&d->digest[0];

  for (int i = 0; i < 5; i++)
    write_be(p + 4 * i, st.h[i]);
}

void hexdigest(digest* d, char* out) {
  const char hex[] = "0123456789abcdef";
  char* p = (char*)&d->digest[0];
  for (int i = 0; i < 20; i++) {
    out[2 * i + 0] = hex[(p[i] >> 4) & 0xf];
    out[2 * i + 1] = hex[(p[i]) & 0xf];
  }
  out[40] = '\0';
}

void dump_W_block(u32 W[80]) {
  printf("==============================================================\n");
  printf("Block Contents:\n");
  for (int t = 0; t < 16; t++)
    printf("W[%d] = %x\n", t, W[t]);
  printf("\n");
  printf("          A          B          C          D          E\n");
}

void dump_H(struct sha1_state* st) {
  printf("==============================================================\n");
  printf("Initial hash value:\n");
  for (int i = 0; i < 5; i++)
    printf("H[%d] = %x\n", i, st->h[i]);
}

void dump_H_update(struct sha1_state* st, u32 d[5]) {
  printf("\n");
  for (int i = 0; i < 5; i++)
    printf("H[%d] = %x + %x = %x\n", i, st->h[i], d[i], st->h[i] + d[i]);
}

void dump_digest(digest* d) {
  char s[41];
  printf("--------------------------------------------------------------\n");
  printf("Message Digest is\n");
  hexdigest(d, s);
  printf("                 %s\n", s);
}

digest hash_sha1(const char* buf, u64 bufsize) {
  char* message;
  u64 N = preprocess(buf, bufsize, &message);

  struct sha1_state st;
  prepare_initial_state(&st);

  if (DEBUG && DEBUG_HASHLIB) {
    dump_H(&st);
  }

  prepare_constants_K(st.K);
  for (unsigned int i = 0; i < N; i++) {
    prepare_message_schedule_W(message, st.W, i);

    if (DEBUG && DEBUG_HASHLIB) {
      dump_W_block(st.W);
    }

    u32 a = st.h[0];
    u32 b = st.h[1];
    u32 c = st.h[2];
    u32 d = st.h[3];
    u32 e = st.h[4];

    for (int t = 0; t < 80; t++) {
      u32 T = rotl(5, a) + f(t, b, c, d) + e + st.K[t] + st.W[t];
      e = d;
      d = c;
      c = rotl(30, b);
      b = a;
      a = T;

      if (DEBUG && DEBUG_HASHLIB) {
        printf("t=%d: %x  %x  %x  %x  %x\n", t, a, b, c, d, e);
      }
    }

    if (DEBUG && DEBUG_HASHLIB) {
      u32 update[] = { a, b, c, d, e };
      dump_H_update(&st, update);
    }

    st.h[0] += a;
    st.h[1] += b;
    st.h[2] += c;
    st.h[3] += d;
    st.h[4] += e;
  }

  FREE(st.K);
  FREE(st.W);
  FREE(message);

  digest d;
  write_digest(&d, st);

  if (DEBUG && DEBUG_HASHLIB) {
    dump_digest(&d);
  }

  return d;
}