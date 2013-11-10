

#ifndef tlib_h
#define tlib_h

#include "tobject.h"
#include "tstring.h"



typedef struct funcdef
{
	char *name;
	nativefunc fun;
	int  pcount;
}funcdef;





void setsymbol(symtable* table,tsstr* key,int data);
int findsymbol(symtable *table,tsstr *key);
tsstr* rawgetstr(symtable *table,const byte *data,uint len);
void initsymbol(symtable *table);

void initcst(csttable *table);
word getconstant(csttable *table,uint64 key);
void initenv(envir *e);
void addfunc(envir *e,int pos,tsfunc *fun);


void addbuiltin(envir *e,funcdef fd[]);

#endif

