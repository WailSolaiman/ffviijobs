#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include "runtime.generated.h"
#define SLOT ((volatile uint32_t *)0x7B5FF0)
/* Runs once at the first WinMain stack-setup call, after 7H has activated file overrides.
 * Only the compact entry patch lives in the verified executable padding.
 * The freestanding core is loaded into newly allocated process memory.
 */
__declspec(dllexport) void __cdecl bootstrap_load(void){
    *(volatile uint32_t *)0x67DB38=0x130E43E8u;
    *(volatile uint8_t *)0x67DB3C=0;
    SLOT[0]=0;SLOT[1]=2;
    volatile uint32_t kernel[4],module[5],create[3],suffix[11];
@KERNEL@
@MODULE@
@CREATE@
@SUFFIX@
    HMODULE k=((HMODULE (WINAPI *)(LPCSTR))*(uintptr_t *)0x7B61E0)((const char *)kernel);
    FARPROC (WINAPI *get)(HMODULE,LPCSTR)=(void *)*(uintptr_t *)0x7B6110;
    if(!k || !get)return;
    DWORD (WINAPI *get_path)(HMODULE,LPWSTR,DWORD)=(void *)get(k,(const char *)module);
    HANDLE (WINAPI *open)(LPCWSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE)=(void *)get(k,(const char *)create);
    if(!get_path || !open)return;
    wchar_t path[768];DWORD n=get_path(NULL,path,740);if(!n || n>=740)return;
    while(n && path[n-1]!='\\')--n;if(!n)return;
    --n;const wchar_t *ending=(const wchar_t *)suffix;unsigned i=0;
    do{path[n+i]=ending[i];}while(ending[i++]);
    HANDLE file=open(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return;
    uint8_t *data=((void *(WINAPI *)(void *,SIZE_T,DWORD,DWORD))*(uintptr_t *)0x7B6120)(NULL,RUNTIME_SIZE,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
    if(!data){((BOOL (WINAPI *)(HANDLE))*(uintptr_t *)0x7B6108)(file);return;}
    DWORD read=0;
    BOOL ok=((BOOL (WINAPI *)(HANDLE,void *,DWORD,DWORD *,OVERLAPPED *))*(uintptr_t *)0x7B60B8)(file,data,RUNTIME_SIZE,&read,NULL);
    ((BOOL (WINAPI *)(HANDLE))*(uintptr_t *)0x7B6108)(file);
    uint32_t hash=2166136261u;for(unsigned j=0;j<read;++j)hash=(hash^data[j])*16777619u;
    uint32_t *head=(uint32_t *)data;
    if(!ok || read!=RUNTIME_SIZE || hash!=RUNTIME_HASH || head[0]!=0x31584A46u || head[5]!=RUNTIME_SIZE ||
        head[1]>RUNTIME_SIZE-32 || head[3]>=head[1] || head[4]>(RUNTIME_SIZE-32-head[1])/4){
        SLOT[1]=3;((BOOL (WINAPI *)(void *,SIZE_T,DWORD))*(uintptr_t *)0x7B614C)(data,0,MEM_RELEASE);return;
    }
    uint8_t *base=data+32;uint32_t delta=(uint32_t)(uintptr_t)base-head[2];
    uint32_t *relocs=(uint32_t *)(base+head[1]);
    for(unsigned j=0;j<head[4];++j){
        unsigned offset=relocs[j];if(offset>head[1]-4){SLOT[1]=3;return;}
        *(uint32_t *)(base+offset)+=delta;
    }
    SLOT[0]=(uint32_t)(uintptr_t)base;
    SLOT[1]=((int (__cdecl *)(void))(base+head[3]))()?1:4;
}
__declspec(dllexport) __attribute__((naked)) void bootstrap_bridge(void){
    __asm__ volatile(".intel_syntax noprefix\n pushfd\n pushad\n call _bootstrap_load\n popad\n popfd\n push 0x67DB38\n ret\n .att_syntax prefix\n");
}
