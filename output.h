#ifndef OUTPUT_H
#define OUTPUT_H

// TODO

/* wrap fprintf as print() so extra things may be added if wanted */
#define print(to, fmt, args...) fprintf(to, fmt, ##args)

#ifdef DEBUG
  #define DBG(fmt, args...) printf(fmt, ##args)
#else
  #define DBG(fmt, args...) (void)0
#endif

#define DBG_MEM(fmt, args...) (void)0

#endif
