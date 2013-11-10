#include <stdio.h>
#include <stdlib.h>
#include "tlib.h"
#include "tio.h"






void printarray(tsarray *arr);
void printtable(tstable *tab);

void printarray(tsarray *arr)
{
	int i;
	tsval *pv;
	printf("[");
	for(i=0;i<arr->used;++i)
	{
		pv = &arr->data[i];
		switch(pv->t)
		{
		case type_null:
			printf("null");
			break;
		case type_int:
			printf("%d",getint(pv));
			break;
		case type_float:
			printf("%.06f",getfloat(pv));
			break;
		case type_str:
			printf("\'%s\'",getstring(pv)->buf);
			break;
		case type_func:
			printf("function object at:0x%08x",(uint)getfunc(pv));
			break;
		case type_array:
			printarray(getarray(pv));
			break;
		case type_table:
			printtable(gettable(pv));
			break;
		}
		if(i<arr->used-1)
			printf(",");
	}
	printf("]");
}


void printtable(tstable *tab)
{
	tnode *h;
	h = tab->head;
	printf("{");
	while(h)
	{
		switch(h->key.t)
		{
		case type_int:
			printf("%d",h->key.inum);
			break;
		case type_float:
			printf("%.06f",h->key.fnum);
			break;
		case type_str:
			printf("\'%s\'",h->key.str->buf);
			break;
		}
		printf(":");

		switch(h->data.t)
		{
		case type_int:
			printf("%d",h->data.inum);
			break;
		case type_float:
			printf("%.06f",h->data.fnum);
			break;
		case type_str:
			printf("\'%s\'",h->data.str->buf);
			break;
		case type_func:
			printf("function object at:0x%08x",(uint)h->data.func);
			break;
		case type_table:
			printtable(h->data.tab);
			break;
		case type_array:
			printarray(h->data.a);
		}
		if(h->next)
			printf(",");
		h = h->next;
	}
	printf("}");
}

int tslib_print(tsvm *vm,int pcount)
{
	int i;
	tsval *pv;
	for(i=vm->dsp-pcount;i<vm->dsp;++i)
	{
		pv = &vm->dstack[i];
		switch(pv->t)
		{
		case type_null:
			printf("null");
			break;
		case type_int:
			printf("%d",getint(pv));
			break;
		case type_float:
			printf("%.06f",getfloat(pv));
			break;
		case type_str:
			printf("\'%s\'",getstring(pv)->buf);
			break;
		case type_func:
			printf("function object at:0x%08x",getfunc(pv));
			break;
		case type_array:
			printarray(getarray(pv));
			break;
		case type_table:
			printtable(gettable(pv));
			break;
		}
		if(i<vm->dsp-1)
		{
			printf(",");
		}
	}
	printf("\n");
	return 0;
}


funcdef iolib[] = {
	{"print",tslib_print,-1},
	{"",0}
};

void ts_init_iolib(envir *e)
{
	addbuiltin(e,iolib);
}
