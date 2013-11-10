
#ifndef tlex_h
#define tlex_h

#include "tlib.h"

//#define _DEBUG_HELP_

#define GETTOKEN(f)        (f->cur.t)
#define GETNEXTTOKEN(f)    (f->next.t)


enum 
{
	tk_int = 256,
	tk_float,
	tk_str,
	tk_name,
	tk_global,
	tk_func,
	tk_ret,
	tk_for,
	tk_do,
	tk_while,
	tk_break,
	tk_continue,
	tk_if,
	tk_else,
	tk_true,
	tk_false,
	tk_rol,
	tk_ror,
	tk_le,
	tk_ge,
	tk_equ,
	tk_nequ,
	tk_and,
	tk_or,
	tk_in,
	tk_eof,
};

typedef struct token
{
	int t;
	char *pos;
	union
	{
		tsstr *str;
		int inum;
		double fnum;
	};
}token;

typedef struct filestate
{
	envir   *envirnment;
	tsfunc  *curframe;
	byte  enterlevel;
	byte  looplevel;
	byte  inifblock;
	byte  usedreg[256];
	byte  regpos;
	tsval *cst2reg;    //const number register

	token cur;
	token next;

	char *buff;
	char *pos;
	char *curl;                 //current line's beginning position of the buffer
	char *nextl;

	int size;                 //size of buffer	
	int line;                 //current line's number
	char tkbuf[65535];
}filestate;


#ifdef  _DEBUG_HELP_

	void printlexinfo(filestate *fs);
#endif

int gettoken(filestate *fs);
void initlex(filestate *fs,char *buf);
void freelex(filestate *fs);


#endif

