"""Create and verify an uncompressed IRO 1.2 and source bundle."""
from pathlib import Path
import struct,xml.etree.ElementTree as ET,zipfile
root=Path(__file__).resolve().parent.parent;mod=root/'mod'
manifest=ET.parse(mod/'mod.xml')
paths=[mod/'mod.xml',mod/'Readme.txt',mod/'CLASS_PERMISSIONS.txt',mod/'preview.png']
paths.extend(sorted((mod/'previews').rglob('*.png')))
paths.extend(sorted((mod/'hext').rglob('*.txt')))
for folder in manifest.findall('ModFolder'):
 paths.extend(sorted(p for p in (mod/folder.attrib['Folder'].replace('\\','/')).rglob('*') if p.is_file()))
files=[(str(p.relative_to(mod)).replace('/','\\'),p.read_bytes()) for p in paths]
entries=[];position=20+sum(20+len(n.encode('utf-16-le')) for n,_ in files)
for name,data in files:
 encoded=name.encode('utf-16-le');entries.append(struct.pack('<HH',20+len(encoded),len(encoded))+encoded+struct.pack('<IqI',0,position,len(data)));position+=len(data)
out=root/'FFVIIJobSystem-0.8.0.iro'
out.write_bytes(struct.pack('<IIIIi',0x534F5249,0x10002,0,16,len(files))+b''.join(entries)+b''.join(d for _,d in files))
raw=out.read_bytes();pos=20
for name,data in files:
 size,nlen=struct.unpack_from('<HH',raw,pos);assert raw[pos+4:pos+4+nlen].decode('utf-16-le')==name
 flags,off,length=struct.unpack_from('<IqI',raw,pos+4+nlen);assert flags==0 and raw[off:off+length]==data;pos+=size
assert position==len(raw)
print('Verified',out.name,len(files),'files',len(raw),'bytes')
bundle=root.parent/'FFVIIJobSystem-source-0.8.0.zip'
with zipfile.ZipFile(bundle,'w',zipfile.ZIP_DEFLATED) as z:
 deliverables=paths+[out,root/'README.txt',root/'VALIDATION.txt',root/'CLASS_PERMISSIONS.txt',root/'REFERENCE.md',root/'CLASS_REFERENCE.json',root/'ACCESSORY_PROPOSAL.json',root/'ART_SOURCES.txt',root/'PREVIEW_PROMPT.txt',root/'PREVIEW_EDIT_PROMPT.txt',root/'RELEASE_NOTES.txt',root/'PUBLISHING.txt',root/'PULL_REQUEST.txt']
 deliverables+=sorted(p for p in (root/'source').iterdir() if p.suffix in ('.c','.h','.py','.json','.cs','.il','.md'))
 for p in deliverables:z.write(p,root.name+'/'+str(p.relative_to(root)))
print('Created',bundle.name)
