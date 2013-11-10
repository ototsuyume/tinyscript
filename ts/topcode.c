

#include "stdlib.h"
#include "topcode.h"

#define GENOPR(op)                        (op<<24)
#define GENCODE(op,a)                     ((op<<24) | (a&0xffff)) 
#define GENCODE2(op,a,b)                  ((op<<24) | ((a&0xff)<<16) | (b&0xffff))
#define GENCODE3(op,a,b,c)                ((op<<24) | ((a&0xff)<<16) | ((b&0xff)<<8) | (c&0xff))



void gen(tsfunc *fun,instruction code)
{
	if(fun->used==fun->codesize)
	{
		fun->codesize*=2;
		fun->code = (instruction*)realloc(fun->code,fun->codesize*sizeof(instruction));
	}
	fun->code[fun->used++] = code;
}

void gencode(tsfunc *fun,byte code)
{
	gen(fun,GENOPR(code));
}

void gencodeABC(tsfunc *cur,byte code,byte a,byte b,byte c)
{
	gen(cur,GENCODE3(code,a,b,c));
}

void gencodeAB(tsfunc *cur,byte code,byte a,word b)
{
	gen(cur,GENCODE2(code,a,b));
}

void gencodeA(tsfunc *cur,byte code,word a)
{
	gen(cur,GENCODE(code,a));
}

/* maximum line is 16777215(0xffffff) */
void insertline(tsfunc *fun,int line)
{
	//gen(fun,GENOPR(op_nop)|(line&0xffffff));	
}
