////////////////////////////////////////////////////////////////////////////////
// File:        jinter.c
// Author:      M. Saurer, 15.12.2014
// Description: J INTERface (jinter) implementation.
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Windows ?
#ifdef _WINDOWS

#include <windows.h>

#ifdef JINTER_EXPORTS
#define JINTER_API __declspec(dllexport)
#else
#define JINTER_API __declspec(dllimport)
#endif

#else // NOT Windows

#define MAX_PATH 256

#define __USE_GNU
#include <dlfcn.h>

// Dummy
#define JINTER_API
#define __stdcall

#endif // _WINDOWS

// Exports
JINTER_API int   jtest();
JINTER_API int   jinit();
JINTER_API int   jsend(char* input);
JINTER_API char* jrecv();
JINTER_API int   jstop();

////////////////////////////////////////////////////////////////////////////////
// Some definitions of the J engine
////////////////////////////////////////////////////////////////////////////////

// SM Options
#define SMWIN    0   // j.exe    Jwdw (Windows) front end
#define SMJAVA   2   // j.jar    Jwdp (Java) front end
#define SMCON    3   // jconsole

// Output Type
#define MTYOFM   1   // formatted result array output
#define MTYOER   2   // error output
#define MTYOLOG  3   // output log
#define MTYOSYS  4   // system assertion failure
#define MTYOEXIT 5   // exit
#define MTYOFILE 6   // output 1!:2[2

////////////////////////////////////////////////////////////////////////////////
// A pointer to the J interpreter (we use a void* here)
// A global variable called jt, which holds the J interpreter
////////////////////////////////////////////////////////////////////////////////

typedef void* J;
static  J     jt = NULL;

////////////////////////////////////////////////////////////////////////////////
// Type definitions and function declarators we use to import from j.dll
////////////////////////////////////////////////////////////////////////////////

typedef J   (__stdcall *JInitType)();
typedef int (__stdcall *JFreeType)(J jt);
typedef int (__stdcall *JDoType  )(J jt, char* expr);
typedef int (__stdcall *JGetMType)(J jt, char* name, int* type, int* rank, int* shape, int* data);
typedef int (__stdcall *JSetMType)(J jt, char* name, int* type, int* rank, int* shape, int* data);
typedef void(__stdcall *JSMType  )(J jt, void*callbacks[]);

static JInitType JInit;
static JFreeType JFree;
static JDoType   JDo;
static JGetMType JGetM;
static JSetMType JSetM;
static JSMType   JSM;

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////

void __stdcall JOutput(J jt, int type, char* s);

////////////////////////////////////////////////////////////////////////////////
// The global char pointer to hold the output from j.dll
////////////////////////////////////////////////////////////////////////////////

// The maximum I/O buffer can hold 100KB
#define MAXIO (100*1024)

// The global char pointer to hold the input  to   j.dll
char gInput[MAXIO];

// The global char pointer to hold the output from j.dll
char gOutput[MAXIO];

////////////////////////////////////////////////////////////////////////////////
// The global library handle
////////////////////////////////////////////////////////////////////////////////

// Windows ?
#ifdef _WINDOWS

HMODULE gModule;

#endif // _WINDOWS

////////////////////////////////////////////////////////////////////////////////
// The one and only main function in a Windows DLL
////////////////////////////////////////////////////////////////////////////////

// Windows ?
#ifdef _WINDOWS

// DllMain
BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD   ul_reason_for_call,
                      LPVOID  lpReserved)
{
   // Assign gModule
   gModule = hModule;

   // Attach or detach ?
   switch(ul_reason_for_call)
   {
      // Attach
      case DLL_PROCESS_ATTACH:
      case DLL_THREAD_ATTACH:

         // Do nothing

         break; // End attach

      // Detach
      case DLL_THREAD_DETACH:
      case DLL_PROCESS_DETACH:

         // Do nothing

         break; // End detach
   }

   // Return
   return TRUE;
}

#endif // _WINDOWS

////////////////////////////////////////////////////////////////////////////////
// Some utility functions (callbacks) for the J interpreter
// We don't really use this stuff, but it seems necessary
////////////////////////////////////////////////////////////////////////////////

// Dummy function to handle console output
void __stdcall JOutput(J jt, int type, char* s)
{
   // Local variables
   int len = 0;

   // Get length of s
   len = strlen(s);

   // Initialize gOutput
   memset(gOutput,0,MAXIO);

   // Check len to MAXIO
   if(len<MAXIO)
   {
      memcpy(gOutput,s,len);
   }
   else
   {
      memcpy(gOutput,s,MAXIO-1);
   }

   // Do nothing else
   return;
}

////////////////////////////////////////////////////////////////////////////////
// DLL/dylib/so Interface
////////////////////////////////////////////////////////////////////////////////

#ifdef _WINDOWS

JINTER_API int jinit()
{
   // Local variables
   int       status = 0;
   int       i      = 0;

   char      ExeFileName[MAX_PATH] = { "" };
   char      ExeFilePath[MAX_PATH] = { "" };
   char      DllFileName[MAX_PATH] = { "" };
   char      ProFileName[MAX_PATH] = { "" };

   char      ARGV_z_    [MAX_PATH] = { "" };
   char      BINPATH_z_ [MAX_PATH] = { "" };

   char      TmpStr     [MAX_PATH] = { "" };

   void*     JCallBacks []         = { JOutput, 0, 0, 0, (void*)SMCON };

   HINSTANCE jdll;

   // Get exe file name
   status = GetModuleFileName(gModule,ExeFileName,MAX_PATH);
   if(status==0)
   {
      //MessageBox(NULL,"Cannot get module file name.","jinter",MB_ICONEXCLAMATION);
      return 1001;
   }

   // Get exe file path
   strcpy_s(ExeFilePath,MAX_PATH,ExeFileName);
   for(i=(int)strlen(ExeFilePath);i>=0;i--)
   {
      if(ExeFilePath[i]=='\\')
      {
         ExeFilePath[i] = '\0';
         break;
      }
   }

   // Build full path of j.dll, assuming it's in the same directory
   // as this dll
   sprintf_s(DllFileName,MAX_PATH,"%s\\j.dll",ExeFilePath);
   jdll = LoadLibrary(DllFileName);
   if(jdll==NULL)
   {
      sprintf_s(TmpStr,MAX_PATH,"Cannot load %s.",DllFileName);
      //MessageBox(NULL,TmpStr,"jinter",MB_ICONEXCLAMATION);
      return 1002;
   }

   // Get the necessary functions from j.dll
   JInit = (JInitType)GetProcAddress(jdll,"JInit");
   JFree = (JFreeType)GetProcAddress(jdll,"JFree");
   JDo   = (JDoType)  GetProcAddress(jdll,"JDo"  );
   JSetM = (JSetMType)GetProcAddress(jdll,"JSetM");
   JGetM = (JGetMType)GetProcAddress(jdll,"JGetM");
   JSM   = (JSMType)  GetProcAddress(jdll,"JSM"  );

   // Initialize J
   jt = JInit();
   if(jt==NULL)
   {
      //MessageBox(NULL,"Cannot initialize J engine.","jinter",MB_ICONEXCLAMATION);
      return 1003;
   }

   // Install callbacks in JSM
   JSM(jt,JCallBacks);

   // Setup ARGV_z_
   // NOTE: We pass ARGV_z_ always as "" because loading a GTK enabled
   //       script via ARGV_z_ result in errors
   strcpy_s(ARGV_z_,MAX_PATH,"ARGV_z_ =: ''");
   status = JDo(jt,ARGV_z_);
   if(status!=0)
   {
      //MessageBox(NULL,"Cannot initialize ARGV_z_.","jinter",MB_ICONEXCLAMATION);
      JFree(jt);
      return 1004;
   }

   // Setup BINPATH_z_
   sprintf_s(BINPATH_z_,MAX_PATH,"BINPATH_z_ =: '%s'",ExeFilePath);
   status = JDo(jt,BINPATH_z_);
   if(status!=0)
   {
      //MessageBox(NULL,"Cannot initialize BINPATH_z_.","jinter",MB_ICONEXCLAMATION);
      JFree(jt);
      return 1005;
   }

   // Run profile.ijs
   sprintf_s(ProFileName, MAX_PATH,"0!:0 <'%s\\profile.ijs'",ExeFilePath);
   status = JDo(jt,ProFileName);
   if(status!=0)
   {
      //MessageBox(NULL,"Cannot load profile.ijs.","jinter",MB_ICONEXCLAMATION);
      JFree(jt);
      return 1006;
   }

   // Return
   return 0;
}

#else // NOT Windows

int jinit()
{
   // Local variables
   int       status = 0;
   int       i      = 0;

   char      ExeFileName[MAX_PATH] = { "" };
   char      ExeFilePath[MAX_PATH] = { "" };
   char      DllFileName[MAX_PATH] = { "" };
   char      ProFileName[MAX_PATH] = { "" };

   char      ARGV_z_    [MAX_PATH] = { "" };
   char      BINPATH_z_ [MAX_PATH] = { "" };

   char      TmpStr     [MAX_PATH] = { "" };

   void*     JCallBacks []         = { JOutput, 0, 0, 0, (void*)SMCON };

   char*     DynLibrPath;
   char*     DynLibrTemp;
   size_t    DynLibrSize;

   void*     jdll;

   Dl_info   info;

   // Get exe file name
   if(dladdr(jinit,&info)==0)
   {
      return 1001;
   }
   strcpy(ExeFileName,info.dli_fname);

   // Get exe file path
   strcpy(ExeFilePath,ExeFileName);
   for(i=(int)strlen(ExeFilePath);i>=0;i--)
   {
      if(ExeFilePath[i]=='/')
      {
         ExeFilePath[i] = '\0';
         break;
      }
   }

   // Set (DY)LD_LIBRARY_PATH
   if(!strcmp(LIBEXT,"dylib"))
   {
      DynLibrTemp = getenv("DYLD_LIBRARY_PATH");
   }
   else
   {
      DynLibrTemp = getenv("LD_LIBRARY_PATH");
   }
   if(DynLibrTemp!=NULL)
   {
      DynLibrSize = (size_t)(strlen(DynLibrTemp) + strlen(ExeFilePath) + 32);
   }
   else
   {
      DynLibrSize = (size_t)(strlen(ExeFilePath) + 32);
   }
   DynLibrPath = malloc(DynLibrSize);
   sprintf(DynLibrPath,"%s:%s",DynLibrTemp,ExeFilePath);
   if(!strcmp(LIBEXT,"dylib"))
   {
      setenv("DYLD_LIBRARY_PATH",DynLibrPath,1);
   }
   else
   {
      setenv("LD_LIBRARY_PATH",DynLibrPath,1);
   }
   free(DynLibrPath);

   // Build full path of libj.so/dylib, assuming it's in the same directory
   // as this dll
   sprintf(DllFileName,"%s/libj.%s",ExeFilePath,LIBEXT);
   jdll = dlopen(DllFileName,RTLD_LAZY);
   if(!jdll)
   {
      return 1002;
   }

   // Get the necessary functions from j.dll
   JInit = (JInitType)dlsym(jdll,"JInit");
   JFree = (JFreeType)dlsym(jdll,"JFree");
   JDo   = (JDoType)  dlsym(jdll,"JDo"  );
   JSetM = (JSetMType)dlsym(jdll,"JSetM");
   JGetM = (JGetMType)dlsym(jdll,"JGetM");
   JSM   = (JSMType)  dlsym(jdll,"JSM"  );

   // Initialize J
   jt = JInit();
   if(jt==NULL)
   {
      return 1003;
   }

   // Install callbacks in JSM
   JSM(jt,JCallBacks);

   // Setup ARGV_z_
   // NOTE: We pass ARGV_z_ always as "" because loading a GTK enabled
   //       script via ARGV_z_ result in errors
   strcpy(ARGV_z_,"ARGV_z_ =: ''");
   status = JDo(jt,ARGV_z_);
   if(status!=0)
   {
      JFree(jt);
      return 1004;
   }

   // Setup BINPATH_z_
   sprintf(BINPATH_z_,"BINPATH_z_ =: '%s'",ExeFilePath);
   status = JDo(jt,BINPATH_z_);
   if(status!=0)
   {
      JFree(jt);
      return 1005;
   }

   // Run profile.ijs
   sprintf(ProFileName,"0!:0 <'%s/profile.ijs'",ExeFilePath);
   status = JDo(jt,ProFileName);
   if(status!=0)
   {
      JFree(jt);
      return 1006;
   }

   // Return
   return 0;
}

#endif // _WINDOWS

JINTER_API int jsend(char* input)
{
   // Local variables
   int status = 0;
   int len    = 0;

   // Check jt is clean
   if(jt==NULL)
   {
      status = jinit();
   }

   // Check status
   if(status!=0)
   {
      return status;
   }

   // Get length of s
   len = strlen(input);

   // Initialize gOutput
   memset(gInput,0,MAXIO);

   // Check len to MAXIO
   if(len<MAXIO)
   {
      memcpy(gInput,input,len);
   }
   else
   {
      memcpy(gInput,input,MAXIO-1);
   }

   // Send input to J interpreter
   status = JDo(jt,gInput);

   // Return status
   return status;
}

JINTER_API char* jrecv()
{
   // Local variables
   static char *out = { "\0" };

   // Return
   if(gOutput!=NULL)
   {
      return gOutput;
   }
   else
   {
      return out;
   }
}

JINTER_API int jstop()
{
   // Local variables
   int status = 0;

   // Check jt is clean
   if(jt==NULL)
   {
      status = jinit();
   }

   // Check status
   if(status!=0)
   {
      return status;
   }

   // Free J environment
   status = JFree(jt);

   // Reset jt;
   jt = NULL;

   // Return
   return status;
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
