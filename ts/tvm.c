#include <stdlib.h>
#include "tvm.h"
#include "tgc.h"
#include "tarray.h"
#include "tstring.h"
#include "ttable.h"

#define MAX_DATASTACK_SIZE   409600
#define MAX_CALLSTACK_SIZE   1024

#define UNHASHABLE(v)                      (v->t!=type_int&&v->t!=type_float&&v->t!=type_str)

#define GETREGVAL(vm,frame,num)            (vm->dstack[frame->regbase+num])
#define PUSHVAL(vm,val)                    (vm->dstack[vm->dsp++]=val)
#define POPVAL(vm)                         (vm->dstack[--vm->dsp])

#define ADDGCOBJ(vm,obj)                   {\
	((tsgarbage*)(obj))->next = vm->gclist;\
	vm->gclist=((tsgarbage*)(obj));\
	++vm->gcobj_count;\
}


#define TESTIFTRUE(vm,val)   \
{\
	if(val.t==type_null)\
		vm->flag = 0;\
	else if(val.t==type_int)\
		vm->flag = val.inum!=0;\
	else if(val.t==type_float)\
		vm->flag = val.fnum!=0.0;\
	else if(val.t==type_str)\
		vm->flag = (val.str->len >1 || (val.str->len==1 && val.str->buf[0]!=0));\
	else if(val.t==type_array)\
		vm->flag = val.a->used>0;\
	else if(val.t==type_table)\
		vm->flag = val.tab->used>0;\
	else\
		vm->flag = 1;\
}

typedef struct _codeinfo
{
	char *code;
	char oprend;
}codeinfo;

const static codeinfo opinfo[] = {
	{"push",1},
	{"pop",1},            
	{"move",2},
	{"moveR",2},
	{"loadg",2},
	{"saveg",2},
	{"loadl",2},
	{"savel",2},
	{"loads",2},
	{"loadci",2},
	{"loadcf",2},
	{"news",2},
	{"add",3},
	{"sub",3},
	{"mul",3},
	{"div",3},
	{"mod",3},
	{"and",3},
	{"or",3},
	{"xor",3},
	{"not",2},
	{"rol",3},
	{"ror",3},
	{"neg",2},
	{"lnot",2},
	{"gt",2},
	{"ge",2},
	{"lt",2},
	{"le",2},
	{"equ",2},
	{"nequ",2},
	{"setv",1},
	{"sett",0},
	{"setf",0},
	{"jmp",1},
	{"loop",1},
	{"jt",1},
	{"jnt",1},
	{"callg",1},  
	{"calll",1},    
	{"callo",1},      
	{"ret",1},
	{"loadret",1},
	{"loadflg",1},
	{"newarr",2},
	{"newtab",2},
	{"gettab",3},
	{"settab",3},
	{"setitor",1},
	{"mvlist",2},
	{"nop",0}
};


void disassfunc(tsfunc *f)
{
	int ins,a,b,c,code,pc,oprcount;
	symnode *node;
	cstnode *cnode;
	if(f->name == NULL)
		printf("Global code\n");
	else
		printf("In function:%s\n",f->name->buf);

	pc = 0;

	while(pc<f->used)
	{
		code = f->code[pc++];
		ins  = GETINS(code);
		oprcount = opinfo[ins].oprend;
		printf("%d\t",pc);
		if(oprcount == 1)
		{
			GETOPREND(code,a);
			if(ins == op_callg)
			{
				node = f->e->funt.head;
				while(a>0)
				{
					node = node->next;
					--a;
				}
				printf("callg\t%s\n",node->key->buf);
			}
			else if(ins == op_calll)
			{
				node = f->symbol.head;
				
				printf("calll\tr%d\n",a);
			}
			else if(ins >= op_jmp &&ins <= op_jnt)
			{
				if(ins == op_loop)
					a = pc-a;
				else
					a = pc+a;
				printf("%s\t%d\n",opinfo[ins].code,a);
			}
			else
				printf("%s\tr%d\n",opinfo[ins].code,a);
		}
		else if(oprcount == 2)
		{
			GETOPREND2(code,a,b);
			if(ins == op_move)
				printf("move\tr%d,%d\n",a,b);
			else if(ins == op_moveR)
				printf("moveR\tr%d,r%d\n",a,b);
			else if(ins==op_saveg||ins==op_loadg)
			{
				if(f->parent)
				node = f->parent->symbol.head;
				else
					node = f->symbol.head;
				while(b>0)
				{
					node = node->next;
					--b;
				}
				printf("%s\tr%d,%s\n",opinfo[ins].code,a,node->key->buf);
			}
			else if(ins==op_savel||ins==op_loadl)
			{
				node = f->symbol.head;
				while(b>0)
				{
					--b;
					node = node->next;
					if((node->data&0x80000000)!=0)   /* skip global varibles' name */
						node = node->next;	
				}
				printf("%s\tr%d,%s\n",opinfo[ins].code,a,node->key->buf);
			}
			else if(ins==op_loads)
			{
				node = f->e->strt.head;
				while(b>0)
				{
					node = node->next;
					--b;
				}
				printf("loads\tr%d,'%s'\n",a,node->key->buf);
			}
			else if(ins == op_loadci)
			{
				cnode = f->e->cstt.head;
				while(b>0)
				{
					cnode = cnode->next;
					--b;
				}
				printf("loadci\tr%d,%d\n",a,cnode->key);
			}
			else if(ins == op_loadcf)
			{
				cnode = f->e->cstt.head;
				while(b>0)
				{
					cnode = cnode->next;
					--b;
				}	
				printf("loadcf\tr%d,%.06f\n",a,*(double*)&cnode->key);
			}
			else if(ins ==  op_newtable)
			{
				printf("newtab\tr%d,%d\n",a,b);
			}
			else if(ins == op_newarray)
			{
				printf("newarr\tr%d,%d\n",a,b);
			}
			else
				printf("%s\tr%d,r%d\n",opinfo[ins].code,a,b);
		}
		else if(oprcount == 3)
		{
			GETOPREND3(code,a,b,c);
			printf("%s\tr%d,r%d,r%d\n",opinfo[ins].code,a,b,c);
		}
		else
			printf("%s\n",opinfo[ins].code);
	}
}

void disassemble(envir *e)
{
	int i;
	tsval v;
	for(i=0;i<e->funlist.used;++i)
	{
		ts_getarray(&e->funlist,i,&v);
		if(v.func&&v.func->functype == tinyscript)
			disassfunc(v.func);
	}
	disassfunc(&e->global);
}







tsvm *newvm(envir *e)
{
	tsvm *vm;
	symnode *node;
	cstnode *cnode;
	int i;
	vm = (tsvm*)malloc(sizeof(tsvm));
	vm->e      = e;
	vm->dstack = (tsval*)malloc(sizeof(tsval)*MAX_DATASTACK_SIZE);
	vm->cstack = (tsframe*)malloc(sizeof(tsframe)*MAX_CALLSTACK_SIZE);
	vm->csp    = 0;
	
	//vm->curf   = &vm->cstack[0];
	vm->curf.funbody = &e->global;
	vm->curf.varbase = 0;
	vm->curf.regbase = e->global.symbol.used;
	vm->curf.pc = 0;
	vm->dsp = vm->curf.regbase+256;
	vm->lastret = 0;
	/* construct string table */
	vm->stsize = e->strt.used;
	vm->strtable = (tsstr**)malloc(sizeof(tsstr*)*vm->stsize);
	node = e->strt.head;
	for(i=0;i<vm->stsize;++i,node=node->next)
		vm->strtable[i] = node->key;

	/* construct constant table */
	vm->cstsize = e->cstt.used;
	vm->cstt = (uint64*)malloc(sizeof(uint64)*vm->cstsize);
	cnode = e->cstt.head;
	for(i=0;i<vm->cstsize;++i,cnode=cnode->next)
		vm->cstt[i] = cnode->key;

	/* construct global function table */
	vm->ftsize = e->funlist.used;
	vm->ftlist = (tsfunc**)malloc(vm->ftsize*sizeof(tsfunc*));
	for(i=0;i<vm->ftsize;++i)
		vm->ftlist[i] = e->funlist.data[i].func;
	vm->line    = 1;
	vm->gcflag  = 0;
	vm->gclist  = NULL;
	vm->gcstate = state_mark;
	vm->gcobj_count = 0;
	vm->step_count  = 0;
	vm->flag = 0;
	memset(vm->dstack,0,sizeof(tsval)*MAX_DATASTACK_SIZE);
	memset(vm->cstack,0,sizeof(tsframe)*MAX_CALLSTACK_SIZE);
	return vm;
}

void runtime_error(tsvm *vm,int err_code)
{
	printf("runtime error.\n");
	exit(0);
}



int step(tsvm *vm)
{
	int code,tmp;
	tsval v,*pv,*pv1,*pv2;
	tsframe *cur;
	tsfunc  *func;
	
	int ins,oprendA,oprendB,oprendC;
	
	cur  = &vm->curf;
	func = cur->funbody; 
	if(cur->pc==func->used)
		return 1;
	
	code = func->code[cur->pc++];
	ins = GETINS(code);
	
		
	switch(ins)
	{
	case op_push:
		GETOPREND(code,oprendA);
		PUSHVAL(vm,GETREGVAL(vm,cur,oprendA));
		break;
	case op_pop:                 /* pop a number from stack */
		GETOPREND(code,oprendA);
		GETREGVAL(vm,cur,oprendA) = POPVAL(vm);
		break;
	case op_move:
		GETOPREND2(code,oprendA,oprendB);
		v.t    = type_int;
		v.inum = oprendB;
		GETREGVAL(vm,cur,oprendA) = v;
		break;
	case op_moveR:
		GETOPREND2(code,oprendA,oprendB);
		GETREGVAL(vm,cur,oprendA) = GETREGVAL(vm,cur,oprendB);
		break;
	case op_loadg:
		GETOPREND2(code,oprendA,oprendB);
		GETREGVAL(vm,cur,oprendA) = vm->dstack[oprendB];
		if(undefined(&vm->dstack[oprendB]))
			runtime_error(vm,0);
		TESTIFTRUE(vm,vm->dstack[oprendB]);
		break;
	case op_saveg:
		GETOPREND2(code,oprendA,oprendB);
		vm->dstack[oprendB] = GETREGVAL(vm,cur,oprendA);
		TESTIFTRUE(vm,vm->dstack[oprendB]);
		break;
	case op_loadl:
		GETOPREND2(code,oprendA,oprendB);
		GETREGVAL(vm,cur,oprendA) = vm->dstack[oprendB+cur->varbase];
		if(undefined(&vm->dstack[oprendB+cur->varbase]))
			runtime_error(vm,0);
		TESTIFTRUE(vm,vm->dstack[oprendB+cur->regbase]);
		break;
	case op_savel:
		GETOPREND2(code,oprendA,oprendB);
		vm->dstack[oprendB+cur->varbase] = GETREGVAL(vm,cur,oprendA);
		TESTIFTRUE(vm,vm->dstack[oprendB+cur->regbase]);
		break;
	case op_loads:
		GETOPREND2(code,oprendA,oprendB);
		pv = &GETREGVAL(vm,cur,oprendA);
		pv->t   = type_str;
		if(oprendB>=vm->stsize)
			runtime_error(vm,0);
		pv->str = vm->strtable[oprendB];
		break;
	case op_loadci:
		GETOPREND2(code,oprendA,oprendB);
		pv = &GETREGVAL(vm,cur,oprendA);
		pv->t = type_int;
		if(oprendB>=vm->cstsize)
			runtime_error(vm,0);
		pv->inum = (int)vm->cstt[oprendB];
		break;
	case op_loadcf:
		GETOPREND2(code,oprendA,oprendB);
		pv = &GETREGVAL(vm,cur,oprendA);
		pv->t = type_float;
		if(oprendB>=vm->cstsize)
			runtime_error(vm,0);
		pv->fnum = *((double*)&vm->cstt[oprendB]);
		break;
	case op_news:
		break;
	case op_add:
		GETOPREND3(code,oprendA,oprendB,oprendC);
		if(ts_add(&GETREGVAL(vm,cur,oprendB),&GETREGVAL(vm,cur,oprendC),&GETREGVAL(vm,cur,oprendA))!=0)
			runtime_error(vm,0);
		TESTIFTRUE(vm,GETREGVAL(vm,cur,oprendA));
		break;
	case op_sub:
		GETOPREND3(code,oprendA,oprendB,oprendC);
		if(ts_sub(&GETREGVAL(vm,cur,oprendB),&GETREGVAL(vm,cur,oprendC),&GETREGVAL(vm,cur,oprendA))!=0)
			runtime_error(vm,0);
		TESTIFTRUE(vm,GETREGVAL(vm,cur,oprendA));
		break;
	case op_mul:
		GETOPREND3(code,oprendA,oprendB,oprendC);
		if(ts_mul(&GETREGVAL(vm,cur,oprendB),&GETREGVAL(vm,cur,oprendC),&GETREGVAL(vm,cur,oprendA))!=0)
			runtime_error(vm,0);
		TESTIFTRUE(vm,GETREGVAL(vm,cur,oprendA));
		break;
	case op_div:
		GETOPREND3(code,oprendA,oprendB,oprendC);
		if(ts_div(&GETREGVAL(vm,cur,oprendB),&GETREGVAL(vm,cur,oprendC),&GETREGVAL(vm,cur,oprendA))!=0)
			runtime_error(vm,0);
		TESTIFTRUE(vm,GETREGVAL(vm,cur,oprendA));
		break;
	case op_mod:
		GETOPREND3(code,oprendA,oprendB,oprendC);
		if(ts_div(&GETREGVAL(vm,cur,oprendB),&GETREGVAL(vm,cur,oprendC),&GETREGVAL(vm,cur,oprendA))!=0)
			runtime_error(vm,0);
		TESTIFTRUE(vm,GETREGVAL(vm,cur,oprendA));
		break;
	case op_and:
		GETOPREND3(code,oprendA,oprendB,oprendC);
		if(ts_and(&GETREGVAL(vm,cur,oprendB),&GETREGVAL(vm,cur,oprendC),&GETREGVAL(vm,cur,oprendA))!=0)
			runtime_error(vm,0);
		TESTIFTRUE(vm,GETREGVAL(vm,cur,oprendA));
		break;
	case op_or:
		GETOPREND3(code,oprendA,oprendB,oprendC);
		if(ts_or(&GETREGVAL(vm,cur,oprendB),&GETREGVAL(vm,cur,oprendC),&GETREGVAL(vm,cur,oprendA))!=0)
			runtime_error(vm,0);
		TESTIFTRUE(vm,GETREGVAL(vm,cur,oprendA));
		break;
	case op_xor:
		GETOPREND3(code,oprendA,oprendB,oprendC);
		if(ts_xor(&GETREGVAL(vm,cur,oprendB),&GETREGVAL(vm,cur,oprendC),&GETREGVAL(vm,cur,oprendA))!=0)
			runtime_error(vm,0);
		TESTIFTRUE(vm,GETREGVAL(vm,cur,oprendA));
		break;
	case op_not:
		GETOPREND(code,oprendA);
		pv = &GETREGVAL(vm,cur,oprendA);
		if(!isint(pv))
			runtime_error(vm,0);
		pv->inum=~pv->inum;
		TESTIFTRUE(vm,(*pv));
		break;
	case op_rol:
		GETOPREND3(code,oprendA,oprendB,oprendC);
		if(ts_xor(&GETREGVAL(vm,cur,oprendB),&GETREGVAL(vm,cur,oprendC),&GETREGVAL(vm,cur,oprendA))!=0)
			runtime_error(vm,0);
		TESTIFTRUE(vm,GETREGVAL(vm,cur,oprendA));
		break;
	case op_ror:
		GETOPREND3(code,oprendA,oprendB,oprendC);
		if(ts_xor(&GETREGVAL(vm,cur,oprendB),&GETREGVAL(vm,cur,oprendC),&GETREGVAL(vm,cur,oprendA))!=0)
			runtime_error(vm,0);
		TESTIFTRUE(vm,GETREGVAL(vm,cur,oprendA));
		break;
	case op_neg:
		GETOPREND(code,oprendA);
		pv = &GETREGVAL(vm,cur,oprendA);
		if(isfloat(pv))
			pv->fnum = -pv->fnum;
		else if(isint(pv))
			pv->inum = -pv->inum;
		else
			runtime_error(vm,0);
		break;
	case op_lnot:
		GETOPREND(code,oprendA);
		pv = &GETREGVAL(vm,cur,oprendA);
		TESTIFTRUE(vm,(*pv));
		vm->flag = !vm->flag;
		pv->t    = type_int;
		pv->inum = vm->flag;
		break;
	case op_gt:
		GETOPREND2(code,oprendA,oprendB);
		pv  = &GETREGVAL(vm,cur,oprendA);
		pv1 = &GETREGVAL(vm,cur,oprendB);
		ins = ts_cmp(pv,pv1);
		if(ins==-2)
			runtime_error(vm,0);
		vm->flag = ins>0;
		break;
	case op_ge:
		GETOPREND2(code,oprendA,oprendB);
		pv  = &GETREGVAL(vm,cur,oprendA);
		pv1 = &GETREGVAL(vm,cur,oprendB);
		ins = ts_cmp(pv,pv1);
		if(ins==-2)
			runtime_error(vm,0);
		vm->flag = ins>=0;
		break;
	case op_lt:
		GETOPREND2(code,oprendA,oprendB);
		pv  = &GETREGVAL(vm,cur,oprendA);
		pv1 = &GETREGVAL(vm,cur,oprendB);
		ins = ts_cmp(pv,pv1);
		if(ins==-2)
			runtime_error(vm,0);
		vm->flag = ins<0;
		break;
	case op_le:
		GETOPREND2(code,oprendA,oprendB);
		pv  = &GETREGVAL(vm,cur,oprendA);
		pv1 = &GETREGVAL(vm,cur,oprendB);
		ins = ts_cmp(pv,pv1);
		if(ins==-2)
			runtime_error(vm,0);
		vm->flag = ins<=0;
		break;
	case op_equ:
		GETOPREND2(code,oprendA,oprendB);
		vm->flag = ts_cmp(&GETREGVAL(vm,cur,oprendA),&GETREGVAL(vm,cur,oprendB))==0;
		break;
	case op_nequ:
		GETOPREND2(code,oprendA,oprendB);
		vm->flag = ts_cmp(&GETREGVAL(vm,cur,oprendA),&GETREGVAL(vm,cur,oprendB))!=0;
		break;
	case op_setv:
		GETOPREND(code,oprendA);
		TESTIFTRUE(vm,GETREGVAL(vm,cur,oprendA));
		break;
	case op_sett:
		vm->flag = 1;
		break;
	case op_setf:
		vm->flag = 0;
		break;
	case op_jmp:
		GETOPREND(code,oprendA);
		cur->pc = cur->pc + oprendA - 1;
		break;
	case op_loop:
		GETOPREND(code,oprendA);
		cur->pc = cur->pc - oprendA - 1;
		break;
	case op_jt:
		GETOPREND(code,oprendA);
		if(vm->flag)
			cur->pc = cur->pc + oprendA - 1;
		break;
	case op_jnt:
		GETOPREND(code,oprendA);
		if(!vm->flag)
			cur->pc = cur->pc + oprendA - 1;
		break;
	case op_callg:
		GETOPREND2(code,oprendA,oprendB);
		if(oprendB>=vm->ftsize)
			runtime_error(vm,0);
		if(vm->ftlist[oprendB] == NULL)
			runtime_error(vm,0);
		if(vm->ftlist[oprendB]->paramnum!=-1&&vm->ftlist[oprendB]->paramnum!=oprendA)
			runtime_error(vm,0);
		if(vm->ftlist[oprendB]->functype==native)
		{
			vm->ftlist[oprendB]->funbody(vm,oprendA);
			vm->dsp -= oprendA;
		}
		else
		{
			vm->cstack[vm->csp++] = vm->curf;
			vm->curf.funbody = vm->ftlist[oprendB];
			vm->curf.pc = 0;
			vm->curf.varbase = vm->dsp - oprendA;
			vm->curf.regbase = vm->curf.varbase+vm->curf.funbody->symbol.used;
			vm->dsp = vm->curf.regbase+256;
		}
		
		break;
	case op_calll:
		break;
	case op_callo:
		break;
	case op_ret:
		GETOPREND(code,oprendA);

		tmp = cur->regbase + oprendA;
		vm->dsp  = cur->varbase - cur->funbody->local.used;
		vm->curf = vm->cstack[--vm->csp];
		vm->retval = vm->dstack[tmp];
		vm->lastret = vm->curf.pc;
		TESTIFTRUE(vm,vm->retval);
		break;
	case op_loadret:
		GETOPREND(code,oprendA);
		if((cur->pc-1)!=vm->lastret)
			runtime_error(vm,0);
		GETREGVAL(vm,cur,oprendA) = vm->retval;
		TESTIFTRUE(vm,GETREGVAL(vm,cur,oprendA));
		break;
	case op_newarray:
		GETOPREND2(code,oprendA,oprendB);
		pv = &GETREGVAL(vm,cur,oprendA);
		pv->t   = type_array;
		getarray(pv)  = ts_newarray();
		for(tmp=oprendB;tmp>0;--tmp)
		{
			ts_appendarray(getarray(pv),&vm->dstack[vm->dsp - tmp]);
		}
		
		vm->dsp -= oprendB;
		vm->flag = getarray(pv)->used>0;
		ADDGCOBJ(vm,pv->a);
		break;
	case op_newtable:
		GETOPREND2(code,oprendA,oprendB);
		pv = &GETREGVAL(vm,cur,oprendA);
		pv->t   = type_table;
		gettable(pv) = ts_newtable(); 
		for(tmp=oprendB;tmp>0;--tmp)
		{
			pv1 = &vm->dstack[vm->dsp - tmp*2];
			pv2 = &vm->dstack[vm->dsp - tmp*2 + 1];
			if(UNHASHABLE(pv1))
				runtime_error(vm,0);
			ts_settable(gettable(pv),pv1,pv2);
		}
		vm->dsp -= (oprendB*2);
		vm->flag =gettable(pv)->used>0;
		ADDGCOBJ(vm,pv->tab);
		break;
	case op_gettable:
		GETOPREND3(code,oprendA,oprendB,oprendC);
		pv  = &GETREGVAL(vm,cur,oprendA);
		pv1 = &GETREGVAL(vm,cur,oprendB);
		pv2 = &GETREGVAL(vm,cur,oprendC);
		if(istable(pv1))
		{
			if(ts_findtable(gettable(pv1),pv2,pv)!=0)
				runtime_error(vm,0);
			
		}
		else if(isarray(pv1)&&isint(pv2))
		{
			if(ts_getarray(getarray(pv1),getint(pv2),pv)!=0)
				runtime_error(vm,0);
		}
		else
		{
			runtime_error(vm,0);
		}
		break;
	case op_settable:
		GETOPREND3(code,oprendA,oprendB,oprendC);
		pv  = &GETREGVAL(vm,cur,oprendA);
		pv1 = &GETREGVAL(vm,cur,oprendB);
		pv2 = &GETREGVAL(vm,cur,oprendC);
		if(istable(pv))
		{
			ts_settable(gettable(pv),pv1,pv2);
		}
		else if(isarray(pv)&&isint(pv1))
		{
			if(ts_setarray(getarray(pv),getint(pv1),pv2)!=0)
				runtime_error(vm,0);
		}
		else
		{
			runtime_error(vm,0);
		}
		TESTIFTRUE(vm,GETREGVAL(vm,cur,oprendC));
		break;
	case op_setitor:
		GETOPREND(code,oprendA);
		pv = &GETREGVAL(vm,cur,oprendA);
		if(isarray(pv))
			pv->a->enumptr = 0;
		else if(istable(pv))
			pv->tab->enumptr = pv->tab->head;
		else
			runtime_error(vm,0);
		break;
	case op_mvlist:
		GETOPREND2(code,oprendA,oprendB);
		pv  = &GETREGVAL(vm,cur,oprendA);
		pv1 = &GETREGVAL(vm,cur,oprendB); 
		if(isarray(pv1))
		{
			if(pv1->a->enumptr>pv1->a->used)
				pv->t = type_null;
			else
				*pv = pv1->a->data[pv1->a->enumptr++];
		}
		else if(istable(pv1))
		{
			if(pv1->tab->enumptr==NULL)
				pv->t = type_null;
			else
			{
				*pv = pv1->tab->enumptr->key;
				pv1->tab->enumptr = pv1->tab->enumptr->next;
			}
		}
		else
			runtime_error(vm,0);

		TESTIFTRUE(vm,(*pv1));
		break;
	case op_nop:
		vm->line = code&0xffffff;
		break;
	}
	STARTGC(vm);
	return 0;
}

void runvm(tsvm *vm)
{
	while(step(vm)==0);
}