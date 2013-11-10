#include <stdlib.h>
#include "tstring.h"
#include "ttable.h"
#include "tlib.h"

tsstr* ts_newstring(envir *env,const byte *s,uint len)
{
	byte *buf;
	tsstr *ret;
	if((ret=rawgetstr(&env->strt,s,len))!=0)
		return ret;
	buf = (byte*)malloc(len+1);
	memcpy(buf,s,len);
	buf[len] = 0;
	ret = (tsstr*)malloc(sizeof(tsstr));
	ret->buf  = buf;
	ret->len  = len;
	ret->hash = genhash(buf,len);

	setsymbol(&env->strt,ret,-1); 
	return ret;
}


tsstr* _raw_newstring(envir *env,byte *s,uint len)
{
	tsstr *ret;
	if((ret=rawgetstr(&env->strt,s,len))!=NULL)
	{
		free(s);
		return ret;
	}
	
	ret = (tsstr*)malloc(sizeof(tsstr));
	ret->buf  = s;
	ret->len  = len;
	ret->hash = genhash(s,len);

	setsymbol(&env->strt,ret,-1);
	return ret;
}

tsstr* ts_stringadd(const tsstr *a,const tsstr *b)
{
	tsstr *s;
	byte  *buf;
	int    len;
	len = a->len+b->len;
	buf = (byte*)malloc(len+1);
	memcpy(buf,a->buf,a->len);
	memcpy(buf+a->len,b->buf,b->len);
	buf[len]=0;
	s = (tsstr*)malloc(sizeof(tsstr));
	s->buf  = buf;
	s->hash = 0;
	s->len  = len;
	s->gchead.type = type_str;
	return s;
	
}

tsstr* ts_stringmul(const tsstr *a,int c)
{
	tsstr *s;
	byte  *buf;
	int    len,i,blocksize;
	blocksize = a->len;
	len = blocksize*c;
	buf = malloc(len+1);
	
	for(i=0;i<c;++i)
	{
		memcpy(buf+blocksize*i,a->buf,blocksize);
	}
	buf[len] = 0;
	s = (tsstr*)malloc(sizeof(tsstr));
	s->len = len;
	s->buf = buf;
	s->gchead.type = type_str;
	return s;
}