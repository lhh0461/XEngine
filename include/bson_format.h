#ifndef __BSON_FORMAT__
#define __BSON_FORMAT__

#ifdef __cplusplus
extern "C" {
#endif

#include <bson.h>
#include <Python.h>

int pydict_to_bson(PyObject *dict, bson_t * out);
int bson_to_pydict(const bson_t *b, PyObject *dict);

#define MAX_KEY_LEN			(100)
#define MAX_KEY_DEPTH		(50)

#define MAX_CAT_KEY_LEN		(MAX_KEY_LEN * MAX_KEY_DEPTH)

#define	I_FORMAT		("__i__%d")
#define F_FORMAT		("__f__%f")

#define I_LPC_T_BSON(buf, len, i)		\
		memset(buf, 0, len); 			\
		snprintf(buf,len,I_FORMAT,i); 	\
			

#define F_LPC_T_BSON(buf, len, f)		\
		memset(buf, 0, len); 			\
		snprintf(buf,len,F_FORMAT,f); 	\


#define QTZ_BSON_PRINT(b)	{\
	char *tmpmsg = bson_as_json(b, NULL); \
	printf("%s:%s,%s\n", __FILE__, __func__, tmpmsg); \
	fflush(stdout);\
	bson_free(tmpmsg); \
	}\

void format_test();

#ifdef __cplusplus
}
#endif

#endif //__BSON_FORMAT__

