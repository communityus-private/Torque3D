//-----------------------------------------------------------------------------
// Copyright (C) 2006 Sickhead Games, LLC.
// Portions Copyright (C) 2003 Pat Wilson and Flaming Duck Studio
// Portions Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUITEXTURECANVAS_H_
#define _GUITEXTURECANVAS_H_

#include "gfx/gfxTextureHandle.h"
#include "gui/core/guiCanvas.h"
//#include "core/tVector.h"
#include "scene/sceneObject.h"

#include "T3D/tsStatic.h"

class ShapeBase;

class GuiTextureCanvas : public GuiCanvas
{
   typedef GuiCanvas Parent;

private:

   GFXTexHandle mTextureHandle;

   Vector<StringTableEntry> mObjects;

   const char* mGuiControlName;
   GuiControl* mGuiControl;

   bool mDirty;

   bool mShowTexCursor;
   StringTableEntry mCursorBitmap;
   GFXTexHandle mCursorTextureHandle;

   typedef Vector<GuiTextureCanvas*> TextureCanvasVector;
   static TextureCanvasVector smTextureCanvases;
   void init();
   void unregister();

   static F32 sRayLength;
   static bool sInteract;
   static U32 sInteractingWith;

public:

   DECLARE_CONOBJECT(GuiTextureCanvas);

   GuiTextureCanvas();
   ~GuiTextureCanvas();
   static void initPersistFields();

   static void updateCanvases();

   virtual bool onAdd();
   virtual void onRemove();
   virtual void inspectPostApply();

   virtual void renderFrame( bool preRenderOnly, bool bufferSwap = true );

   static bool castRay(TSStatic* object, const Point3F& start, const Point3F& end, RayInfo* rayInfo);
   static F32 getRayLength() { return sRayLength; }
   static bool getInteract(){ return sInteract; };
   static void setInteract(bool interact, RayInfo* info);

   void setControl( GuiControl* control );
   GuiControl* getControl(){ return mGuiControl; };
   void setDirty();
   void setCursorBitmap(const char *name);
   GFXTexHandle getTextureHandle() const;

    void doMouseClick(Point2I pt);
};


inline GFXTexHandle GuiTextureCanvas::getTextureHandle() const
{
   return mTextureHandle;
}

inline void GuiTextureCanvas::setDirty()
{
	mDirty = true;
}


#endif // _GUITEXTURECANVAS_H_
