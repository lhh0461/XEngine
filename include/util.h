#ifndef __UTIL_H__
#define __UTIL_H__

/*
#define TIME_TAG_BEGIN(sw, what)    \
    struct timeval what##_begin, what##_end, what##_dif; \
    if ((sw)) { \
        gettimeofday(&what##_begin, NULL);  \
    }
#define TIME_TAG_END(sw, what,fmt,args...) \
    if ((sw)) {\
        gettimeofday(&what##_end, NULL); \
        timersub(&what##_end, &what##_begin, &what##_dif); \
        fprintf(stderr, #what " timediff sec=%ld,usec=%d,"fmt"\n", what##_dif.tv_sec, what##_dif.tv_usec, ##args);\
    }
    */


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
