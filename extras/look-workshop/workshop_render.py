"""Render every Film Look on a few DNG scenes for the look workshop page.

Usage: python3 workshop_render.py PHOTOS_DIR OUT_DIR
Each scene names a DNG under PHOTOS_DIR. EV is the gain per camera, set by
calibration against camera JPEGs (README.md).
"""
import glob, sys, json, numpy as np
from pathlib import Path
from PIL import Image
sys.path.insert(0,str(Path(__file__).parent)); import render
ROOT=Path(sys.argv[1]) if len(sys.argv)>1 else Path('.')
def f(p): return str(next(ROOT.glob(p)))
EV={'GR IIIx':0.7,'GR IV':0.75}
scenes=[('people','People','GR IIIx','shoots4/gr3x/**/R0001104','Daylight portrait, overcast'),
        ('greenery','Greenery','GR IV','shoots3/gr4/**/R0001280','Garden plants, overcast daylight'),
        ('interior','Interior','GR IV','shoots4/gr4/**/R0001287','Shoes by a white wall, window light'),
        ('colour','Strong colour','GR IV','shoots2/gr4/**/R0001273','Purple kettlebell and bag, indoor'),
        ('indoor','Indoor warm','GR IV','shoots2/gr4/**/R0001274','Table by a window, mixed light')]
c=render.catalog(); size=c['preview']['size']
looks=[{'id':'standard','name':'Camera Standard','mono':False,'look':'The camera’s own Standard Image Control (no Film Look)','preview':c['preview']['baseline']}]
looks+= [{'id':e['id'],'name':e['name'],'mono':e['monochrome'],'look':e.get('look',''),'preview':e['preview']} for e in c['effects']]
out=Path(sys.argv[2]) if len(sys.argv)>2 else Path('workshop'); (out/'img').mkdir(parents=True,exist_ok=True)
for sid,label,cam,base,desc in scenes:
    L=render.resize_linear(render.decode(f(base+'.DNG')),1100)
    L=np.clip(L.astype(np.float64)*2**EV[cam],0,65535).astype(np.uint16)
    for lk in looks:
        im=Image.fromarray(render.apply(L,render.load_table(lk['preview'],size),size))
        im.save(out/'img'/f'{sid}--{lk["id"]}.jpg',quality=82,optimize=True)
        th=im.copy(); th.thumbnail((380,380)); th.save(out/'img'/f'{sid}--{lk["id"]}--t.jpg',quality=78,optimize=True)
    print(sid, L.shape)
meta={'scenes':[{'id':s,'label':l,'camera':cam,'file':Path(b).name,'desc':d} for s,l,cam,b,d in scenes],
      'looks':[{k:v for k,v in lk.items() if k!='preview'} for lk in looks]}
(out/'meta.json').write_text(json.dumps(meta,ensure_ascii=False,indent=1))
