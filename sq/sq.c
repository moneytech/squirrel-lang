/*	see copyright notice in squirrel.h */

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifdef _WIN32
//#include <conio.h>
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#include <conio.h>
#endif
#include <squirrel.h>
#include <sqstdblob.h>
#include <sqstdmodule.h>
#include <sqstdsystem.h>
#include <sqstdio.h>
#include <sqstdmath.h>	
#include <sqstdstring.h>

#include <time.h>
#include <math.h>

#ifdef SQUNICODE
#define scfprintf fwprintf
#define scfopen	_wfopen
#define scvprintf vwprintf
#else
#define scfprintf fprintf
#define scfopen	fopen
#define scvprintf vprintf
#endif

int CompileScriptFromFile(HSQUIRRELVM,const SQChar *name,int bprinterror);
void PrintCallStack(HSQUIRRELVM);
void PrintVersionInfos();

#if defined(_MSC_VER) && defined(_DEBUG)
int MemAllocHook( int allocType, void *userData, size_t size, int blockType, 
   long requestNumber, const unsigned char *filename, int lineNumber)
{
	//if(requestNumber==2822)_asm int 3;
	return 1;
}
#endif

int printerror(HSQUIRRELVM v)
{
	const SQChar *sErr=NULL;
	if(sq_gettop(v)>=1){
		if(SQ_SUCCEEDED(sq_getstring(v,2,&sErr)))	{
			scfprintf(stderr,_SC("\nAN ERROR HAS OCCURED [%s]\n"),sErr);
		}
		else{
			scfprintf(stderr,_SC("\nAN ERROR HAS OCCURED [unknown]\n"));
		}
		PrintCallStack(v);
	}
	return 0;
}

int compile_file(HSQUIRRELVM v)
{
	const SQChar *sFileName;
	if(SQ_SUCCEEDED(sq_getstring(v,2,&sFileName))){
		return CompileScriptFromFile(v,sFileName,0);
	}
	return sq_throwerror(v,_SC("wrong argument"));
}

int quit(HSQUIRRELVM v)
{
	int *done;
	sq_getuserpointer(v,-1,(SQUserPointer*)&done);
	*done=1;
	return 0;
}

void printfunc(HSQUIRRELVM v,const SQChar *s,...)
{
	va_list vl;
	va_start(vl, s);
	scvprintf( s, vl);
	va_end(vl);
}

#define _INTERACTIVE 0
#define _VERSION 1
#define _DONE 2
//<<FIXME>> this func is a mess
int getargs(HSQUIRRELVM v,int argc, char* argv[])
{
	int i;
	static SQChar temp[500];
	const SQChar *ret=NULL;
	int lineinfo=0;
	if(argc>1)
	{
		int arg=1,exitloop=0;
		while(arg<argc && !exitloop)
		{

			if(argv[arg][0]=='-')
			{
				switch(argv[arg][1])
				{
				case 'd': //DEBUG(debug infos)
					sq_enabledebuginfo(v,1);
					break;
				case 'v':
					return _VERSION;
				default:
					exitloop=1;
				}
			}else break;
			arg++;
		}

		// src file
		
		if(arg<argc){
			const SQChar *filename=NULL;
#ifdef SQUNICODE
			mbstowcs(temp,argv[arg],strlen(argv[arg]));
			filename=temp;
#else
			filename=argv[arg];
#endif

			arg++;
			sq_pushroottable(v);
			sq_pushstring(v,_SC("ARGS"),-1);
			sq_newarray(v,0);
			for(i=arg;i<argc;i++)
			{
				const SQChar *a;
#ifdef SQUNICODE
				int alen=strlen(argv[i]);
				a=sq_getscratchpad(v,alen*sizeof(SQChar));
				mbstowcs(sq_getscratchpad(v,-1),argv[i],alen);
				sq_getscratchpad(v,-1)[alen] = _SC('\0');
#else
				a=argv[i];
#endif
				sq_pushstring(v,a,-1);

				sq_arrayappend(v,-2);
			}
			sq_createslot(v,-3);
			sq_pop(v,1);
			if(CompileScriptFromFile(v,filename,1)>0){
				sq_pushroottable(v);
				sq_call(v,1,0);
				return _DONE;
			}
			else{
				const SQChar *err;
				sq_getlasterror(v);
				if(SQ_SUCCEEDED(sq_getstring(v,-1,&err))){
					scprintf(_SC("Error [%s]\n"),err);
				}
				return _VERSION;
			}
		}
	}

	return _INTERACTIVE;
}


void PrintVersionInfos()
{
	scfprintf(stdout,_SC("%s %s\n"),SQUIRREL_VERSION,SQUIRREL_COPYRIGHT);
}

void PrintUsage()
{
	scfprintf(stderr,_SC("usage: sq <options> <scriptpath [args]>.\n")
		_SC("Available options are:\n")
		_SC("   -d              generate debug infos\n")
		_SC("   -v				display version infos\n"));
}

void PrintCallStack(HSQUIRRELVM v)
{
	SQStackInfos si;
	SQInteger i;
	SQFloat f;
	const SQChar *s;
	int level=1; //1 is to skip this function that is level 0
	const SQChar *name=NULL; 
	int seq=0;
	scfprintf(stderr,_SC("\nCALLSTACK\n"));
	while(SQ_SUCCEEDED(sq_stackinfos(v,level,&si)))
	{
		const SQChar *fn=_SC("unknown");
		const SQChar *src=_SC("unknown");
		if(si.funcname)fn=si.funcname;
		if(si.source)src=si.source;
		scfprintf(stderr,_SC("*FUNCTION [%s] %s line [%d]\n"),fn,src,si.line);
		level++;
	}
	level=0;
	scfprintf(stderr,_SC("\nLOCALS\n"));
	
	for(level=0;level<10;level++){
		seq=0;
		while(name=sq_getlocal(v,level,seq))
		{
			seq++;
			switch(sq_gettype(v,-1))
			{
			case OT_NULL:
				scfprintf(stderr,_SC("[%s] NULL\n"),name);
				break;
			case OT_INTEGER:
				sq_getinteger(v,-1,&i);
				scfprintf(stderr,_SC("[%s] %d\n"),name,i);
				break;
			case OT_FLOAT:
				sq_getfloat(v,-1,&f);
				scfprintf(stderr,_SC("[%s] %.14g\n"),name,f);
				break;
			case OT_USERPOINTER:
				scfprintf(stderr,_SC("[%s] USERPOINTER\n"),name);
				break;
			case OT_STRING:
				sq_getstring(v,-1,&s);
				scfprintf(stderr,_SC("[%s] \"%s\"\n"),name,s);
				break;
			case OT_TABLE:
				scfprintf(stderr,_SC("[%s] TABLE\n"),name);
				break;
			case OT_ARRAY:
				scfprintf(stderr,_SC("[%s] ARRAY\n"),name);
				break;
			case OT_CLOSURE:
				scfprintf(stderr,_SC("[%s] CLOSURE\n"),name);
				break;
			case OT_NATIVECLOSURE:
				scfprintf(stderr,_SC("[%s] NATIVECLOSURE\n"),name);
				break;
			case OT_USERDATA:
				scfprintf(stderr,_SC("[%s] USERDATA\n"),name);
				break;
			case OT_THREAD:
				scfprintf(stderr,_SC("[%s] THREAD\n"),name);
				break;
			}
			sq_pop(v,1);
		}
	}
}

SQInteger file_lexfeedASCII(SQUserPointer file)
{
	int ret;
	char c;
	if( ( ret=fread(&c,sizeof(c),1,(FILE *)file )>0) )
		return c;
	return 0;
}

SQInteger file_lexfeedWCHAR(SQUserPointer file)
{
	int ret;
	wchar_t c;
	if( ( ret=fread(&c,sizeof(c),1,(FILE *)file )>0) )
		return (SQChar)c;
	return 0;
}

int file_read(SQUserPointer file,SQUserPointer buf,int size)
{
	int ret;
	if( ( ret=fread(buf,1,size,(FILE *)file )!=0) )return ret;
	return -1;
}

int file_write(SQUserPointer file,SQUserPointer p,int size)
{
	return fwrite(p,1,size,(FILE *)file);
}

int CompileScript(HSQUIRRELVM v,SQLEXREADFUNC read,SQUserPointer p,const SQChar *sourcename,int bprinterror)
{
	int ret=-1;
	if(SQ_SUCCEEDED(sq_compile(v,read,p,sourcename,bprinterror))){
		ret=1;
	}
	return ret;
}

int CompileScriptFromFile(HSQUIRRELVM v,const SQChar *filename,int bprinterror)
{
	FILE *file=scfopen(filename,_SC("rb"));
	int ret;
	unsigned short uc;
	SQLEXREADFUNC func=file_lexfeedASCII;
	if(file && (ret=fread(&uc,1,2,file))){
		if(ret==2 && uc==0xFEFF)
			func=file_lexfeedWCHAR;
		else
			fseek(file,0,SEEK_SET);

		if(CompileScript(v,func,file,filename,bprinterror)>0){
			fclose(file);
			return 1;
		}
		fclose(file);
		return 0;
	}
	return sq_throwerror(v,_SC("cannot open the file"));
}

void compiler_error(HSQUIRRELVM v,const SQChar *sErr,const SQChar *sSource,int line,int column)
{
	scfprintf(stderr,_SC("ERROR %s line=(%d) column=(%d) [%s]\n"),sErr,line,column,sSource);
}

void Interactive(HSQUIRRELVM v)
{
	
#define MAXINPUT 1024
	SQChar buffer[MAXINPUT];
	int blocks =0;
	int string=0;
	int retval=0;
	int done=0;
	PrintVersionInfos();
		
	sq_pushroottable(v);
	sq_pushstring(v,_SC("quit"),-1);
	sq_pushuserpointer(v,&done);
	sq_newclosure(v,quit,1);
	sq_setparamscheck(v,1,NULL);
	sq_createslot(v,-3);
	sq_pop(v,1);

    while (!done) 
	{
		int i = 0;
		scprintf(_SC("\nsq>"));
		for(;;) {
			int c;
			if(done)return;
			c = getchar();
			if (c == _SC('\n')) {
				if (i>0 && buffer[i-1] == _SC('\\'))
				{
					buffer[i-1] = _SC('\n');
				}
				else if(blocks==0)break;
				buffer[i++] = _SC('\n');
			}
			else if (c==_SC('}')) {blocks--; buffer[i++] = (SQChar)c;}
			else if(c==_SC('{') && !string){
					blocks++;
					buffer[i++] = (SQChar)c;
			}
			else if(c==_SC('"') || c==_SC('\'')){
					string=!string;
					buffer[i++] = (SQChar)c;
			}
			else if (i >= MAXINPUT-1) {
				scfprintf(stderr, _SC("sq : input line too long\n"));
				break;
			}
			else{
				buffer[i++] = (SQChar)c;
			}
		}
		buffer[i] = _SC('\0');
		
		if(buffer[0]==_SC('=')){
			scsprintf(sq_getscratchpad(v,MAXINPUT),_SC("return (%s)"),&buffer[1]);
			memcpy(buffer,sq_getscratchpad(v,-1),(scstrlen(sq_getscratchpad(v,-1))+1)*sizeof(SQChar));
			retval=1;
		}
		i=scstrlen(buffer);
		if(i>0){
			int oldtop=sq_gettop(v);
			if(SQ_SUCCEEDED(sq_compilebuffer(v,buffer,i,_SC("interactive console"),1))){
				sq_pushroottable(v);
				if(SQ_SUCCEEDED(sq_call(v,1,retval)) &&	retval){
					scprintf(_SC("\n"));
					sq_pushroottable(v);
					sq_pushstring(v,_SC("print"),-1);
					sq_get(v,-2);
					sq_pushroottable(v);
					sq_push(v,-4);
					sq_call(v,2,0);
					retval=0;
					scprintf(_SC("\n"));
				}
			}
			
			sq_settop(v,oldtop);
		}
	}
}

SQRESULT _sqstd_moduleapi_openmodule(HSQUIRRELVM v,const SQChar* modulename ,SQSTDHMODULE* pm)
{
#define MAX_MODULE_PATH 512
	char realmodulename[MAX_MODULE_PATH];
#ifdef _UNICODE
	int slen = wcstombs(realmodulename,modulename,MAX_MODULE_PATH-1);
	realmodulename[slen] = _SC('\0');
#else
	strcpy(realmodulename,modulename);
#endif
#ifdef _WIN32
	*pm = LoadLibrary(realmodulename);
#else
	*pm = dlopen(modulename,RTLD_NOW);
#endif
	if(*pm == NULL)
		return SQ_ERROR;
	return SQ_OK;
}

void *_sqstd_moduleapi_getsymbol(HSQUIRRELVM v,SQSTDHMODULE m,const SQChar* symbol)
{
#define MAX_SYMBOL_NAME 128
	char realsymbolname[MAX_SYMBOL_NAME];
#ifdef _UNICODE
	int slen = wcstombs(realsymbolname,symbol,MAX_SYMBOL_NAME-1);
	realsymbolname[slen] = _SC('\0');
#else
	strcpy(realsymbolname,symbol);
#endif

#ifdef _WIN32
	return (void *)GetProcAddress((HMODULE)m,realsymbolname);
#else
	return dlsym(m,realsymbolname);
#endif
}

void _sqstd_moduleapi_closemodule(SQSTDHMODULE m)
{
#ifdef _WIN32
	FreeLibrary((HMODULE)m);
#else
	dlclose(m);
#endif
}

SQSTDUserModuleAPI api = {
	_sqstd_moduleapi_openmodule,
	_sqstd_moduleapi_getsymbol,
	_sqstd_moduleapi_closemodule
};

void *x_malloc(unsigned int size) {
	return malloc(size);
}

void x_free(void *p,unsigned int size) {
	free(p);
}

int main(int argc, char* argv[])
{
	const SQChar *error;
	HSQUIRRELVM v;
	
	const SQChar *filename=NULL;
#if defined(_MSC_VER) && defined(_DEBUG)
	_CrtSetAllocHook(MemAllocHook);
#endif
	
	v=sq_open(1024);
	sq_setprintfunc(v,printfunc);

	sq_pushroottable(v);

	sqstd_register_bloblib(v);
	sqstd_register_iolib(v);
	if(SQ_FAILED(sqstd_register_modulelib(v,&api))) {
		sq_getlasterror(v);
		sq_getstring(v,-1,&error);
		scprintf(_SC("MODULE LIB [%s]\n"),error);
		sq_pop(v,1);
	}
	
	sqstd_register_systemlib(v);
	sqstd_register_mathlib(v);
	sqstd_register_stringlib(v);
	

	//sets error handlers
	sq_setcompilererrorhandler(v,compiler_error);
	sq_newclosure(v,printerror,0);
	sq_seterrorhandler(v);

	sq_pushroottable(v);
	sq_pushstring(v,_SC("compile_file"),-1);
	sq_newclosure(v,compile_file,0);
	sq_setparamscheck(v,-2,NULL);
	sq_createslot(v,-3);
	sq_pop(v,1);

	//gets arguments
	switch(getargs(v,argc,argv))
	{
	case _VERSION:
		PrintVersionInfos();
		break;
	case _INTERACTIVE:
		Interactive(v);
		break;
	case _DONE:
	default: 
		break;
	}
		/* SAVE & LOAD THE BYTECODE BEFORE EXECUTE IT(is just a sample)
		FILE *bytecodefile=fopen("./bytecode.cnut","wb+");
		sq_writeclosure(v,file_write,bytecodefile);
		fclose(bytecodefile);

		sq_pop(v,1);

		bytecodefile=fopen("./bytecode.cnut","rb");
		sq_readclosure(v,file_read,bytecodefile);
		fclose(bytecodefile);*/

	sq_close(v);
	
#if defined(_MSC_VER) && defined(_DEBUG)
	_getch();
	_CrtMemDumpAllObjectsSince( NULL );
#endif
	return 0;
}

