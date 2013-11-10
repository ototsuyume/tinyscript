

#ifndef topcode_h
#define topcode_h

#include "tobject.h"
#include "tlib.h"

/* opcodes definition */
/* all numeric operand must be less than 0x10000.If a numeric operand was greater than 0xffff,it should be stored 
 in the constant table. */
enum{
	op_push,               /* push a register id */
	op_pop,                 /* pop a number from stack */
	op_move,
	op_moveR,
	op_loadg,
	op_saveg,
	op_loadl,
	op_savel,
	op_loads,
	op_loadci,
	op_loadcf,
	op_news,
	op_add,
	op_sub,
	op_mul,
	op_div,
	op_mod,
	op_and,
	op_or,
	op_xor,
	op_not,
	op_rol,
	op_ror,
	op_neg,
	op_lnot,
	op_gt,
	op_ge,
	op_lt,
	op_le,
	op_equ,
	op_nequ,
	op_setv,
	op_sett,
	op_setf,
	op_jmp,
	op_loop,
	op_jt,
	op_jnt,
	op_callg,        /* call global method */
	op_calll,        /* call local method */
	op_callo,        /* call object method */
	op_ret,
	op_loadret,
	op_loadflg,
	op_newarray,
	op_newtable,
	op_gettable,
	op_settable,
	op_setitor,
	op_mvlist,
	op_nop,
	op_halt,
};



#define GETINS(op)                         (op>>24)
#define GETOPREND(op,a)                    {a=op&0xffff;}
#define GETOPREND2(op,a,b)                 {b=op&0xffff;a=(op>>16)&0xff;}
#define GETOPREND3(op,a,b,c)               {c=op&0xff;b=(op>>8)&0xff;a=(op>>16)&0xff;}


void gencodeABC(tsfunc *cur,byte code,byte a,byte b,byte c);
void gencodeAB(tsfunc *cur,byte code,byte a,word b);
void gencodeA(tsfunc *cur,byte code,word a);
void gencode(tsfunc *cur,byte code);

void disassemble(envir *e);


#endif