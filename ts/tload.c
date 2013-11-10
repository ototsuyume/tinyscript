#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include "tparser.h"
#include "tload.h"

#ifndef WIN32
#include <unistd.h>
#endif


char* readfile(const char *filename)
{
	uint size;
	char *buf = NULL; 
	FILE *fp  = fopen(filename,"r");

	if(fp==NULL)
		goto err;
	if(fseek(fp,0,SEEK_END)!=0)
		goto err;

	size = (uint)ftell(fp)+1;

	if(fseek(fp,0,SEEK_SET)!=0)
		goto err;

	buf  = (char*)malloc(size+1);
	memset(buf,0,size+1);
	fread(buf,1,size,fp);
	buf[size]=0;

err:
	if(fp!=NULL)
		fclose(fp);
	return buf;
}



tsvm* loadbinary(char *buf)
{
	return NULL;
}

tsvm* loadsrc(char *buf)
{
	filestate fs;	
	tsvm *vm;
	initlex(&fs,buf);

	gettoken(&fs);
	gettoken(&fs);
	while(GETTOKEN((&fs))!=tk_eof)
		dostatement(&fs);

	vm = newvm(fs.envirnment);

	return vm;
}


tsvm* loadfile(const char* filename)
{
	char binfile[512];
	char *buf;
	uint filetime;
	tsvm *vm = NULL;
#ifdef WIN32
	struct _stat st;
	if(stat(filename,&st)!=0)
		goto err;
	filetime = (uint)st.st_mtime;
#else
	struct stat st;
	if(lstat(filename,&st)!=0)
		goto err;
	filetime = (uint)st.st_mtime;
#endif
	
	buf = readfile(filename);
	vm  = loadsrc(buf);
err:
	return vm;
}