//-----------------------------------------------------------------------------
// Copyright (C) 2006 Sickhead Games, LLC.
// Portions Copyright (C) 2003 Pat Wilson and Flaming Duck Studio
// Portions Copyright (C) GarageGames.com, Inc.
// Portions (Porting to T3D) Copyright (C) 2009 Lethal Concept, LLC
//-----------------------------------------------------------------------------

#include "gui/core/guiTextureCanvas.h"
#include "gui/core/guiControl.h"
#include "gfx/gfxDevice.h"
#include "console/consoleTypes.h"
#include "T3D/shapeBase.h"
#include "ts/tsShapeInstance.h"
#include "materials/materialList.h"
#include "materials/matInstance.h"
#include "materials/processedMaterial.h"
#include "gfx/primBuilder.h"
#include "gui/core/guiControl.h"
#include "console/engineAPI.h"

GuiTextureCanvas::TextureCanvasVector GuiTextureCanvas::smTextureCanvases;
F32 GuiTextureCanvas::sRayLength = 0.0f;
bool GuiTextureCanvas::sInteract = false;
U32 GuiTextureCanvas::sInteractingWith = false;

Point2F gCurPos = Point2F(0.5, 0.5);

IMPLEMENT_CONOBJECT(GuiTextureCanvas);

extern ColorI gCanvasClearColor;

// texture coords for the quads (when drawn as a trianglestrip)
static const Point2F cursorTexCoords[4] =
{
   Point2F(0, 0),
   Point2F(1, 0),
   Point2F(0, 1),
   Point2F(1, 1)
};

GuiTextureCanvas::GuiTextureCanvas()
{
   mGuiControlName = NULL;
   mGuiControl = NULL;
   mDirty = false;
   smTextureCanvases.push_back( this );
   mShowTexCursor = true;
   mCursorBitmap = StringTable->insert("art/gui/weaponHud/crossHair.png");
   mCursorTextureHandle = NULL;
}


GuiTextureCanvas::~GuiTextureCanvas() 
{
	unregister();
}


void GuiTextureCanvas::unregister()
{
   TextureCanvasVector::iterator i = smTextureCanvases.begin();
   for( ; i != smTextureCanvases.end(); i++ )
   {
      if( (*i) == this )
      {
         smTextureCanvases.erase( i );
         return;
      }
   }
}


void GuiTextureCanvas::initPersistFields()
{
   Parent::initPersistFields();

   addField( "guiControl", TypeString, Offset( mGuiControlName, GuiTextureCanvas ) );
   addField( "showCursor", TypeString, Offset( mShowTexCursor, GuiTextureCanvas ) );
   addField( "cursorBitmap", TypeFilename, Offset(mCursorBitmap, GuiTextureCanvas));
   
}


bool GuiTextureCanvas::onAdd()
{
   /* bad idea, jeans
   if(!Parent::onAdd())
      return false; */

   //this isnt perfect either, but our direct Parent::onAdd() wants to
   //register another window, which is silly... 
   Parent::Parent::onAdd();

   setControl(dynamic_cast<GuiControl*>(Sim::findObject(mGuiControlName)));
   
   // Scales the distance of the ray used to pick the input target.
   sRayLength = Con::getFloatVariable("$pref::GuiTextureCanvas::RayLength", 2.0f);

   setCursorBitmap(mCursorBitmap);
   return true;
}


void GuiTextureCanvas::onRemove()
{
   Parent::onRemove();

   mGuiControl = NULL;
   mTextureHandle = NULL;
}


void GuiTextureCanvas::inspectPostApply()
{
   Parent::inspectPostApply();

   setControl( dynamic_cast<GuiControl*>( Sim::findObject( mGuiControlName ) ) );
}


void GuiTextureCanvas::setControl( GuiControl* control )
{
   if ( mGuiControl != control ) {
      mGuiControl = control;
   }

   if ( !mGuiControl ) 
   {
      mTextureHandle = NULL;
      return;
   }

   if (  !mTextureHandle || 
         mTextureHandle->getWidth() != mGuiControl->getExtent().x ||
         mTextureHandle->getHeight() != mGuiControl->getExtent().y ) 
   {
      mTextureHandle.set(  mGuiControl->getExtent().x, 
                           mGuiControl->getExtent().y, 
                           GFXFormatR8G8B8A8, 
                           &GFXDefaultRenderTargetProfile, 
                           "" );
   }
}

//---------------------------------------------------------------------------
void GuiTextureCanvas::setCursorBitmap(const char *name)
{
   mCursorBitmap = StringTable->insert(name);

   if (*mCursorBitmap)
      mCursorTextureHandle = GFXTexHandle(mCursorBitmap, &GFXDefaultStaticDiffuseProfile, "Adescription");
   else
      // Reset handles if UI object is hidden
      mCursorTextureHandle = NULL;

   setUpdate();
}

void GuiTextureCanvas::renderFrame( bool preRenderOnly, bool bufferSwap )
{
   if ( !mDirty || !mGuiControl || !mTextureHandle )
      return;

    if(mPlatformWindow)
    {
        mPlatformWindow->hide();
        mPlatformWindow->setCursorVisible(false);
    }

   if ( mGuiControl->getGroup() != this )
      pushDialogControl( mGuiControl );

   iterator i;
   for(i = begin(); i != end(); i++)
   {
      GuiControl *contentCtrl = static_cast<GuiControl*>(*i);
      contentCtrl->preRender();
   }

   if (preRenderOnly)
      return;

    // rendersurface was changed to render target
    
   GFX->pushActiveRenderTarget();
   
   GFXTextureTarget * mRenderTarget;
   // get us some new off-screen rendering target
   mRenderTarget = GFX->allocRenderToTextureTarget();  
   mRenderTarget->attachTexture(GFXTextureTarget::Color0, mTextureHandle);
   GFX->setActiveRenderTarget( mRenderTarget);
   GFX->clear( GFXClearZBuffer | GFXClearStencil | GFXClearTarget, gCanvasClearColor, 1.0f, 0 );
   
   for(i = begin(); i != end(); i++)
   {
      GuiControl *contentCtrl = static_cast<GuiControl*>(*i);
      contentCtrl->onRender(contentCtrl->getPosition(), contentCtrl->getBounds());
   }
   Point2I scale = mGuiControl->getExtent();
   Point2I scaledCurSize = Point2I(scale.x*0.025, scale.y*0.025);
   // render cursor
   if (mShowTexCursor && sInteract)
   {
      GFX->setTexture(0, mCursorTextureHandle);
      PrimBuild::color4i(255, 255, 255, 200);

      PrimBuild::begin(GFXTriangleStrip, 4);
      PrimBuild::texCoord2f(cursorTexCoords[0].x, cursorTexCoords[0].y);
      PrimBuild::vertex2f(gCurPos.x - scaledCurSize.x, gCurPos.y - scaledCurSize.y);
      PrimBuild::texCoord2f(cursorTexCoords[1].x, cursorTexCoords[1].y);
      PrimBuild::vertex2f(gCurPos.x + scaledCurSize.x, gCurPos.y - scaledCurSize.y);
      PrimBuild::texCoord2f(cursorTexCoords[2].x, cursorTexCoords[2].y);
      PrimBuild::vertex2f(gCurPos.x - scaledCurSize.x, gCurPos.y + scaledCurSize.y);
      PrimBuild::texCoord2f(cursorTexCoords[3].x, cursorTexCoords[3].y);
      PrimBuild::vertex2f(gCurPos.x + scaledCurSize.x, gCurPos.y + scaledCurSize.y);
      PrimBuild::end();
   }

   GFX->popActiveRenderTarget();

   mDirty = false;
}

bool GuiTextureCanvas::castRay(TSStatic* object, const Point3F& start, const Point3F& end, RayInfo* rayInfo)
{
   TSShapeInstance* si = object->getShapeInstance();
   
   TSShape* tsShape = si->getShape();
   if (!si || !tsShape || tsShape->details.empty())
      return false;

   Point3F tstart, tend;

   MatrixF mat = object->getWorldTransform();
   mat.mulP(start, &tstart);
   mat.mulP(end, &tend);

   RayInfo info;
   info.generateTexCoord = true;
   rayInfo->distance = F32_MAX;
   if (si->castRayOpcode(0,tstart, tend, &info) && info.distance < rayInfo->distance)
   {
      *rayInfo = info;
      // Ok... we got a hit... cast back into world 
      // space and return it.
      MatrixF invmat(mat);
      invmat.inverse();
      invmat.mulP(rayInfo->point);
      gCurPos = Point2F(info.mUVtexCoord.x, info.mUVtexCoord.y);
      return rayInfo->distance < F32_MAX;
   }
   sInteractingWith = NULL;
   return NULL;
}

void GuiTextureCanvas::updateCanvases()
{
   TextureCanvasVector::iterator i = smTextureCanvases.begin();


   for (; i != smTextureCanvases.end(); i++)
   {
      (*i)->renderFrame(false, false);
      if (sInteract)
      {
         //Point2I scale = (*i)->getExtent();
         //Point2I scaledCurPos = Point2I(scale.x*gCurPos.x, scale.y*gCurPos.y);
         (*i)->mCursorPt = gCurPos;
      }
   }
}

void GuiTextureCanvas::setInteract(bool interact, RayInfo* info)
{
   sInteract = interact;
   if (interact)
   {
      MatInstance* matinst = dynamic_cast<MatInstance*>(info->material);
      if (matinst)
      {
         ProcessedMaterial* mat = matinst->getProcessedMaterial();
         if (mat && mat->guiTexCanvas && mat->guiTexCanvas->mGuiControl)
         {

            const Point2I& extent = mat->guiTexCanvas->mGuiControl->getExtent();
            Point2F scaledCurPos = Point2F(info->mUVtexCoord.x * extent.x, info->mUVtexCoord.y * extent.y);
            sInteractingWith = mat->guiTexCanvas->getId();
            gCurPos = scaledCurPos;
         }
      }
   }
   else
   {
      sInteractingWith = NULL;
      gCurPos = Point2F(0.5, 0.5);
   }

   Con::setBoolVariable("$guiCanvas::Interact", interact);
   Con::setFloatVariable("$guiCanvas::PosX", gCurPos.x);
   Con::setFloatVariable("$guiCanvas::PosY", gCurPos.y);
   Con::setFloatVariable("$guiCanvas::InteractingWith", sInteractingWith);
};

ConsoleMethod( GuiTextureCanvas, doMouseClick, void, 3, 4, "(Point2F pos)")
{
   Point2F pos(0,0);
   if(argc == 4)
      pos.set(dAtof(argv[2]), dAtof(argv[3]));
   else
      dSscanf(argv[2], "%g %g", &pos.x, &pos.y);
    
    object->doMouseClick(Point2I(pos.x, pos.y));

}

void GuiTextureCanvas::doMouseClick(Point2I pt)
{
   GuiEvent event;
   event.mousePoint = Point2I( pt.x, pt.y );
   rootMouseDown( event );
   rootMouseUp( event );
}