"""Generate 7H's selection-dependent information previews. Requires Pillow.
All contents are original text layouts. No game or other mod artwork is used.
"""
import json
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont,PngImagePlugin

FOOTER='Accessory suggestions are information only. Current accessory access still follows character rules.'

def load_guide(root,r):
 descriptions=json.loads((root/'source/class_descriptions.json').read_text(encoding='utf-8'))
 proposal=json.loads((root/'ACCESSORY_PROPOSAL.json').read_text(encoding='utf-8'))
 classes={j['name'] for j in r['classes']}
 assert set(descriptions)==classes
 assert len(proposal['accessories'])==32
 assert {a['name'] for a in proposal['accessories']}==set(r['accessory_ids'])
 for a in proposal['accessories']:
  assert a['classes'] and set(a['classes'])<=classes
  assert len(a['classes'])==len(set(a['classes']))
 return descriptions,proposal

def class_accessories(proposal,name):
 return [a for a in proposal['accessories'] if name in a['classes']]

def accessory_key(r,name):return 'accessory_'+str(r['accessory_ids'][name]).zfill(2)

def render_card(path,title,role,sections,metadata,font_dir):
 # 640 logical pixels produces a readable preview in 7H's default right panel.
 # Variable height keeps short classes large; the longest pools still fit.
 scale=2;width=640;pad=24;available=width-2*pad
 font=lambda n,bold=False:ImageFont.truetype(str(font_dir/('segoeuib.ttf' if bold else 'segoeui.ttf')),n*scale)
 fonts={'title':font(34,True),'body':font(25),'label':font(22,True),'foot':font(22)}
 probe=ImageDraw.Draw(Image.new('RGB',(1,1)))
 commands=[];y=18
 def block(text,which,color,after=10):
  nonlocal y
  f=fonts[which];line_height={'title':44,'body':33,'label':30,'foot':29}[which]
  for paragraph in text.split('\n'):
   line=''
   for word in paragraph.split():
    trial=(line+' '+word).strip()
    if line and probe.textlength(trial,font=f)>available*scale:
     commands.append((pad,y,line,f,color));y+=line_height;line=word
    else:line=trial
   if line:commands.append((pad,y,line,f,color));y+=line_height
  y+=after
 block(title,'title','#f2f4f5',6)
 block(role,'body','#c8d0d6',14)
 for label,text in sections:
  block(label,'label','#92c4d8',0)
  block(text,'body','#eef1f3',14)
 divider_y=y;y+=12
 block(FOOTER,'foot','#bdc6cd',0)
 height=y+18
 image=Image.new('RGB',(width*scale,height*scale),'#20262b');draw=ImageDraw.Draw(image)
 draw.line((pad*scale,divider_y*scale,(width-pad)*scale,divider_y*scale),fill='#424c54',width=scale)
 for x,y,text,f,color in commands:
  assert draw.textlength(text,font=f)<=available*scale,(path.name,text)
  draw.text((x*scale,y*scale),text,font=f,fill=color,stroke_width=0)
 info=PngImagePlugin.PngInfo()
 info.add_text('Title',title);info.add_text('Description',metadata);info.add_text('AccessoryStatus','Information only; existing character accessory permissions are active.')
 path.parent.mkdir(parents=True,exist_ok=True)
 image.save(path,pnginfo=info,optimize=True)
 return {'file':str(path),'width':width*scale,'height':height*scale,'logical_height':height,'title':title,'sections':sections}

def generate(root,r,font_dir=Path('C:/Windows/Fonts')):
 descriptions,proposal=load_guide(root,r);mod=root/'mod';rendered=[]
 for j in r['classes']:
  accessories=class_accessories(proposal,j['name'])
  sections=[('MATERIA',', '.join(j['materia'])),('ACCESSORY SUGGESTIONS',', '.join(a['name'] for a in accessories))]
  metadata=j['name']+'\n'+descriptions[j['name']]+'\n'+'\n'.join(label+'\n'+text for label,text in sections)+'\n'+FOOTER
  rendered.append(render_card(mod/'previews/classes'/(j['key']+'.png'),j['name'],descriptions[j['name']],sections,metadata,font_dir))
 for a in sorted(proposal['accessories'],key=lambda a:r['accessory_ids'][a['name']]):
  sections=[('SUGGESTED CLASSES',', '.join(a['classes'])),('WHY IT FITS',a['rationale'])]
  metadata=a['name']+'\n'+a['effect']+'\n'+'\n'.join(label+'\n'+text for label,text in sections)+'\n'+FOOTER
  rendered.append(render_card(mod/'previews/accessories'/(accessory_key(r,a['name'])+'.png'),a['name'],a['effect'],sections,metadata,font_dir))
 return descriptions,proposal,rendered
