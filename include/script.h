#ifndef __SCRIPT__
#define __SCRIPT__

void init_python_vm();
void destroy_python_vm();
int call_script_function(const char *module, const char *function, const char *buf, int len);

#endif //__SCRIPT_H__
