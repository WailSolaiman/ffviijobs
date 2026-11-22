"""Build HEXT FFVII Job System. Python 3 + Zig 0.17.1; no managed bridge.
Usage: py source/build.py --zig C:/path/to/zig.exe --test-output work/jobs-tests
"""
import argparse,json,subprocess,struct,xml.etree.ElementTree as ET
from pathlib import Path
from audit_rules import audit
from configure_guide import generate,accessory_key,class_accessories
root=Path(__file__).resolve().parent.parent;src=root/'source';mod=root/'mod';mod.mkdir(exist_ok=True)
p=argparse.ArgumentParser();p.add_argument('--zig',default='zig');p.add_argument('--test-output');p.add_argument('--skip-runtime','--skip-native',dest='skip_runtime',action='store_true');p.add_argument('--runtime-work',default='work/ffvii-jobs-runtime');p.add_argument('--python-tools');args=p.parse_args()
r=audit(root);chars=r['characters'];classes=r['classes'];mids=r['materia_ids'];aids=r['accessory_ids']
descriptions,proposal,previews=generate(root,r)
defaults=[next(i for i,j in enumerate(classes) if j['key']==c['default_class']) for c in chars]
table=[[int(i in {mids[m] for m in j['materia']}) for i in range(256)] for j in classes]
owners=[sum(1<<i for i,c in enumerate(chars) if name in c['accessories']) for name in aids]
header='/* Generated from rules.json. Do not edit. */\n#define CLASS_COUNT 25\n'
header+='#define DEFAULT_JOB_VALUES {'+','.join(map(str,defaults))+'}\n'
header+='static const int DEFAULT_JOBS[9]=DEFAULT_JOB_VALUES;\n'
for name,values in [('CHARACTER_NAMES',[c['name'] for c in chars]),('CHARACTER_KEYS',[c['key'] for c in chars]),('CLASS_NAMES',[j['name'] for j in classes]),('CLASS_KEYS',[j['key'] for j in classes])]:
 header+='static const char *'+name+'['+str(len(values))+']={'+','.join(json.dumps(v) for v in values)+'};\n'
header+='static const unsigned char CLASS_MATERIA[CLASS_COUNT][256]={\n'+',\n'.join('{'+','.join(map(str,t))+'}' for t in table)+'\n};\n'
header+='static const unsigned short ACCESSORY_OWNERS[32]={'+','.join(map(str,owners))+'};\n'
(src/'rules.generated.h').write_text(header,encoding='utf-8')
info=ET.Element('ModInfo')
for key,val in {'ID':'2a737e43-7bcd-4da4-962d-3c87631fca5e','Name':'FFVII Job System','Author':'Willard82',
 'Version':'0.80','ReleaseDate':'2026-11-22','Category':'Gameplay','GameLanguage':'EN',
 'Description':'Choose one of 25 Final Fantasy jobs for each character. Your choices decide which Materia they can equip, with weapon icons showing eligible characters. Configure Mod includes class and accessory guides. For original English FF7 PC.',
 'Link':'https://github.com/WailSolaiman/ffviijobs',
 'DonationLink':'https://ko-fi.com/ffviijobs','PreviewFile':'preview.png'}.items():ET.SubElement(info,key).text=val
for i,c in enumerate(chars):
 cfg=ET.SubElement(info,'ConfigOption');option_id='job_v070_'+c['key']
 for k,v in {'Name':c['name'],'ID':option_id,'Type':'List','Default':str(defaults[i]),
  'Description':'Choose '+c['name']+'\'s class. Default: '+classes[defaults[i]]['name']+'. Details below follow your selection. Classes can be shared; changes apply next launch.'}.items():ET.SubElement(cfg,k).text=v
 for n,j in enumerate(classes):
  ET.SubElement(cfg,'Option',Value=str(n),Name=j['name'],PreviewFile='previews\\classes\\'+j['key']+'.png')
  folder='choices/'+c['key']+'/'+j['key']
  ET.SubElement(info,'ModFolder',Folder=folder.replace('/','\\'),ActiveWhen=option_id+' = '+str(n))
  marker=mod/folder/'ffvii-job-system-v070'/(c['key']+'.class')
  marker.parent.mkdir(parents=True,exist_ok=True);marker.write_bytes((j['key']+'\n').encode('ascii'))
reference=ET.SubElement(info,'ConfigOption')
for k,v in {'Name':'Accessory reference','ID':'accessory_reference_v071','Type':'List','Default':'0',
 'Description':'Select an accessory to read its effect and suggested classes. Reference only: these class assignments are not active. Accessories still follow the existing character rules.'}.items():ET.SubElement(reference,k).text=v
for a in sorted(proposal['accessories'],key=lambda a:aids[a['name']]):
 ET.SubElement(reference,'Option',Value=str(aids[a['name']]),Name=a['name'],PreviewFile='previews\\accessories\\'+accessory_key(r,a['name'])+'.png')
ET.SubElement(info,'ModFolder',Folder='runtime')
ET.indent(info,space='  ');ET.ElementTree(info).write(mod/'mod.xml',encoding='utf-8',xml_declaration=True)
details=['FFVII JOB SYSTEM 0.8.0 - CLASSES AND PERMISSIONS','',
 'Choose one of the same 25 classes for each character in Configure Mod before launching.',
 'Characters can share classes. Icons show exactly which characters can equip a Materia under your current selections.',
 'A Materia with no eligible character remains visible, without icons, and cannot be equipped.',
 'The defaults do not activate every class; some Materia will therefore have no icons until you choose a suitable class.',
 'Original English FF7 names are used: HP/MP Up = HP/MP Plus; Assess = Sense; Binding = Seal; Transformations = Transform.',
 'Counter (blue support) and Counter Attack (purple) are different Materia. Monk has Counter Attack only.',
 'Configure Mod now shows the selected class\'s role, Materia and suggested accessories.',
 'Accessory suggestions are INFORMATION ONLY. Actual accessories still use the existing character permissions.',
 'DEFAULTS']
for c,n in zip(chars,defaults):details.append('  '+c['name']+': '+classes[n]['name'])
details+=['','25 SHARED CLASSES']
for j in classes:
 details+=['',j['name'],'  Role: '+descriptions[j['name']],'  Materia: '+', '.join(j['materia']),
  '  Accessory suggestions (not active): '+', '.join(a['name'] for a in class_accessories(proposal,j['name']))]
details+=['','ACCESSORIES (existing character permissions, independent of selected class)']
for c in chars:details+=['  '+c['name']+': '+', '.join(c['accessories'])]
details+=['','ACCESSORY REFERENCE (proposed classes; not active)']
for a in sorted(proposal['accessories'],key=lambda a:aids[a['name']]):
 details+=['',a['name']+' - '+a['effect'],'  Suggested classes: '+', '.join(a['classes']),'  Reason: '+a['rationale']]
(root/'CLASS_PERMISSIONS.txt').write_text('\n'.join(details)+'\n',encoding='utf-8')
(mod/'CLASS_PERMISSIONS.txt').write_bytes((root/'CLASS_PERMISSIONS.txt').read_bytes())
from build_hext import build as build_runtime,test as test_runtime
if args.skip_runtime:
 assert (mod/'runtime/ffvii-jobs-v080.bin').is_file() and (mod/'hext/FFVIIJobs.txt').is_file()
else:
 build_runtime(root,args.zig,args.runtime_work,args.python_tools)
print('Built DLL-free HEXT package with nine class dropdowns, accessory reference and 57 guide previews.')
if args.test_output:
 test_runtime(root,args.zig,Path(args.runtime_work).resolve(),args.test_output)
