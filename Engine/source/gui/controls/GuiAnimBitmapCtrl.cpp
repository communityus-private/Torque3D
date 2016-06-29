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
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"
#include "gfx/gfxDevice.h"
#include "gui/controls/GuiAnimBitmapCtrl.h"
#include "core/stream/fileStream.h"
#include "gfx/gfxDrawUtil.h"

IMPLEMENT_CONOBJECT(GuiAnimBitmapCtrl);

GuiAnimBitmapCtrl::GuiAnimBitmapCtrl(void)
{
   mBitmapName = StringTable->insert("");
   mDmlFilename = StringTable->insert("");

   for (int i = 0; i<50; i++)
   {
      mBitmapNames[i] = StringTable->insert("");
      mTextureHandles[i] = NULL;
   }

   mStartPoint.set(0, 0);
   mWrap = false;
   mIs3d = false;
   mInterval = 0;
   mLastTime = 0;
   mMaxBitmaps = 0;
   mIsPlaying = false;
   mStopAtEnd = false;

   mRotation = 0.0f;
   mDeltaRotation = 0.0f;
   mLastDeltaRotationTime = -1; // set when deltaRotation becomes non zero
   mAlpha = 1.0f;
   mColor = ColorF::WHITE;
}

void GuiAnimBitmapCtrl::setDML(const char *dml)
{
   mMaxBitmaps = 0;
   mDmlFilename = dml;
   if (dStrlen(mDmlFilename))
      loadDml();
}

void GuiAnimBitmapCtrl::initPersistFields()
{
   Parent::initPersistFields();
   addField("is3d", TypeBool, Offset(mIs3d, GuiAnimBitmapCtrl));
   //
   removeField("Bitmap");
   addGroup("Animation");
   addField("Interval", TypeS32, Offset(mInterval, GuiAnimBitmapCtrl));
   addField("DML", TypeFilename, Offset(mDmlFilename, GuiAnimBitmapCtrl));
   addField("autoPlay", TypeBool, Offset(mIsPlaying, GuiAnimBitmapCtrl));
   //
   addField("rotation", TypeF32, Offset(mRotation, GuiAnimBitmapCtrl));
   addField("deltaRotation", TypeF32, Offset(mDeltaRotation, GuiAnimBitmapCtrl));
   addField("alpha", TypeF32, Offset(mAlpha, GuiAnimBitmapCtrl));
   endGroup("Animation");
}

void GuiAnimBitmapCtrl::setIs3d(bool temp)
{
   mIs3d = temp;
}

void GuiAnimBitmapCtrl::SetFPS(F32 temp)
{
   mInterval = temp;
}


void GuiAnimBitmapCtrl::setBitmap(S32 index, const char *name)
{
   if (index < 0 || index >= 50)
      return;

   mBitmapNames[index] = StringTable->insert(name);
   if (*mBitmapNames[index])
   {
      mTextureHandles[index].set(mBitmapNames[index], &GFXDefaultGUIProfile, avar("%s() - mTextureHandles[index] (line %d)", __FUNCTION__, __LINE__));
   }
   else
      mTextureHandles[index] = NULL;
   setUpdate();
}

bool GuiAnimBitmapCtrl::onWake()
{
   if (!Parent::onWake())
      return false;
   setActive(true);

   if (dStrlen(mDmlFilename))
      loadDml();

   mIndex = 0;
   Parent::setBitmap(mTextureHandles[mIndex], false);
   mLastTime = Sim::getCurrentTime();

   return true;
}
void GuiAnimBitmapCtrl::onSleep()
{

   for (S32 i = 0; i < mMaxBitmaps; i++)
   {
      mTextureHandles[i] = NULL;
   }
   mMaxBitmaps = 0;
   Parent::onSleep();
}

bool GuiAnimBitmapCtrl::isImageFileName(const char *filename)
{
   char buffer[1025];
   dStrncpy(buffer, filename, 1024);
   dStrlwr(buffer);
   const char *p = buffer;

   return  dStrstr(p, ".jpg") ||
      dStrstr(p, ".bmp") ||
      dStrstr(p, ".png") ||
      dStrstr(p, ".gif");
}


void GuiAnimBitmapCtrl::loadDml()
{
   mMaxBitmaps = 0;

   FileStream  *stream = FileStream::createAndOpen(mDmlFilename, Torque::FS::File::Read);

   if (stream == NULL)
   {
      return;
   }

   if (isImageFileName(mDmlFilename))
   {
      mMaxBitmaps = 1;
      setBitmap(0, mDmlFilename);
   }
   else
   {
      mMaterialList.read(*stream);
      stream->close();

      delete stream;

      const Torque::Path  thePath(mDmlFilename);

      if (!mMaterialList.load(thePath.getPath()))
      {
         return;
      }

      S32 x;

      for (x = 0; x < mMaterialList.size(); ++x, ++mMaxBitmaps)
      {
         if (x >= 50) break;
         mTextureHandles[x] = mMaterialList.getDiffuseTexture(x);
      }
   }
}

void GuiAnimBitmapCtrl::onPreRender()
{
   S32 thisTime = Sim::getCurrentTime();
   S32 timeDelta = thisTime - mLastTime;
   if (timeDelta > mInterval)
   {
      setUpdate();
   }
}

void GuiAnimBitmapCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   S32 thisTime = Sim::getCurrentTime();
   S32 timeDelta = thisTime - mLastTime;
   if (timeDelta > mInterval && mIsPlaying)
   {
      mIndex++;
      if (mIndex >= mMaxBitmaps)
      {
         if (mStopAtEnd)
         {
            mIndex--;  // stay at end and stop playing...
            stop();
            Con::executef(this, "onStoppedAtEnd");
         }
         else
         {
            mIndex = 0;
         }
      }
      mLastTime = thisTime;
   }
   if (mTextureHandles[mIndex])
      Parent::setBitmap(mTextureHandles[mIndex]);

   S32 a = (S32)(255 * mAlpha);
   ColorI alphaColor(mColor.red, mColor.green, mColor.blue, a);

   // update rotation?
   if (mDeltaRotation != 0)
   {
      // how much time?
      S32 now = Sim::getCurrentTime();
      F32 factor = 0.0f;
      if (mLastDeltaRotationTime >= 0)
         factor = F32(now - mLastDeltaRotationTime) / 1000.0f;
      mLastDeltaRotationTime = now;

      mRotation += mDeltaRotation * factor;
   }

   if (mTextureHandles[mIndex])
   {
      GFX->getDrawUtil()->clearBitmapModulation();
      GFX->getDrawUtil()->setBitmapModulation(alphaColor);

      if (mWrap)
      {
         GFXTextureObject* texture = mTextureHandles[mIndex];
         RectI srcRegion;
         RectI dstRegion;
         float xdone = ((float)getExtent().x / (float)texture->mBitmapSize.x) + 1;
         float ydone = ((float)getExtent().y / (float)texture->mBitmapSize.y) + 1;

         int xshift = mStartPoint.x%texture->mBitmapSize.x;
         int yshift = mStartPoint.y%texture->mBitmapSize.y;
         for (int y = 0; y < ydone; ++y)
            for (int x = 0; x < xdone; ++x)
            {
               srcRegion.set(0, 0, texture->mBitmapSize.x, texture->mBitmapSize.y);
               dstRegion.set(((texture->mBitmapSize.x*x) + offset.x) - xshift,
                  ((texture->mBitmapSize.y*y) + offset.y) - yshift,
                  texture->mBitmapSize.x,
                  texture->mBitmapSize.y);
               GFX->getDrawUtil()->drawBitmapStretchSR(texture, dstRegion, srcRegion);
            }

      }
      else
      {
         RectI rect(offset, getExtent());
         GFXTextureObject* texture = mTextureHandles[mIndex];
         RectI subRegion(0, 0, texture->mBitmapSize.x, texture->mBitmapSize.y);
         GFX->getDrawUtil()->drawBitmapStretchSR(texture, rect, subRegion);
      }
   }
   else
   {
      // if we have NO bitmap..then just draw a white squre in the current color...
      RectI rect(offset.x, offset.y, getExtent().x, getExtent().y);
      GFX->getDrawUtil()->drawRectFill(rect, mColor);
   }

   // if we are "playing" then rendering causes the next rendering...
   if (mIsPlaying)
      setUpdate();

   //render childern
   renderChildControls(offset, updateRect);
}


void GuiAnimBitmapCtrl::play(bool stopAtEnd)
{
   mIsPlaying = true;
   mStopAtEnd = stopAtEnd;
   mLastTime = Platform::getVirtualMilliseconds();
   setUpdate();
}


S32 GuiAnimBitmapCtrl::setFrame(S32 index)
{
   S32 ret = mIndex;
   if (index >= 0 && index < mMaxBitmaps)
   {
      mIndex = index;
      setUpdate();
   }

   return ret;
}

void GuiAnimBitmapCtrl::stop()
{
   mIsPlaying = false;
}


void GuiAnimBitmapCtrl::rewind()
{
   mIndex = 0;
   if (mIsPlaying)
      mLastTime = Platform::getVirtualMilliseconds();
   setUpdate();
}

void GuiAnimBitmapCtrl::onMouseUp(const GuiEvent &event)
{
   //if this control is a dead end, make sure the event stops here
   if (!mVisible || !mAwake || mAlpha <= 0.0)
      return;

   UTF8 *pointStr = Con::getArgBuffer(128);
   dSprintf(pointStr, 128, "%d %d", event.mousePoint.x, event.mousePoint.y);

   Con::executef(this, "onMouseUp", pointStr);
}

void GuiAnimBitmapCtrl::onMouseDown(const GuiEvent &event)
{
   //if this control is a dead end, make sure the event stops here
   if (!mVisible || !mAwake || mAlpha <= 0.0)
      return;

   UTF8 *pointStr = Con::getArgBuffer(128);
   dSprintf(pointStr, 128, "%d %d", event.mousePoint.x, event.mousePoint.y);

   Con::executef(this, "onMouseDown", pointStr);
}

DefineEngineMethod(GuiAnimBitmapCtrl, setIs3d, void, (bool _Is3d), (false),
   "GuiAnimBitmapCtrl.setIs3d(bool Is3d)")
{
   object->setIs3d(_Is3d);
}

DefineEngineMethod(GuiAnimBitmapCtrl, SetFPS, void, (F32 _fps), ,
   "GuiAnimBitmapCtrl.setIs3d(F32 fps)")
{
   object->SetFPS(_fps);
}

DefineEngineMethod(GuiAnimBitmapCtrl, play, void, (bool _stopAtEnd), (false),
   "GuiAnimBitmapCtrl.play(bool stopAtEnd)")
{
   object->play(_stopAtEnd);
}

DefineEngineMethod(GuiAnimBitmapCtrl, stop, void, (), ,
   "GuiAnimBitmapCtrl.stop()")
{
   object->stop();
}

DefineEngineMethod(GuiAnimBitmapCtrl, rewind, void, (), ,
   "GuiAnimBitmapCtrl.rewind()")
{
   object->rewind();
}

DefineEngineMethod(GuiAnimBitmapCtrl, isPlaying, bool, (), ,
   "bool GuiAnimBitmapCtrl.isPlaying()")
{
   return object->isPlaying();
}

DefineEngineMethod(GuiAnimBitmapCtrl, getFrame, S32, (), ,
   "S32 GuiAnimBitmapCtrl.getFrame()")
{
   return object->getFrame();
}

DefineEngineMethod(GuiAnimBitmapCtrl, getTotalFrames, S32, (), ,
   "S32 GuiAnimBitmapCtrl.getTotalFrames()")
{
   return object->getTotalFrames();
}

DefineEngineMethod(GuiAnimBitmapCtrl, setFrame, S32, (S32 _frame), ,
   "S32 GuiAnimBitmapCtrl.setFrame(S32 frame)")
{
   return object->setFrame(_frame);
}

DefineEngineMethod(GuiAnimBitmapCtrl, setDML, void, (const char * _filename), (""),
   "S32 GuiAnimBitmapCtrl.setDML(S32 frame)")
{
   object->setDML(_filename);
}
