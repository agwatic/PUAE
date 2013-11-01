// TODO: insert header

#ifndef UAE_PPAPI_H_
#define UAE_PPAPI_H_

#include "ppapi/c/ppb_instance.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t NaCl_LoadUrl(const char* name, char** data);
const void *NaCl_GetInterface(const char *interface_name);
PP_Instance NaCl_GetInstance(void);

#ifdef __cplusplus
}
#endif

#endif // UAE_PPAPI_H_
