#include <stddef.h>
#include <stdint.h>
#include <wchar.h>
void *memcpy(void *to,const void *from,size_t n){unsigned char *d=to;const unsigned char *s=from;for(size_t i=0;i<n;++i)d[i]=s[i];return to;}
void *memset(void *to,int value,size_t n){unsigned char *d=to;for(size_t i=0;i<n;++i)d[i]=(unsigned char)value;return to;}
int memcmp(const void *a,const void *b,size_t n){const unsigned char *x=a,*y=b;for(size_t i=0;i<n;++i)if(x[i]!=y[i])return (int)x[i]-(int)y[i];return 0;}
size_t strlen(const char *s){size_t n=0;while(s[n])++n;return n;}
size_t wcslen(const wchar_t *s){size_t n=0;while(s[n])++n;return n;}
wchar_t *wcscat(wchar_t *a,const wchar_t *b){size_t n=wcslen(a),i=0;do{a[n+i]=b[i];}while(b[i++]);return a;}
wchar_t *wcsrchr(const wchar_t *s,wchar_t c){const wchar_t *last=0;do{if(*s==c)last=s;}while(*s++);return (wchar_t *)last;}
__attribute__((naked)) void ___chkstk_ms(void){
 __asm__ volatile(".intel_syntax noprefix\n push ecx\n push eax\n lea ecx,[esp+12]\n cmp eax,0x1000\n jb 2f\n 1: sub ecx,0x1000\n test DWORD PTR [ecx],eax\n sub eax,0x1000\n cmp eax,0x1000\n ja 1b\n 2: sub ecx,eax\n test DWORD PTR [ecx],eax\n pop eax\n pop ecx\n ret\n .att_syntax prefix\n");
}
/* x86 Windows stack-allocation probe: EAX is the requested byte count. */
__asm__(".intel_syntax noprefix\n .globl __alloca\n __alloca:\n push ecx\n lea ecx,[esp+8]\n 1: cmp eax,4096\n jb 2f\n sub ecx,4096\n test DWORD PTR [ecx],ecx\n sub eax,4096\n jmp 1b\n 2: sub ecx,eax\n test DWORD PTR [ecx],ecx\n lea eax,[esp+4]\n mov esp,ecx\n mov ecx,[eax-4]\n push DWORD PTR [eax]\n sub eax,esp\n ret\n .att_syntax prefix\n");
