#include <stdlib.h>
#include "tobject.h"
#include "ttable.h"
#include "tarray.h"
#include "tstring.h"
#include "tlib.h"

void ts_copyarray(tsarray *a,tsarray *b)
{
	if(b->size<a->size)
		b->data = (tsval*)malloc(sizeof(tsval)*a->size);
	memcpy(b->data,a->data,a->used*sizeof(tsval));
	b->size = a->size;
	b->used = a->used;

}

static int ts_cmparray(tsarray *a,tsarray *b)
{
	int sa,sb;
	int r;

	sa = a->used-1;
	sb = b->used-1;
	while(sa>=0&&sb>=0)
	{
		r=ts_cmp(&a->data[sa--] , &b->data[sb--]);
		if(r>0)
			return 1;
		else if(r<0)
			return -1;
	}
	if(sa>0)
		return 1;
	if(sb>0)
		return -1;
	return 0;
}

static int ts_cmptable(tstable *a,tstable *b)
{
	return 0;
}

int ts_cmp(tsval *a,tsval *b)
{
	double r;
	int len;
	tsstr *as,*bs;
	if(isnumber(a)&&isnumber(b))
	{
		r = getnumber(a)-getnumber(b);
		if(r>0)
			return 1;
		else if(r<0)
			return -1;
		else
			return 0;
	}
	if(a->t!=b->t)
		return -2;             
	if(isbool(a))
	{
		return getbool(a) == getbool(b);
	}
	else if(isstr(a))
	{
		as = getstring(a);
		bs = getstring(b);
		len = as->len>bs->len?bs->len:as->len;
		return memcmp(as->buf,bs->buf,len);
	}
	else if(istable(a))
	{
		
	}
	else if(isarray(a))
	{
		return ts_cmparray(getarray(a),getarray(b));
	}
	return -2;
}

int ts_add(tsval *a,tsval *b,tsval *res)
{
	if(isnumber(a)&&isnumber(b))
	{
		if(isfloat(a)||isfloat(b))
		{
			res->t = type_float;
			res->fnum = getnumber(a)+getnumber(b);
		}
		else
		{
			res->t = type_int;
			res->inum = getnumber(a)+getnumber(b);
		}
		return 0;
	}
	if(a->t!=b->t)
		return -1;

	if(a->t == type_array)
	{
		res->t = type_array;
		res->a = ts_arrayadd(a->a,b->a);
		return 0;
	}
	else if(a->t == type_str)
	{
		res->t = type_str;
		res->str = ts_stringadd(a->str,b->str);
		return 0;
	}
	
	return -1;
}


int ts_sub(tsval *a,tsval *b,tsval *res)
{
	if(isnumber(a)&&isnumber(b))
	{
		if(isfloat(a)||isfloat(b))
		{
			res->t = type_float;
			res->fnum = getnumber(a)-getnumber(b);
		}
		else
		{
			res->t = type_int;
			res->inum = getnumber(a)-getnumber(b);
		}
		return 0;
	}
	return -1;
}

int ts_mul(tsval *a,tsval *b,tsval *res)
{
	tsval *num,*obj;
	if(isnumber(a)&&isnumber(b))
	{
		if(isfloat(a)||isfloat(b))
		{
			res->t = type_float;
			res->fnum = getnumber(a)*getnumber(b);
		}
		else
		{
			res->t = type_int;
			res->inum = getnumber(a)*getnumber(b);
		}
		return 0;
	}
	
	if(isint(a))
	{
		num = a;
		obj = b;
	}
	else if(isint(b))
	{
		num = b;
		obj = a;
	}
	else
		return -1;
	if(isstr(obj))
	{
		res->t   = type_str;
		res->str = ts_stringmul(obj->str,num->inum);
	}
	else if(isarray(obj))
	{
		res->t = type_array;
		res->a = ts_arraymul(obj->a,num->inum);
	}
	return 0;
}

int ts_div(tsval *a,tsval *b,tsval *res)
{
	if(isnumber(a)&&isnumber(b))
	{
		if(isfloat(a)||isfloat(b))
		{
			res->t = type_float;
			res->fnum = getnumber(a)/getnumber(b);
		}
		else
		{
			res->t = type_int;
			res->inum = getnumber(a)/getnumber(b);
		}
		return 0;
	}
	return -1;
}

int ts_mod(tsval *a,tsval *b,tsval *res)
{
	if(isint(a)&&isint(b))
	{
		res->t = type_int;
		res->inum = getint(a) % getint(b);
	}
	return -1;
}

int ts_and(tsval *a,tsval *b,tsval *res)
{
	if(isint(a)&&isint(b))
	{
		res->t = type_int;
		res->inum = getint(a) & getint(b);
	}
	return -1;
}

int ts_or(tsval *a,tsval *b,tsval *res)
{
	if(isint(a)&&isint(b))
	{
		res->t = type_int;
		res->inum = getint(a) | getint(b);
	}
	return -1;
}

int ts_xor(tsval *a,tsval *b,tsval *res)
{
	if(isint(a)&&isint(b))
	{
		res->t = type_int;
		res->inum = getint(a) ^ getint(b);
	}
	return -1;
}

int ts_rol(tsval *a,tsval *b,tsval *res)
{
	if(isint(a)&&isint(b))
	{
		res->t = type_int;
		res->inum = getint(a) << getint(b);
	}
	return -1;
}

int ts_ror(tsval *a,tsval *b,tsval *res)
{
	if(isint(a)&&isint(b))
	{
		res->t = type_int;
		res->inum = getint(a) >> getint(b);
	}
	return -1;
}

void initfunc(tsfunc *f)
{
	memset(f,0,sizeof(tsfunc));
	f->code = (instruction*)malloc(sizeof(instruction)*INITIAL_CODE_SIZE);
	f->codesize = INITIAL_CODE_SIZE;

	initsymbol(&f->symbol);
	ts_initarray(&f->local);
	ts_initarray(&f->global);
}



tsfunc *ts_newfunc(tsstr *name,envir *e,byte nati)
{
	tsfunc *p;
	p = (tsfunc*)malloc(sizeof(tsfunc));
	if(!nati)
		initfunc(p);
	p->name = name;
	p->e    = e;
	return p;
}

