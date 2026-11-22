"""Build the freestanding HEXT core and compact early-game entry patch.
Intermediate PE files are build artifacts only; none are distributed as DLLs.
Requires pefile and Zig 0.17.1. No game assets are read by this build script.
"""
from pathlib import Path
import json,struct,subprocess,shutil

BOOT_SITE=0x67DB38
CAVE=0x7B5800
STATE=0x7B5FF0

def _stores(var,text,wide=False):
 raw=(text+'\0').encode('utf-16le' if wide else 'ascii');raw+=bytes((-len(raw))%4)
 return '\n'.join('    '+var+'['+str(i//4)+']=0x%08Xu;'%struct.unpack_from('<I',raw,i)[0] for i in range(0,len(raw),4))

def build(root,zig,work,python_tools=None):
 import sys
 if python_tools:sys.path.insert(0,str(Path(python_tools).resolve()))
 import pefile
 root=Path(root);src=root/'source';work=Path(work).resolve();work.mkdir(parents=True,exist_ok=True)
 zig=shutil.which(zig) or str(Path(zig).resolve());lib=Path(zig).parent/'lib/libc/include'
 flags=[zig,'cc','-target','x86-windows-gnu','-mstackrealign','-O2','-Wall','-Wextra','-ffreestanding','-fno-builtin','-nostdlib',
  '-isystem',str(lib/'any-windows-any'),'-isystem',str(lib/'x86-windows-gnu'),'-I',str(src),'-shared']
 subprocess.run(flags+['-Wl,--entry=jobs_hext_entry',str(src/'ffvii_jobs.c'),str(src/'libc.c'),'-o',str(work/'core.dll')],check=True)
 core=pefile.PE(str(work/'core.dll'));assert not core.OPTIONAL_HEADER.DATA_DIRECTORY[1].VirtualAddress
 image=bytearray(core.OPTIONAL_HEADER.SizeOfImage)
 for section in core.sections:
  data=section.get_data();image[section.VirtualAddress:section.VirtualAddress+len(data)]=data
 relocations=[]
 for block in core.DIRECTORY_ENTRY_BASERELOC:
  for relocation in block.entries:
   assert relocation.type in (0,3)
   if relocation.type:relocations.append(relocation.rva)
 symbols={e.name.decode():e.address for e in core.DIRECTORY_ENTRY_EXPORT.symbols if e.name}
 # A 32-byte header keeps the relocated image base 16-byte aligned for SSE.
 size=32+len(image)+len(relocations)*4
 blob=struct.pack('<8I',0x31584A46,len(image),core.OPTIONAL_HEADER.ImageBase,symbols['jobs_hext_entry'],len(relocations),size,0,0)+image+struct.pack('<'+'I'*len(relocations),*relocations)
 runtime=root/'mod/runtime/ffvii-jobs-v080.bin';runtime.parent.mkdir(parents=True,exist_ok=True);runtime.write_bytes(blob)
 hash=2166136261
 for value in blob:hash=((hash^value)*16777619)&0xffffffff
 (work/'runtime.generated.h').write_text('#define RUNTIME_SIZE '+str(size)+'u\n#define RUNTIME_HASH 0x%08Xu\n'%hash)
 source=(src/'bootstrap.template.c').read_text(encoding='utf-8')
 for key,var,text,wide in [('KERNEL','kernel','kernel32.dll',False),('MODULE','module','GetModuleFileNameW',False),('CREATE','create','CreateFileW',False),('SUFFIX','suffix','\\ffvii-jobs-v080.bin',True)]:
  source=source.replace('@'+key+'@',_stores(var,text,wide))
 (work/'bootstrap.c').write_text(source,encoding='utf-8')
 subprocess.run(flags+['-I',str(work),'-Wl,--entry=bootstrap_bridge',str(work/'bootstrap.c'),'-o',str(work/'bootstrap.dll')],check=True)
 boot=pefile.PE(str(work/'bootstrap.dll'))
 assert not boot.OPTIONAL_HEADER.DATA_DIRECTORY[1].VirtualAddress and not getattr(boot,'DIRECTORY_ENTRY_BASERELOC',[])
 section=next(s for s in boot.sections if s.Name.rstrip(b'\0')==b'.text');code=section.get_data()[:section.Misc_VirtualSize]
 assert len(code)<STATE-CAVE,'Entry code must not overlap its state slots'
 bridge=next(e.address for e in boot.DIRECTORY_ENTRY_EXPORT.symbols if e.name==b'bootstrap_bridge')-section.VirtualAddress
 lines=['FFVII Job System 0.8.0 - early HEXT runtime entry','%X:800'%CAVE]
 for offset in range(0,len(code),32):lines.append('%X = '%(CAVE+offset)+code[offset:offset+32].hex(' '))
 lines+=['%X = 00:8'%STATE,'%X = '%BOOT_SITE+(b'\xE9'+struct.pack('<i',CAVE+bridge-BOOT_SITE-5)).hex(' '),
         '%X:800'%CAVE,'%X:5'%BOOT_SITE]
 target=root/'mod/hext/FFVIIJobs.txt';target.parent.mkdir(parents=True,exist_ok=True);target.write_text('\n'.join(lines)+'\n')
 (work/'bootstrap.generated.h').write_text('static const unsigned char HEXT_ENTRY[]={'+','.join(str(b) for b in code)+'};\n#define HEXT_BRIDGE_OFFSET '+str(bridge)+'\n')
 (work/'symbols.json').write_text(json.dumps(symbols,indent=2))
 (work/'runtime.json').write_text(json.dumps({'size':size,'hash':hash,'entry_size':len(code),'bridge_offset':bridge,'boot_site':BOOT_SITE,'cave':CAVE,'state':STATE},indent=2))
 print('Built HEXT runtime:',size,'bytes; entry:',len(code),'bytes; no distributed DLL.')
 return work

def test(root,zig,work,output):
 root=Path(root);work=Path(work);output=Path(output).resolve();output.mkdir(parents=True,exist_ok=True)
 zig=shutil.which(zig) or str(Path(zig).resolve())
 symbols=json.loads((work/'symbols.json').read_text());assembly='.intel_syntax noprefix\n.text\n'
 for name,rva in symbols.items():assembly+='.globl _'+name+'\n_'+name+':\n mov eax,DWORD PTR [0x7B5FF0]\n add eax,'+str(rva)+'\n jmp eax\n'
 (output/'api_wrappers.S').write_text(assembly)
 s=(root/'source/test_native_legacy.c').read_text(encoding='utf-8').replace('#include "rules.generated.h"','#include "rules.generated.h"\n#include "bootstrap.generated.h"')
 s=s.replace('uintptr_t starts[]={0x5C0000,','uintptr_t starts[]={0x670000,0x7A0000,0x7B0000,0x5C0000,')
 pos=s.index('int main(')
 s=s[:pos]+'''static void fixture_bootstrap(void){
 U32(0x7B6110)=(uint32_t)GetProcAddress;U32(0x7B61E0)=(uint32_t)GetModuleHandleA;
 U32(0x7B6120)=(uint32_t)VirtualAlloc;U32(0x7B614C)=(uint32_t)VirtualFree;
 U32(0x7B60B8)=(uint32_t)ReadFile;U32(0x7B6108)=(uint32_t)CloseHandle;
 memcpy((void *)0x7B5800,HEXT_ENTRY,sizeof(HEXT_ENTRY));U32(0x7B5FF0)=U32(0x7B5FF4)=0;
 *(uint8_t *)0x7AE980=0xC3;*(uint8_t *)0x67DB3D=0xC3;
 write_jump(0x67DB38,(void *)(0x7B5800+HEXT_BRIDGE_OFFSET));((void (__cdecl *)(void))0x67DB38)();
}
'''+s[pos:]
 s=s.replace('        CHECK(cmk_install()==0);','        fixture_bootstrap();CHECK(U32(0x7B5FF4)==4);\n        CHECK(cmk_install()==0);',1)
 s=s.replace('    CHECK(cmk_install()==1);','    reset_data();fixture_bootstrap();CHECK(U32(0x7B5FF4)==1);\n    CHECK(cmk_install()==1);',1)
 a=s.index('    /* 7H 4.6.1 loads libraries before activating file hooks.');b=s.index('    for(unsigned bad=',a);s=s[:a]+s[b:]
 # Startup failures must restore the original entry call and leave game hooks untouched.
 marker='    if(argc>1 && strcmp(argv[1],"esui")!=0)'
 pos=s.index(marker)
 s=s[:pos]+'''    if(argc>1 && (!strcmp(argv[1],"missing-runtime") || !strcmp(argv[1],"corrupt-runtime"))){
      reset_data();fixture_bootstrap();CHECK(U32(0x7B5FF0)==0);
      CHECK(U32(0x7B5FF4)==(!strcmp(argv[1],"missing-runtime")?2u:3u));
      CHECK(U32(0x67DB38)==0x130E43E8u);CHECK(*(uint8_t *)0x67DB3C==0);
      CHECK(*(uint8_t *)0x5CB2CC==0x55);CHECK(*(uint8_t *)0x70DC14==0x68);
      printf("PASS %u startup failure checks\\n",count);return 0;
    }
'''+s[pos:]
 (output/'test_hext.c').write_text(s,encoding='utf-8')
 shutil.copy2(work/'bootstrap.generated.h',output/'bootstrap.generated.h')
 shutil.copy2(root/'source/rules.generated.h',output/'rules.generated.h')
 blobpath=output/'ffvii-jobs-v080.bin';blob=(root/'mod/runtime/ffvii-jobs-v080.bin').read_bytes();blobpath.write_bytes(blob)
 exe=output/'test_hext.exe'
 subprocess.run([zig,'cc','-target','x86-windows-gnu','-mstackrealign','-O2','-Wall','-Wextra','-Wl,--image-base=0x400000','-Wl,--stack=1048576',str(output/'test_hext.c'),str(output/'api_wrappers.S'),'-o',str(exe)],check=True)
 image=bytearray(exe.read_bytes());off=struct.unpack_from('<I',image,0x3c)[0]+24+70;struct.pack_into('<H',image,off,struct.unpack_from('<H',image,off)[0]&~0x40);exe.write_bytes(image)
 for args in [[],['conflict'],['ui-conflict'],['accessory-conflict'],['accessory-ui-conflict']]:subprocess.run([str(exe)]+args,check=True)
 try:
  blobpath.unlink();subprocess.run([str(exe),'missing-runtime'],check=True)
  broken=bytearray(blob);broken[-1]^=1;blobpath.write_bytes(broken);subprocess.run([str(exe),'corrupt-runtime'],check=True)
  blobpath.write_bytes(blob[:80]);subprocess.run([str(exe),'corrupt-runtime'],check=True)
 finally:blobpath.write_bytes(blob)
