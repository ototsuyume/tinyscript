

#ifndef tarray_h
#define tarray_h

#include "tobject.h"

tsarray *ts_newarray();
tsarray *ts_newarray2(uint size);
void ts_initarray(tsarray *arr);
int ts_appendarray(tsarray *arr,tsval *val);
int ts_getarray(tsarray *arr,int pos,tsval *val);
int ts_delarray(tsarray *arr,int pos);
int ts_setarray(tsarray *arr,int pos,tsval *val);
tsarray* ts_arrayadd(const tsarray *a,const tsarray *b);
tsarray *ts_arraymul(const tsarray *a,int c);

#endif