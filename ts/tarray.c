#include <stdlib.h>
#include "tarray.h"

/********************************************array manipulation functions*******************************************/


void ts_initarray(tsarray *arr)
{
	arr->data = (tsval*)malloc(sizeof(tsval)*INITIAL_ARRAY_SIZE);
	memset(arr->data,0,sizeof(tsval)*INITIAL_ARRAY_SIZE);
	arr->size = INITIAL_ARRAY_SIZE;
	arr->used = 0;
	arr->enumptr = 0;
	arr->gchead.type  = type_array;
	arr->gchead.color = -1;
}

static void ts_resizearray(tsarray *arr)
{
	arr->size *= 2;
	arr->data  = realloc(arr->data,arr->size); 
}

int ts_appendarray(tsarray *arr,tsval *val)
{
	if(arr->used==arr->size)
		ts_resizearray(arr);
	arr->data[arr->used] = *val;
	return arr->used++;
}

int ts_getarray(tsarray *arr,int pos,tsval *val)
{
	if(pos+1>arr->used)
		return -1;
	*val = arr->data[pos];
	return 0;
}

int ts_delarray(tsarray *arr,int pos)
{
	if(pos+1>arr->used)
		return -1;
	memmove(arr->data+pos,arr->data+1+pos,arr->used-pos-1);
	--arr->used;
	return 0;
}

int ts_setarray(tsarray *arr,int pos,tsval *val)
{
	if(pos+1>arr->used||pos<0)
		return -1;
	arr->data[pos] = *val;
	return 0;
}

tsarray* ts_newarray()
{
	tsarray *t;
	t = (tsarray*)malloc(sizeof(tsarray));
	ts_initarray(t);
	return t;
}

tsarray* ts_newarray2(uint size)
{

	tsarray *t;
	int newsize;
	newsize = size+32;
	t = (tsarray*)malloc(sizeof(tsarray));
	t->used = 0;
	t->data = (tsval*)malloc(sizeof(tsval)*newsize);
	memset(t->data,0,sizeof(tsval)*newsize);
	t->size = newsize;
	t->gchead.type = type_array;
	return t;
}

tsarray* ts_arrayadd(const tsarray *a,const tsarray *b)
{
	tsarray *res;
	int tmp;
	tmp = a->used+b->used;
	res = ts_newarray2(tmp);
	memcpy(res->data,a->data,sizeof(tsval)*a->used);
	memcpy(res->data+a->used,b->data,sizeof(tsval)*b->used);
	res->used = tmp;
	return res;
}

tsarray *ts_arraymul(const tsarray *a,int c)
{
	tsarray *res;
	int tmp,i,blocksize;
	tmp = a->used*c;
	
	res = ts_newarray2(tmp);
	res->used = tmp;
	blocksize = sizeof(tsval)*a->used;
	for(i=0;i<tmp;++i)
	{
		memcpy(res->data+i*blocksize,a->data,blocksize);
	}
	return res;
}


/*******************************************************************************************************************/

