import cv2, numpy as np
from PIL import Image
import os
U=os.path.join(os.path.dirname(os.path.abspath(__file__)),'photos','photo_2026-11-22_22-31-')
# (file, crop box (x0,y0,x1,y1), head ellipse in 640-space: cx, cy, rx, ry)
# (file, head center x,y, half-width a, half-height b) in ORIGINAL photo pixels (hair-top to chin)
HEAD = {
 'p1': (U+'41.jpg', 100,  86, 30, 45),
 'p2': (U+'40.jpg', 446, 470, 46, 73),
 'p3': (U+'39.jpg', 428, 352, 49, 69),
 'p4': (U+'35.jpg', 264, 482, 49, 65),
 'p5': (os.path.join(os.path.dirname(os.path.abspath(__file__)),'photos','photo_2026-11-22_23-05-08.jpg'), 646, 298, 193, 290),
}
ZOOM=1.72     # crop side = ZOOM * head height
HEAD_CY=0.41 # head centre as fraction of crop height
def geom(k):
    f,hx,hy,a,b=HEAD[k]
    side=int(round(2*b*ZOOM))
    cx=hx; cy=hy+(0.5-HEAD_CY)*side
    x0=int(round(cx-side/2)); y0=int(round(cy-side/2))
    return f,(x0,y0,x0+side,y0+side),(0.5,HEAD_CY,a/side,b/side)
SRC={k:geom(k) for k in HEAD}
S=640
def load(k):
    f,box,_=SRC[k]
    im=cv2.imread(f)
    P=600  # pad so a crop that runs past the photo edge just extends the background
    im=cv2.copyMakeBorder(im,P,P,P,P,cv2.BORDER_REPLICATE)
    x0,y0,x1,y1=[v+P for v in box]
    c=im[y0:y1,x0:x1]
    c=cv2.resize(c,(S,S),interpolation=cv2.INTER_CUBIC)
    return c

def segment(img,ell):
    cx,cy,rx,ry=ell
    mask=np.full((S,S),cv2.GC_BGD,np.uint8)
    # probable fg: bigger ellipse (head+shoulders); definite fg: smaller head ellipse
    yy,xx=np.mgrid[0:S,0:S]
    def e(cx,cy,rx,ry): return ((xx-cx*S)/(rx*S))**2+((yy-cy*S)/(ry*S))**2<=1
    mask[e(cx,cy,rx*1.9,ry*1.6)]=cv2.GC_PR_BGD
    mask[e(cx,cy+0.30,rx*2.0,0.34)]=cv2.GC_PR_FGD  # shoulders/torso
    mask[e(cx,cy,rx*1.25,ry*1.2)]=cv2.GC_PR_FGD
    mask[e(cx,cy,rx*0.8,ry*0.8)]=cv2.GC_FGD
    bg=np.zeros((1,65),np.float64); fg=np.zeros((1,65),np.float64)
    cv2.grabCut(img,mask,None,bg,fg,8,cv2.GC_INIT_WITH_MASK)
    m=((mask==cv2.GC_FGD)|(mask==cv2.GC_PR_FGD)).astype(np.uint8)
    m=cv2.morphologyEx(m,cv2.MORPH_OPEN,np.ones((7,7),np.uint8))
    m=cv2.morphologyEx(m,cv2.MORPH_CLOSE,np.ones((15,15),np.uint8))
    # keep largest component
    n,lab,st,_=cv2.connectedComponentsWithStats(m)
    if n>1:
        i=1+np.argmax(st[1:,cv2.CC_STAT_AREA]); m=(lab==i).astype(np.uint8)
    m=cv2.GaussianBlur(m.astype(np.float32),(0,0),2.2)
    return np.clip(m,0,1)

def cartoonize(img,levels=7):
    # smooth flat regions
    x=cv2.bilateralFilter(img,9,40,9)
    x=cv2.pyrMeanShiftFiltering(x,sp=9,sr=26)
    x=cv2.bilateralFilter(x,9,30,7)
    # boost saturation a touch, quantize value channel for cel shading
    hsv=cv2.cvtColor(x,cv2.COLOR_BGR2HSV).astype(np.float32)
    hsv[...,1]=np.clip(hsv[...,1]*1.18,0,255)
    v=hsv[...,2]/255.0
    v=np.round(v*levels)/levels
    hsv[...,2]=np.clip(v*255*1.04,0,255)
    x=cv2.cvtColor(hsv.astype(np.uint8),cv2.COLOR_HSV2BGR)
    # ink lines
    g=cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)
    g=cv2.medianBlur(g,7)
    e=cv2.adaptiveThreshold(g,255,cv2.ADAPTIVE_THRESH_MEAN_C,cv2.THRESH_BINARY,13,9)
    e=cv2.morphologyEx(e,cv2.MORPH_OPEN,np.ones((2,2),np.uint8))
    e=cv2.erode(e,np.ones((2,2),np.uint8))
    out=cv2.bitwise_and(x,x,mask=e)
    return out

if __name__=='__main__':
    tiles=[]
    for k in SRC:
        im=load(k); m=segment(im,SRC[k][2]); c=cartoonize(im)
        bgc=np.zeros_like(c); bgc[:]=(40,30,25)
        comp=(c*m[...,None]+bgc*(1-m[...,None])).astype(np.uint8)
        cv2.imwrite(f'{k}_cart.png',comp); cv2.imwrite(f'{k}_mask.png',(m*255).astype(np.uint8))
        tiles.append(cv2.resize(comp,(320,320)))
        tiles.append(cv2.resize(cv2.cvtColor((m*255).astype(np.uint8),cv2.COLOR_GRAY2BGR),(320,320)))
    row1=np.hstack(tiles[0:4]); row2=np.hstack(tiles[4:8])
    cv2.imwrite('proof.png',np.vstack([row1,row2]))
