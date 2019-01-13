#ifndef __UTIL_H__
#define __UTIL_H__

#ifdef NDEBUG
#define INTERNAL_ASSERT(cond) 
#else
#define INTERNAL_ASSERT(cond)   \
    do {                        \
        if (!(cond)) {              \
            (void)fprintf(stderr,               \
                    "%s:%d: Assertion %s failed in %s",     \
                    __FILE__,__LINE__,#cond,__func__);      \
        }                       \
    } while (0);

#endif

#endif //__UTIL_H__
