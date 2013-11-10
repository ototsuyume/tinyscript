

#ifndef tvm_h
#define tvm_h

#include "tobject.h"
#include "topcode.h"
#include "tgc.h"


#define SETRET(vm,ret)      {vm->retval = ret;vm->lastret = vm->curf.pc;}



typedef struct tsframe
{
	int pc;
	int regbase;
	int varbase;
	tsfunc *funbody;
}tsframe;

typedef struct tsvm
{
	envir      *e;
	
	int        line;          /* current source code line */
	tsframe    curf;          /* current function frame */
	tsframe    *cstack;       /* call stack */
	tsval      *dstack;       /* data stack */

	tsstr      **strtable;     /* constant table and string table wouldn't change after they had been built */
	int        stsize;
	uint64     *cstt;
	int        cstsize;
	tsfunc     **ftlist;
	int        ftsize;

	tsval      retval;
	int        csp;
	int        dsp;
	char       flag;
	int        lastret;

	tsgarbage  *gclist;
	int        step_count;     /* used for deciding whether to start collecting garbage */
	int        gcobj_count;
	byte       gcstate;
	byte       gcflag;
}tsvm;

void  runvm(tsvm *vm);
tsvm* newvm(envir *e);
int   step(tsvm *vm);
void  disassemble(envir *e);




#endif