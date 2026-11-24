"""Audit against the approved research catalogue and original accessory index."""
from pathlib import Path
import json,re

def audit(root):
 r=json.loads((root/'source/rules.json').read_text(encoding='utf-8'))
 reference=json.loads((root/'CLASS_REFERENCE.json').read_text(encoding='utf-8'))
 assert r['version']=='1.0.0'
 assert len(r['classes'])==25 and len(r['characters'])==9
 assert len(r['materia_ids'])==83 and len(set(r['materia_ids'].values()))==83
 assert len(r['accessory_ids'])==32 and list(r['accessory_ids'].values())==list(range(32))
 actual={j['name']:j['materia'] for j in r['classes']}
 assert actual==reference,'Class names/pools differ from the approved research catalogue'
 assert len({j['key'] for j in r['classes']})==25
 for j in r['classes']:
  assert re.fullmatch('[a-z_]+',j['key'])
  assert len(set(j['materia']))==len(j['materia'])
  assert set(j['materia'])<=r['materia_ids'].keys()
 assert {m for j in r['classes'] for m in j['materia']}==r['materia_ids'].keys()
 defaults=['Mystic Knight','Paladin','Monk','White Mage','Blue Mage','Thief','Gambler','Black Mage','Dragoon']
 names={j['key']:j['name'] for j in r['classes']}
 assert [names[c['default_class']] for c in r['characters']]==defaults
 assert [c['game_character_id'] for c in r['characters']]==list(range(9))
 assert [c['name'] for c in r['characters']]==['Cloud','Barret','Tifa','Aerith','Red XIII','Yuffie','Cait Sith','Vincent','Cid']
 text=(root/'REFERENCE.md').read_text(encoding='utf-8-sig')
 text=text.split('## Complete item index (coverage audit)',1)[1].split('### Accessories',1)[1].split('## Sources',1)[0]
 expected={}
 for line in text.splitlines():
  m=re.match(r'^\| ([^|]+) \| ([^|]+) \|$',line)
  if m and m[1].strip()!='Item':expected[m[1].strip()]={v.strip() for v in m[2].split(',')}
 actual={name:{c['name'] for c in r['characters'] if name in c['accessories']} for name in r['accessory_ids']}
 assert len(expected)==32 and actual==expected,'Accessory permissions differ from the original file'
 print('PASS: 25 exact researched classes, 83 Materia, nine defaults, 32 original accessory permissions.')
 return r

if __name__=='__main__':audit(Path(__file__).resolve().parent.parent)
