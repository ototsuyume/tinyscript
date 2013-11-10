

#ifndef tstring_h
#define tstring_h

#include "tobject.h"


tsstr* ts_newstring(envir *env,const byte *s,uint len);
tsstr* ts_stringadd(const tsstr *a,const tsstr *b);
tsstr* ts_stringmul(const tsstr *a,int c);


#endif