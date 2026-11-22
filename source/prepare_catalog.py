"""Stage catalogue submission for the announced GitHub release.
Run --verify only AFTER uploading the IRO and preview to the release.
No uploads, account changes or submissions are performed by this script.
"""
from pathlib import Path
import argparse,hashlib,math,copy,xml.etree.ElementTree as ET,urllib.request
root=Path(__file__).resolve().parent.parent
p=argparse.ArgumentParser();p.add_argument('--verify',action='store_true');a=p.parse_args()
info=ET.parse(root/'mod/mod.xml').getroot()
iro=root/'FFVIIJobSystem-0.7.2.iro';preview=root/'preview.png'
base='https://github.com/WailSolaiman/ffviijobs/releases/download/v0.7.2/'
download=base+iro.name;image=base+preview.name
entry=ET.Element('Mod')
for key in ['ID','Name','Author','Category','Description','Link','DonationLink','GameLanguage']:
 ET.SubElement(entry,key).text=info.findtext(key)
ET.SubElement(entry,'MetaVersion').text='1.00'
tags=ET.SubElement(entry,'Tags')
for tag in ['Gameplay','Jobs','Classes','Materia','Willard82']:ET.SubElement(tags,'string').text=tag
version=ET.SubElement(entry,'LatestVersion')
for k,v in {'Link':'iros://Url/'+download.replace('https://','https$',1),'Version':info.findtext('Version'),
 'ReleaseDate':info.findtext('ReleaseDate'),'CompatibleGameVersions':'Steam',
 'PreviewImage':image,'DownloadSize':str(math.ceil(iro.stat().st_size/1024)),
 'ReleaseNotes':info.findtext('ReleaseNotes')}.items():ET.SubElement(version,k).text=v
out=root/'catalog-submission';target=out/'mods/qhimm/FFVII Job System/mod.xml';target.parent.mkdir(parents=True,exist_ok=True)
ET.indent(entry,space='  ');ET.ElementTree(entry).write(target,encoding='utf-8',xml_declaration=True)
catalog=ET.Element('Catalog',Name='Willard82 - FFVII Job System');ET.SubElement(catalog,'Mods').append(copy.deepcopy(entry))
ET.indent(catalog,space='  ');ET.ElementTree(catalog).write(out/'ffviijobs-catalog.xml',encoding='utf-8',xml_declaration=True)
manifest={'status':'Staged; upload release assets before submitting this entry.','repository':info.findtext('Link'),
 'tag':'v0.7.2','iro_url':download,'preview_url':image,'mod_id':info.findtext('ID'),'version':info.findtext('Version'),
 'download_size_kb':math.ceil(iro.stat().st_size/1024),'sha256':hashlib.sha256(iro.read_bytes()).hexdigest()}
import json
(out/'release-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
(root/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+f.name+'\n' for f in [iro,preview]),encoding='utf-8')
if a.verify:
 for path,url in [(iro,download),(preview,image)]:
  request=urllib.request.Request(url,headers={'User-Agent':'FFVIIJobs-release-check'})
  with urllib.request.urlopen(request,timeout=60) as response:data=response.read()
  assert hashlib.sha256(data).digest()==hashlib.sha256(path.read_bytes()).digest(),path.name+' differs from staged release'
  print('VERIFIED public asset:',path.name)
else:print('Staged catalogue XML. Public download URLs are planned; run --verify after uploading both release assets.')
