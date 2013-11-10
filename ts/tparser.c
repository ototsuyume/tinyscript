#include <stdlib.h>
#include "tparser.h"
#include "topcode.h"
#include "tarray.h"
#include "tlib.h"

extern char* dbginfo[];

#ifdef _debug
void CHECK_TOKEN(filestate *fs,int t)
{
	if(GETTOKEN(fs)!=t) 
	{
		if(GETTOKEN(fs)<256&&t<256)
			printf("syntax error.expect[%c] actual[%c]\n",t,GETTOKEN(fs));
		else if(GETTOKEN(fs)<256&&t>=256)
			printf("syntax error.expect[%s] actual[%c]\n",dbginfo[t-256],GETTOKEN(fs));
		else if(GETTOKEN(fs)>256&&t<256)
			printf("syntax error.expect[%c] actual[%s]\n",t,dbginfo[GETTOKEN(fs)-256]);
		else
			printf("syntax error.expect[%s] actual[%s]\n",dbginfo[t-256],dbginfo[GETTOKEN(fs)-256]);
		printf("in line %d:  ",fs->line);
		while(*fs->curl!=0&&*fs->curl!='\n')
			printf("%c",*fs->curl++);
		printf("\n");
		exit(0);
	}
}
#else

#define      CHECK_TOKEN(fs,t)       \
if(GETTOKEN(fs)!=t)\
{\
	if(t<256)\
	printf("missing '%c'\n",t);\
	syntaxerr(fs);\
	exit(0);\
}

#endif

#define      INTERNALEXCEPTION(s)    {\
	puts(s);\
	exit(0);\
}


#define    unaryopr(c)         (c=='-' || c=='~' || c=='!')
#define    setregflag(func,num)        (func->regus[num/8] |= 1<<num%8)
#define    clrregflag(func,num)        (func->regus[num/8] &= ~(1<<num%8))
#define    testregflag(func,num)       (func->regus[num/8] & 1<<num%8)

int dostatement(filestate *fs);
int doifblock(filestate *fs);
int dowhile(filestate *fs);
int dofunc(filestate *fs);
int doblock(filestate *fs);
int docontinue(filestate *fs);
int dobreak(filestate *fs);
int doreturn(filestate *fs);
int dodowhile(filestate *fs);
int dofor(filestate *fs);



void enterlevel(filestate *fs)
{
	if(fs->enterlevel==0xff)
	{
		INTERNALEXCEPTION("error\n");
	}
	++fs->enterlevel;
}

void leavelevel(filestate *fs)
{
	if(fs->enterlevel == 0)
	{
		INTERNALEXCEPTION("error\n");
	}
	--fs->enterlevel;
}


void syntaxerr(filestate *fs)
{
	char *pos = fs->cur.pos;
	int   len = fs->cur.pos - fs->curl;
	printf("syntax error.line %d\n",fs->line);
	while(*pos!='\n'&&*pos!=0)++pos;
	if(*pos!=0)*pos = 0;
	printf("%s\n",fs->curl);
	while(len>0)
	{
		printf("%c",' ');
		--len;
	}
	printf("^\n");
	exit(0);
}




int binopr(int c)
{
	switch(c)
	{
	case '+':
	case '-':
	case '/':
	case '*':
	case '%':
	case '>':
	case '<':
	case '|':
	case '&':
	case '^':
	case tk_equ:
	case tk_nequ:
	case tk_ge:
	case tk_le:
	case tk_or:
	case tk_and:
	case tk_ror:
	case tk_rol:
		return 1;
	}
	return 0;
}

int getreg(tsfunc *fun)
{
	int count;
	count = 0;
	if(fun->pc > 255)
		fun->pc = 0;
	while(testregflag(fun,fun->pc) && count<256)
	{
		++count;
		++fun->pc;
	}
	if(count>=256)
		INTERNALEXCEPTION("Experssion is too complicated.\n")
	/*Invalidate the pointer of the varaibale*/
	if(fun->reg[fun->pc])
		fun->reg[fun->pc]->reg = -1;
	return fun->pc++;
}

int alloclocal(filestate *fs,tsstr *name)
{
	int val;
	tsval tmp;
	tsfunc *cur;
	tmp.reg = -1;
	cur = fs->curframe;
	if((val = findsymbol(&cur->symbol,name))!=-1)
		return val;
	val = ts_appendarray(&cur->local,&tmp);
	setsymbol(&cur->symbol,name,val);
	return val;
}

int getfunction(envir *env,tsstr *str)
{
	int pos;
	tsval v;
	if((pos = findsymbol(&env->funt,str))==-1)
	{
		memset(&v,0,sizeof(tsval));
		pos = env->funt.used;
		setsymbol(&env->funt,str,pos);
		ts_appendarray(&env->funlist,&v);
	}
	return pos;
}

void clearregcache(tsfunc *fun)
{
	int i;
	for(i=0;i<256;++i)
		if(fun->reg[i])
			fun->reg[i]->reg = -1;
}

/* Used for clearing global varaibles reg info */
void clearregche2(filestate *fs)
{
	int i;
	tsfunc *fun = fs->curframe;
	for(i=0;i!=0xff;++i)
	{
		if(fs->usedreg[i]!=0)
		{
			fun->reg[fs->usedreg[i]]->reg = -1;
			fs->usedreg[i] = 0;
		}
	}
	fs->regpos = 0;
}

int getvarreg(filestate *fs,int pos)
{
	int reg;
	tsfunc *parent,*frame = fs->curframe;
	if(pos&0x80000000)
	{
		pos = pos& ~0x80000000;
		parent = &fs->envirnment->global;
		if(parent->local.data[pos].reg==-1)
		{
			reg = getreg(frame);
			parent->local.data[pos].reg = reg;
			frame->reg[reg] = &parent->local.data[pos];
			gencodeAB(frame,op_loadg,reg,pos);
		}
		else
			reg = parent->local.data[pos].reg;
	}
	else if(frame->local.data[pos].reg<0)
	{
		reg = getreg(frame);
		if(frame->local.data[pos].reg!=-2)
		{
			frame->reg[reg] = &frame->local.data[pos];
			frame->local.data[pos].reg = reg;
		}
		
		gencodeAB(frame,op_loadl,reg,pos);
	}
	else
		reg = frame->local.data[pos].reg;
	if(fs->inifblock)
		fs->usedreg[fs->regpos++] = reg;
	return reg;
}
/*
void setvarreg(tsfunc *frame,int pos,int reg)
{
	if(pos&0x80000000)
	{
		pos = pos& ~0x80000000;
		frame = frame->parent;
	}
	frame->local.data[pos].reg = reg;

}*/

#define getvar(pos)                       (pos&0x80000000?op_loadg:op_loadl)

byte savevar(filestate *fs,uint pos,int reg)
{

	if(pos&0x80000000)
	{
		fs->envirnment->global.local.data[pos&0x7fffffff].reg = reg;
		fs->curframe->reg[reg] = &fs->envirnment->global.local.data[pos&0x7fffffff];
		return op_saveg;
	}
	else
	{
		if(fs->curframe->local.data[pos].reg!=-2)
		{
			fs->curframe->local.data[pos].reg = reg;
			if(fs->curframe->reg[reg])
				fs->curframe->reg[reg]->reg = -1;
			fs->curframe->reg[reg] = &fs->curframe->local.data[pos];
		}
		
		return op_savel;
	}
}

void genjumpcode(filestate *fs,byte op)
{
	
	int distance;
	tsfunc *func;
	
	func = fs->curframe;
	if(func->lastjump!=-1)
		distance = func->used - func->lastjump;
	else 
		distance = 0;
	if(distance > 0xffff)
		syntaxerr(fs);
	func->lastjump = func->used;
	gencodeAB(func,op,fs->enterlevel,distance);
}

int getopriviledge(int tk)
{
	int r;
	switch(tk)
	{
	case tk_in:
	case tk_or:
	case tk_and:
		r=1;
		break;
	case '|':
		r=2;
		break;
	case '^':
		r=3;
		break;
	case '&':
		r=4;
		break;
	case tk_equ:
	case tk_nequ:
		r=5;
		break;
	case tk_ge:
	case tk_le:
	case '>':
	case '<':
		r=6;
		break;
	case tk_rol:
	case tk_ror:
		r=7;
		break;
	case '+':
	case '-':
		r=8;
		break;
	case '*':
	case '/':
	case '%':
		r=9;
		break;
	default:
		return -1;
	}
	return r;
}


typedef enum expkind
{
	VARIABLE,
	BOOLEAN,
	INTEGER,
	FLOATNUM,
	REGISTER,
	STR,
	TABLEVAL,         /* used only in assignment */
	FUNCTION,
}expkind;

typedef struct expdesc
{
	expkind k;
	union
	{
		int info;
		double number;
		tsstr *str;
	};
	int key;
	int t;
	int f;
}expdesc;

typedef struct assignlist
{
	expdesc content;
	struct assignlist *next;
}assignlist;

#define EXPISNUM(a)            (a->t==INTEGER||a->t==FLOATNUM)
#define EXPGETNUM(a)           (a->t==INTEGER?(double)a->info:a->number)




void convertvar(filestate *fs,expdesc *e)
{
	int reg;
	if(e->t == TABLEVAL)
	{
		reg = getreg(fs->curframe);
		gencodeABC(fs->curframe,op_gettable,reg,e->info,e->key);
		e->t = REGISTER;
		e->info = reg;
	}
	else if(e->t == INTEGER)
	{
		
		if((unsigned)e->info>0xffff)
		{
			reg = getreg(fs->curframe);
			gencodeAB(fs->curframe,op_loadci,reg,getconstant(&fs->envirnment->cstt,e->info));
		}
		else if((unsigned)e->info>0xff||fs->cst2reg[e->info].reg==-1)
		{
			reg = getreg(fs->curframe);
			gencodeAB(fs->curframe,op_move,reg,e->info);
			if(e->info<=0xff)
			{
				fs->cst2reg[e->info].reg = reg;
				fs->curframe->reg[reg] = &fs->cst2reg[e->info];
				if(fs->inifblock)
					fs->usedreg[fs->regpos++] = reg;
			}
		}
		else
			reg = fs->cst2reg[e->info].reg;
		
		
		e->info = reg;
		e->t = REGISTER;
	}
	else if(e->t == FLOATNUM)
	{
		reg = getreg(fs->curframe);
		gencodeAB(fs->curframe,op_loadcf,reg,getconstant(&fs->envirnment->cstt,*(uint64*)&e->number));
		e->info = reg;
		e->t = REGISTER;
	}
	else if(e->t == STR)
	{
		reg = getreg(fs->curframe);
		gencodeAB(fs->curframe,op_loads,reg,findsymbol(&fs->envirnment->strt,e->str));
		e->info = reg;
		e->t = REGISTER;
	}
	else if(e->t==VARIABLE)
	{
		reg = getvarreg(fs,e->info);
		e->t    = REGISTER;
		e->info = reg;
	}
	else if(e->t==FUNCTION)
	{
		reg = getreg(fs->curframe);
		gencodeA(fs->curframe,op_loadret,reg);
		e->info = reg;
		e->t   = REGISTER;

	}
	else if(e->t==BOOLEAN)
	{
		reg = getreg(fs->curframe);
		gencodeA(fs->curframe,op_loadflg,reg);
		e->info = reg;
		e->t   = REGISTER;
	}
}



void handleunaryopr(filestate *fs,expdesc *e,int opr)
{
	switch(opr)
	{
	case '-':
		if(e->t == INTEGER)
			e->info = -e->info;
		else if(e->t == FLOATNUM)
			e->number = -e->number;
		else if(e->t == REGISTER)
			gencodeA(fs->curframe,op_neg,e->info);
		else
			syntaxerr(fs);
		break;
	case '~':
		if(e->t == INTEGER)
			e->info = -e->info;
		else if(e->t == FLOATNUM)
			e->number = -e->number;
		else if(e->t == REGISTER ||e->t == VARIABLE||e->t == TABLEVAL)
		{
			convertvar(fs,e);
			gencodeA(fs->curframe,op_not,e->info);
			if(fs->curframe->reg[e->info])
			{
				fs->curframe->reg[e->info]->reg = -1;
				fs->curframe->reg[e->info] = NULL;
			}
		}
		else
			syntaxerr(fs);
		break;
	case '!':
		if(e->t == INTEGER)
			e->info = !e->info;
		else if(e->t == FLOATNUM)
		{
			e->t = INTEGER;
			e->number = e->number==0.0?0:1;
		}
		else if(e->t == REGISTER ||e->t == VARIABLE||e->t == TABLEVAL)
		{
			convertvar(fs,e);
			gencodeA(fs->curframe,op_lnot,e->info);
			if(fs->curframe->reg[e->info])
			{
				fs->curframe->reg[e->info]->reg = -1;
				fs->curframe->reg[e->info] = NULL;
			}
		}
		else if(e->t == STR)
		{
			e->t = INTEGER;
			if(e->str->len==1)
				e->info = 1;
			else
				e->info = 0;
		}
		else
			syntaxerr(fs);
		break;
	}
}

void infixexp(filestate *fs,int opr)
{
	int distance;
	if(opr!=tk_and&&opr!=tk_or)
		return;
	distance = fs->curframe->used-fs->curframe->lastjump;
	if(distance>0xffff)
		syntaxerr(fs);
	if(fs->curframe->lastjump==0)
		distance = 0;
	if(opr==tk_and)
	{
		gencodeAB(fs->curframe,op_jnt,fs->enterlevel,distance);
		fs->curframe->lastjump = fs->curframe->used-1;
	}
	else if(opr==tk_or)
	{
		gencodeAB(fs->curframe,op_jt,fs->enterlevel,distance);
		fs->curframe->lastjump = fs->curframe->used-1;
	}
}


void fixjump(filestate *fs)
{
	byte level,jlevel,change;
	word last;
	int cur,addr,distance,tmpaddr,lastaddr;
	tsfunc *frame;
	frame = fs->curframe;
	if(fs->enterlevel == 0||frame->lastjump==-1)
		return;
	frame = fs->curframe;
	level = fs->enterlevel;
	cur   = frame->used;
	addr  = frame->lastjump;
	change = 0;
	GETOPREND2(frame->code[addr],jlevel,last);
	tmpaddr  = 0;
	lastaddr = -1;
	
	while(1)
	{
		if(jlevel != level)
		{
			
			if(lastaddr==-1)
				lastaddr = addr;
			if(last==0)
				break;
			
			tmpaddr = addr;
			addr -= last;
			GETOPREND2(frame->code[addr],jlevel,last);
			continue;
		}
		distance = cur - addr;
		if(distance > 0xffff)
			syntaxerr(fs);
		
		frame->code[addr] &= 0xff000000;
		frame->code[addr] |= distance;
		change = 1;
		if(last == 0)
		{
			if(tmpaddr)
				frame->code[tmpaddr] &= 0xffff0000;
			break;
		}
		else
		{
			addr  -= last;
			GETOPREND2(frame->code[addr],jlevel,last);
			if(tmpaddr)
			{
				distance = tmpaddr - addr;
				if(distance > 0xffff)
					syntaxerr(fs);
				frame->code[tmpaddr] &= 0xffff0000;
				frame->code[tmpaddr] |= distance;
			}
		}
	}
	if(lastaddr>=0)
		frame->lastjump = lastaddr;
	else
		frame->lastjump = -1;
}

void patchloop(filestate *fs)
{
	int cur,addr;
	tsfunc *frame;
	if(fs->enterlevel == 0)
		return;
	frame = fs->curframe;
	cur   = frame->used;
	addr  = cur - frame->lastloop;
	gencodeA(frame,op_loop,addr);
	
}


/* calculate the oprend and generate the codes */
void opradd(filestate *fs,expdesc *e1,expdesc *e2)
{
	int reg;
	if(e1->t==INTEGER)
	{
		if(e2->t==INTEGER)
			e1->info += e2->info;
		else if(e2->t==FLOATNUM)
		{
			e1->number = (double)e1->info + e2->number;
			e1->t =FLOATNUM;
		}
		else
		{
			reg = getreg(fs->curframe);
			convertvar(fs,e1);
			if(e2->t!=REGISTER)
				convertvar(fs,e2);
			gencodeABC(fs->curframe,op_add,reg,e1->info,e2->info);
			e1->info = reg;
		}

	}
	else if(e1->t==FLOATNUM)
	{
		if(e2->t==INTEGER)
			e1->number += (double)e2->info;
		else if(e2->t==FLOATNUM)
			e1->number += e2->number;
		else
		{
			reg = getreg(fs->curframe);
			if(e2->t!=REGISTER)
				convertvar(fs,e2);
			convertvar(fs,e1);
			gencodeABC(fs->curframe,op_add,reg,e1->info,e2->info);
			e1->info = reg;
		}

	}
	else
	{
		reg = getreg(fs->curframe);
		if(e1->t!=REGISTER)
			convertvar(fs,e1);
		if(e2->t!=REGISTER)
			convertvar(fs,e2);
		gencodeABC(fs->curframe,op_add,reg,e1->info,e2->info);
		e1->info = reg;
	}
}



void oprsub(filestate *fs,expdesc *e1,expdesc *e2)
{
	int reg;
	if(e1->t==INTEGER)
	{
		if(e2->t==INTEGER)
			e1->info -= e2->info;
		else if(e2->t==FLOATNUM)
		{
			e1->number = (double)e1->info - e2->number;
			e1->t =FLOATNUM;
		}
		else
		{
			reg = getreg(fs->curframe);
			convertvar(fs,e1);
			if(e2->t!=REGISTER)
				convertvar(fs,e2);
			gencodeABC(fs->curframe,op_sub,reg,e1->info,e2->info);
			e1->info = reg;
		}

	}
	else if(e1->t==FLOATNUM)
	{
		if(e2->t==INTEGER)
			e1->number -= (double)e2->info;
		else if(e2->t==FLOATNUM)
			e1->number -= e2->number;
		else
		{
			reg = getreg(fs->curframe);
			if(e2->t!=REGISTER)
				convertvar(fs,e2);
			convertvar(fs,e1);
			gencodeABC(fs->curframe,op_sub,reg,e1->info,e2->info);
			e1->info = reg;
		}

	}
	else
	{
		reg = getreg(fs->curframe);
		if(e1->t!=REGISTER)
			convertvar(fs,e1);
		if(e2->t!=REGISTER)
			convertvar(fs,e2);
		gencodeABC(fs->curframe,op_sub,reg,e1->info,e2->info);
		e1->info = reg;
	}
}

void oprmul(filestate *fs,expdesc *e1,expdesc *e2)
{
	int reg;
	if(e1->t==INTEGER)
	{
		if(e2->t==INTEGER)
			e1->info *= e2->info;
		else if(e2->t==FLOATNUM)
		{
			e1->number = (double)e1->info * e2->number;
			e1->t =FLOATNUM;
		}
		else
		{
			reg = getreg(fs->curframe);
			convertvar(fs,e1);
			if(e2->t!=REGISTER)
				convertvar(fs,e2);
			gencodeABC(fs->curframe,op_mul,reg,e1->info,e2->info);
			e1->info = reg;
		}

	}
	else if(e1->t==FLOATNUM)
	{
		if(e2->t==INTEGER)
			e1->number *= (double)e2->info;
		else if(e2->t==FLOATNUM)
			e1->number *= e2->number;
		else
		{
			reg = getreg(fs->curframe);
			if(e2->t!=REGISTER)
				convertvar(fs,e2);
			convertvar(fs,e1);
			gencodeABC(fs->curframe,op_mul,reg,e1->info,e2->info);
			e1->info = reg;
		}

	}
	else
	{
		reg = getreg(fs->curframe);
		if(e1->t!=REGISTER)
			convertvar(fs,e1);
		if(e2->t!=REGISTER)
			convertvar(fs,e2);
		gencodeABC(fs->curframe,op_mul,reg,e1->info,e2->info);
		e1->info = reg;
	}
}

void oprdiv(filestate *fs,expdesc *e1,expdesc *e2)
{
	int reg;
	if(e1->t==INTEGER)
	{
		if(e2->t==INTEGER)
			e1->info /= e2->info;
		else if(e2->t==FLOATNUM)
		{
			e1->number = (double)e1->info / e2->number;
			e1->t =FLOATNUM;
		}
		else
		{
			reg = getreg(fs->curframe);
			convertvar(fs,e1);
			if(e2->t!=REGISTER)
				convertvar(fs,e2);
			gencodeABC(fs->curframe,op_div,reg,e1->info,e2->info);
			e1->info = reg;
		}

	}
	else if(e1->t==FLOATNUM)
	{
		if(e2->t==INTEGER)
			e1->number /= (double)e2->info;
		else if(e2->t==FLOATNUM)
			e1->number /= e2->number;
		else
		{
			reg = getreg(fs->curframe);
			if(e2->t!=REGISTER)
				convertvar(fs,e2);
			convertvar(fs,e1);
			gencodeABC(fs->curframe,op_div,reg,e1->info,e2->info);
			e1->info = reg;
		}

	}
	else
	{
		reg = getreg(fs->curframe);
		if(e1->t!=REGISTER)
			convertvar(fs,e1);
		if(e2->t!=REGISTER)
			convertvar(fs,e2);
		gencodeABC(fs->curframe,op_div,reg,e1->info,e2->info);
		e1->info = reg;
	}
}

void oprmod(filestate *fs,expdesc *e1,expdesc *e2)
{
	int reg;
	if(e1->t==FLOATNUM || e2->t==FLOATNUM || e1->t==STR || e2->f==STR)
		syntaxerr(fs);
	if(e1->t==INTEGER&&e2->t==INTEGER)
	{
		e1->info %= e2->info;
	}
	else
	{
		convertvar(fs,e1);
		convertvar(fs,e2);
		reg = getreg(fs->curframe);
		gencodeABC(fs->curframe,op_mod,reg,e1->info,e2->info);
	}

}

void oprrol(filestate *fs,expdesc *e1,expdesc *e2)
{
	int reg;
	if(e1->t!=INTEGER||e2->t!=INTEGER)
	{
		convertvar(fs,e1);
		convertvar(fs,e2);
		reg = getreg(fs->curframe);
		gencodeABC(fs->curframe,op_rol,reg,e1->info,e2->info);
	}
	else
		e1->info <<= e2->info;
}



void oprror(filestate *fs,expdesc *e1,expdesc *e2)
{
	int reg;
	if(e1->t!=INTEGER||e2->t!=INTEGER)
	{
		convertvar(fs,e1);
		convertvar(fs,e2);
		reg = getreg(fs->curframe);
		gencodeABC(fs->curframe,op_ror,reg,e1->info,e2->info);
	}
	else
		e1->info >>= e2->info;
}

void opror(filestate *fs,expdesc *e1,expdesc *e2)
{
	int reg;
	if(e1->t!=INTEGER||e2->t!=INTEGER)
	{
		convertvar(fs,e1);
		convertvar(fs,e2);
		reg = getreg(fs->curframe);
		gencodeABC(fs->curframe,op_or,reg,e1->info,e2->info);
	}
	else
		e1->info |= e2->info;
}

void oprxor(filestate *fs,expdesc *e1,expdesc *e2)
{
	int reg;
	if(e1->t!=INTEGER||e2->t!=INTEGER)
	{
		convertvar(fs,e1);
		convertvar(fs,e2);
		reg = getreg(fs->curframe);
		gencodeABC(fs->curframe,op_xor,reg,e1->info,e2->info);
	}
	else
		e1->info ^= e2->info;
}

void oprand(filestate *fs,expdesc *e1,expdesc *e2)
{
	int reg;
	if(e1->t!=INTEGER||e2->t!=INTEGER)
	{
		convertvar(fs,e1);
		convertvar(fs,e2);
		reg = getreg(fs->curframe);
		gencodeABC(fs->curframe,op_and,reg,e1->info,e2->info);
	}
	else
		e1->info &= e2->info;
}

/* && and || operation */
void oprlogical(filestate *fs,expdesc *e1,expdesc *e2,int c)
{
	int info;
	/* swap e1 and e2 if e1 is a variable */
	if(e1->t==BOOLEAN && e2->t==BOOLEAN)
		return;
	
	if(e1->t==REGISTER)
	{
		info = e1->info;
		*e1  = *e2;
		e2->t    = REGISTER;
		e2->info = info;
	}

	/* convert other types to boolean(interger) type */
	if(e1->t==FLOATNUM)
	{
		e1->t=INTEGER;
		if(e1->number==0.0)
			e1->info = 0;
		else
			e1->info = 1;
	}
	else if(e1->t==STR)
	{
		e1->t=INTEGER;
		if(e1->str->len<=1)
			e1->info = 0;
		else
			e1->info = 1;
	}
	

	if(e2->t==FLOATNUM)
	{
		e2->t=INTEGER;
		if(e2->number==0.0)
			e2->info = 0;
		else
			e2->info = 1;
	}
	else if(e2->t==STR)
	{
		e2->t=INTEGER;
		if(e2->str->len<=1)
			e2->info = 0;
		else
			e2->info = 1;
	}
	
	/* if e2 is boolean type,we can simply calculate the value */
	if(e2->t==INTEGER)
	{
		if(c==tk_or)
			e1->info = e1->info||e2->info;
		else
			e1->info = e1->info&&e2->info;
		if(e1->info)
		{
			e1->info = 1;
			gencode(fs->curframe,op_sett);
		}
		else
		{
			e1->info = 0;
			gencode(fs->curframe,op_setf);
		}
		
		return;
	}

	/* if e2 is a variable,since we know e1 is ture or false,we can simplify the calculation */
	if(e1->info != 0)
	{
		if(c==tk_or)
		{
			e1->info   = 1;
			gencode(fs->curframe,op_sett);
		}
		else
		{
			gencodeA(fs->curframe,op_setv,e2->info);
			e1->t    = BOOLEAN;
		}
	}
	else
	{
		if(c==tk_or)
		{
			gencodeA(fs->curframe,op_setv,e2->info);
			e1->t    = BOOLEAN;
		}
		else
		{
			e1->info   = 0;
			gencode(fs->curframe,op_setf);
		}
	}
}

void oprge(filestate *fs,expdesc *e1,expdesc *e2)
{
	double a,b;
	if(EXPISNUM(e1)&&EXPISNUM(e2))
	{
		a = EXPGETNUM(e1);
		b = EXPGETNUM(e2);
		if(a>=b)
		{
			e1->info = 1;
		}
		else
		{
			e1->info = 0;
		}
		e1->info = INTEGER;
		gencode(fs->curframe,e1->info?op_sett:op_setf);
	}
	else if(e1->t==STR&&e1->t==e2->t)
	{
		if(e1->str->hash == e2->str->hash)
			e1->info = memcmp(e1->str->buf,e2->str->buf,e1->str->len)>=0;
		else
			e1->info = 1;
		e1->info = INTEGER;
		gencode(fs->curframe,e1->info?op_sett:op_setf);
	}
	else
	{
		convertvar(fs,e1);
		convertvar(fs,e2);
		gencodeAB(fs->curframe,op_ge,e1->info,e2->info);
		e1->t = BOOLEAN;
	}
}

void oprle(filestate *fs,expdesc *e1,expdesc *e2)
{
	double a,b;
	if(EXPISNUM(e1)&&EXPISNUM(e2))
	{
		a = EXPGETNUM(e1);
		b = EXPGETNUM(e2);
		if(a<=b)
		{
			e1->info = 1;
		}
		else
		{
			e1->info = 0;
		}
		e1->info = INTEGER;
		gencode(fs->curframe,e1->info?op_sett:op_setf);
	}
	else if(e1->t==STR&&e1->t==e2->t)
	{
		if(e1->str->hash == e2->str->hash)
			e1->info = memcmp(e1->str->buf,e2->str->buf,e1->str->len)<=0;
		else
			e1->info = 1;
		e1->info = INTEGER;
		gencode(fs->curframe,e1->info?op_sett:op_setf);
	}
	else
	{
		convertvar(fs,e1);
		convertvar(fs,e2);
		gencodeAB(fs->curframe,op_le,e1->info,e2->info);
		e1->t = BOOLEAN;
	}
}

void oprlt(filestate *fs,expdesc *e1,expdesc *e2)
{
	double a,b;
	if(EXPISNUM(e1)&&EXPISNUM(e2))
	{
		a = EXPGETNUM(e1);
		b = EXPGETNUM(e2);
		if(a<b)
		{
			e1->info = 1;
		}
		else
		{
			e1->info = 0;
		}
		e1->info = INTEGER;
		gencode(fs->curframe,e1->info?op_sett:op_setf);
	}
	else if(e1->t==STR&&e1->t==e2->t)
	{
		if(e1->str->hash == e2->str->hash)
			e1->info = memcmp(e1->str->buf,e2->str->buf,e1->str->len)<0;
		else
			e1->info = 1;
		e1->info = INTEGER;
		gencode(fs->curframe,e1->info?op_sett:op_setf);
	}
	else
	{
		convertvar(fs,e1);
		convertvar(fs,e2);
		gencodeAB(fs->curframe,op_lt,e1->info,e2->info);
		e1->t = BOOLEAN;
	}
}

void oprgt(filestate *fs,expdesc *e1,expdesc *e2)
{
	double a,b;
	if(EXPISNUM(e1)&&EXPISNUM(e2))
	{
		a = EXPGETNUM(e1);
		b = EXPGETNUM(e2);
		if(a>=b)
		{
			e1->info = 1;
		}
		else
		{
			e1->info = 0;
		}
		e1->info = INTEGER;
		gencode(fs->curframe,e1->info?op_sett:op_setf);
	}
	else if(e1->t==STR&&e1->t==e2->t)
	{
		if(e1->str->hash == e2->str->hash)
			e1->info = memcmp(e1->str->buf,e2->str->buf,e1->str->len)>0;
		else
			e1->info = 1;
		e1->info = INTEGER;
		gencode(fs->curframe,e1->info?op_sett:op_setf);
	}
	else
	{
		convertvar(fs,e1);
		convertvar(fs,e2);
		gencodeAB(fs->curframe,op_gt,e1->info,e2->info);
		e1->t = BOOLEAN;
	}
}


void oprnequ(filestate *fs,expdesc *e1,expdesc *e2)
{
	double a,b;
	

	if(EXPISNUM(e1)&&EXPISNUM(e2))
	{
		a = EXPGETNUM(e1);
		b = EXPGETNUM(e2);
		if(a!=b)
		{
			
			e1->info = 1;
		}
		else
		{
			e1->info = 0;
		}
		e1->info = INTEGER;
		gencode(fs->curframe,e1->info?op_sett:op_setf);
	}
	else if(e1->t==STR&&e1->t==e2->t)
	{
		if(e1->str->hash == e2->str->hash)
			e1->info = memcmp(e1->str->buf,e2->str->buf,e1->str->len)!=0;
		else
			e1->info = 1;
		e1->info = INTEGER;
		gencode(fs->curframe,e1->info?op_sett:op_setf);
	}
	else
	{
		convertvar(fs,e1);
		convertvar(fs,e2);
		gencodeAB(fs->curframe,op_nequ,e1->info,e2->info);
		e1->t = BOOLEAN;
	}
}

void oprequ(filestate *fs,expdesc *e1,expdesc *e2)
{
	double a,b;
	if(e1->t==REGISTER||e2->t==REGISTER||e1->t==TABLEVAL||e2->t==TABLEVAL)
	{
		convertvar(fs,e1);
		convertvar(fs,e2);
		gencodeAB(fs->curframe,op_equ,e1->info,e2->info);
		e1->info = BOOLEAN;
		return;
	}
	
		
	if(EXPISNUM(e1)&&EXPISNUM(e2))
	{
		a = EXPGETNUM(e1);
		b = EXPGETNUM(e2);
		if(a==b)
		{
			e1->info = 1;
		}
		else
		{
			e1->info = 0;
		}
	}
	else if(e1->t==STR&&e1->t==e2->t)
	{
		if(e1->str->hash == e2->str->hash)
			e1->info = memcmp(e1->str->buf,e2->str->buf,e1->str->len)==0;
		else
			e1->info = 0;
	}
	else
	{
		e1->t = INTEGER;
		e1->info = 0;
	}
}




/* calculation function end */

void postfixexp(filestate *fs,expdesc *e1,expdesc *e2,int opr)
{
	
	if(e1->t==VARIABLE||e2->t==VARIABLE||e1->t==FUNCTION||e2->t==FUNCTION||e1->t==REGISTER||e2->t==REGISTER)
	{
		convertvar(fs,e1);
		convertvar(fs,e2);
	}
	switch(opr)
	{
	case '+':
		opradd(fs,e1,e2);
		break;
	case '-':
		oprsub(fs,e1,e2);
		break;
	case '*':
		oprmul(fs,e1,e2);
		break;
	case '/':
		oprdiv(fs,e1,e2);
		break;
	case '%':
		oprmod(fs,e1,e2);
		break;
	case '&':
		oprand(fs,e1,e2);
		break;
	case '^':
		opror(fs,e1,e2);
		break;
	case tk_ror:
		oprror(fs,e1,e2);
		break;
	case tk_rol:
		oprrol(fs,e1,e2);
		break;
	case tk_equ:
		oprequ(fs,e1,e2);
		break;
	case tk_nequ:
		oprnequ(fs,e1,e2);
		break;
	case tk_le:
		oprle(fs,e1,e2);
		break;
	case tk_ge:
		oprge(fs,e1,e2);
		break;
	case '>':
		oprgt(fs,e1,e2);
		break;
	case '<':
		oprlt(fs,e1,e2);
		break;
	case tk_and:
	case tk_or:
		//oprlogical(fs,e1,e2,opr);
		break;
	case tk_in:
		break;
	}
}

/* primaryexp -> ( Name | '(' expr ')' ){ '(' arg ')' | '[' expr ']' } */
void primaryexp(filestate *fs,expdesc *e)
{
	int pos,flag,count;
	expdesc e2;
	//tsval val;
	if(GETTOKEN(fs)=='(')
	{
		gettoken(fs);
		subexp(fs,e,0);
		CHECK_TOKEN(fs,')');
		gettoken(fs);
	}
	else if(GETNEXTTOKEN(fs)!='(')
	{
		CHECK_TOKEN(fs,tk_name);
		pos = alloclocal(fs,fs->cur.str);
		if(pos==-1)
			syntaxerr(fs);
		gettoken(fs);
		
		e->t = VARIABLE;
		e->info = pos;
	}
	else   /* function call */
	{
		pos = findsymbol(&fs->curframe->symbol,fs->cur.str);
		if(pos==-1)
		{
			pos  = getfunction(fs->envirnment,fs->cur.str);
			flag = 1;
		}
		else
			flag = 0;
		count = 0;
		gettoken(fs);
		if(GETNEXTTOKEN(fs)!=')')
		{
			do 
			{
				++count;
				gettoken(fs);
				subexp(fs,e,0);
				convertvar(fs,e);
				gencodeA(fs->curframe,op_push,e->info);
			} while (GETTOKEN(fs)==',');
			CHECK_TOKEN(fs,')');
		}
		else
			gettoken(fs);
		
		
		if(count>255)
			syntaxerr(fs);
		gettoken(fs);
		if(flag)
			gencodeAB(fs->curframe,op_callg,count,pos);
		else
			gencodeAB(fs->curframe,op_calll,count,pos);
		e->t    = FUNCTION;
		if(GETTOKEN(fs)!=';')
			convertvar(fs,e);
		
	}
	while(1)
	{
		if(GETTOKEN(fs)=='(')
		{
			count = 0;
			if(e->t==TABLEVAL)
			{
				++count;
				gencodeA(fs->curframe,op_push,e->info);
			}
			gettoken(fs);
			
			while(GETTOKEN(fs)==tk_name)
			{
				++count;
				subexp(fs,&e2,0);
				convertvar(fs,&e2);
				gettoken(fs);
				gencodeA(fs->curframe,op_push,e2.info);
				if(GETTOKEN(fs)!=',')
					break;
				gettoken(fs);
			} 
			CHECK_TOKEN(fs,')');
			if(count>255)
				syntaxerr(fs);
			convertvar(fs,e);
			gencodeAB(fs->curframe,op_calll,count,e->info);
			e->t    = FUNCTION;
			if(GETTOKEN(fs)!=';')
				convertvar(fs,e);
		}
		else if(GETTOKEN(fs)=='[')
		{
			convertvar(fs,e); 
			gettoken(fs);
			subexp(fs,&e2,0);
			CHECK_TOKEN(fs,']');
			e->t = TABLEVAL;
			convertvar(fs,&e2);
			e->key = e2.info;
		}
		else if(GETTOKEN(fs)=='.')
		{
			convertvar(fs,e);
			gettoken(fs);
			CHECK_TOKEN(fs,tk_name);
			e2.t = STR;
			e2.str = fs->cur.str;
			convertvar(fs,&e2);
			e->t = TABLEVAL;
			e->key = e2.info;
		}
		else
			break;

		gettoken(fs);
	}
	
	
}

/* simpleexp -> NUMBER | STRING | true | false | primaryexp */
void simpleexp(filestate *fs,expdesc *e)
{
	int count,reg;

	switch(GETTOKEN(fs))
	{
	case tk_int:
		e->t    = INTEGER;
		e->info = fs->cur.inum; 
		gettoken(fs);
		break;
	case tk_float:
		e->t      = FLOATNUM;
		e->number = fs->cur.fnum; 
		gettoken(fs);
		break;
	case tk_str:
		e->t    = STR;
		e->str  = fs->cur.str;
		gettoken(fs);
		break;
	case tk_true:
		e->t    = INTEGER;
		e->info = 1;
		gettoken(fs);
		break;
	case tk_false:
		e->t    = INTEGER;
		e->info = 0;
		gettoken(fs);
		break;
	case '[':
		
		count = 0;
		do
		{
			gettoken(fs);
			subexp(fs,e);
			convertvar(fs,e);
			gencodeA(fs->curframe,op_push,e->info);
			//gettoken(fs);
			++count;
		}while(GETTOKEN(fs)==',');
		CHECK_TOKEN(fs,']');
		gettoken(fs);
		reg = getreg(fs->curframe);
		gencodeAB(fs->curframe,op_newarray,reg,count);
		e->t = REGISTER;
		e->info = reg;
		break;

	case '{':
		
		count = 0;
		do 
		{
			gettoken(fs);
			subexp(fs,e,0);
			convertvar(fs,e);
			gencodeA(fs->curframe,op_push,e->info);
			CHECK_TOKEN(fs,':');
			gettoken(fs);
			subexp(fs,e,0);
			convertvar(fs,e);
			gencodeA(fs->curframe,op_push,e->info);
			++count;
			//gettoken(fs);

		} while (GETTOKEN(fs)==',');
		CHECK_TOKEN(fs,'}');
		gettoken(fs);
		reg = getreg(fs->curframe);
		gencodeAB(fs->curframe,op_newtable,reg,count);
		e->t = REGISTER;
		e->info = reg;
		break;
	default:
		primaryexp(fs,e);
	}
}

/* subexp -> (simpleexp | uniopr subexp) [ binopr subexp ] */
int subexp(filestate *fs,expdesc *e,int limit)
{
	int opr,pri,tmp;
	expdesc e2;
	opr = GETTOKEN(fs);
	if(opr==';')
		return 0;
	if(unaryopr(opr))
	{
		gettoken(fs);
		subexp(fs,e,0);
		handleunaryopr(fs,e,opr);
	}
	else
		simpleexp(fs,e);
	opr = GETTOKEN(fs);

	while(binopr(opr) && (pri = getopriviledge(opr))>limit)
	{
		gettoken(fs);
	
		infixexp(fs,opr);
		tmp = subexp(fs,&e2,pri);
		postfixexp(fs,e,&e2,opr);
		opr = tmp;
	}
	return opr;
}

void expr(filestate *fs)
{
	expdesc e;
	subexp(fs,&e,0);
}



void assignment(filestate *fs,expdesc *e)
{
	assignlist *alhead,*alp;
	int count,i,reg;
	
	if(e->t != VARIABLE && e->t != TABLEVAL)
		syntaxerr(fs);
	alhead = (assignlist*)malloc(sizeof(assignlist));
	alp = alhead;
	alhead->content = *e;
	alhead->next = NULL;
	count = 1;
	if(GETTOKEN(fs)==',') 
	{
		do 
		{
			gettoken(fs);
			primaryexp(fs,e);
			if(e->t == TABLEVAL)
				setregflag(fs->curframe,e->key);
			++count;
			alp->next = (assignlist*)malloc(sizeof(assignlist));
			
			alp = alp->next;
			alp->next = NULL;
			alp->content = *e;
			
		} while (GETTOKEN(fs)==',');
	}
	CHECK_TOKEN(fs,'=');
	gettoken(fs);
	subexp(fs,e,0);
	if(GETTOKEN(fs)==';')
	{
		
		if(count>1)
		{
			alp = alhead;
			reg = getreg(fs->curframe);
			gencodeA(fs->curframe,op_setitor,e->info);
			while(alp!=NULL)
			{
				gencodeAB(fs->curframe,op_mvlist,reg,e->info);
				if(alp->content.t==TABLEVAL)
				{
					gencodeABC(fs->curframe,op_settable,alp->content.info,alp->content.key,reg);
					clrregflag(fs->curframe,alp->content.key);
				}
				else
					gencodeAB(fs->curframe,savevar(fs,alp->content.info,reg),reg,alp->content.info&0x7fffffff);
					
					
				alp = alp->next;
			}
		}
		else
		{
			convertvar(fs,e);

			if(alp->content.t==TABLEVAL)
			{
				gencodeABC(fs->curframe,op_settable,alp->content.info,alp->content.key,e->info);
				clrregflag(fs->curframe,alp->content.key);
			}
			else
				gencodeAB(fs->curframe,savevar(fs,alp->content.info,e->info),e->info,alp->content.info&0x7fffffff);
		}
	}
	else
	{
		convertvar(fs,e);
		
		if(alhead->content.t==TABLEVAL)
		{
			gencodeABC(fs->curframe,op_settable,alhead->content.info,alhead->content.key,e->info);
			clrregflag(fs->curframe,alhead->content.key);
		}
		else
			gencodeAB(fs->curframe,savevar(fs,alhead->content.info,e->info),e->info,alhead->content.info&0x7fffffff);
		alp = alhead->next;
		for(i=1;i<count;++i)
		{
			CHECK_TOKEN(fs,',');
			gettoken(fs);
			subexp(fs,e,0);
			convertvar(fs,e);
			if(alp->content.t==TABLEVAL)
			{
				gencodeABC(fs->curframe,op_settable,alp->content.info,alp->content.key,e->info);
				clrregflag(fs->curframe,alp->content.key);
			}
			else
				gencodeAB(fs->curframe,savevar(fs,alp->content.info,e->info),e->info,alp->content.info&0x7fffffff);
			alp = alp->next;
		}
		CHECK_TOKEN(fs,';');
	}

	alp = alhead;
	while(alp!=NULL)
	{
		alhead = alp->next;
		free(alp);
		alp = alhead;
	}
}

/*
expstat-> assigment | expr
*/
int doexpression(filestate *fs)
{
	int tk;
	expdesc e;
	enterlevel(fs);
	
	//simpleexp(fs,&e);
	subexp(fs,&e,0);

	tk  = GETTOKEN(fs);
	if(tk==','||tk=='=')
		assignment(fs,&e);
	//else
		
	CHECK_TOKEN(fs,';');
	gettoken(fs);
	leavelevel(fs);
	return 0;
}


int doif(filestate *fs)
{
	byte skip,lst;
	
	expdesc e;
	skip = 0;
	gettoken(fs);
	enterlevel(fs);
	CHECK_TOKEN(fs,'(');
	gettoken(fs);
	subexp(fs,&e,0);
	CHECK_TOKEN(fs,')');
	gettoken(fs);
	if(e.t == INTEGER)
	{
		if(e.info==0)
			genjumpcode(fs,op_jmp);
		else
			skip = 1;
		
	}
	else if(e.t == FLOATNUM)
	{
		if(e.number==0.0)
			genjumpcode(fs,op_jmp);
		else
			skip = 1;
	}
	else if(e.t == STR)
	{
		if(e.str->len == 0 ||(e.str->len==1&&e.str->buf[0]==0))
			genjumpcode(fs,op_jmp);
		else
			skip = 1;
	}
	else
	{
		++fs->curframe->used;
		fixjump(fs);
		--fs->curframe->used;
		genjumpcode(fs,op_jnt);
	}
	lst = fs->inifblock;
	fs->inifblock = 1;
	if(GETTOKEN(fs)=='{')
	{
		gettoken(fs);
		do
		{
			dostatement(fs);
		}while(GETTOKEN(fs)!='}');
		gettoken(fs);
	}
	else
		dostatement(fs);
	clearregche2(fs);
	fs->inifblock = lst;
	
	
	if(GETTOKEN(fs)==tk_else)
	{
		leavelevel(fs);
		genjumpcode(fs,op_jmp);
		enterlevel(fs);
	}
	if(!skip)
		fixjump(fs);
	leavelevel(fs);
	
	return 0;

}

int doelse(filestate *fs)
{
	byte lst;
	gettoken(fs);
	if(GETTOKEN(fs)==tk_if)
		doif(fs);
	else 
	{
		lst = fs->inifblock;
		fs->inifblock = 1;
		if(GETTOKEN(fs)=='{')
		{
			enterlevel(fs);
			gettoken(fs);
			do 
			{
				dostatement(fs);
			} while (GETTOKEN(fs)!='}'&&GETTOKEN(fs)!=tk_eof);
			CHECK_TOKEN(fs,'}');
			gettoken(fs);
			leavelevel(fs);
		}
		else
		{
			enterlevel(fs);
			dostatement(fs);
			leavelevel(fs);
		}
		clearregche2(fs);
		fs->inifblock = lst;
	}
	
	
	return 0;
}

int doifblock(filestate *fs)
{
	enterlevel(fs);
	doif(fs);
	
	while(GETTOKEN(fs)==tk_else) 
		doelse(fs);
	fixjump(fs);
	leavelevel(fs);
	return 0;
}

int dofunc(filestate *fs)
{
	tsfunc *func,*parent;
	tsval tmp,*csttab;
	int pos,lastlevel,count,i;
	if(fs->curframe->parent!=NULL)
		syntaxerr(fs);
	count = 0;
	if(fs->enterlevel!=0)
		syntaxerr(fs);
	lastlevel = fs->enterlevel;
	fs->enterlevel = 0;
	gettoken(fs);
	
	CHECK_TOKEN(fs,tk_name);

	func = ts_newfunc(fs->cur.str,fs->envirnment,0);
	func->functype = tinyscript;
	func->name = fs->cur.str;
	pos  = getfunction(fs->envirnment,fs->cur.str);
	func->parent = &fs->envirnment->global;
	addfunc(fs->envirnment,pos,func);

	
	parent = fs->curframe;
	fs->curframe = func;
	/* parse the parameters */
	gettoken(fs);
	CHECK_TOKEN(fs,'(');
	if(GETNEXTTOKEN(fs)!=')')
	{
		do
		{
			gettoken(fs);
			CHECK_TOKEN(fs,tk_name);
			alloclocal(fs,fs->cur.str);
			gettoken(fs);
			++count;
		} while(GETTOKEN(fs)==',') ;
	}
	else
		gettoken(fs);
	
	CHECK_TOKEN(fs,')');
	gettoken(fs);
	
	csttab = fs->cst2reg;
	fs->cst2reg = (tsval*)malloc(sizeof(tsval)*256);
	memset(fs->cst2reg,0,sizeof(tsval)*256);
	for(i=0;i<256;++i)
		fs->cst2reg[i].reg = -1;

	CHECK_TOKEN(fs,'{');
	gettoken(fs);
	func->paramnum = count;
	tmp.t = type_func;
	tmp.func = func;
	ts_setarray(&fs->envirnment->funlist,pos,&tmp);
	do
	{
		dostatement(fs);
	}while(GETTOKEN(fs)!='}');
	CHECK_TOKEN(fs,'}');
	if(GETINS(func->code[func->used-1])!=op_ret)
		gencodeA(func,op_ret,0);

	gettoken(fs);
	fs->enterlevel = lastlevel;
	fs->curframe = parent;
	free(fs->cst2reg);
	fs->cst2reg = csttab;

	/* reset global varibles' register */
	for(i=0;i<func->global.used;++i)
	{
		pos = func->global.data[i].inum;
		parent->local.data[pos].reg = -2;
	}
	return 0;
}



int docontinue(filestate *fs)
{
	patchloop(fs);
	return 0;
}

int dobreak(filestate *fs)
{
	byte tmp;
	if(fs->looplevel==0)
		syntaxerr(fs);
	tmp = fs->enterlevel;
	fs->enterlevel = fs->looplevel;
	genjumpcode(fs,op_jmp);
	gettoken(fs);
	CHECK_TOKEN(fs,';');
	gettoken(fs);
	fs->enterlevel = tmp;
	return 0;
}

int doreturn(filestate *fs)
{
	expdesc e;
	int count,reg;
	tsfunc *func;
	func = fs->curframe;

	gettoken(fs);
	if(GETTOKEN(fs)!=';')
	{
		count = 0;
		subexp(fs,&e,0);
		convertvar(fs,&e);
		
		if(GETTOKEN(fs)==',')
		{
			gencodeA(func,op_push,e.info);
			++count;
		}
		while(GETTOKEN(fs)==',')
		{
			++count;
			gettoken(fs);
			subexp(fs,&e,0);
			convertvar(fs,&e);
			gencodeA(func,op_push,e.info);
		}
		CHECK_TOKEN(fs,';');
		
		
		if(count)
		{
			reg = getreg(fs->curframe);
			gencodeAB(func,op_newarray,reg,count);
			e.info = reg;
		}
		gencodeA(func,op_ret,e.info);
	}
	else
		gencodeA(func,op_ret,0);
	gettoken(fs);
	
	return 0;
}



int dowhile(filestate *fs)
{
	int lastloop,lastlevel,lastif;
	expdesc e;
	tsfunc *func = fs->curframe;
	lastloop = func->lastloop;
	lastlevel = fs->looplevel;
	lastif = fs->inifblock;
	fs->inifblock = 1;
	func->lastloop = func->used;
	clearregcache(func);
	enterlevel(fs);
	fs->looplevel = fs->enterlevel;
	gettoken(fs);
	CHECK_TOKEN(fs,'(');
	gettoken(fs);
	subexp(fs,&e,0);
	CHECK_TOKEN(fs,')');
	gettoken(fs);
	if(e.t == INTEGER)
	{
		if(e.info==0)
			return 0;
		//genjumpcode(fs,op_jmp);
	}
	else if(e.t == FLOATNUM)
	{
		if(e.number==0.0)
			return 0;
		//genjumpcode(fs,op_jmp);
	}
	else if(e.t == STR)
	{
		if(e.str->len == 0 ||(e.str->len==1&&e.str->buf[0]==0))
			return 0;
		//genjumpcode(fs,op_jmp);
	}
	else
	{
		fixjump(fs);
		genjumpcode(fs,op_jnt);
	}
	
	
	if(GETTOKEN(fs)=='{')
	{
		gettoken(fs);
		do 
		{
			dostatement(fs);
		} while (GETTOKEN(fs)!='}');
		gettoken(fs);
	}
	else
		dostatement(fs);
	patchloop(fs);
	fixjump(fs);
	leavelevel(fs);
	clearregche2(fs);
	fs->inifblock = lastif;
	fs->looplevel = lastlevel;
	func->lastloop = lastloop;

	return 0;
}

int dodowhile(filestate *fs)
{
	int lastloop,looplevel,lastif;
	expdesc e;
	tsfunc *func = fs->curframe;
	lastloop = func->lastloop;
	func->lastloop = func->used;
	looplevel = fs->looplevel;
	lastif = fs->inifblock;
	fs->inifblock = 1;
	clearregcache(func);
	enterlevel(fs);
	fs->looplevel = fs->enterlevel;
	gettoken(fs);
	
	CHECK_TOKEN(fs,'{');
	gettoken(fs);
	do 
	{
		dostatement(fs);
	} while (GETTOKEN(fs)!='}');

	CHECK_TOKEN(fs,'}');
	gettoken(fs);
	CHECK_TOKEN(fs,tk_while);
	gettoken(fs);

	CHECK_TOKEN(fs,'(');
	gettoken(fs);
	subexp(fs,&e,0);
	CHECK_TOKEN(fs,')');
	gettoken(fs);
	CHECK_TOKEN(fs,';');
	gettoken(fs);
	if(e.t == INTEGER)
	{
		if(e.info!=0)
			patchloop(fs);
	}
	else if(e.t == FLOATNUM)
	{
		if(e.number!=0.0)
			patchloop(fs);
	}
	else if(e.t == STR)
	{
		if((e.str->len>0&&e.str->buf[0]!=0)||e.str->len>1)
			patchloop(fs);
	}
	else
	{
		fixjump(fs);
		genjumpcode(fs,op_jnt);
		patchloop(fs);
	}
	fixjump(fs);
	leavelevel(fs);
	clearregche2(fs);
	fs->inifblock = lastif;
	func->lastloop = lastloop;
	fs->looplevel  = looplevel;

	return 0;
}

int doblock(filestate *fs)
{
	gettoken(fs);
	while(GETTOKEN(fs)!='}')
		dostatement(fs);
	gettoken(fs);
	return 0;
}

int dofor(filestate *fs)
{
	expdesc e;
	int lastloop,reg,reg2,sp,pos,i,looplevel;
	int al[64];
	tsfunc *func = fs->curframe;
	lastloop  = func->lastloop;
	looplevel = fs->looplevel;
	
	sp=0;
	enterlevel(fs);
	fs->looplevel = fs->enterlevel;
	gettoken(fs);
	CHECK_TOKEN(fs,'(');
	gettoken(fs);

	CHECK_TOKEN(fs,tk_name);
	pos = alloclocal(fs,fs->cur.str);
	al[sp++] = pos;
	gettoken(fs);
	while(GETTOKEN(fs)==','&&sp<64)
	{
		gettoken(fs);
		pos = alloclocal(fs,fs->cur.str);
		al[sp++] = pos;
		gettoken(fs);
	}
	if(sp>=64)
		syntaxerr(fs);
	CHECK_TOKEN(fs,tk_in);
	gettoken(fs);
	subexp(fs,&e,0);
	convertvar(fs,&e);

	gencodeA(func,op_setitor,e.info);
	reg = getreg(func);
	setregflag(func,reg);
	func->lastloop = func->used;
	clearregcache(func);
	if(sp>1)
	{
		gencodeAB(func,op_mvlist,reg,e.info);
		for(i=0;i<sp;++i)
		{
			reg2 = getreg(func);
			gencodeAB(func,op_mvlist,reg2,reg);
			gencodeAB(func,savevar(fs,al[i],reg2),reg2,al[i]&0x7fffffff);
		}
	}
	else
	{
		gencodeAB(func,op_mvlist,reg,e.info);
		gencodeAB(func,savevar(fs,al[0],reg),reg,al[0]&0x7fffffff);
	}
	CHECK_TOKEN(fs,')');
	gettoken(fs);
	genjumpcode(fs,op_jnt);
	if(GETTOKEN(fs)=='{')
	{
		gettoken(fs);
		do 
		{
			dostatement(fs);
		} while (GETTOKEN(fs)!='}');
		gettoken(fs);
	}
	else
		dostatement(fs);
	patchloop(fs);
	fixjump(fs);
	clrregflag(func,reg);
	leavelevel(fs);
	func->lastloop = lastloop;
	fs->looplevel  = looplevel;
	return 0;
}


int doglobal(filestate *fs)
{
	int pos;
	tsfunc *func;
	tsval val;
	val.t = type_int;
	func = fs->curframe;
	
	if(func->parent==NULL)
	{
		do 
		{
			gettoken(fs);
			CHECK_TOKEN(fs,tk_name);
			alloclocal(fs,fs->cur.str);
			gettoken(fs);
		} while (GETTOKEN(fs)==',');
		CHECK_TOKEN(fs,';');
		gettoken(fs);
		return 0;
	}


	do 
	{
		gettoken(fs);
		CHECK_TOKEN(fs,tk_name);
		if((pos = findsymbol(&func->parent->symbol,fs->cur.str))==-1)
		{
			fs->curframe = func->parent;
			pos = alloclocal(fs,fs->cur.str);
			fs->curframe = func;
		}
		
		setsymbol(&func->symbol,fs->cur.str,pos|0x80000000);
		val.inum = pos;  
		ts_appendarray(&func->global,&val);
		func->parent->local.data[pos].reg = -1;
		gettoken(fs);
	} while (GETTOKEN(fs)==',');
	CHECK_TOKEN(fs,';');
	gettoken(fs);
	return 0;
}

int dostatement(filestate *fs)
{
	int ret = 0;
	int tk  = GETTOKEN(fs);
	switch(tk)
	{
	case '}':
		gettoken(fs);
		break;
	case '{':
		ret = doblock(fs);
		break;
	case ';':
		ret = 0;
		gettoken(fs);
		break;
	case tk_global:
		ret = doglobal(fs);
		break;
	case tk_for:
		ret = dofor(fs);
		break;
	case tk_do:
		ret = dodowhile(fs);
		break;
	case tk_while:
		ret = dowhile(fs);
		break;
	case tk_if:
		ret = doifblock(fs);
		break;
	case tk_break:
		ret = dobreak(fs);
		break;
	case tk_continue:
		ret = docontinue(fs);
		break;
	case tk_ret:
		ret = doreturn(fs);
		break;
	case tk_func:
		ret = dofunc(fs);
		break;
	default:
		ret = doexpression(fs);
	}
	return ret;
}

 
