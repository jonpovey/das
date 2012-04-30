#ifndef OUTPUT_H
#define OUTPUT_H

/* wrap fprintf as print() so extra things may be added if wanted */
#define print(to, fmt, args...) fprintf(to, fmt, ##args)

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
  #define DBG(fmt, args...) printf(fmt, ##args)
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
