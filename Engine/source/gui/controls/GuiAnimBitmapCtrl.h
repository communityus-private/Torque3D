//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// Original GuiAnimatedBitmapCtrl Made by Robert Brower
// Modified by Jeff Bakke for C3 command www.c3command.com
// Modified by Paul Dana for Flash Bios
// Modified again by Paul Dana for Plastic Gems for Torque 1.5 + codebases
// Modified by Jesse Lord for compatibilty to version Torque Game Engine Advance 1.8.1
//-----------------------------------------------------------------------------
#ifndef _GuiAnimBitmapCtrl_H_
#define _GuiAnimBitmapCtrl_H_

#ifndef _GUIBITMAPCTRL_H_
#include "gui/controls/guiBitmapCtrl.h"
#endif

#ifndef _MATERIALLIST_H_
#include "materials/materialList.h"
#endif

class GuiAnimBitmapCtrl : public GuiBitmapCtrl
{
public:
   enum
   {
      MaxFrames = 200,
   };

private:
   typedef GuiBitmapCtrl Parent;

protected:
   bool mIs3d;
   S32      mInterval;
   S32      mLastTime;
   S32      mIndex;
   S32      mMaxBitmaps;
   F32      mAlpha;
   ColorF   mColor;
   MaterialList   mMaterialList;
   StringTableEntry mDmlFilename;
   StringTableEntry mBitmapNames[MaxFrames];
   GFXTexHandle mTextureHandles[MaxFrames];
   bool mIsPlaying;      
   bool mStopAtEnd;

   F32 mRotation; // degrees counterclockwise from nomrmal
   F32 mDeltaRotation; // if non zero automatically update rotation by this degreee/sec
   S32 mLastDeltaRotationTime; // if non negative then time of last delta rotation

public:
   //creation methods
   GuiAnimBitmapCtrl();
   static void initPersistFields();

   //Parental methods
   bool onWake();
   void onSleep();
   void loadDml();
   bool isImageFileName(const char *filename);
   void setDML(const char *dmlFileName);
   void setBitmap(S32 index, const char *name);
   void setIs3d(bool temp);
   void SetFPS(F32 temp);
   void onPreRender();
   void onRender(Point2I offset, const RectI &updateRect); 

   virtual void onMouseUp(const GuiEvent &event);
   virtual void onMouseDown(const GuiEvent &event);

	void play(bool stopAtEnd);
	void stop();
	void rewind();
	bool isPlaying() {return mIsPlaying;}
	S32  getFrame() {return mIndex;}
	S32 setFrame(S32 index);
	S32  getTotalFrames() {return mMaxBitmaps;}

   DECLARE_CONOBJECT(GuiAnimBitmapCtrl);
   DECLARE_CATEGORY("Gui Images");

};

#endif 