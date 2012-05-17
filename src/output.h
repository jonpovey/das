#ifndef OUTPUT_H
#define OUTPUT_H
/*
 * output messages and location tracking.
 * perhaps needs a bit of a rethink and refactoring.
 */
#include <stdio.h>

extern int das_error;

/*
 * source file location-tracking type for error reporting
 */
typedef struct loctype {
	int line;
	// later maybe start-end characters, file(name) reference
} LOCTYPE;
#define LOCFMT "line %2d"	/* for printf */

extern struct outopts {
	int stack_style_sp;
	int omit_dump_header;
} outopts;

/* would be nice to always evaluate args in case of side-effects */
#define info(fmt, args...) do { \
	if (options.verbose) \
		printf(fmt, ##args); \
} while (0)

/*
 * error and warning ugly macros:
 * _warn()      appends \n and writes to stderr
 * _error()     calls _warn() and sets das_error.
 * warn()       prepends "Warning: " and calls _warn()
 * error()      prepends "Error: " and calls _error()
 * loc_warn()   prepends location info, "Warning: ", calls _warn()
 * loc_err()    blah blah blah you get the idea
 */
#define _warn(fmt, args...) fprintf(stderr, fmt "\n", ##args)

#define _error(fmt, args...) do { \
	_warn(fmt, ##args); \
	das_error = 1; \
} while (0)

#define error(fmt, args...) _error("Error: " fmt, ##args)
#define warn(fmt, args...) _warn("Warning: " fmt, ##args)

#define loc_err(loc, fmt, args...) \
	_error("line %2d: Error: " fmt, loc.line, ##args)
#define loc_warn(loc, fmt, args...) \
	_warn("line %2d: Warning: " fmt, loc.line, ##args)

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
