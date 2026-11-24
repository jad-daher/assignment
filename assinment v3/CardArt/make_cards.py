"""Generates card_1..card_5.jpg (cartoon portraits) in the same frame style as the originals.
Edit NAMES below to change the printed titles, then re-run:  python3 make_cards.py
"""
import cv2, numpy as np, math, sys, os
from PIL import Image, ImageDraw, ImageFont
import cartoon as C

OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'GLProject', 'textures')
FONT_B = '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'
import os
if not os.path.exists(FONT_B): FONT_B = '/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf'

# key: (title, subtitle, accent RGB, bg-top RGB, bg-bottom RGB)
NAMES = {
 'p1': ('RADWAN',   'THE CALM ONE',       (255,176, 64), (58,44,28), (30,22,14)),
 'p2': ('MICHEL',   'THE ADVENTURER',     (110,205,255), (30,52,88), (14,26,48)),
 'p3': ('ANTOUN',   'THE NIGHT OWL',      (255, 96, 96), (84,26,28), (30,12,14)),
 'p4': ('JAD',      'THE MIRROR MASTER',  (186,140,255), (60,36,92), (24,14,40)),
 'p5': ('MR.BEAST', 'THE MILLIONAIRE',    (120,235,130), (22,78,46), (8,34,20)),
}
CARD_ORDER = ['p1','p2','p3','p4','p5']     # -> card_1 .. card_5
GOLD=(228,194,97); CREAM=(250,232,184); PLATE=(20,16,12)

HM={'p1':0.97,'p2':1.0,'p3':1.04,'p4':1.04,'p5':0.90}
KEY=None
def bust_mask(ell):
    cx,cy,rx,ry=ell
    yy,xx=np.mgrid[0:C.S,0:C.S].astype(np.float32)
    def e(cx,cy,rx,ry): return (((xx-cx*C.S)/(rx*C.S))**2+((yy-cy*C.S)/(ry*C.S))**2<=1)
    head=e(cx,cy,rx*HM.get(KEY,1.04),ry*(HM.get(KEY,1.04)-0.03))
    neck=(np.abs(xx-cx*C.S)<=rx*C.S*0.72)&(yy>=(cy+ry*0.7)*C.S)&(yy<=(cy+ry*1.9)*C.S)
    shoulders=e(cx,cy+ry+0.31,rx*2.7,0.30)
    return (head|neck|shoulders).astype(np.float32)

def portrait(k):
    f,box,ell=C.SRC[k]
    img=C.load(k)
    if k=='p1': img=cv2.fastNlMeansDenoisingColored(img,None,9,9,7,21)
    global KEY; KEY=k
    m=bust_mask(ell)
    m=(cv2.GaussianBlur(m,(0,0),3)>0.5).astype(np.float32)
    m=cv2.GaussianBlur(m,(0,0),1.6)
    cart=C.cartoonize(img if k!='p1' else cv2.GaussianBlur(img,(0,0),1.4))
    # ink outline around silhouette
    mb=(m>0.5).astype(np.uint8)
    edge=cv2.dilate(mb,np.ones((9,9),np.uint8))-cv2.erode(mb,np.ones((3,3),np.uint8))
    return cart,m,edge.astype(np.float32)

def backdrop(size,top,bot,accent):
    w=size; yy,xx=np.mgrid[0:w,0:w].astype(np.float32)
    t=(yy/w)[...,None]
    bg=np.array(top)[None,None,:]*(1-t)+np.array(bot)[None,None,:]*t
    cx,cy=w*0.5,w*0.46
    ang=np.arctan2(yy-cy,xx-cx); r=np.hypot(xx-cx,yy-cy)
    rays=(np.sin(ang*14)>0).astype(np.float32)[...,None]
    fade=np.clip(1-r/(w*0.75),0,1)[...,None]
    bg=bg*(1-0.22*rays*fade)+np.array(accent)[None,None,:]*0.16*rays*fade
    glow=np.exp(-(r/(w*0.30))**2)[...,None]
    bg=bg+np.array(accent)[None,None,:]*0.18*glow
    return np.clip(bg,0,255)

def build(k,title,sub,accent,top,bot):
    W=512
    card=Image.new('RGB',(W,W),tuple(int(v*0.55) for v in top))
    # background gradient of the card itself
    g=np.zeros((W,W,3),np.float32)
    t=(np.arange(W)/W)[:,None,None]
    g[:]=np.array(top)[None,None,:]*(1-t)+np.array(bot)[None,None,:]*t
    card=Image.fromarray(g.astype(np.uint8))
    d=ImageDraw.Draw(card)
    # frames
    d.rectangle((10,10,501,501),outline=GOLD,width=8)
    d.rectangle((28,28,483,483),outline=accent,width=3)
    # medallion
    cx,cy,R=256,214,158
    S=2*R
    cart,m,edge=portrait(k)
    L=int(S*1.12)
    cartR=cv2.resize(cart,(L,L),interpolation=cv2.INTER_AREA)
    mR=cv2.resize(m,(L,L),interpolation=cv2.INTER_AREA)
    eR=cv2.resize(edge,(L,L),interpolation=cv2.INTER_AREA)
    bgm=backdrop(L,top,bot,accent)
    ink=np.array([14,10,8],np.float32)
    person=cartR[...,::-1].astype(np.float32)
    comp=bgm*(1-mR[...,None])+person*mR[...,None]
    # outline: ink ring just outside the silhouette
    ring=np.clip(eR*(1-mR)*1.6,0,1)[...,None]
    comp=comp*(1-ring)+ink*ring
    comp=Image.fromarray(np.clip(comp,0,255).astype(np.uint8))
    # place and clip to circle
    layer=Image.fromarray(np.clip(backdrop(S,top,bot,accent),0,255).astype(np.uint8))
    ox=(S-L)//2; oy=(S-L)//2+int(S*0.10)
    layer.paste(comp,(ox,oy))
    mask=Image.new('L',(S*4,S*4),0); ImageDraw.Draw(mask).ellipse((0,0,S*4-1,S*4-1),fill=255)
    mask=mask.resize((S,S),Image.LANCZOS)
    card.paste(layer,(cx-R,cy-R),mask)
    # rings
    ring=Image.new('RGBA',(W*4,W*4),(0,0,0,0)); rd=ImageDraw.Draw(ring)
    rd.ellipse(((cx-R)*4,(cy-R)*4,(cx+R)*4,(cy+R)*4),outline=GOLD+(255,),width=20)
    ring=ring.resize((W,W),Image.LANCZOS); card.paste(ring,(0,0),ring)
    d=ImageDraw.Draw(card)
    # name plate
    d.rectangle((38,384,474,474),fill=PLATE,outline=GOLD,width=4)
    ft=ImageFont.truetype(FONT_B,38); fs=ImageFont.truetype(FONT_B,22)
    d.text((256,414),title,font=ft,fill=CREAM,anchor='mm')
    d.text((256,452),sub,font=fs,fill=accent,anchor='mm')
    return card

if __name__=='__main__':
    sheet=Image.new('RGB',(512*5,512))
    for i,k in enumerate(CARD_ORDER):
        title,sub,acc,top,bot=NAMES[k]
        c=build(k,title,sub,acc,top,bot)
        c.save(f'{OUT}/card_{i+1}.jpg',quality=95)
        sheet.paste(c,(i*512,0))
    sheet.resize((1920,384)).save(os.path.join(os.path.dirname(os.path.abspath(__file__)),'preview_sheet.png'))
    print('done')
