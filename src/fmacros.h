#ifndef __EAGLE_MACROS_H__
#define __EAGLE_MACROS_H__

#define _BSD_SOURCE

#if defined(__linux__)
	#define _GNU_SOURCE
#endif

#if defined(__linux__) || defined(__OpenBSD__)
	#define _XOPEN_SOURCE 700
#else
	#define _XOPEN_SOURCE
#endif

#endif
