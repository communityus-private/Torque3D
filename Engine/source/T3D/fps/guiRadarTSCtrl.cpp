//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
// Class: GuiRadarTSCtrl for TGEA
// Tested with TGEA 1.0.3
// A 3d Spherical Radar system, ideal for space and flight games
// If you've played Elite, you know the score :)
// Uses some ideas from the GuiRadarCtrl by Matt "CaptFallout" Webster
// Guy Allard 2008
//-----------------------------------------------------------------------------
// Upgraded and cleaned by James Laker
//-----------------------------------------------------------------------------
// Updated for T3D 1.1 Beta 2 and Added Blip for Object Type by Saitta Alfio
//-----------------------------------------------------------------------------
#include "platform/platform.h"

#include "gui/core/guiControl.h"
#include "gfx/gfxDrawUtil.h"
#include "T3D/player.h"

#include "console/console.h"

#include "gfx/gfxDevice.h"
#include "app/game.h"

#include "T3D/ShapeBase.h"
#include "guiRadarTSCtrl.h"
#include "console/consoleTypes.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/shapeBase.h"
#include "gfx/primBuilder.h"
#include "math/util/sphereMesh.h"
IMPLEMENT_CONOBJECT(GuiRadarTSCtrl);

// vertices for the quad used for rendering the elliptic plane
static const Point3F planeVerts[4]=
{
	Point3F(-1, 1, 0),
	Point3F(1, 1, 0),
	Point3F(-1, -1, 0),
	Point3F(1, -1, 0)
};

// and these are for the blip billboards
static const Point3F quadVerts[4]=
{
	Point3F(-1, 0, 1),
	Point3F(1, 0, 1),
	Point3F(-1, 0, -1),
	Point3F(1, 0, -1)
};

// texture coords for the quads (when drawn as a trianglestrip)
static const Point2F quadTexCoords[4] =
{
	Point2F(0,0),
	Point2F(1,0),
	Point2F(0,1),
	Point2F(1,1)
};

// sphere
static SphereMesh sphere(SphereMesh::Icosahedron);

//--------------------------------------------------------------------------
GuiRadarTSCtrl::GuiRadarTSCtrl()
{

	mSphereDetail = 2;

	// textures
	mEllipticBitmap = StringTable->insert("art/gui/playgui/radar3D.png");
	mEllipticTextureHandle = NULL;

	mPlayerBitmap = StringTable->insert("art/gui/playgui/player.png");
	mPlayerTextureHandle = NULL;

	mBotBitmap = StringTable->insert("art/gui/playgui/bot.png");
	mPlayerTextureHandle = NULL;

	mVehicleBitmap = StringTable->insert("art/gui/playgui/vehicle.png");
	mPlayerTextureHandle = NULL;

	mUpOverlayBitmap = StringTable->insert("art/gui/playgui/vehicle.png");
	mUpOverlayTextureHandle = NULL;
	
	mDownOverlayBitmap = StringTable->insert("art/gui/playgui/vehicle.png");
	mDownOverlayTextureHandle = NULL;

	strcpy(mTargetShapeName,"");

	// colors
	mEllipticColor.set(0.0, 0.75, 0.0, 0.75);
	mSphereColor.set(0.0f, 0.5f, 0.0f, 0.25f);
	mBlipColor.set(1.0f, 1.0f, 1.0f, 1.0f);
	mStemColor.set(1.0f, 1.0f, 1.0f, 1.0f);

	// range
	mRange = 250.0f;
	mOverlayGive = 0.25;

	// blip size
	mBlipSize = 1.0f;

	// As default show only players
	mShowVehicles = true;
	mShowPlayers = true;
	mShowBots = true;
    flashPace = nextFlash = 100;
    flipColors = false;
}

//---------------------------------------------------------------------------
void GuiRadarTSCtrl::initPersistFields()
{
	Parent::initPersistFields();

	addGroup("Radar");
	addField("PlaneBitmap",		TypeFilename,	Offset(mEllipticBitmap,		GuiRadarTSCtrl));
	addField("PlaneColor",		TypeColorF,		Offset(mEllipticColor,		GuiRadarTSCtrl));
	addField("BlipColor",		TypeColorF,		Offset(mBlipColor,			GuiRadarTSCtrl));
	addField("BlipSize",		TypeF32,		Offset(mBlipSize,			GuiRadarTSCtrl));
	addField("StemColor",		TypeColorF,		Offset(mStemColor,			GuiRadarTSCtrl));
	addField("SphereColor",		TypeColorF,		Offset(mSphereColor,		GuiRadarTSCtrl));
	addField("SphereDetail",	TypeS32,		Offset(mSphereDetail,		GuiRadarTSCtrl));
	addField("Range",			TypeF32,		Offset(mRange,				GuiRadarTSCtrl));
	addField("OverlayGive",		TypeF32,		Offset(mOverlayGive,		GuiRadarTSCtrl));
	addField("FlashPace",		TypeS8,			Offset(flashPace,			GuiRadarTSCtrl));
	endGroup("Radar");

	addGroup("Bitmaps");
	addField("Players",		TypeFilename,	Offset(mPlayerBitmap,		GuiRadarTSCtrl));
	addField("Bots",		TypeFilename,	Offset(mBotBitmap,			GuiRadarTSCtrl));
	addField("Vehicles",	TypeFilename,	Offset(mVehicleBitmap,		GuiRadarTSCtrl));
	addField("UpOverlay",	TypeFilename,	Offset(mUpOverlayBitmap,	GuiRadarTSCtrl));
	addField("DownOverlay",	TypeFilename,	Offset(mDownOverlayBitmap,	GuiRadarTSCtrl));
	endGroup("Bitmaps");

	addGroup("ShowObjects");
	addField("Players",		TypeBool,	Offset(mShowPlayers,     GuiRadarTSCtrl),"Show Players on Radar");
	addField("Vehicles",	TypeBool,	Offset(mShowVehicles,    GuiRadarTSCtrl),"Show Vehicles on Radar");
	addField("Bots",		TypeBool,	Offset(mShowBots,        GuiRadarTSCtrl),"Show Bots on Radar");
	endGroup("ShowObject");
}

void GuiRadarTSCtrl::setTarget(const char *TargetShapeBaseName)
{
	if (*TargetShapeBaseName)
		strcpy(mTargetShapeName,TargetShapeBaseName);	
}

//---------------------------------------------------------------------------
void GuiRadarTSCtrl::setEllipticBitmap(const char *name)
{
	mEllipticBitmap = StringTable->insert(name);

	if (*mEllipticBitmap) 
		mEllipticTextureHandle = GFXTexHandle(mEllipticBitmap, &GFXDefaultStaticDiffuseProfile, "Adescription");
	//mEllipticTextureHandle = GFXTexHandle(mEllipticBitmap, &GFXDefaultStaticDiffuseProfile);
	else 
		// Reset handles if UI object is hidden
		mEllipticTextureHandle = NULL;

	setUpdate();
}


//---------------------------------------------------------------------------
bool GuiRadarTSCtrl::onWake()
{
	if (! Parent::onWake())
		return false;
	setActive(true);

	setEllipticBitmap(mEllipticBitmap);

	setPlayerBitmap(mPlayerBitmap);
	setBotBitmap(mBotBitmap);
	setVehicleBitmap(mVehicleBitmap);
	setUpOverlayBitmap(mUpOverlayBitmap);
	setDownOverlayBitmap(mDownOverlayBitmap);

	return true;
}

//--------------------------------------------------------------------------
void GuiRadarTSCtrl::onSleep()
{
	mEllipticTextureHandle = NULL;
	mPlayerTextureHandle = NULL;
	mBotTextureHandle = NULL;
	mVehicleTextureHandle = NULL;
	mUpOverlayTextureHandle = NULL;
	mDownOverlayTextureHandle = NULL;

	Parent::onSleep();
}

//---------------------------------------------------------------------------
// set up the camera position for rendering the 3D scene within this control
bool GuiRadarTSCtrl::processCameraQuery(CameraQuery *camq)
{
	// pretty hacky hardcoded values
	// gives a reasonable viewing angle
	camq->nearPlane = 0.1f; // near clip plane
	camq->farPlane = 20.0f; // far clip plane
	camq->fov = (F32)M_PI/6;     // field of view
	MatrixF cam;
	cam.set(EulerF((F32)M_PI/8, 0, 0)); // rotation
	cam.setColumn(3, Point3F(0, -4, 1.65f)); // position
	camq->cameraMatrix = cam;
	return (true);
}

//---------------------------------------------------------------------------
// render the world that this control sees
void GuiRadarTSCtrl::renderWorld(const RectI &updateRect)
{
	// Must have a connection
	GameConnection* conn = GameConnection::getConnectionToServer();
	if (!conn) return;

	// Must have controlled object
	GameBase * control = dynamic_cast<GameBase*>(conn->getControlObject());
	if (!control) return;

	// first draw the elliptic plane if we need to
	renderRadarGround(updateRect);
	// then the sphere
	renderRadarSphere(updateRect);
	//then, the blips
	renderRadarBlip(updateRect,conn,control);
	//finally, the directional markers
	renderRadarOverlay(updateRect,conn,control);
	// all done
	GFX->setClipRect(updateRect);
}

void GuiRadarTSCtrl::renderRadarGround(const RectI &updateRect)
{
	GFXStateBlockDesc desc;
	desc.zEnable = false;
	desc.ffLighting = false;  
	desc.setCullMode( GFXCullNone );  
	desc.setBlend(true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha);  
	desc.samplers[0].textureColorOp = GFXTOPModulate;
   desc.samplers[1].textureColorOp = GFXTOPDisable;
   desc.samplersDefined = true;
	GFXStateBlockRef elipseState = GFX->createStateBlock(desc);  
   GFX->setStateBlock(elipseState);

	// first draw the elliptic plane if we need to
	if(mEllipticTextureHandle)
	{
		GFXTexHandle texture = mEllipticTextureHandle;
		GFX->setTexture(0, texture);

		PrimBuild::color(mEllipticColor);
		PrimBuild::begin(GFXTriangleStrip, 4);
		for(int i=0; i<4; i++)
		{
			PrimBuild::texCoord2f(quadTexCoords[i].x, quadTexCoords[i].y);
			PrimBuild::vertex3fv(planeVerts[i]);
		}
		PrimBuild::end();
	}

}

void GuiRadarTSCtrl::renderRadarSphere(const RectI &updateRect)
{
	if (mSphereDetail > 4)
		mSphereDetail = 4;

	const SphereMesh::TriangleMesh *sphereMesh = sphere.getMesh(mSphereDetail); // sphere.getMesh(2) gives a slighty 'chunky' sphere - go higher if you can handle the framerate hit
	S32 numPoly = sphereMesh->numPoly;
	S32 totalPoly = 0;

	GFXVertexBufferHandle<GFXVertexPC> verts(GFX, numPoly*3, GFXBufferTypeVolatile);
	verts.lock();
	S32 vertexIndex = 0;
	for (S32 i=0; i<numPoly; i++)
	{
		totalPoly++;

		verts[vertexIndex].point = sphereMesh->poly[i].pnt[0];
		verts[vertexIndex].color = mSphereColor;
		vertexIndex++;

		verts[vertexIndex].point = sphereMesh->poly[i].pnt[1];
		verts[vertexIndex].color = mSphereColor;
		vertexIndex++;

		verts[vertexIndex].point = sphereMesh->poly[i].pnt[2];
		verts[vertexIndex].color = mSphereColor;
		vertexIndex++;
	}
	verts.unlock();

	GFXStateBlockDesc sphereDesc;
	sphereDesc.setCullMode(GFXCullNone);
	sphereDesc.zEnable = false;
	sphereDesc.setBlend(true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha);
   GFX->setStateBlockByDesc(sphereDesc);
   GFX->setupGenericShaders(GFXDevice::GSColor);

	GFX->setVertexBuffer( verts );
	GFX->drawPrimitive( GFXTriangleList, 0, totalPoly );
}

void GuiRadarTSCtrl::renderRadarBlip(const RectI &updateRect, GameConnection* conn, GameBase * control)
{
   // get the camera transform for the connection
   MatrixF camMat;
   camMat = control->getTransform();
   //conn->getControlCameraTransform(0,&camMat);
   // get the position for use later
   Point3F camPos = camMat.getPosition();

   // invert the camera transform - 
   // this will then allow us to transform other objects into our own object space
   camMat.inverse();

   GFXStateBlockDesc desc;
   desc.zEnable = false;
   desc.ffLighting = false;
   desc.setCullMode(GFXCullNone);
   desc.setBlend(true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha);
   desc.samplers[0].textureColorOp = GFXTOPModulate;
   desc.samplers[1].textureColorOp = GFXTOPDisable;
   desc.samplersDefined = true;
   desc.vertexColorEnable = true;
   GFXStateBlockRef blipState = GFX->createStateBlock(desc);
   GFX->setStateBlock(blipState);

   // Now do the radar signatures
   // firstly, we will be drawing billboards, so get the current world transform matrix
   MatrixF worldMat = GFX->getWorldMatrix();
   // extract the up and right vectors
   Point3F up;
   Point3F right;
   worldMat.getRow(0, &right);
   worldMat.getRow(2, &up);
   right.normalize();
   up.normalize();
   // then build the coordinates that we'll use for the billboards
   // oops, another hard coded value, ah well...
   right *= mBlipSize * 0.05f;
   up *= mBlipSize * 0.1f;

   // set up the texture that we'll use for the blips
   // is this the correct way to do it???
   GFX->getDrawUtil()->clearBitmapModulation();
   GFXTexHandle mRadarPlayer = mPlayerTextureHandle;
   GFXTexHandle mRadarBot = mBotTextureHandle;
   GFXTexHandle mRadarVehicle = mVehicleTextureHandle;

   // Go through all ghosted objects on connection (client-side)
   for (SimSetIterator itr(conn); *itr; ++itr)
   {
      //if (SO->getTypeMask() && ShapeBaseObjectType)
      //{
      //ShapeBase* shape = static_cast<ShapeBase*>(*itr);
      ShapeBase* shape = dynamic_cast<ShapeBase*>(*itr);
      if (shape)
      {
         // Make sure that the object isn't the client
         if (shape != control)
         {
            // Make sure the shapebase object is a player (or Vehicle)
            if (shape->getTypeMask() & (PlayerObjectType | AIObjectType | VehicleObjectType))
            {
               // get position of object
               Point3F objPos = shape->getPosition();

               // transform it into the coordinate space of the viewer
               camMat.mulP(objPos);
               // scale it according to the current radar range
               objPos /= mRange;

               // don't draw if outside of current radar range
               if (objPos.len() > 1)
               {
                  if (shape->getTypeMask() & (VehicleObjectType)) continue;
                  objPos *= 1 / objPos.len();
               }

               // compress the z transform a bit, just for looks
               //objPos.z *= 0.5f;

               ShapeBaseData* DB = dynamic_cast<ShapeBaseData*>(shape->getDataBlock());
               if (DB)
               {
                  if (DB->mHideIcon) continue;
                  if ((DB->mIcon) && (DB->mIcon[0]) && (DB->mIconHandle))
                  {
                     GFXTexHandle mRadarObject = (GFXTexHandle)(DB->mIconHandle);
                     if (mRadarObject) GFX->setTexture(0, mRadarObject);
                  }
                  else
                  {
                     if (shape->getTypeMask()&(PlayerObjectType)) GFX->setTexture(0, mRadarPlayer);
                     if (shape->getTypeMask()&(VehicleObjectType)) GFX->setTexture(0, mRadarVehicle);
                     if (shape->getTypeMask()&(AIObjectType)) GFX->setTexture(0, mRadarBot);
                  }
                  // draw a line from the blip to the elliptic - will have to use quads if you want a line thicker than 1px
                  // end point of line
                  Point3F endPos = objPos;
                  endPos.z = 0;

                  nextFlash--;
                  if (nextFlash < 0)
                  {
                     nextFlash = flashPace;
                     flipColors = !flipColors;
                  }

                  // draw the blip
                  // unfortunately, DX doesn't appear to allow specification of point size or line width like opengl
                  // SO, lets build a quad using the view aligned coordinates we generated earlier....
                  GFX->setStateBlockByDesc(desc);

                  if ((flipColors) && (shape->getDamageValue()>0.05f))
                  {
                     ColorF tBlipColor;
                     tBlipColor.set(1.0f, 0.0f, 0.0f, 1.0f);
                     PrimBuild::color(tBlipColor);
                  }
                  else PrimBuild::color(mBlipColor);

                  PrimBuild::begin(GFXTriangleStrip, 4);
                  PrimBuild::texCoord2f(quadTexCoords[0].x, quadTexCoords[0].y);
                  PrimBuild::vertex3fv(objPos - right + up);
                  PrimBuild::texCoord2f(quadTexCoords[1].x, quadTexCoords[1].y);
                  PrimBuild::vertex3fv(objPos + right + up);
                  PrimBuild::texCoord2f(quadTexCoords[2].x, quadTexCoords[2].y);
                  PrimBuild::vertex3fv(objPos - right);
                  PrimBuild::texCoord2f(quadTexCoords[3].x, quadTexCoords[3].y);
                  PrimBuild::vertex3fv(objPos + right);
                  PrimBuild::end();
               }
            }
         }
      }
   }
}

void GuiRadarTSCtrl::renderRadarOverlay(const RectI &updateRect, GameConnection* conn, GameBase * control)
{
	// get the camera transform for the connection
    MatrixF camMat;
    camMat = control->getTransform();
	//conn->getControlCameraTransform(0,&camMat);
	// get the position for use later
	Point3F camPos = camMat.getPosition();

	// invert the camera transform - 
	// this will then allow us to transform other objects into our own object space
	camMat.inverse();

	GFXStateBlockDesc desc;
	desc.zEnable = false;
	desc.ffLighting = false;  
	desc.setCullMode( GFXCullNone );  
	desc.setBlend(true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha);  
	desc.samplers[0].textureColorOp = GFXTOPModulate;
	desc.samplers[1].textureColorOp = GFXTOPDisable;
   desc.samplersDefined = true;
   GFXStateBlockRef blipState = GFX->createStateBlock(desc);
   GFX->setStateBlock(blipState);

	// Now do the radar signatures
	// firstly, we will be drawing billboards, so get the current world transform matrix
	MatrixF worldMat = GFX->getWorldMatrix();
	// extract the up and right vectors
	Point3F up;
	Point3F right;
	worldMat.getRow(0,&right);
	worldMat.getRow(2,&up);
	right.normalize();
	up.normalize();
	// then build the coordinates that we'll use for the billboards
	// oops, another hard coded value, ah well...
	right *= mBlipSize * 0.05f;
	up    *= mBlipSize * 0.1f; 

	// set up the texture that we'll use for the blips
	// is this the correct way to do it???
	GFX->getDrawUtil()->clearBitmapModulation();

	// Go through all ghosted objects on connection (client-side)
	for (SimSetIterator itr(conn); *itr; ++itr) 
	{
		//if (SO->getTypeMask() && ShapeBaseObjectType)
		//{
			//ShapeBase* shape = static_cast<ShapeBase*>(*itr);
        ShapeBase* shape = dynamic_cast<ShapeBase*>(*itr);
        if(shape)
        {
			// Make sure that the object isn't the client
			if ( shape != control )
			{
				// Make sure the shapebase object is a player (or Vehicle)
				if (shape->getTypeMask() & ( PlayerObjectType | AIObjectType | VehicleObjectType ) ) 
				{
				   // get position of object
				   Point3F objPos = shape->getPosition();
				                     
				   // transform it into the coordinate space of the viewer
				   camMat.mulP(objPos);
				   // scale it according to the current radar range
				   objPos /= mRange;

				   // don't draw if outside of current radar range
				   if (objPos.len() > 1)
				   {
						if (shape->getTypeMask() & ( VehicleObjectType )) continue;
						objPos *= 1/objPos.len();
				   }

					// compress the z transform a bit, just for looks
					//objPos.z *= 0.5f;

               ShapeBaseData* DB = dynamic_cast<ShapeBaseData*>(shape->getDataBlock());
               if (DB)
               {
                  if (DB->mHideIcon) continue;
                  if (mFabs(objPos.z)<mOverlayGive) continue;

                  bool drawUp = false;
                  if (objPos.z>0) drawUp = true;

                  GFXTexHandle overlay;
                  if (drawUp) overlay = mUpOverlayTextureHandle;
                  else overlay = mDownOverlayTextureHandle;

                  if (overlay)
                  {
                     GFX->setTexture(0, overlay);

                     // draw the blip
                     // unfortunately, DX doesn't appear to allow specification of point size or line width like opengl
                     // SO, lets build a quad using the view aligned coordinates we generated earlier....
                     GFX->setStateBlockByDesc(desc);
                     PrimBuild::color(mBlipColor);
                     PrimBuild::begin(GFXTriangleStrip, 4);
                     PrimBuild::texCoord2f(quadTexCoords[0].x, quadTexCoords[0].y);
                     PrimBuild::vertex3fv(objPos - right + up);
                     PrimBuild::texCoord2f(quadTexCoords[1].x, quadTexCoords[1].y);
                     PrimBuild::vertex3fv(objPos + right + up);
                     PrimBuild::texCoord2f(quadTexCoords[2].x, quadTexCoords[2].y);
                     PrimBuild::vertex3fv(objPos - right);
                     PrimBuild::texCoord2f(quadTexCoords[3].x, quadTexCoords[3].y);
                     PrimBuild::vertex3fv(objPos + right);
                     PrimBuild::end();
                  }
                  else
                  {
                     // draw a line from the blip to the elliptic - will have to use quads if you want a line thicker than 1px
                     // end point of line
                     Point3F endPos = objPos;
                     endPos.z = 0;
                     GFXStateBlockDesc desc3;
                     desc3.addDesc(desc);
                     GFXStateBlockRef lineState = GFX->createStateBlock(desc3);
                     GFX->setStateBlock(lineState);
                     GFX->setupGenericShaders(GFXDevice::GSColor);
                     // draw the line
                     PrimBuild::begin(GFXLineList, 2);
                     if (drawUp) PrimBuild::color(mStemColor);
                     else PrimBuild::color(mStemColor / 2);
                     PrimBuild::vertex3fv(objPos);
                     PrimBuild::vertex3fv(endPos);
                     PrimBuild::end();
                  }
               }
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
void GuiRadarTSCtrl::setPlayerBitmap(const char *name)
{
	mPlayerBitmap = StringTable->insert(name);

	if (*mPlayerBitmap) 
		mPlayerTextureHandle = GFXTexHandle(mPlayerBitmap, &GFXDefaultStaticDiffuseProfile, "Adescription");
	else 
		// Reset handles if UI object is hidden
		mPlayerTextureHandle = NULL;

	setUpdate();
}

//---------------------------------------------------------------------------
void GuiRadarTSCtrl::setBotBitmap(const char *name)
{
	mBotBitmap = StringTable->insert(name);

	if (*mBotBitmap) 
		mBotTextureHandle = GFXTexHandle(mBotBitmap, &GFXDefaultStaticDiffuseProfile, "Adescription");
	else 
		// Reset handles if UI object is hidden
		mBotTextureHandle = NULL;

	setUpdate();
}

//---------------------------------------------------------------------------
void GuiRadarTSCtrl::setVehicleBitmap(const char *name)
{
	mVehicleBitmap = StringTable->insert(name);

	if (*mVehicleBitmap) 
		mVehicleTextureHandle = GFXTexHandle(mVehicleBitmap, &GFXDefaultStaticDiffuseProfile, "Adescription");
	else 
		// Reset handles if UI object is hidden
		mVehicleTextureHandle = NULL;

	setUpdate();
}

//---------------------------------------------------------------------------
void GuiRadarTSCtrl::setUpOverlayBitmap(const char *name)
{
	mUpOverlayBitmap = StringTable->insert(name);

	if (*mUpOverlayBitmap) 
		mUpOverlayTextureHandle = GFXTexHandle(mUpOverlayBitmap, &GFXDefaultStaticDiffuseProfile, "Adescription");
	else 
		// Reset handles if UI object is hidden
		mUpOverlayTextureHandle = NULL;

	setUpdate();
}

//---------------------------------------------------------------------------
void GuiRadarTSCtrl::setDownOverlayBitmap(const char *name)
{
	mDownOverlayBitmap = StringTable->insert(name);

	if (*mDownOverlayBitmap) 
		mDownOverlayTextureHandle = GFXTexHandle(mDownOverlayBitmap, &GFXDefaultStaticDiffuseProfile, "Adescription");
	else 
		// Reset handles if UI object is hidden
		mDownOverlayTextureHandle = NULL;

	setUpdate();
}
//------------------------------------------------------------------------
// script interface
ConsoleMethod( GuiRadarTSCtrl, setEllipticBitmap, void, 3, 3, "(string filename)"
			  "Set the bitmap for the horizontal plane of the control.")
{
	object->setEllipticBitmap(argv[2]);
}

ConsoleMethod( GuiRadarTSCtrl, SetRange, void, 3, 3, "(float)"
			  "Sets the Range of the the control. Default is 250")
{
	object->setRange(dAtof(argv[2]));
}

ConsoleMethod( GuiRadarTSCtrl, getRange, F32, 2, 2, "()"
			  "gets the current range of the control")
{
	return object->getRange();
}

ConsoleMethod( GuiRadarTSCtrl, setEllipticColor, void, 3, 3, "(Color)"
			  "set the color of the horizontal plane")
{
	ColorF color;
	dSscanf(argv[2], "%g %g %g %g", &color.red, &color.green, &color.blue, &color.alpha);
	object->setEllipticColor(color);
}

ConsoleMethod( GuiRadarTSCtrl, setSphereColor, void, 3, 3, "(Color)"
			  "set the color of the sphere")
{
	ColorF color;
	dSscanf(argv[2], "%g %g %g %g", &color.red, &color.green, &color.blue, &color.alpha);
	object->setSphereColor(color);
}

ConsoleMethod( GuiRadarTSCtrl, setBlipColor, void, 3, 3, "(Color)"
			  "set the color of the radar blips")
{
	ColorF color;
	dSscanf(argv[2], "%g %g %g %g", &color.red, &color.green, &color.blue, &color.alpha);
	object->setBlipColor(color);
}

ConsoleMethod( GuiRadarTSCtrl, setStemColor, void, 3, 3, "(Color)"
			  "set the color of the blip lines")
{
	ColorF color;
	dSscanf(argv[2], "%g %g %g %g", &color.red, &color.green, &color.blue, &color.alpha);
	object->setStemColor(color);
}

ConsoleMethod( GuiRadarTSCtrl, setTarget, void, 2, 3, "(str Target ShapebaseName)"
			  "Set the target ShapeBaseName, if no Target is sent it is cleared.")
{
   object->setTarget((const char*)argv[2]);
}
