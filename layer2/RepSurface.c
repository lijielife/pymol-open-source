/* 
A* -------------------------------------------------------------------
B* This file contains source code for the PyMOL computer program
C* copyright 1998-2000 by Warren Lyford Delano of DeLano Scientific. 
D* -------------------------------------------------------------------
E* It is unlawful to modify or remove this copyright notice.
F* -------------------------------------------------------------------
G* Please see the accompanying LICENSE file for further information. 
H* -------------------------------------------------------------------
I* Additional authors of this source file include:
-* 
-* 
-*
Z* -------------------------------------------------------------------
*/

#include"os_predef.h"
#include"os_std.h"
#include"os_gl.h"

#include"Base.h"
#include"MemoryDebug.h"
#include"OOMac.h"
#include"RepSurface.h"
#include"Map.h"
#include"Scene.h"
#include"Sphere.h"
#include"Setting.h"
#include"Color.h"
#include"ObjectMolecule.h"
#include"Triangle.h"
#include"Vector.h"
#include"Feedback.h"
#include"main.h"
#include"Util.h"
#include"CGO.h"
#include"P.h"

#ifdef NT
#undef NT
#endif

typedef struct RepSurface {
  Rep R;
  int N;
  int NT;
  int proximity;
  float *V,*VN,*VC;
  int *Vis;
  int *T,*S; /* S=strips */
  int NDot;
  float *Dot;
  int solidFlag;
  int oneColorFlag,oneColor;
  int allVisibleFlag;
  int *LastVisib;
  int *LastColor;
  float max_vdw;
  CGO *debug;
} RepSurface;


void RepSurfaceRender(RepSurface *I,CRay *ray,Pickable **pick);
void RepSurfaceFree(RepSurface *I);
int RepSurfaceSameVis(RepSurface *I,CoordSet *cs);

void RepSurfaceColor(RepSurface *I,CoordSet *cs);

void RepSurfaceFree(RepSurface *I)
{
  FreeP(I->V);
  FreeP(I->VN);
  FreeP(I->VC);
  FreeP(I->Vis);
  FreeP(I->LastColor);
  FreeP(I->LastVisib);
  CGOFree(I->debug);
  VLAFreeP(I->T);
  VLAFreeP(I->S);
  /*  VLAFreeP(I->N);*/
  OOFreeP(I);
}

void RepSurfaceGetSolventDots(RepSurface *I,CoordSet *cs,
                              float probe_radius,SphereRec *sp,
                              float *extent,int *present);

static int ZOrderFn(float *array,int l,int r)
{
  return (array[l]<=array[r]);
}

static int ZRevOrderFn(float *array,int l,int r)
{
  return (array[l]>=array[r]);
}

void RepSurfaceRender(RepSurface *I,CRay *ray,Pickable **pick)
{
  float *v=I->V;
  float *vn=I->VN;
  float *vc=I->VC;
  int *t=I->T;
  int *s=I->S;
  int c=I->N;
  int *vi=I->Vis;
  float *col;
  float alpha;
  int t_mode;
  alpha = SettingGet_f(I->R.cs->Setting,I->R.obj->Setting,cSetting_transparency);
  alpha=1.0-alpha;
  if(fabs(alpha-1.0)<R_SMALL4)
    alpha=1.0;
  if(ray) {
    ray->fTransparentf(ray,1.0-alpha);
	 c=I->NT;
    if(I->oneColorFlag) {
      col=ColorGet(I->oneColor);
      while(c--)
        {
          if((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
             ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2))))))
            ray->fTriangle3fv(ray,v+(*t)*3,v+(*(t+1))*3,v+(*(t+2))*3,
                              vn+(*t)*3,vn+(*(t+1))*3,vn+(*(t+2))*3,
                              col,col,col);
          t+=3;
        }
    } else {
      while(c--)
        {
          if((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
             ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2))))))
          if((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2)))))
            ray->fTriangle3fv(ray,v+(*t)*3,v+(*(t+1))*3,v+(*(t+2))*3,
                              vn+(*t)*3,vn+(*(t+1))*3,vn+(*(t+2))*3,
                              vc+(*t)*3,vc+(*(t+1))*3,vc+(*(t+2))*3);
          t+=3;
        }
    }
    ray->fTransparentf(ray,0.0);
  } else if(pick&&PMGUI) {
  } else if(PMGUI) {



    if(I->debug)
      CGORenderGL(I->debug,NULL,NULL,NULL);
	 if(I->S) {
      if(alpha!=1.0) {

        t_mode  = SettingGet_f(I->R.cs->Setting,I->R.obj->Setting,cSetting_transparency_mode);

        if(t_mode) {

          float **t_buf=NULL,**tb;
          float *z_value=NULL,*zv;
          int *ix=NULL;
          int n_tri = 0;
          float sum[3];
          float matrix[16];

          glGetFloatv(GL_MODELVIEW_MATRIX,matrix);

          t_buf = Alloc(float*,I->NT*9);

          z_value = Alloc(float,I->NT);
          ix = Alloc(int,I->NT);

          zv = z_value;
          tb = t_buf;
          c = I->NT;
          if(I->oneColorFlag) {
            while(c--) {       
              if((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
                 ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2)))))) {

                *(tb++) = vn+(*t)*3;
                *(tb++) = v+(*t)*3;
                *(tb++) = vn+(*(t+1))*3;
                *(tb++) = v+(*(t+1))*3;
                *(tb++) = vn+(*(t+2))*3;
                *(tb++) = v+(*(t+2))*3;
              
                add3f(tb[-1],tb[-3],sum);
                add3f(sum,tb[-5],sum);

                *(zv++) = matrix[2]*sum[0]+matrix[6]*sum[1]+matrix[10]*sum[2];
                n_tri++;
              }
              t+=3;
            
            }
          } else {
            while(c--) {
              if((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
                 ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2))))))
                if((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))) {
                
                  *(tb++) = vc+(*t)*3;
                  *(tb++) = vn+(*t)*3;
                  *(tb++) = v+(*t)*3;

                  *(tb++) = vc+(*(t+1))*3;
                  *(tb++) = vn+(*(t+1))*3;
                  *(tb++) = v+(*(t+1))*3;

                  *(tb++) = vc+(*(t+2))*3;
                  *(tb++) = vn+(*(t+2))*3;
                  *(tb++) = v+(*(t+2))*3;
                
                  add3f(tb[-1],tb[-4],sum);
                  add3f(sum,tb[-7],sum);

                  *(zv++) = matrix[2]*sum[0]+matrix[6]*sum[1]+matrix[10]*sum[2];
                  n_tri++;
                }
              t+=3;
            }
          }
        
          switch(t_mode) {
          case 1:
            UtilSortIndex(n_tri,z_value,ix,(UtilOrderFn*)ZOrderFn);
            break;
          default:
            UtilSortIndex(n_tri,z_value,ix,(UtilOrderFn*)ZRevOrderFn);
            break;
          }

          c=n_tri;
          if(I->oneColorFlag) {
            col=ColorGet(I->oneColor);
          
            glColor4f(col[0],col[1],col[2],alpha);
            glBegin(GL_TRIANGLES);
            for(c=0;c<n_tri;c++) {
            
              tb = t_buf+6*ix[c];
            
              glNormal3fv(*(tb++));
              glVertex3fv(*(tb++));
              glNormal3fv(*(tb++));
              glVertex3fv(*(tb++));
              glNormal3fv(*(tb++));
              glVertex3fv(*(tb++));
            }
            glEnd();
          } else {
            glBegin(GL_TRIANGLES);
            for(c=0;c<n_tri;c++) {
              float *vv;
            
              tb = t_buf+9*ix[c];
            
              vv = *(tb++);
            
              glColor4f(vv[0],vv[1],vv[2],alpha);
              glNormal3fv(*(tb++));
              glVertex3fv(*(tb++));
            
              vv = *(tb++);
              glColor4f(vv[0],vv[1],vv[2],alpha);
              glNormal3fv(*(tb++));
              glVertex3fv(*(tb++));
            
              vv = *(tb++);
              glColor4f(vv[0],vv[1],vv[2],alpha);
              glNormal3fv(*(tb++));
              glVertex3fv(*(tb++));
            
            }
            glEnd();
          }
        
          FreeP(ix);
          FreeP(z_value);
          FreeP(t_buf);
        } else { /* fast and ugly */
          /*          glCullFace(GL_BACK);
          glEnable(GL_CULL_FACE);
          glDepthMask(GL_FALSE);*/
          if(I->allVisibleFlag) {
            if(I->oneColorFlag) {
              col = ColorGet(I->oneColor);
              glColor4f(col[0],col[1],col[2],alpha);
              c=*(s++);
              while(c) {
                glBegin(GL_TRIANGLE_STRIP);
                glNormal3fv(vn+(*s)*3);
                glVertex3fv(v+(*s)*3);
                s++;
                glNormal3fv(vn+(*s)*3);
                glVertex3fv(v+(*s)*3);
                s++;
                while(c--)
                  {
                    glNormal3fv(vn+(*s)*3);
                    glVertex3fv(v+(*s)*3);
                    s++;
                  }
                glEnd();
                c=*(s++);
              }
            } else {
              c=*(s++);
              while(c) {
                glBegin(GL_TRIANGLE_STRIP);
                col = vc+(*s)*3;
                glColor4f(col[0],col[1],col[2],alpha);            
                glNormal3fv(vn+(*s)*3);
                glVertex3fv(v+(*s)*3);
                s++;
                col = vc+(*s)*3;
                glColor4f(col[0],col[1],col[2],alpha);            
                glNormal3fv(vn+(*s)*3);
                glVertex3fv(v+(*s)*3);
                s++;
                while(c--)
                  {
                    col = vc+(*s)*3;
                    glColor4f(col[0],col[1],col[2],alpha);            
                    glNormal3fv(vn+(*s)*3);
                    glVertex3fv(v+(*s)*3);
                    s++;
                  }
                glEnd();
                c=*(s++);
              }
            }
          
          } else { /* subset s*/
            c=I->NT;
            if(c) {
              glBegin(GL_TRIANGLES);
            
              if(I->oneColorFlag) {
                col = ColorGet(I->oneColor);
                glColor4f(col[0],col[1],col[2],alpha);
                while(c--) {

                  if((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
                     ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2)))))) {

                    col = vc+(*t)*3;
                    glNormal3fv(vn+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                    col = vc+(*t)*3;
                    glNormal3fv(vn+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                    col = vc+(*t)*3;
                    glNormal3fv(vn+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                  } else
                    t+=3;
                }
              } else {
                while(c--) {
                  if((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
                     ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2)))))) {
                  
                    col = vc+(*t)*3;
                    glColor4f(col[0],col[1],col[2],alpha);            
                    glNormal3fv(vn+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                    col = vc+(*t)*3;
                    glColor4f(col[0],col[1],col[2],alpha);            
                    glNormal3fv(vn+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                    col = vc+(*t)*3;
                    glColor4f(col[0],col[1],col[2],alpha);            
                    glNormal3fv(vn+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                  } else
                    t+=3;
                }
              }
              glEnd();
            }
          }
          /*          glDisable(GL_CULL_FACE);
                      glDepthMask(GL_TRUE);*/
        }
      } else { /* opaque */

        int use_dlst,simplify=0;
        use_dlst = (int)SettingGet(cSetting_use_display_lists);
        simplify = (int)SettingGet(cSetting_simplify_display_lists);
        if(use_dlst&&I->R.displayList) {
          glCallList(I->R.displayList);
        } else { 
          
          if(use_dlst) {
            if(!I->R.displayList) {
              I->R.displayList = glGenLists(1);
              if(I->R.displayList) {
                glNewList(I->R.displayList,GL_COMPILE_AND_EXECUTE);
              }
            }
          }
          if(I->allVisibleFlag) {
            if(I->oneColorFlag) {
              if(use_dlst&&simplify) {
                glColor3fv(ColorGet(I->oneColor));
                c=*(s++);
                while(c) {
                  glBegin(GL_TRIANGLES);
                  s+=2;
                  while(c--)
                    {
                      s-=2;
                      glNormal3fv(vn+(*s)*3);
                      glVertex3fv(v+(*s)*3);
                      s++;
                      glNormal3fv(vn+(*s)*3);
                      glVertex3fv(v+(*s)*3);
                      s++;
                      glNormal3fv(vn+(*s)*3);
                      glVertex3fv(v+(*s)*3);
                      s++;
                    }
                  glEnd();
                  c=*(s++);
                }
              } else {
                glColor3fv(ColorGet(I->oneColor));
                c=*(s++);
                while(c) {
                  glBegin(GL_TRIANGLE_STRIP);
                  glNormal3fv(vn+(*s)*3);
                  glVertex3fv(v+(*s)*3);
                  s++;
                  glNormal3fv(vn+(*s)*3);
                  glVertex3fv(v+(*s)*3);
                  s++;
                  while(c--)
                    {
                      glNormal3fv(vn+(*s)*3);
                      glVertex3fv(v+(*s)*3);
                      s++;
                    }
                  glEnd();
                  c=*(s++);
                }
              } /* use_dlst&&simplify */
            } else {
              if(use_dlst&&simplify) {
                c=*(s++);
                while(c) {
                  glBegin(GL_TRIANGLES);
                  s+=2;
                  while(c--)
                    {
                      s-=2;
                      glColor3fv(vc+(*s)*3);
                      glNormal3fv(vn+(*s)*3);
                      glVertex3fv(v+(*s)*3);
                      s++;
                      glColor3fv(vc+(*s)*3);
                      glNormal3fv(vn+(*s)*3);
                      glVertex3fv(v+(*s)*3);
                      s++;
                      glColor3fv(vc+(*s)*3);
                      glNormal3fv(vn+(*s)*3);
                      glVertex3fv(v+(*s)*3);
                      s++;
                    }
                  glEnd();
                  c=*(s++);
                }
              } else {
                c=*(s++);
                while(c) {
                  glBegin(GL_TRIANGLE_STRIP);
                  glColor3fv(vc+(*s)*3);
                  glNormal3fv(vn+(*s)*3);
                  glVertex3fv(v+(*s)*3);
                  s++;
                  glColor3fv(vc+(*s)*3);
                  glNormal3fv(vn+(*s)*3);
                  glVertex3fv(v+(*s)*3);
                  s++;
                  while(c--)
                    {
                      glColor3fv(vc+(*s)*3);
                      glNormal3fv(vn+(*s)*3);
                      glVertex3fv(v+(*s)*3);
                      s++;
                    }
                  glEnd();
                  c=*(s++);
                }
              }
            } /* one color */
          } else { /* subsets */
            c=I->NT;
            if(c) {
              glBegin(GL_TRIANGLES);
              if(I->oneColorFlag) {
                glColor3fv(ColorGet(I->oneColor));
                while(c--) {
                  if((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
                     ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2)))))) {
                    
                    glNormal3fv(vn+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                    glNormal3fv(vn+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                    glNormal3fv(vn+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                  } else
                    t+=3;
                }
              } else {
                while(c--) {
                  if((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
                     ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2)))))) {
                    
                    glColor3fv(vc+(*t)*3);
                    glNormal3fv(vn+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                    glColor3fv(vc+(*t)*3);
                    glNormal3fv(vn+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                    glColor3fv(vc+(*t)*3);
                    glNormal3fv(vn+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                  } else
                    t+=3;
                }
              }
              glEnd();
            }
          }
          if(use_dlst&&I->R.displayList) {
            glEndList();
          }
        }
      }
    }
    if(SettingGet(cSetting_surface_debug)) {
      
      c=I->NT;
      if(c) {
        glBegin(GL_TRIANGLES);
        while(c--) {
          glNormal3fv(vn+(*t)*3);
          glVertex3fv(v+(*t)*3);
          t++;
          glNormal3fv(vn+(*t)*3);
          glVertex3fv(v+(*t)*3);
          t++;
          glNormal3fv(vn+(*t)*3);
          glVertex3fv(v+(*t)*3);
          t++;
        }
        glEnd();
      }
      
      t=I->T;
      c=I->NT;
      if(c) {
        glColor3f(0.0,1.0,0.0);
        
        while(c--)
          {
            glBegin(GL_LINE_STRIP);
            
            glNormal3fv(vn+(*t)*3);
            glVertex3fv(v+(*t)*3);
            t++;
            glNormal3fv(vn+(*t)*3);
            glVertex3fv(v+(*t)*3);
            t++;
            glNormal3fv(vn+(*t)*3);
            glVertex3fv(v+(*t)*3);
            t++;
            glEnd();
          }
      }
      c=I->N;
      if(c) {
        glColor3f(1.0,0.0,0.0);
        glBegin(GL_LINES);
        SceneResetNormal(true);
        while(c--)
          {
            glVertex3fv(v);
            glVertex3f(v[0]+vn[0]/2,v[1]+vn[1]/2,v[2]+vn[2]/2);
            v+=3;
            vn+=3;
          }
        glEnd();
      }
      glColor3f(0.3,0.3,1.0);
      v=TestLine;
      c=NTestLine;
      glBegin(GL_LINES);
      while(c--) {
        glVertex3fv(v);
        v+=3;
        glVertex3fv(v);
        v+=3;
      }
      glEnd();
    }
  }
}

#define solv_tole 0.02

int RepSurfaceSameVis(RepSurface *I,CoordSet *cs)
{
  int same = true;
  int *lv,*lc,*cc;
  int a;
  AtomInfoType *ai;

  ai = cs->Obj->AtomInfo;
  lv = I->LastVisib;
  lc = I->LastColor;
  cc = cs->Color;

  for(a=0;a<cs->NIndex;a++)
    {
      if(*(lv++)!=(ai + cs->IdxToAtm[a])->visRep[cRepSurface] ) {
        same=false;
        break;
      }
      if(*(lc++)!=*(cc++)) {
        same=false;
        break;
      }
    }
  return(same);
}

void RepSurfaceColor(RepSurface *I,CoordSet *cs)
{
  MapType *map;
  int a,i0,i,j,c1;
  float *v0,*vc,*c0;
  int *vi,*lv,*lc,*cc;
  int first_color;
  ObjectMolecule *obj;
  float probe_radius;
  float dist,minDist;
  float cutoff;
  int inclH;
  int cullByFlag = false;
  int surface_mode;
  int surface_color;
  int *present=NULL,*ap;
  AtomInfoType *ai2,*ai1;

  obj=cs->Obj;
  surface_mode = SettingGet_i(cs->Setting,obj->Obj.Setting,cSetting_surface_mode);
  surface_color = SettingGet_color(cs->Setting,obj->Obj.Setting,cSetting_surface_color);
  cullByFlag = (surface_mode==cRepSurface_by_flags);
  inclH = !(surface_mode==cRepSurface_heavy_atoms);
  probe_radius = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_solvent_radius);
  I->proximity = SettingGet_b(cs->Setting,obj->Obj.Setting,cSetting_surface_proximity);

  cutoff = I->max_vdw+2*probe_radius;

  if(!I->LastVisib) I->LastVisib = Alloc(int,cs->NIndex);
  if(!I->LastColor) I->LastColor = Alloc(int,cs->NIndex);
  lv = I->LastVisib;
  lc = I->LastColor;
  cc = cs->Color;
  ai2=obj->AtomInfo;
  for(a=0;a<cs->NIndex;a++)
    {
      *(lv++) = (ai2 + cs->IdxToAtm[a])->visRep[cRepSurface];
      *(lc++) = *(cc++);
    }
  
  if(I->N) {
    if(!I->VC) I->VC = Alloc(float,3*I->N);
    vc=I->VC;
    if(!I->Vis) I->Vis = Alloc(int,I->N);
    vi=I->Vis;
    
    if(ColorCheckRamped(surface_color)) {
      I->oneColorFlag=false;
    } else {
      I->oneColorFlag=true;
    }
    first_color=-1;

    present = Alloc(int,cs->NIndex); 
    ap = present;
    for(a=0;a<cs->NIndex;a++) {
      ai1 = obj->AtomInfo + cs->IdxToAtm[a];
      if(ai1->visRep[cRepSurface]&&
         (inclH||(!ai1->hydrogen))&&
         ((!cullByFlag)||
            (!(ai1->flags&(cAtomFlag_ignore|cAtomFlag_exfoliate)))))
        *ap = 2; 
      else 
        *ap = 0;
      ap++;
    }
    
    map=MapNewFlagged(2*I->max_vdw+probe_radius,cs->Coord,cs->NIndex,NULL,present);
    MapSetupExpress(map);
    
    for(a=0;a<cs->NIndex;a++)
      if(!present[a])
        {
          ai1 = obj->AtomInfo+cs->IdxToAtm[a];
          if((!cullByFlag)||!(ai1->flags&cAtomFlag_ignore)) {
            v0 = cs->Coord+3*a;
            i=*(MapLocusEStart(map,v0));
            if(i) {
              j=map->EList[i++];
              while(j>=0) {
                if(present[j]>1) {
                  ai2 = obj->AtomInfo+cs->IdxToAtm[j];                  
                  if(within3f(cs->Coord+3*j,v0,ai1->vdw+ai2->vdw+probe_radius))
                      {
                        present[a]=1;
                        break;
                      }
                }
                
                j=map->EList[i++];
              }
            }
          }
        }
    MapFree(map);
    map = NULL;
      
    
    /* now, assign colors to each point */
    map=MapNewFlagged(cutoff,cs->Coord,cs->NIndex,NULL,present);
    if(map)
      {
        MapSetupExpress(map);
        for(a=0;a<I->N;a++)
          {
            c1=1;
            minDist=MAXFLOAT;
            i0=-1;
            v0 = I->V+3*a;
            vi = I->Vis+a;
            /* colors */
            i=*(MapLocusEStart(map,v0));
            if(i) {
              j=map->EList[i++];
              while(j>=0) {
                ai2 = obj->AtomInfo + cs->IdxToAtm[j];
                if((inclH||(!ai2->hydrogen))&&
                   ((!cullByFlag)||
                    (!(ai2->flags&cAtomFlag_ignore))))  
                  {
                    dist = diff3f(v0,cs->Coord+j*3)-ai2->vdw;
                    if(dist<minDist)
                      {
                        i0=j;
                        minDist=dist;
                      }
                  }
                j=map->EList[i++];
              }
            }
            if(i0>=0) {
              c1=*(cs->Color+i0);
              if(I->oneColorFlag) {
                if(first_color>=0) {
                  if(first_color!=c1)
                    I->oneColorFlag=false;
                } else first_color=c1;
              }
              if(I->allVisibleFlag)
                *vi = 1;
              else {
                ai2 = obj->AtomInfo+cs->IdxToAtm[i0];                
                if(ai2->visRep[cRepSurface]&&
                   (inclH||(!ai2->hydrogen))&&
                   ((!cullByFlag)||
                    (!(ai2->flags&(cAtomFlag_ignore|cAtomFlag_exfoliate)))))
                  *vi = 1;
                else
                  *vi = 0;
              }
            } else {
              *vi = 0;
            }
            if(ColorCheckRamped(surface_color)) {
              c1 = surface_color;
            }
            if(ColorCheckRamped(c1)) {
              I->oneColorFlag=false;
              ColorGetRamped(c1,v0,vc);
              vc+=3;
            } else {
              c0 = ColorGet(c1);
              
              *(vc++) = *(c0++);
              *(vc++) = *(c0++);
              *(vc++) = *(c0++);
            }
            vi++;
          }
        MapFree(map);
      }
    if(I->oneColorFlag) {
      I->oneColor=first_color;
    }
  }
  if(surface_color>=0) {
    I->oneColorFlag=true;
    I->oneColor=surface_color;
  }
  if(PMGUI) {
    if(I->R.displayList) {
      if(PIsGlutThread()) {
        glDeleteLists(I->R.displayList,1);
        I->R.displayList = 0;
      } else {
        char buffer[255]; /* pass this off to the main thread */
        sprintf(buffer,"_cmd.gl_delete_lists(%d,%d)\n",I->R.displayList,1);
        PParse(buffer);
      }
    }
  }

  FreeP(present);
}

Rep *RepSurfaceNew(CoordSet *cs)
{
  ObjectMolecule *obj;
  int a,b,i,j,c;
  MapType *map,*solv_map;
  float *v0=NULL,*v,*vn=NULL,*vn0=NULL,*extent=NULL;
  int SurfaceFlag = false;
  float probe_radius,probe_radius2;
  float probe_rad_tol,probe_rad_tol2;
  int inclH = true;
  int cullByFlag = false;
  int flag,*dot_flag,*p;
  float minimum_sep;
  int visFlag;
  int surface_quality;
  int surface_mode;
  int *present = NULL,*ap;
  int pres_flag;
  SphereRec *sp = Sphere0;
  SphereRec *ssp = Sphere0;
  AtomInfoType *ai1,*ai2;
  #if 0
  int c1;
  float v1[3];
  float vdw;
#endif
  OOAlloc(RepSurface);

  obj = cs->Obj;

  I->max_vdw = ObjectMoleculeGetMaxVDW(obj);

  surface_mode = SettingGet_i(cs->Setting,obj->Obj.Setting,cSetting_surface_mode);

  cullByFlag = (surface_mode==cRepSurface_by_flags);
  inclH = !(surface_mode==cRepSurface_heavy_atoms);

  visFlag=false;
  for(a=0;a<cs->NIndex;a++) {
     ai1=obj->AtomInfo+cs->IdxToAtm[a];
     if(ai1->visRep[cRepSurface]&&
        (inclH||(!ai1->hydrogen))&&
        ((!cullByFlag)|
         (!(ai1->flags&(cAtomFlag_exfoliate|cAtomFlag_ignore)))))
       {
         visFlag=true;
         break;
       }
  }
  if(!visFlag) {
    OOFreeP(I);
    return(NULL); /* skip if no thing visible */
  }

  RepInit(&I->R);

  surface_quality = SettingGet_i(cs->Setting,obj->Obj.Setting,cSetting_surface_quality);
  if(surface_quality>=3) { /* perfect but totally impractical */
    minimum_sep = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_surface_best)/9;
    sp=Sphere4;
    ssp=Sphere4;
  } else if(surface_quality>=2) { /* nearly perfect */
    minimum_sep = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_surface_best)/3;
    sp=Sphere3;
    ssp=Sphere3;
  } else if(surface_quality>=1) { /* good */
    minimum_sep = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_surface_best);
    sp=Sphere2;
    ssp=Sphere3;
  } else if(!surface_quality) { /* 0 - normal */
    minimum_sep = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_surface_normal);
    sp=Sphere1;
    ssp=Sphere2;
  } else if(surface_quality==-1) { /* -1 */
    minimum_sep = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_surface_poor);
    sp=Sphere1;
    ssp=Sphere2;
  } else if(surface_quality==-2) { /* -2 god awful*/
    minimum_sep = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_surface_poor);
    sp=Sphere1;
    ssp=Sphere1;
  } else { /* -3 miserable */
    minimum_sep = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_surface_miserable);
    sp=Sphere0;
    ssp=Sphere1;
  }

  probe_radius = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_solvent_radius);
  probe_radius2 = probe_radius*probe_radius;
  probe_rad_tol = probe_radius + solv_tole;
  probe_rad_tol2 = probe_rad_tol * probe_rad_tol;

  I->N=0;
  I->NT=0;
  I->S=NULL;
  I->V=NULL;
  I->VC=NULL;
  I->Vis=NULL;
  I->VN=NULL;
  I->T=NULL;
  I->Dot=NULL;
  I->NDot=0;
  I->LastVisib=NULL;
  I->LastColor=NULL;
  I->R.fRender=(void (*)(struct Rep *, CRay *, Pickable **))RepSurfaceRender;
  I->R.fFree=(void (*)(struct Rep *))RepSurfaceFree;
  I->R.fRecolor=(void (*)(struct Rep*, struct CoordSet*))RepSurfaceColor;
  I->R.fSameVis=(int (*)(struct Rep*, struct CoordSet*))RepSurfaceSameVis;
  I->R.obj = (CObject*)(cs->Obj);
  I->R.cs = cs;
  I->Dot=NULL;
  I->allVisibleFlag=true;
  I->debug = NULL;
  obj = cs->Obj;

  /* don't waist time computing a Surface unless we need it!! */
  for(a=0;a<cs->NIndex;a++) {
    ai1=obj->AtomInfo+cs->IdxToAtm[a];
	 if(ai1->visRep[cRepSurface]&&
       ((!cullByFlag)|
        (!(ai1->flags&(cAtomFlag_exfoliate)))))
      SurfaceFlag=true;
    else
      I->allVisibleFlag=false;
  }
  if(SurfaceFlag) {
      
	 OrthoBusyFast(0,1);

    if(1&&!I->allVisibleFlag) {
      /* optimize the space over which we calculate a surface */
      
      /* first find out which atoms are really present */

      present = Alloc(int,cs->NIndex); 
      ap = present;
      for(a=0;a<cs->NIndex;a++) {
        ai1 = obj->AtomInfo + cs->IdxToAtm[a];
        if(ai1->visRep[cRepSurface]&&
           (inclH||(!ai1->hydrogen))&&
           ((!cullByFlag)||
            (!(ai1->flags&(cAtomFlag_ignore|cAtomFlag_exfoliate)))))
          *ap = 2; 
        else 
          *ap = 0;
        ap++;
      }

      map=MapNewFlagged(2*I->max_vdw+probe_radius,cs->Coord,cs->NIndex,extent,present);
      MapSetupExpress(map);
      
      for(a=0;a<cs->NIndex;a++)
        if(!present[a])
          {
            ai1 = obj->AtomInfo+cs->IdxToAtm[a];
            if((!cullByFlag)||!(ai1->flags&cAtomFlag_ignore)) {
              v0 = cs->Coord+3*a;
              i=*(MapLocusEStart(map,v0));
              if(i) {
                j=map->EList[i++];
                while(j>=0) {
                  if(present[j]>1) {
                    ai2 = obj->AtomInfo+cs->IdxToAtm[j];                  
                    if(within3f(cs->Coord+3*j,v0,ai1->vdw+ai2->vdw+probe_radius))
                      {
                        present[a]=1;
                        break;
                      }
                  }
                  
                  j=map->EList[i++];
                }
              }
            }
          }
      MapFree(map);
      map = NULL;
    }
    
	 I->V=Alloc(float,cs->NIndex*3*sp->nDot*9);
    ErrChkPtr(I->V);
	 I->VN=Alloc(float,cs->NIndex*3*sp->nDot*9);
    ErrChkPtr(I->VN);
	 I->N=0;
    v=I->V;
    vn=I->VN;
    
	 RepSurfaceGetSolventDots(I,cs,probe_radius,ssp,extent,present);

	 solv_map=MapNew(probe_radius,I->Dot,I->NDot,extent);
	 map=MapNewFlagged(I->max_vdw+probe_radius,cs->Coord,cs->NIndex,extent,present);

    /*    I->debug=CGONew();

    CGOBegin(I->debug,GL_POINTS);
    for(a=0;a<I->NDot;a++)
      CGOVertexv(I->debug,I->Dot+3*a);
    CGOEnd(I->debug);
    */
	 if(map&&solv_map)
		{
		  MapSetupExpress(solv_map);
		  MapSetupExpress(map);

#if _PYMOL_UNUSED
        for(a=0;a<cs->NIndex;a++)
          {
            OrthoBusyFast(a+cs->NIndex,cs->NIndex*5); /* 1/5 - 2/5 */
            ai1 = obj->AtomInfo+cs->IdxToAtm[a];
            if(present)
              pres_flag=present[a];
            else
              pres_flag=((inclH||(!ai1->hydrogen))&&
                             ((!cullByFlag)||
                              (!(ai1->flags&(cAtomFlag_exfoliate|cAtomFlag_ignore)))));
            if(pres_flag) {
              c1=*(cs->Color+a);
              v0 = cs->Coord+3*a;
              vdw = ai1->vdw;
              
              for(b=0;b<sp->nDot;b++)
                {
                  v1[0]=v0[0]+vdw*sp->dot[b][0];
                  v1[1]=v0[1]+vdw*sp->dot[b][1];
                  v1[2]=v0[2]+vdw*sp->dot[b][2];
                  
                  flag=true;
                  
                  i=*(MapLocusEStart(map,v1));
                  if(i) {
                    j=map->EList[i++];
                    while(j>=0) {
                      ai2 = obj->AtomInfo + cs->IdxToAtm[j];
                      if(present)
                        pres_flag=present[j];
                      else
                        pres_flag=((inclH||(!ai2->hydrogen))&&
                                   ((!cullByFlag)||
                                    (!(ai2->flags&cAtomFlag_ignore))));
                      if(pres_flag)
                        if(j!=a)
                          if(within3f(cs->Coord+3*j,v1,ai2->vdw))
                            {
                              flag=false;
                              break;
                            }
                      j=map->EList[i++];
                    }
                  }
                  if(0&&flag) /* doesn't intersect any atom */
                    {
                      flag=true;
                      i=*(MapLocusEStart(solv_map,v1));
                      if(i) {
                        j=solv_map->EList[i++];
                        while(j>=0) {
                          if(within3fsq(I->Dot+3*j,v1,probe_rad_tol,probe_rad_tol2))
                            {
                              flag=false;
                              break;
                            }
                          j=solv_map->EList[i++];
                        }
                      }
                      if(!flag) /* appropriately close to a water center */
                        {
                          *(v++)=v1[0];
                          *(v++)=v1[1];
                          *(v++)=v1[2];
                          *(vn++)=sp->dot[b][0];
                          *(vn++)=sp->dot[b][1];
                          *(vn++)=sp->dot[b][2];
                          I->N++;
                        }
                    }
                }
            }
          }

        PRINTFD(FB_RepSurface)
          " RepSurfaceNew-DEBUG: convex surfaces complete...\n"
          ENDFD;
#endif

		  if(I->NDot) {

          Vector3f *dot = NULL;

          dot=Alloc(Vector3f,sp->nDot);
          for(b=0;b<sp->nDot;b++) {
            scale3f(sp->dot[b],probe_radius,dot[b]);
          }
          v0 = I->Dot;

			 /* concave surfaces - SLOW */
			 for(a=0;a<I->NDot;a++)
				{
              OrthoBusyFast(a+I->NDot*2,I->NDot*5); /* 2/5 to 3/5 */
				  for(b=0;b<sp->nDot;b++)
					 {
						v[0]=v0[0]+dot[b][0];
						v[1]=v0[1]+dot[b][1];
						v[2]=v0[2]+dot[b][2];
						flag=true;
						i=*(MapLocusEStart(solv_map,v));
						if(i) {
						  j=solv_map->EList[i++];
						  while(j>=0) {
							 if(j!=a) 
								{
								  if(within3fsq(I->Dot+3*j,v,probe_radius,probe_radius2)) {
									 flag=false;
									 break;
								  }
								}
							 j=solv_map->EList[i++];
						  }
						}
						if(flag)
						  {
							 flag=true;
							 i=*(MapLocusEStart(map,v));
							 if(i) {
								j=map->EList[i++];
								while(j>=0) {
								  ai2 = obj->AtomInfo + cs->IdxToAtm[j];
                          if(present)
                            pres_flag=present[j];
                          else
                            pres_flag=((inclH||(!ai2->hydrogen))&&
                                       ((!cullByFlag)||
                                        (!(ai2->flags&cAtomFlag_ignore))));
                          if(pres_flag)
									 if(j!=a)
										if(within3f(cs->Coord+3*j,v,ai2->vdw+probe_rad_tol))
										  {
											 flag=false;
											 break;
										  }
								  j=map->EList[i++];
								}
							 }
							 if(!flag) {
								I->N++;
								v+=3;
								*(vn++)=-sp->dot[b][0];
								*(vn++)=-sp->dot[b][1];
								*(vn++)=-sp->dot[b][2];
							 }
						  }
					 }
				  v0 +=3;
				}
          FreeP(dot);
		  }
		  MapFree(solv_map);
		  MapFree(map);
		  FreeP(I->Dot);	 
		}
	 /* now, eliminate dots that are too close to each other*/

    /*    CGOColor(I->debug,0.0,1.0,0.0);
    CGOBegin(I->debug,GL_POINTS);
    for(a=0;a<I->N;a++)
      CGOVertexv(I->debug,I->V+3*a);
    CGOEnd(I->debug);
    */

    PRINTFB(FB_RepSurface,FB_Details)
      " RepSurface: %i surface points.\n",I->N
      ENDFB;

	 if(I->N) 
		{
		  dot_flag=Alloc(int,I->N);
		  for(a=0;a<I->N;a++) dot_flag[a]=1;
		  map=MapNew(minimum_sep,I->V,I->N,extent);
        MapSetupExpress(map);		  
        v=I->V;
        vn=I->VN;
        for(a=0;a<I->N;a++) {
          if(dot_flag[a]) {
            i=*(MapLocusEStart(map,v));
            if(i) {
              j=map->EList[i++];
              while(j>=0) {
                if(j!=a) 
                  {
                    if(dot_flag[j]) {
                      if(within3f(I->V+(3*j),v,minimum_sep)) {
                        dot_flag[j]=0;
                        add3f(vn,I->VN+(3*j),vn);
                        average3f(I->V+(3*j),v,v);
                      } 
                    }
                  }
                j=map->EList[i++];
              }
            }
          }
          v+=3;
          vn+=3;
        }
        MapFree(map);
        v0=I->V;
        v=I->V;
        vn0=I->VN;
        vn=I->VN;
        p=dot_flag;
		  c=I->N;
		  I->N=0;
		  for(a=0;a<c;a++)
			 {
				if(*(p++)) {
				  *(v0++)=*(v++);
				  *(v0++)=*(v++);
				  *(v0++)=*(v++);
              normalize3f(vn);
				  *(vn0++)=*(vn++);
				  *(vn0++)=*(vn++);
				  *(vn0++)=*(vn++);
				  I->N++;
				} else {
				  v+=3;
				  vn+=3;
				}
			 }
		  FreeP(dot_flag);
      }

    if(I->N) {	
      I->V = Realloc(I->V,float,(v0-I->V));
      I->VN = Realloc(I->VN,float,(vn0-I->VN));
    }

    PRINTFD(FB_RepSurface)
      " RepSurfaceNew-DEBUG: %i surface points after trimming.\n",I->N
      ENDFD;

	 RepSurfaceColor(I,cs);

    PRINTFD(FB_RepSurface)
      " RepSurfaceNew-DEBUG: %i surface points after coloring.\n",I->N
      ENDFD;

	 OrthoBusyFast(3,5);

    if(I->N) {
      I->T=TrianglePointsToSurface(I->V,I->VN,I->N,probe_radius,&I->NT,&I->S,extent);
      PRINTFB(FB_RepSurface,FB_Details)
        " RepSurface: %i triangles.\n",I->NT
        ENDFB;
    } else {
      I->V = Realloc(I->V,float,1);
      I->VN = Realloc(I->VN,float,1);
    }
    
  }
  if(I->debug)
    CGOStop(I->debug);
  OrthoBusyFast(4,4);
  FreeP(present);
  return((void*)(struct Rep*)I);
}

void RepSurfaceGetSolventDots(RepSurface *I,CoordSet *cs,
                              float probe_radius,SphereRec *sp,
                              float *extent,int *present)
{
  ObjectMolecule *obj;
  int a,b,c=0,flag,i,j;
  float *v,*v0,vdw;
  MapType *map;
  int *p,*dot_flag;
  int cavity_cull;
  float probe_radius_plus;
  int dotCnt,maxCnt,maxDot=0;
  int cnt;
  int surface_mode;
  int cullByFlag;
  int inclH;
  int pres_flag;
  AtomInfoType *ai1,*ai2;

  obj = cs->Obj;

  surface_mode = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_surface_mode);
  cullByFlag = (surface_mode==cRepSurface_by_flags);
  inclH = !(surface_mode==cRepSurface_heavy_atoms);

  cavity_cull = SettingGet_i(cs->Setting,obj->Obj.Setting,cSetting_cavity_cull);

  I->Dot=(float*)mmalloc(sizeof(float)*cs->NIndex*3*sp->nDot);
  ErrChkPtr(I->Dot);

  probe_radius_plus = probe_radius * 1.5;

  I->NDot=0;
  map=MapNewFlagged(I->max_vdw+probe_radius,cs->Coord,cs->NIndex,extent,present);
  if(map)
	 {
		MapSetupExpress(map);
		maxCnt=0;
		v=I->Dot;
		for(a=0;a<cs->NIndex;a++)
		  {
			 OrthoBusyFast(a,cs->NIndex*5);

          ai1 = obj->AtomInfo+cs->IdxToAtm[a];
          if(present)
            pres_flag=present[a];
          else
            pres_flag = (inclH||(!ai1->hydrogen))&&
             ((!cullByFlag)||
              (!(ai1->flags&(cAtomFlag_ignore))));
          if(pres_flag){
            
            dotCnt=0;
            v0 = cs->Coord+3*a;
            vdw = cs->Obj->AtomInfo[cs->IdxToAtm[a]].vdw+probe_radius;
            for(b=0;b<sp->nDot;b++)
              {
                v[0]=v0[0]+vdw*sp->dot[b][0];
                v[1]=v0[1]+vdw*sp->dot[b][1];
                v[2]=v0[2]+vdw*sp->dot[b][2];
                flag=true;
                i=*(MapLocusEStart(map,v));
                if(i) {
                  j=map->EList[i++];
                  while(j>=0) {
                    
                    ai2 = obj->AtomInfo + cs->IdxToAtm[j];
                    if((inclH||(!ai2->hydrogen))&&
                       ((!cullByFlag)||
                        (!(ai2->flags&cAtomFlag_ignore))))
                      if(j!=a) 
                        {
                          if(within3f(cs->Coord+3*j,v,ai2->vdw+probe_radius)) {
                            flag=false;
                            break;
                          }
                        }
                    j=map->EList[i++];
                  }
                }
                if(flag)
                  {
                    dotCnt++;
                    v+=3;
                    I->NDot++;
                  }
              }
            if(dotCnt>maxCnt)
              {
                maxCnt=dotCnt;
                maxDot=I->NDot-1;
              }
          }
        }
      MapFree(map);
	 }

  if(cavity_cull>0) {
	 dot_flag=Alloc(int,I->NDot);
	 ErrChkPtr(dot_flag);
	 for(a=0;a<I->NDot;a++) {
		dot_flag[a]=0;
	 }
	 dot_flag[maxDot]=1; /* this guarantees that we have a valid dot */

	 map=MapNew(probe_radius_plus,I->Dot,I->NDot,extent);
	 if(map)
		{
		  MapSetupExpress(map);		  
		  flag=true;
		  while(flag) {
			 p=dot_flag;
			 v=I->Dot;
		  
			 flag=false;
			 for(a=0;a<I->NDot;a++)
				{
				  if(!dot_flag[a]) {
					 cnt=0;
					 i=*(MapLocusEStart(map,v));
					 if(i) {
						j=map->EList[i++];
						while(j>=0) {
						  if(j!=a) 
							 {
								if(within3f(I->Dot+(3*j),v,probe_radius_plus)) {
								  if(dot_flag[j]) {
									 *p=true;
									 flag=true;
									 break;
								  }
								  cnt++;
								  if(cnt>cavity_cull) 
									 {
										*p=true;
										flag=true;
										break;
									 }
								}
							 }
						  j=map->EList[i++];
						}
					 }
				  }
				  v+=3;
				  p++;
				}
		  }
		  MapFree(map);
		}
	 v0=I->Dot;
	 v=I->Dot;
	 p=dot_flag;
	 c=I->NDot;
	 I->NDot=0;
	 for(a=0;a<c;a++)
		{
		  if(*(p++)) {
			 *(v0++)=*(v++);
			 *(v0++)=*(v++);
			 *(v0++)=*(v++);
			 I->NDot++;
		  } else {
			 v+=3;
		  }
		}
	 FreeP(dot_flag);
  }
  
  PRINTFD(FB_RepSurface)
    " GetSolventDots-DEBUG: %d->%d\n",c,I->NDot
    ENDFD;
           

}


