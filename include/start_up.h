#ifndef __START_UP_H__
#define __START_UP_H__

extern struct event_base *ev_base;

typedef void (*module_fun_t)(void); 
                                                                
extern module_fun_t module_init_fun;
extern module_fun_t module_startup_fun;

void init();
void startup();

#endif //__START_UP_H__

