#include <stdlib.h>
#include "ttable.h"

/************************************table manipulation functions***********************************************/
int genhash(const byte *data,int len)
{
	uint s;
	int  ret;
	s = len;
	ret = (*data+1)<<7;
	while(--len >= 0)
		ret = (1000003*ret)^*data++;
	ret^=s;
	if(ret == -1)
		ret = -2;
	return ret;
}

int ts_genhash(tsval *key)
{
	uint len;
	uint ret;
	byte* data;

	if(isfloat(key))
	{
		data = (byte*)&getfloat(key);
		len  = sizeof(double);
	}
	else if(isint(key))
	{
		data = (byte*)&getint(key);
		len  = sizeof(int);
	}
	else if(isstr(key))
		return getstring(key)->hash;
	else /* unhashable type */	
		return -1;

	ret = genhash(data,len);
	if(ret == -1)
		ret = -2;
	return ret;
}


int ts_findtable(tstable *table,tsval *key,tsval *data)
{
	uint size = table->size;
	uint x;

	x = ts_genhash(key);
	if(x==-1)
		return 0;
	while(table->slot[x%size].used)
	{
		if(ts_cmp(key,&table->slot[x%size].key))
		{
			*data = table->slot[x%size].data;
			return 0;
		}
		++x;
	}
	return 1;
}


static void ts_rehash(tstable* table)
{
	uint size = table->size*2;
	tnode *n = (tnode*)malloc(sizeof(tnode)*size);
	tnode *tmp = table->slot;
	memset(n,0,size*sizeof(tnode));
	table->slot = n;
	n = table->tail;
	table->head = table->tail = NULL;
	while(n)
	{
		ts_settable(table,&n->key,&n->data);
		n=n->next;
	}
	free(tmp);
}

void ts_settable(tstable* table,tsval* key,tsval* data)
{
	tnode *n;
	uint x;
	uint size = table->size;
	if(table->used*2>table->size)
		ts_rehash(table);
	x = ts_genhash(key);
	while(table->slot[x%size].used)
	{
		if(ts_cmp(key,&table->slot[x%size].key)==0)
		{
			table->slot[x%size].data = *data;
			table->slot[x%size].used = 1;
			return;
		}
		++x;
	}

	n = &table->slot[x%size];
	n->key   = *key;
	n->data  = *data;
	n->used  = 1;
	++table->used;
	if(table->head==NULL)
	{
		table->head = table->tail = n;
		n->pre=n->next = NULL;
		table->enumptr = table->head;
	}
	else
	{
		table->tail->next = n;
		n->next = NULL;
		n->pre  = table->tail;
		table->tail = n;
	}
}

void ts_deltable(tstable* table,tsval* key)
{
	uint hash;
	hash = ts_genhash(key);
	while(table->slot[hash%table->size].used)
	{
		if(ts_cmp(&table->slot[hash%table->size].key,key)==0)
		{
			table->slot[hash%table->size].used = 0;
			table->slot[hash%table->size].pre->next = table->slot[hash%table->size].next;
			table->slot[hash%table->size].next->pre = table->slot[hash%table->size].pre;
			break;
		}
		++hash;
	}
}

void ts_inittable(tstable *table)
{
	table->size = INITIAL_TABLE_SIZE;
	table->used = 0;
	table->tail = table->head = NULL;
	table->slot = (tnode*)malloc(sizeof(tnode)*INITIAL_TABLE_SIZE);
	table->gchead.type  = type_table;
	table->gchead.color = -1;
	memset(table->slot,0,sizeof(tnode)*INITIAL_TABLE_SIZE);
}

tstable* ts_newtable()
{
	tstable *t;
	t = (tstable*)malloc(sizeof(tstable));
	ts_inittable(t);
	return t;
}


/*******************************************************************************************************************/





