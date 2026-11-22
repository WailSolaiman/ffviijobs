"""Read an installed ESUI copy; emulate its font stub privately (no game launch).
Usage: py audit_esui.py --esui PATH --python-tools PATH [--fixture PATH]
The optional private fixture contains third-party code; never distribute it.
"""
import argparse,re,struct,sys
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--esui',required=True);p.add_argument('--python-tools');p.add_argument('--fixture');a=p.parse_args()
if a.python_tools:sys.path.insert(0,a.python_tools)
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_PROT_READ,UC_PROT_WRITE,UC_PROT_EXEC,UC_ERR_FETCH_PROT
from unicorn.x86_const import UC_X86_REG_ESP
base=Path(a.esui)
def writes(path):
 offset=0
 for line in path.read_text(encoding='utf-8-sig',errors='replace').splitlines():
  line=line.split('#')[0].strip()
  if re.fullmatch(r'[+-][0-9A-Fa-f]+',line):offset=int(line,16)
  match=re.fullmatch(r'([0-9A-Fa-f]+)\s*=\s*((?:[0-9A-Fa-f]{2}\s*)+)',line)
  if match:yield int(match[1],16)+offset,bytes.fromhex(match[2])
patch=dict(writes(base/'EnhancedStock/hext/00-Spacing.txt'));stub=bytearray(0x415);covered=set()
for at,data in patch.items():
 for i,value in enumerate(data):
  if 0x914000<=at+i<0x914415:stub[at+i-0x914000]=value;covered.add(at+i-0x914000)
assert len(covered)==len(stub)
h=2166136261
for v in stub:h=((h^v)*16777619)&0xffffffff
assert h==0x46edd751 and patch[0x7217c5]==bytes.fromhex('E9 36 28 1F 00 90 90 90 90')
print('PASS installed ESUI 2.968 entry and full startup routine match the guarded repair fingerprint.')
m=Uc(UC_ARCH_X86,UC_MODE_32)
for at in (0x721000,0x914000,0x99d000,0xdd6000,0x20000000):m.mem_map(at,0x1000)
m.mem_write(0x914000,bytes(stub));m.mem_write(0x7217ce,b'\xc3')
m.reg_write(UC_X86_REG_ESP,0x20000f00);m.mem_write(0x20000f00,struct.pack('<I',0x721fff))
m.mem_protect(0x914000,0x1000,UC_PROT_READ|UC_PROT_WRITE)
try:m.emu_start(0x914000,0x721fff,count=10000)
except UcError as e:assert e.errno==UC_ERR_FETCH_PROT
else:raise AssertionError('Non-executable page did not reproduce the startup fault')
m.mem_protect(0x914000,0x1000,UC_PROT_READ|UC_PROT_WRITE|UC_PROT_EXEC)
m.emu_start(0x914000,0x721fff,count=10000)
assert m.mem_read(0x99dda8,4)==patch[0x99dda8][:4]
assert m.mem_read(0x914000,len(stub))==stub
print('PASS reproduced execute-protection failure at 0x914000; permission repair runs the unchanged routine and refreshes font metrics.')
source=(Path(__file__).parent/'ffvii_jobs.c').read_text()
sites=[(int(addr,16),int(size)) for addr,size in re.findall(r'\{(0x[0-9A-Fa-f]+),\w+,(\d+),\{',source)]
assert len(sites)==16
files=list(base.rglob('*.txt'));n=0
for file in files:
 if 'hext' not in [part.lower() for part in file.parts]:continue
 n+=1
 for at,data in writes(file):
  assert all(at+len(data)<=hook or at>=hook+size for hook,size in sites),(file,hex(at))
print(f'PASS no literal HEXT byte-write overlap with any of the sixteen job hooks in {n} installed ESUI patch files.')
if a.fixture:Path(a.fixture).write_bytes(stub)
