
#ifndef tgc_h
#define tgc_h

#include "tobject.h"
#include "tvm.h"



enum{
	state_mark,
	state_collect,
};


#define          GCOBJ_LIMIT    10
#define          GCSTEP_LIMIT   100


#define   STARTGC(vm)            {\
	++vm->step_count;\
	if(vm->gcobj_count>GCOBJ_LIMIT)\
		gchandler(vm);\
}

void gchandler(struct tsvm *vm);


#endif