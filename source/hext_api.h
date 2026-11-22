/* Freestanding HEXT runtime: APIs resolved from the game's own kernel32 IAT.
 * No DLL import table or operating-system module loader is used by this core.
 */
typedef struct {
 HANDLE (WINAPI *create_a)(LPCSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE);
 HANDLE (WINAPI *create_w)(LPCWSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE);
 BOOL (WINAPI *read)(HANDLE,LPVOID,DWORD,LPDWORD,LPOVERLAPPED);
 BOOL (WINAPI *write)(HANDLE,LPCVOID,DWORD,LPDWORD,LPOVERLAPPED);
 BOOL (WINAPI *close)(HANDLE);
 DWORD (WINAPI *module_name)(HMODULE,LPWSTR,DWORD);
 DWORD (WINAPI *last_error)(void);
 SIZE_T (WINAPI *query)(LPCVOID,PMEMORY_BASIC_INFORMATION,SIZE_T);
 LPVOID (WINAPI *alloc)(LPVOID,SIZE_T,DWORD,DWORD);
 BOOL (WINAPI *protect)(LPVOID,SIZE_T,DWORD,PDWORD);
 BOOL (WINAPI *flush)(HANDLE,LPCVOID,SIZE_T);
} HextApi;
static HextApi hext_api;
static int hext_resolve_api(void){
 HMODULE k=((HMODULE (WINAPI *)(LPCSTR))*(uintptr_t *)0x7B61E0)("kernel32.dll");
 FARPROC (WINAPI *get)(HMODULE,LPCSTR)=(void *)*(uintptr_t *)0x7B6110;
 if(!k || !get)return 0;
 const char *names[]={"CreateFileA","CreateFileW","ReadFile","WriteFile","CloseHandle","GetModuleFileNameW","GetLastError","VirtualQuery","VirtualAlloc","VirtualProtect","FlushInstructionCache"};
 uintptr_t *slots=(uintptr_t *)&hext_api;
 for(unsigned i=0;i<sizeof(names)/sizeof(names[0]);++i){slots[i]=(uintptr_t)get(k,names[i]);if(!slots[i])return 0;}
 return 1;
}
#define CreateFileA hext_api.create_a
#define CreateFileW hext_api.create_w
#define ReadFile hext_api.read
#define WriteFile hext_api.write
#define CloseHandle hext_api.close
#define GetModuleFileNameW hext_api.module_name
#define GetLastError hext_api.last_error
#define VirtualQuery hext_api.query
#define VirtualAlloc hext_api.alloc
#define VirtualProtect hext_api.protect
#define FlushInstructionCache hext_api.flush
#undef GetCurrentProcess
#define GetCurrentProcess() ((HANDLE)(intptr_t)-1)
#undef InterlockedIncrement
#define InterlockedIncrement(p) __atomic_add_fetch((p),1,__ATOMIC_SEQ_CST)
