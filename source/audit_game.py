"""Read game resources and emulate stock routines privately; never launch FF7.
Requires pefile and unicorn. Pass --game PATH and optionally --python-tools PATH.
No original game bytes are saved or distributed.
"""
import argparse,ast,gzip,json,re,struct,sys
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--game',required=True);p.add_argument('--python-tools');args=p.parse_args()
if args.python_tools:sys.path.insert(0,args.python_tools)
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_EBP
root=Path(__file__).resolve().parent.parent;game=Path(args.game)
r=json.loads((root/'source/rules.json').read_text(encoding='utf-8'))
def sections(path):
 raw=path.read_bytes();pos=0;out=[]
 while pos+6<=len(raw):
  n,size,typ=struct.unpack_from('<HHH',raw,pos);pos+=6
  data=gzip.decompress(raw[pos:pos+n]);pos+=n;assert len(data)==size;out.append(data)
 return out
kernel=sections(game/'data/lang-en/kernel/KERNEL.BIN')
def name(s,i):
 off=struct.unpack_from('<H',s,i*2)[0];data=s[off:].split(b'\xff')[0]
 return ''.join(chr(x+32) for x in data)
canonical=lambda n:re.sub('[^a-z0-9]','',n.lower()).replace('typhoon','typoon').replace('toughring','touphring')
for label,index in [('materia',23),('accessory',22)]:
 for n,i in r[label+'_ids'].items():assert canonical(n)==canonical(name(kernel[index],i)),(label,i,n,name(kernel[index],i))
print('PASS all 83 Materia and 32 accessory IDs match the installed kernel names.')
starters={'Cloud':'Buster Sword','Barret':'Gatling Gun','Tifa':'Leather Glove','Aerith':'Guard Stick',
 'Red XIII':'Mythril Clip','Yuffie':'4-point Shuriken','Cait Sith':'Yellow M-phone','Vincent':'Quicksilver','Cid':'Spear'}
weapons={name(kernel[20],i):i for i in range(struct.unpack_from('<H',kernel[20])[0]//2)}
for c in r['characters']:
 i=weapons[starters[c['name']]];mask=struct.unpack_from('<H',kernel[5],i*44+14)[0]
 assert (mask&0x1FF)==1<<c['game_character_id'],(c['name'],mask)
print('PASS all nine character IDs match their actual starting-weapon equip masks.')
exe=pefile.PE(str(game/'ff7_en.exe'));image=exe.get_memory_mapped_image()
def machine():
 m=Uc(UC_ARCH_X86,UC_MODE_32);m.mem_map(0x400000,0xC00000);m.mem_write(0x400000,image)
 m.mem_map(0x20000000,0x10000);m.mem_map(0x21000000,0x10000);m.mem_map(0x30000000,0x1000)
 return m
def word(m,a,v):m.mem_write(a,struct.pack('<I',v))
def half(m,a,v):m.mem_write(a,struct.pack('<H',v))
m=machine();owners=[sum(1<<i for i,c in enumerate(r['characters']) if n in c['accessories']) for n in r['accessory_ids']]
for a in range(32):half(m,0xDBCAEC+a*16,owners[a]|0x600)
for i in range(320):half(m,0xDC0234+i*2,(0x120+i)|0x200 if i<32 else 0xFFFF)
for c in range(9):
 m.mem_write(0xDC0230,bytes([c,255,255]));word(m,0xDCA4A4,0)
 stack=0x2000F000;word(m,stack,0x30000000);word(m,stack+4,2);m.reg_write(UC_X86_REG_ESP,stack)
 m.emu_start(0x7083ED,0x30000000,count=100000)
 count=struct.unpack('<I',m.mem_read(0xDCA7EC,4))[0]
 assert list(m.mem_read(0xDCA6A8,count))==[a for a in range(32) if owners[a]&(1<<c)]
print('PASS actual stock accessory selection returns the exact allowed list for all nine characters.')
source=(root/'source/ffvii_jobs.c').read_text(encoding='utf-8')
encoded=re.search(r'static const uint8_t uv\[9\]\[2\]=(\{.*?\});',source,re.S)[1]
uvs=ast.literal_eval(encoded.replace('{','[').replace('}',']'))
def item_sprite(c,low):
 m=machine()
 for i in range(5):
  obj=0x21000000+i*0x100;verts=0x21001000+i*0x200
  word(m,0xDC0FC8+i*4 if i<4 else 0xDC104C,obj)
  word(m,obj+0x70,verts);word(m,obj+0x78,0x21003000+i*4)
 calls=[]
 def hook(uc,addr,size,data):
  if addr not in (0x6CDBF2,0x66E272):return
  esp=uc.reg_read(UC_X86_REG_ESP)
  if addr==0x66E272:calls.append(struct.unpack('<I',uc.mem_read(esp+8,4))[0])
  ret=struct.unpack('<I',uc.mem_read(esp,4))[0]
  uc.reg_write(UC_X86_REG_EAX,1);uc.reg_write(UC_X86_REG_ESP,esp+4);uc.reg_write(UC_X86_REG_EIP,ret)
 m.hook_add(UC_HOOK_CODE,hook)
 stack=0x20008000;item=0x80+weapons[starters[c['name']]]
 m.mem_write(stack,struct.pack('<5If',0x30000000,595,214,item,0,0.2));m.reg_write(UC_X86_REG_ESP,stack)
 m.emu_start(0x7177A8 if low else 0x717691,0x30000000,count=100000)
 u,v=uvs[c['game_character_id']];atlas=4 if low else (int(u>=128)+2*int(v>=128))
 assert calls==[0x21000000+atlas*0x100],c['name']
 verts=struct.unpack('<32f',m.mem_read(0x21001000+atlas*0x200,128))
 assert verts[6]==(u if low else (u*2)%256)/256,(c['name'],'u',verts[6],u)
 assert verts[7]==(v if low else (v*2)%256)/256,(c['name'],'v',verts[7],v)
 assert m.mem_read(0x21003000+atlas*4,1)[0]==1,(c['name'],'palette')
 assert verts[16]-verts[0]==(16 if low else 32)
 assert verts[9]-verts[1]==(16 if low else 32)
for c in r['characters']:
 for low in (False,True):item_sprite(c,low)
print('PASS all 18 actual stock Items weapon-renderer cases match this mod: all nine characters, both resolutions, palette and atlas.')
sites={0x70DC14:'68bf010000',0x710437:'8b55fc8b02',0x710BD0:'558bec83ec08',0x5CB2CC:'558bec83ec24',
0x6C545B:'558bec83ec28',0x70C813:'e8eb92feff',0x70C9DE:'e84ce1feff',0x70F394:'e86a67feff',0x70FC78:'e8b2aefeff',0x7083ED:'558bec83ec10',0x705ED1:'e82dfcfeff',0x706857:'e8d342ffff',0x7063C7:'e837f7feff',0x706D49:'e8e13dffff',0x715846:'e8b802feff',0x716203:'e82749feff'}
for addr,b in sites.items():assert image[addr-0x400000:addr-0x400000+len(bytes.fromhex(b))]==bytes.fromhex(b)
print('PASS all sixteen guarded signatures match the installed English executable.')

# Verify accessory IDs/locals at all six ORIGINAL name call sites, executing
# stock item decoding and name-argument construction in private emulation.
menu_cases=[(0x705E33,0x705ED1,0),(0x7067BC,0x706857,0),
 (0x706398,0x7063C7,1),(0x706D1A,0x706D49,1),
 (0x7157A9,0x715846,2),(0x716166,0x716203,2)]
menu_count=0
for a in range(32):
 for start,site,view in menu_cases:
  m=machine();frame=0x20008000;stack=frame-0x200;seen=[];requested=[]
  word(m,frame-0x14,2 if view==0 else 1)
  word(m,frame-0x20,7);word(m,frame-4,0xDBFD8C)
  m.mem_write(0xDBFD8C+7*0x84,bytes([7]));m.mem_write(0xDBFD8C+7*0x84+0x1E,bytes([a]))
  word(m,0xDCA5C4,2);word(m,0xDCA60C,5);m.mem_write(0xDCA6A8+7,bytes([a]))
  # Items Use context 1, with a scrolled stack (quantity must not affect ID).
  word(m,0xDD1A2C+0x38,30);half(m,0xDC0234+2*32,(99<<9)|0x120|a)
  word(m,frame-0x34,2);word(m,frame-0x14,1 if view==2 else 2)
  m.mem_write(0x21003000,b'\x25\x41\x52\x52\x49\x4e\x47\xff')
  def menu_hook(uc,address,size,data):
   esp=uc.reg_read(UC_X86_REG_ESP)
   if address==site:
    seen.append(struct.unpack('<4If',uc.mem_read(esp,20)))
    at=frame-(0x18 if view==0 else 0x3C) if view!=1 else 0xDBFD8C+7*0x84+0x1E
    decoded=struct.unpack('<I',uc.mem_read(at,4))[0] if view!=1 else uc.mem_read(at,1)[0]+0x120
    assert decoded==0x120+a,(view,a,hex(decoded))
    uc.emu_stop();return
   if address not in (0x6C6B1B,0x41963C,0x7150A0):return
   ret=struct.unpack('<I',uc.mem_read(esp,4))[0]
   if address==0x6C6B1B:requested.append(struct.unpack('<I',uc.mem_read(esp+4,4))[0])
   elif address==0x41963C:requested.append(struct.unpack('<I',uc.mem_read(esp+8,4))[0])
   uc.reg_write(UC_X86_REG_EAX,0 if address==0x7150A0 else 0x21003000)
   uc.reg_write(UC_X86_REG_ESP,esp+4);uc.reg_write(UC_X86_REG_EIP,ret)
  m.hook_add(UC_HOOK_CODE,menu_hook);m.reg_write(UC_X86_REG_EBP,frame);m.reg_write(UC_X86_REG_ESP,stack)
  try:m.emu_start(start,site+5,count=10000)
  except Exception as e:raise RuntimeError((a,view,hex(start),hex(m.reg_read(UC_X86_REG_EIP)))) from e
  assert len(seen)==1 and requested==[0x120+a],(a,view,hex(start),seen,requested)
  assert seen[0][2]==0x21003000 and seen[0][3]&255==7
  menu_count+=1
print(f'PASS {menu_count} original accessory name paths: all 32 IDs, six Equipment/Items sites, scrolled inventory and quantity decoding.')
