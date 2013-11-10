
#ifndef  tobject_h
#define  tobject_h


#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>

#define    MAX_STACK_SIZE      1024
#define    MAX_STRTABLE_SIZE   1024
#define    INITIAL_TABLE_SIZE  256
#define    INITIAL_ARRAY_SIZE  16
#define    INITIAL_CODE_SIZE   128
    
#define    undefined(data)        ((data)->t==type_undefined)

#define    isnull(data)           ((data)->t==type_null)
#define    isstr(data)            ((data)->t==type_str)
#define    isint(data)            ((data)->t==type_int)
#define    isfloat(data)          ((data)->t==type_float)
#define    istable(data)          ((data)->t==type_table)
#define    isarray(data)          ((data)->t==type_array)
#define    isfunc(data)           ((data)->t==type_func)
#define    isnumber(data)         ((data)->t==type_int||(data)->t==type_float)
#define    isbool(data)           ((data)->t==type_bool)

#define    getint(data)           (data)->inum
#define    getfloat(data)         (data)->fnum
#define    getstring(data)        (data)->str
#define    getarray(data)         (data)->a
#define    gettable(data)         (data)->tab
#define    getfunc(data)          (data)->func
#define    getbool(data)          (data)->inum
#define    getnumber(data)        (isint(data)?(double)getint(data):getfloat(data))

#define    setint(d,v)            {(d)->t=type_int;(d)->inum=v;}
#define    setfloat(d,v)          {(d)->t=type_float;(d)->fnum=v;}
#define    setstring(d,v)         {(d)->t=type_str;(d)->str=v;}
#define    setarray(d,v)          {(d)->t=type_array;(d)->a=v;}
#define    settable(d,v)          {(d)->t=type_table;(d)->tab=v;}
#define    setfunc(d,v)           {(d)->t=type_func;(d)->func=v;}
#define    setbool(d,v)           {(d)->t=type_bool;(d)->inum=v;}



typedef    unsigned int        instruction;
typedef    unsigned long long  uint64;
typedef    unsigned int        uint;
typedef    unsigned short      word;
typedef    unsigned char       byte;




enum{
	type_undefined=0,
	type_null,
	type_int,
	type_float,
	type_str,
	type_table,
	type_array,
	type_func,
	type_bool,
};

struct tstable;
struct tsstr;
struct tsarray;
struct tsfunc;
struct tsstate;
struct envir;
struct tsvm;

typedef int (*nativefunc)(struct tsvm*,int);

typedef struct tsval{
	byte t;
	int reg;
	union
	{
		struct tstable *tab;
		struct tsstr *str;
		struct tsarray *a;
		struct tsfunc *func;
		double fnum;
		int    inum;
	};
}tsval;

typedef struct tsgarbage
{
	struct tsgarbage *next;
	byte color;
	byte type;
}tsgarbage;

typedef struct tsstr
{
	tsgarbage gchead;
	uint hash;
	byte *buf;
	int len;
}tsstr;

typedef struct tnode
{
	tsval key;
	tsval data;
	struct tnode *next;
	struct tnode *pre;
	byte used;
}tnode;

typedef struct tstable
{
	tsgarbage gchead;
	tnode *slot;
	tnode *head,*tail,*enumptr;
	int size;
	int used;
}tstable;

typedef struct tsarray
{
	tsgarbage gchead;
	tsval *data;
	int enumptr;
	int size;
	int used;
}tsarray;

typedef struct symnode
{
	tsstr *key;
	int data;
	struct symnode *next;
}symnode;

typedef struct symtable
{
	symnode *slot;
	symnode *head,*tail;
	int size;
	int used;
}symtable;


typedef struct cstnode
{
	uint64  key;
	word  data;
	struct cstnode *next;
}cstnode;

typedef struct csttable
{
	cstnode *slot;
	cstnode *head,*tail;
	int size;
	int used;
}csttable;


enum{
	undefine,
	tinyscript,
	native,
};

typedef struct tsfunc
{
	byte functype;
	struct tsfunc *parent;
	struct envir  *e;
	symtable  symbol;
	tsarray  local;           /* local varaibles */
	tsarray  global;          /* used to store global varaibles' register information */
	tsval *reg[256];
	uint regus[32];
	tsstr *name;
	
	
	union{
		instruction *code;
		nativefunc  funbody;
	};
	
	int     lastjump;
	int     lastloop;
	int     paramnum;
	int     codesize;
	int		used;
	int     pc;           
}tsfunc;


typedef struct envir
{
	symtable  strt;
	symtable  funt;
	tsarray   funlist;
	csttable  cstt;
	tsfunc   global;              /* global object */
}envir;





tsfunc *ts_newfunc(tsstr *name,envir *e,byte nati);

int ts_cmp(tsval *a,tsval *b);
int ts_add(tsval *a,tsval *b,tsval *res);
int ts_sub(tsval *a,tsval *b,tsval *res);
int ts_mul(tsval *a,tsval *b,tsval *res);
int ts_div(tsval *a,tsval *b,tsval *res);
int ts_mod(tsval *a,tsval *b,tsval *res);
int ts_xor(tsval *a,tsval *b,tsval *res);
int ts_ror(tsval *a,tsval *b,tsval *res);
int ts_rol(tsval *a,tsval *b,tsval *res);
int ts_or(tsval *a,tsval *b,tsval *res);
int ts_and(tsval *a,tsval *b,tsval *res);




#endif

