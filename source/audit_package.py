"""Verify the supported DLL loading contract in an IRO. Requires pefile."""
from pathlib import Path
import argparse,struct,sys,xml.etree.ElementTree as ET
p=argparse.ArgumentParser();p.add_argument('--iro',required=True);p.add_argument('--python-tools');args=p.parse_args()
if args.python_tools:sys.path.insert(0,str(Path(args.python_tools).resolve()))
import pefile
raw=Path(args.iro).read_bytes();magic,version,flags,start,count=struct.unpack_from('<IIIIi',raw)
assert magic==0x534F5249 and version==0x10002 and flags==0 and start==16
pos=20;files={}
for _ in range(count):
 size,nlen=struct.unpack_from('<HH',raw,pos);name=raw[pos+4:pos+4+nlen].decode('utf-16le');flag,off,length=struct.unpack_from('<IqI',raw,pos+4+nlen);pos+=size
 assert flag==0 and off+length<=len(raw) and name not in files
 files[name]=raw[off:off+length]
info=ET.fromstring(files['mod.xml'])
assert [v.text for v in info.findall('LoadLibrary')]==['FFVIIJobs.dll']
assert not any(info.findall(t) for t in ['LoadPlugin','LoadAssembly','LoadPrograms','ReleaseNotes'])
assert info.findtext('Version')=='1.00'
assert len(info.findall('ModFolder'))==225
assert not any('hext' in n.lower().split('\\') or n.lower().endswith(('.bin','.exe')) for n in files)
assert [n for n in files if n.lower().endswith('.dll')]==['FFVIIJobs.dll']
pe=pefile.PE(data=files['FFVIIJobs.dll'])
assert pe.FILE_HEADER.Machine==0x14c and pe.FILE_HEADER.Characteristics&0x2000
assert pe.OPTIONAL_HEADER.AddressOfEntryPoint and pe.OPTIONAL_HEADER.DATA_DIRECTORY[1].VirtualAddress
exports={e.name.decode() for e in pe.DIRECTORY_ENTRY_EXPORT.symbols if e.name}
assert 'jobs_hext_entry' not in exports
assert {'jobs_read_settings','jobs_configure','cmk_install','cmk_owner_mask'}<=exports
imports={d.dll.decode().lower() for d in pe.DIRECTORY_ENTRY_IMPORT}
assert all(n in {'kernel32.dll','msvcrt.dll'} or (n.startswith('api-ms-win-crt-') and n.endswith('.dll')) for n in imports),imports
assert 'kernel32.dll' in imports
print('PASS standard x86 DLL declaration, PE imports/exports, normal consent-warning flag, 225 settings folders and no HEXT/binary loader.')
print('DLL dependencies:',', '.join(sorted(imports)))
