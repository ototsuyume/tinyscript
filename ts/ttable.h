

#ifndef ttable_h
#define ttable_h

#include "tobject.h"

tstable *ts_newtable();
int genhash(const byte *data,int len);
void ts_settable(tstable* table,tsval* key,tsval* data);
void ts_deltable(tstable* table,tsval* key);
int  ts_findtable(tstable *table,tsval *key,tsval *data);
int  ts_rawfindtable(tstable *table,byte *data,uint len,tsval *key,tsval *val);
void ts_inittable(tstable *table);



#endif