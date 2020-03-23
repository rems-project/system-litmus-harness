#ifndef CONFIG_H
#define CONFIG_H

/* global configuration */
extern uint64_t NUMBER_OF_RUNS;
extern uint8_t ENABLE_PGTABLE;

extern uint8_t DEBUG;
extern uint8_t TRACE;

void display_help_and_quit(void);
void read_args(int argc, char** argv);

#endif /* CONFIG_H */