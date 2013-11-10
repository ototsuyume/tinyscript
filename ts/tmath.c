#include <stdlib.h>
#include <math.h>
#include "terror.h"
#include "tobject.h"
#include "tmath.h"


int tsmath_pow(tsvm *vm,int pcount)
{
	tsval *pv1,*pv2;
	tsval ret;
	pv2 = &vm->dstack[vm->dsp-1];
	pv1 = &vm->dstack[vm->dsp-2];
	if(isnumber(pv1)&&isnumber(pv2))
	{
		ret.t = type_float;
		ret.fnum = pow(getnumber(pv1),getnumber(pv2));
	}
	else
		return rterr_obj_typeerror;

	SETRET(vm,ret);
	return 0;
}


int tsmath_sqrt(tsvm *vm,int pcount)
{
	tsval *pv;
	tsval ret;
	pv = &vm->dstack[vm->dsp-1];
	if(isnumber(pv))
	{
		ret.t = type_float;
		ret.fnum = sqrt(getnumber(pv));
	}
	else
		return rterr_obj_typeerror;

	SETRET(vm,ret);
	return 0;
}

int tsmath_abs(tsvm *vm,int pcount)
{
	tsval *pv;
	tsval ret;
	pv = &vm->dstack[vm->dsp-1];
	if(isint(pv))
	{
		ret.t = type_int;
		ret.inum = abs(getint(pv));
	}
	else if(isfloat(pv))
	{
		ret.t = type_float;
		ret.fnum = abs(getfloat(pv));
	}
	else
		return rterr_obj_typeerror;

	SETRET(vm,ret);
	return 0;
}

funcdef mathlib[] = {
	{"abs",tsmath_abs,1},
	{"pow",tsmath_pow,2},
	{"sqrt",tsmath_sqrt,1},
	{"",0}
};

void ts_init_mathlib(envir *e)
{
	addbuiltin(e,mathlib);
}