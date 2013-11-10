#include <stdlib.h>
#include <memory.h>
#include "tgc.h"

#define ISGARBAGE(vm,obj) 

static void freestr(tsstr *str);
static void freearray(tsarray *arr);
static void freetable(tstable *tab);
static void markarray(tsvm *vm,tsarray *arr);
static void marktable(tsvm *vm,tstable *tab);
static void markframe(tsvm *vm,tsframe *frame);
static void scangcobj(tsvm *vm);
static void collectgcobj(tsvm *vm);

static void freestr(tsstr *str)
{
	free(str->buf);
	free(str);
}

static void freearray(tsarray *arr)
{
	free(arr->data);
	free(arr);
}

static void freetable(tstable *tab)
{
	free(tab->slot);
	free(tab);
}

static void markarray(tsvm *vm,tsarray *arr)
{
	int i;
	tsval *pv;
	arr->gchead.color = vm->gcflag;
	for(i=0;i<arr->used;++i)
	{
		pv = &arr->data[i];
		if(isarray(pv))
			markarray(vm,pv->a);
		else if(istable(pv))
			marktable(vm,pv->tab);
		else if(isstr(pv))
			pv->str->gchead.color = vm->gcflag;
	}
}


static void marktable(tsvm *vm,tstable *tab)
{
	tnode *tmp;
	tab->gchead.color = vm->gcflag;
	tmp = tab->head;
	while(tmp)
	{
		if(isarray(&tmp->data))
			markarray(vm,tmp->data.a);
		else if(istable(&tmp->data))
			marktable(vm,tmp->data.tab);

		if(isarray(&tmp->key))
			markarray(vm,tmp->key.a);
		else if(istable(&tmp->key))
			marktable(vm,tmp->key.tab);
		tmp = tmp->next;
	}
}


static void markframe(tsvm *vm,tsframe *frame)
{
	int i;
	tsval *tmpobj;
	for(i=frame->varbase;i<frame->regbase;++i)
	{
		tmpobj = &vm->dstack[i];
		if(isarray(tmpobj))
			markarray(vm,tmpobj->a);
		else if(istable(tmpobj))
			marktable(vm,tmpobj->tab);
		else if(isstr(tmpobj))
			tmpobj->str->gchead.color = vm->gcflag;
	}

	for(i=frame->regbase;i<frame->regbase+256;++i)
	{
		tmpobj = &vm->dstack[i];
		if(isarray(tmpobj))
			markarray(vm,tmpobj->a);
		else if(istable(tmpobj))
			marktable(vm,tmpobj->tab);
		else if(isstr(tmpobj))
			tmpobj->str->gchead.color = vm->gcflag;
	}
}

static void scangcobj(tsvm *vm)
{
	int clen,i;
	markframe(vm,&vm->curf);
	clen = vm->cstsize;
	for(i=0;i<clen;++i)
		markframe(vm,&vm->cstack[i]);
}

static void collectgcobj(tsvm *vm)
{
	tsgarbage *gb,*last,*head,*tmp;
	gb = vm->gclist;
	last = head = NULL;
	while(gb)
	{
		if(gb->color!=vm->gcflag&&gb->color!=0xff)
		{
			tmp = gb->next;
			if(last)
				last->next = tmp;
			if(gb->type==type_str)
				freestr((tsstr*)gb);
			else if(gb->type==type_array)
				freearray((tsarray*)gb);
			else if(gb->type==type_table)
				freetable((tstable*)gb);
			gb = tmp;
			--vm->gcobj_count;
		}
		else
		{
			last = gb;
			gb   = gb->next;
			if(head==NULL)
				head = last;
		}
	}
	vm->gclist = head;
}

void gchandler(tsvm *vm)
{
	if(vm->gcstate == state_mark)
	{
		scangcobj(vm);
		vm->gcstate = state_collect;
	}
	else 
	{
		collectgcobj(vm);
		vm->gcflag = !vm->gcflag;
		vm->gcstate    = state_mark;
		//vm->step_count = 0;
	}
}