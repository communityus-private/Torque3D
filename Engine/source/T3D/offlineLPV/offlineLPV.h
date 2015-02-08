//-----------------------------------------------------------------------------
// Copyright (c) 2015 Andrew Mac
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

#ifndef _OFFLINELPV_H_
#define _OFFLINELPV_H_

#ifndef _SCENEPOLYHEDRALSPACE_H_
#include "scene/scenePolyhedralSpace.h"
#endif

#ifndef _MSILHOUETTEEXTRACTOR_H_
#include "math/mSilhouetteExtractor.h"
#endif

#ifndef _GFXDEVICE_H_
#include "gfx/gfxDevice.h"
#endif

#ifndef _POSTEFFECTCOMMON_H_
#include "postFx/postEffectCommon.h"
#endif

#include "materials/matTextureTarget.h"
#include "renderInstance/renderBinManager.h"

#define LPV_GRID_RESOLUTION 16
GFX_DeclareTextureProfile( LPVProfile );

/// A volume in space that blocks visibility.
class OfflineLPV : public ScenePolyhedralSpace
{
   public:

      typedef ScenePolyhedralSpace Parent;

   protected:

      struct IndirectLightSource
      {
         ColorF color;
         Point3F position;
      };

      struct ColorVoxelGrid
      {
         ColorF data[LPV_GRID_RESOLUTION][LPV_GRID_RESOLUTION][LPV_GRID_RESOLUTION];
      };

      Vector<IndirectLightSource> indirectLightSources;

      GFXTexHandle mLPVTexture;
      GFXShaderRef mShader;
      GFXStateBlockRef mStateBlock;
      GFXShaderConstBufferRef mShaderConsts;
      GFXShaderConstHandle *mEyePosWorldSC;
      GFXShaderConstHandle *mRTParamsSC;
      GFXShaderConstHandle *mVolumeStartSC;
      GFXShaderConstHandle *mVolumeSizeSC;
      NamedTexTarget* mLightInfoTarget;
      NamedTexTarget* mPrepassTarget;
      GFXTextureTargetRef mRenderTarget;

      bool _initShader();

      U8 mLPVRawData[LPV_GRID_RESOLUTION * LPV_GRID_RESOLUTION * LPV_GRID_RESOLUTION * 4];
      bool mGeometryGrid[LPV_GRID_RESOLUTION][LPV_GRID_RESOLUTION][LPV_GRID_RESOLUTION];

      ColorVoxelGrid    mLightGrid;

      U32               mPropagationStage;
      ColorVoxelGrid*   mPropagatedLightGrid;
      ColorVoxelGrid    mPropagatedLightGridA;
      ColorVoxelGrid    mPropagatedLightGridB;

      typedef SilhouetteExtractorPerspective< PolyhedronType > SilhouetteExtractorType;

      /// Whether the volume's transform has changed and we need to recompute
      /// transform-based data.
      bool mTransformDirty;

      /// World-space points of the volume's polyhedron.
      Vector< Point3F > mWSPoints;

      /// Silhouette extractor when using perspective projections.
      SilhouetteExtractorType mSilhouetteExtractor;
      
      mutable Vector< SceneObject* > mVolumeQueryList;
      
      // SceneSpace.
      virtual void _renderObject( ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* overrideMat );

      void _handleBinEvent(   RenderBinManager *bin,                           
                        const SceneRenderState* sceneState,
                        bool isBinStart );  
      void _renderLPV(const SceneRenderState* sceneState);
      void _updateScreenGeometry( const Frustum &frustum, GFXVertexBufferHandle<PFXVertex> *outVB );

   public:

      OfflineLPV();
      ~OfflineLPV();

      bool mRegenVolume;
      bool mInjectLights;
      bool mPropagateLights;
      bool mShowLightGrid;
      bool mShowPropagatedLightGrid;
      bool mExportGrid;

      // SimObject.
      DECLARE_CONOBJECT( OfflineLPV );
      DECLARE_DESCRIPTION( "Offline Light Propogation Volume" );
      DECLARE_CATEGORY( "3D Scene" );

      virtual bool onAdd();
      virtual void onRemove();
      void inspectPostApply();
      void regenVolume();
      void injectLights();
      void exportGrid();
      ColorF calcLightColor(Point3F position);
      F32 getAttenuation(LightInfo* lightInfo, Point3F position);
      void propagateLights(ColorVoxelGrid* source, ColorVoxelGrid* dest);
      ColorF calcIndirectLightColor(Point3F position);

      // Static Functions.
      static void consoleInit();
      static void initPersistFields();

      // Network
      U32 packUpdate( NetConnection *, U32 mask, BitStream *stream );
      void unpackUpdate( NetConnection *, BitStream *stream );

      // SceneObject.
      virtual void buildSilhouette( const SceneCameraState& cameraState, Vector< Point3F >& outPoints );
      virtual void setTransform( const MatrixF& mat );

      static bool _setRegenVolume( void *object, const char *index, const char *data );
      static bool _setInjectLights( void *object, const char *index, const char *data );
      static bool _setPropagateLights( void *object, const char *index, const char *data );
      static bool _setExportGrid( void *object, const char *index, const char *data );
};

#endif // !_AccumulationVolume_H_
