#include <stdlib.h>
#include "tlex.h"
#include "ttable.h"
#include "tstring.h"

#define COPYTOKEN(fs)              fs->cur = fs->next

extern void initfunc(tsfunc *f);

typedef struct reseinfo{
	int t;
	char * v;
}reseinfo;

const reseinfo tkinfo[] = {{tk_func,"function"},{tk_ret,"return"},{tk_for,"for"},{tk_do,"do"}, 
							{tk_while,"while"}, {tk_break,"break"},{tk_if,"if"},{tk_else,"else"},
							{tk_true,"true"},{tk_false,"false"},{tk_continue,"continue"},{tk_in,"in"},
							{tk_global,"global"}};


char* dbginfo[]={
	"integer",
	"floating number",
	"string",
	"varaible",
	"global",
	"function",
	"return",
	"for",
	"do",
	"while",
	"break",
	"continue",
	"if",
	"else",
	"true",
	"false",
	"rol",
	"ror",
	"le",
	"ge",
	"equ",
	"nequ",
	"and",
	"or",
	"in",
	"eof"
};

#ifdef _DEBUG_HELP_

extern 
void printlexinfo(filestate *fs)
{
	printf("token num[%d]  ",fs->cur.t);
	if(fs->cur.t<256)
		printf("content[ %c ]",(char)fs->cur.t);
	else
	{
		printf("token name[ %s ]  ",dbginfo[fs->cur.t-256]);
		if(fs->cur.t == tk_name||fs->cur.t == tk_str)
			printf("content[ %s ]  address[ 0x%08x ]",fs->cur.str->buf,fs->cur.str->buf);
		else if(fs->cur.t == tk_int)
			printf("content[ %d ]  ",fs->cur.inum);
		else if(fs->cur.t == tk_float)
			printf("content[ %03f ]  ",fs->cur.fnum);
	}
	printf("\n");
}

#endif

int digittoken(filestate *fs)
{
	int num=0,base=1;
	double dvalue;
	char *p = fs->pos;
	COPYTOKEN(fs);
	fs->next.t = tk_int;
	if(*p=='0' && *(p+1)=='x')
	{
		p+=2;
		while(*p!=0)
		{
			if(isdigit(*p))
				num=num*16+*p-'0';
			else if(*p>='A'&&*p<='F')
				num=num*16+*p-'A';
			else if(*p>='a'&&*p<='f')
				num=num*16+*p-'a';
			else
				break;
			++p;
		}
		if(*p==0)
			return -2;
		fs->pos = p;
		fs->next.inum = num;

	}
	else
	{
		while(isdigit(*p))
		{
			num=num*10+*p-'0';
			++p;
		}
		if(*p==0)
		{
			fs->pos = p;
			fs->next.inum = num;
			return 0;
		}
		if(*p=='.')
		{
			++p;
			if(*p==0)
				return -2;
			if(!isdigit(*p))
			{
				/*todo: error handle */
				return -1;
			}
			dvalue = num;
			num = 0;


			while(*p!=0&&isdigit(*p))
			{
				num=num*10+*p-'0';
				base*=10;
				++p;
			}
			dvalue = dvalue + (float)num/base;
			fs->next.t = tk_float;
			fs->next.fnum = dvalue;
		}
		else
			fs->next.inum = num;
	}
	fs->pos = p;
	return 0;
}

int strtoken(filestate *fs)
{
	tsstr *s;
	uint len = 0;
	char c = *fs->pos++;
	char *p  = fs->pos;
	COPYTOKEN(fs);
	fs->next.t = tk_str;
	while(*p!=c&&*p!=0)
	{
		if(*p=='\n')
		{
			/* todo: error handle */
			return -3;
		}
		if(*p=='\\')
		{
			++p;
			if(*p==0)
				break;
			switch(*p)
			{
			case 'n':
				fs->tkbuf[len++]='\n';
				break;
			case 'b':
				fs->tkbuf[len++]='\b';
				break;
			case 't':
				fs->tkbuf[len++]='\t';
				break;
			default:
				fs->tkbuf[len++]=*p;
			}
		}
		else
			fs->tkbuf[len++]=*p;
		++p;
	}
	if(*p==0)
	{
		/* todo: error handle */
		return -2;
	}
	fs->pos = p+1;
	fs->tkbuf[len] = 0;
	s = ts_newstring(fs->envirnment,(byte*)fs->tkbuf,len);
	fs->next.str = s;
	return 0;
}

int nametoken(filestate *fs)
{
	int i=0;
	int len = 0;
	char *pos = fs->pos;
	COPYTOKEN(fs);


	fs->tkbuf[len++] = *pos++;
	while(*pos!=0&&(isdigit(*pos)||isalpha(*pos)||*pos=='_')&&len<65535)
	{
		fs->tkbuf[len++] = *pos++;
	}
	fs->tkbuf[len]=0;
	for(i=0;i<sizeof(tkinfo)/sizeof(reseinfo);++i)
	{
		if(strcmp(fs->tkbuf,tkinfo[i].v)==0)
		{
			fs->next.t = tkinfo[i].t;
			fs->pos = pos;
			return 0;
		}
	}
	fs->next.str = ts_newstring(fs->envirnment,(byte*)fs->tkbuf,len);
	fs->next.t   = tk_name;
	fs->pos = pos;
	return 0;
}

int oprtoken(filestate *fs)
{
	char c  = *fs->pos++;
	
	COPYTOKEN(fs);

	switch(c)
	{
	case '>':
		if(*fs->pos == '>')
		{
			fs->pos++;
			fs->next.t = tk_ror;
		}
		else if(*fs->pos == '=')
		{
			fs->pos++;
			fs->next.t = tk_ge;
		}
		else
			fs->next.t = c;
		break;
	case '<':
		if(*fs->pos == '<')
		{
			fs->pos++;
			fs->next.t = tk_rol;
		}
		else if(*fs->pos == '=')
		{
			fs->pos++;
			fs->next.t = tk_le;
		}
		else
			fs->next.t = c;
		break;
	case '=':
		if(*fs->pos == '=')
		{
			fs->pos++;
			fs->next.t = tk_equ;
		}
		else
			fs->next.t = c;
		break;
	case '|':
		if(*fs->pos == '|')
		{
			fs->pos++;
			fs->next.t = tk_or;
		}
		else
			fs->next.t = c;
		break;
	case '&':
		if(*fs->pos == '&')
		{
			fs->pos++;
			fs->next.t = tk_and;
		}
		else
			fs->next.t = c;
		break;
	case '!':
		if(*fs->pos=='=')
		{
			fs->pos++;
			fs->next.t = tk_nequ;
		}
		else
			fs->next.t = c;
		break;
	default:
		fs->next.t = c;
	
	}
	return 0;
}

int gettoken(filestate *fs)
{
	int r;
	char c,*cur;
	static char endline = 0;
	if(endline)
	{
		endline=0;
		fs->curl = fs->nextl;
		++fs->line;
		//insertline(fs->curframe,fs->line);
	}
	
	while(*fs->pos!=0&&isspace(*fs->pos))
	{
		if(*fs->pos=='\n')
		{
			if(endline)
			{
				fs->curl = fs->nextl;
				++fs->line;
			}
			endline=1;
			fs->nextl = fs->pos+1;
		}
		++fs->pos;
	}
	cur = fs->pos;
	c = *fs->pos;
	if(c>='0' && c<='9')
		r = digittoken(fs);
	else if(isalpha(c)||c=='_')
		r = nametoken(fs);
	else if(c=='"'||c=='\'')
		r = strtoken(fs);
	else if(c==0)
	{
		COPYTOKEN(fs);
		fs->next.t = tk_eof;
		r = 0;
	}
	else 
		r = oprtoken(fs);
	fs->next.pos = cur;
	return r;
}

void initlex(filestate *fs,char *buf)
{
	int i;
	memset(fs,0,sizeof(filestate));
	fs->envirnment = (envir*)malloc(sizeof(envir));
	initenv(fs->envirnment);
	fs->cst2reg  = (tsval*)malloc(sizeof(tsval)*256);
	memset(fs->cst2reg,0,sizeof(tsval)*256);
	for(i=0;i<256;++i)
		fs->cst2reg[i].reg = -1;
	fs->curframe = &fs->envirnment->global;
	fs->curl = fs->pos = buf;
	fs->line = 1;
}

void freelex(filestate *fs)
{
	free(fs->cst2reg);
}
