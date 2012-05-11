#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdio.h>

extern int das_error;

extern struct outopts {
	int stack_style_sp;
} outopts;

/* wrap fprintf as print() so extra things may be added if wanted */
#define print(to, fmt, args...) fprintf(to, fmt, ##args)

/* would be nice to always evaluate args in case of side-effects */
#define info(fmt, args...) do { \
	if (options.verbose) \
		printf(fmt, ##args); \
} while (0)

/* fixme: better errors and warnings, with line numbers and such */
#define error(fmt, args...) do { \
	fprintf(stderr, "error: " fmt "\n", ##args); \
	das_error = 1; \
} while (0)

#define warn(fmt, args...) do { \
	fprintf(stderr, "warning: " fmt "\n", ##args); \
} while (0)

/* report internal bugs */
#define BUG_ON(x) ({ int r = !!(x); \
	if (r) { \
		fprintf(stderr, "%s:%d BUG_ON(%s)\n", __FILE__, __LINE__, #x); \
		das_error = 1; \
	} \
	r; \
})

#define BUG() fprintf(stderr, "BUG at %s:%d in %s()\n", \
						__FILE__, __LINE__, __func__);

#ifdef DEBUG
  #define DBG(fmt, args...) fprintf(stderr, fmt, ##args)
#else
  #define DBG(fmt, args...) (void)0
#endif

#define DBG_FUNC(fmt, args...) DBG("%s: " fmt, __func__, ##args)

#define DBG_MEM(fmt, args...) (void)0

#define TRACE0(fmt, args...) (void)0

//#define TRACE1(fmt, args...) (void)0
#define TRACE1(fmt, args...) DBG(fmt, ##args)

#define TRACE2(fmt, args...) (void)0

#endif
