#ifndef LITMUS_PROT_H
#define LITMUS_PROT_H

typedef enum {
  PROT_AP,
  PROT_ATTRIDX,
} prot_type_t;

char* prot_type_to_str(prot_type_t prot);

#endif /* LITMUS_PROT_H */