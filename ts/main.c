#include "tload.h"
#include "tlex.h"
#include "tvm.h"

/*
int main(int argc,char* argv[])
{
	filestate fs;
	tsvm *vm;
	
	
    
    


	initlex(&fs,
		
		"function printdata(n)"
		"{i=0;"
		"while(i<8)"
		"{"
		"print('+'*n[i]+'Q'+'+'*(7-n[i]));"
		"i=i+1;"
		"}"
		"print('-----------------------------');"
		"}"
		"function check(n,cur)"
		"{i=0;"
		"while(i<cur)"
		"{"

		"if(n[i]==n[cur]||(abs(i-cur)==abs(n[i]-n[cur]))) return false;"
		"i=i+1;"
		"}"
		"return true;"
		"}"   
		"function queen(n,arr)\n"
		"{"
		"global count;"
		"i=0;"
		"while(i<8)"
		"{"

		"arr[n]=i;"
		"if(check(arr,n))"
		"{"
		"if(n>=7){printdata(arr);count=count+1;}else{queen(n+1,arr);}"
		"}"
		"i=i+1;"
		""
		"}"
		"}"
		//"printdata([0,1,2,3,4,5,6,7]);"
		"count=0;"
		"queen(0,[0,0,0,0,0,0,0,0]);"
		"print(count);"
		"b=~count;"
		"print(b);"
		"c=[[1,3,4],[5,6,7]];"
		"print(c[0][1]);"
		"k,f,j=1,2,3;"
		"print(k,f,j);"
		"function test(a,b){print(a+b);}"
		"test(2+3+4,1+3+4);"
		//"f.open();"
		);

	gettoken(&fs);
	gettoken(&fs);
	while(GETTOKEN((&fs))!=tk_eof)
		dostatement(&fs);
	
	vm = newvm(fs.envirnment);
	while(step(vm)==0);//printf("grabage count:%d\n",vm->gcobj_count);
	disassemble(fs.envirnment);
	return 0;
}
*/
void usage()
{
	printf("tinyscript usage:\n");
	printf("ts [source file]\n;");
	printf("ts -b [binary file]\n;");
}

int main(int argc,char *argv[])
{
	tsvm *vm;
	if(argc!=2)
	{
		usage();
		return 0;
	}

	
	vm = loadfile(argv[1]);

	if(vm==NULL)
	{
		printf("input error.\n");
		return 0;
	}
	
	runvm(vm);
	
	return 0;
}