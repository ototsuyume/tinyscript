#include <stdlib.h>
#include "tlib.h"
#include "tarray.h"
#include "terror.h"
#include "tvm.h"
#include "tmath.h"
#include "tio.h"



extern int  genhash(const byte *data,int len);
extern void initfunc(tsfunc *f);


tsstr* rawgetstr(symtable *table,const byte *data,uint len)
{

	uint x;
	uint size = table->size;

	x = genhash(data,len);
	while(table->slot[x%size].key)
	{
		if(strcmp((char*)data,(char*)table->slot[x%size].key->buf)==0)
		{
			return table->slot[x%size].key;
		}
		++x;
	}
	return NULL;
}


int findsymbol(symtable *table,tsstr *key)
{
	uint size = table->size;
	int x;

	x = genhash(key->buf,key->len);

	while(table->slot[x%size].key)
	{
		if(strcmp((char*)key->buf,(char*)table->slot[x%size].key->buf)==0)
			return table->slot[x%size].data;
		++x;
	}


	return -1;
}


void rehashsymbol(symtable* table)
{
	uint size = table->size*2;
	symnode *n = (symnode*)malloc(sizeof(symnode)*size);
	symnode *tmp = table->slot;
	memset(n,0,size*sizeof(symnode));
	table->slot = n;
	n = table->tail;
	table->head = table->tail = NULL;
	while(n)
	{
		setsymbol(table,n->key,n->data);
		n=n->next;
	}
	free(tmp);
}

void setsymbol(symtable* table,tsstr* key,int data)
{
	symnode *n;
	uint x;
	uint size = table->size;
	if(table->used*2>table->size)
		rehashsymbol(table);
	x = genhash(key->buf,key->len);
	while(table->slot[x%size].key)
	{
		if(strcmp((char*)(key->buf),(char*)(table->slot[x%size].key->buf))==0)
		{
			table->slot[x%size].data = data;
			return;
		}
		++x;
	}

	n = &table->slot[x%size];
	n->key   = key;
	if(data == -1)
		n->data  = table->used;
	else
		n->data  = data;

	++table->used;

	if(table->head==NULL)
	{
		table->head = table->tail = n;
		n->next = NULL;
	}
	else
	{
		table->tail->next = n;
		n->next = NULL;
		table->tail = n;
	}
}



void initsymbol(symtable *table)
{
	table->size = INITIAL_TABLE_SIZE;
	table->used = 0;
	table->tail = table->head = NULL;
	table->slot = (symnode*)malloc(sizeof(symnode)*INITIAL_TABLE_SIZE);
	memset(table->slot,0,sizeof(symnode)*INITIAL_TABLE_SIZE);
}






static void setcst(csttable* table,uint64 key,int data)
{
	cstnode *n;
	uint x;
	uint size = table->size;

	x = genhash((byte*)&key,sizeof(uint64));
	while(table->slot[x%size].key)
	{
		if(table->slot[x%size].key==key)
		{
			table->slot[x%size].data = data;
			return;
		}
		++x;
	}

	n = &table->slot[x%size];
	n->key   = key;
	n->data  = data;

	if(table->head==NULL)
	{
		table->head = table->tail = n;
		n->next = NULL;
	}
	else
	{
		table->tail->next = n;
		n->next = NULL;
		table->tail = n;
	}
}


static void rehashcst(csttable* table)
{
	uint size = table->size*2;
	cstnode *n = (cstnode*)malloc(sizeof(cstnode)*size);
	cstnode *tmp = table->slot;
	memset(n,0,size*sizeof(cstnode));
	table->slot = n;
	n = table->tail;
	table->head = table->tail = NULL;
	while(n)
	{
		setcst(table,n->key,n->data);
		n=n->next;
	}
	free(tmp);
}


void initcst(csttable *table)
{
	table->size = INITIAL_TABLE_SIZE;
	table->used = 0;
	table->tail = table->head = NULL;
	table->slot = (cstnode*)malloc(sizeof(cstnode)*INITIAL_TABLE_SIZE);
	memset(table->slot,0,sizeof(cstnode)*INITIAL_TABLE_SIZE);
}

word getconstant(csttable *table,uint64 key)
{
	uint size;
	uint x;
	cstnode *n;
	size = table->size;
	x = genhash((byte*)&key,sizeof(key));
	while(table->slot[x%size].key!=0)
	{
		if(table->slot[x%size].key==key)
		{
			return table->slot[x%size].data;
		}
		++x;
	}
	if(table->used*2>table->size)
		rehashcst(table);
	table->slot[x%size].key  = key;
	table->slot[x%size].data = table->used;
	n = &table->slot[x%size];
	if(table->head==NULL)
	{
		table->head = table->tail = n;
		n->next = NULL;
	}
	else
	{
		table->tail->next = n;
		n->next = NULL;
		table->tail = n;
	}
	return table->used++;

}

void initenv(envir *e)
{
	initsymbol(&e->strt);
	initsymbol(&e->funt);
	ts_initarray(&e->funlist);
	initcst(&e->cstt);
	initfunc(&e->global);
	e->global.e = e;
	ts_init_iolib(e);
	ts_init_mathlib(e);
}

void addfunc(envir *e,int pos,tsfunc *fun)
{
	tsval v;
	setfunc(&v,fun);
	ts_setarray(&e->funlist,pos,&v);
}
/*////////////////////////////////////////////////////////////////////////////*/


void addbuiltin(envir *e,funcdef fd[])
{
	tsstr *pname;
	tsfunc *func;
	tsval v;
	int pos,i=0;
	memset(&v,0,sizeof(tsval));
	while(fd[i].name[0]!=0)
	{
		pname =  ts_newstring(e,(byte*)(fd[i].name),(uint)strlen((char*)fd[i].name));
		func  =  ts_newfunc(pname,e,1);
		func->functype = native;
		func->funbody  = fd[i].fun;
		func->paramnum = fd[i].pcount;
		v.t = type_func;
		v.func = func;
		pos = e->funt.used;	
		setsymbol(&e->funt,pname,pos);
		ts_appendarray(&e->funlist,&v);
		++i;
	}
}