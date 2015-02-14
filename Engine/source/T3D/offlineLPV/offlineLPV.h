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

GFX_DeclareTextureProfile( LPVProfile );

/// A volume in space that blocks visibility.
class OfflineLPV : public ScenePolyhedralSpace
{
   public:

      typedef ScenePolyhedralSpace Parent;

   protected:

      // Volume Textures and raw data buffer used to copy.
      GFXTexHandle            mPropagatedTexture;
      GFXTexHandle            mDirectLightTexture;

      // We sample from prepass and render to light buffer.
      NamedTexTarget*         mPrepassTarget;
      NamedTexTarget*         mLightInfoTarget;
      GFXTextureTargetRef     mRenderTarget;

      // Stateblock for shaders
      GFXStateBlockRef        mStateBlock;

      // Propagated Shader
      GFXShaderRef            mPropagatedShader;
      GFXShaderConstBufferRef mPropagatedShaderConsts;
      GFXShaderConstHandle    *mEyePosWorldPropSC;
      GFXShaderConstHandle    *mRTParamsPropSC;
      GFXShaderConstHandle    *mVolumeStartPropSC;
      GFXShaderConstHandle    *mVolumeSizePropSC;

      // Reflection Shader
      GFXShaderRef            mReflectShader;
      GFXShaderConstBufferRef mReflectShaderConsts;
      GFXShaderConstHandle    *mInverseViewReflectSC;
      GFXShaderConstHandle    *mEyePosWorldReflectSC;
      GFXShaderConstHandle    *mRTParamsReflectSC;
      GFXShaderConstHandle    *mVolumeStartReflectSC;
      GFXShaderConstHandle    *mVolumeSizeReflectSC;

      // Configuratable volume resolution
      F32       mVoxelSize;

      // Volume Texture
      Point3I   mCachedVolumeSize;
      void     _initVolumeTextures(Point3I volumeSize);
      bool     _initShaders();

      // Geometry Grid
      ColorF*   mGeometryGrid;

      // Directly lit voxel grid. Calculated from real light sources in scene.
      ColorF*   mLightGrid;

      // Propagation Grids.
      U32       mPropagationStage;
      ColorF*   mPropagatedLightGrid;
      ColorF*   mPropagatedLightGridA;
      ColorF*   mPropagatedLightGridB;

      // Final Volume Rendering
      void _handleBinEvent( RenderBinManager *bin, const SceneRenderState* sceneState, bool isBinStart );  
      void _renderPropagated(const SceneRenderState* sceneState);
      void _renderReflect(const SceneRenderState* sceneState);
      void _updateScreenGeometry( const Frustum &frustum, GFXVertexBufferHandle<PFXVertex> *outVB );

      // World Editor Visualization.
      typedef SilhouetteExtractorPerspective< PolyhedronType > SilhouetteExtractorType;
      bool mTransformDirty;
      Vector< Point3F > mWSPoints;
      SilhouetteExtractorType mSilhouetteExtractor;
      mutable Vector< SceneObject* > mVolumeQueryList;
      virtual void _renderObject( ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* overrideMat );

   public:

      OfflineLPV();
      ~OfflineLPV();

      // Editor Triggered Flags
      bool mRegenVolume;
      bool mInjectLights;
      bool mPropagateLights;
      bool mShowVoxels;
      bool mShowDirectLight;
      bool mShowPropagated;
      bool mExportPropagated;
      bool mExportDirectLight;
      bool mRenderReflection;
      bool mSaveResults;
      bool mLoadResults;

      // SimObject.
      DECLARE_CONOBJECT( OfflineLPV );
      DECLARE_DESCRIPTION( "Offline Light Propogation Volume" );
      DECLARE_CATEGORY( "3D Scene" );

      virtual bool onAdd();
      virtual void onRemove();
      void inspectPostApply();

      Point3I  getVoxelCount();
      Point3F  getWorldSpaceVoxelSize();

      String   mFileName;
      bool     save();
      bool     load();

      // Editor Triggered Functions
      void     regenVolume();
      void     injectLights();
      void     exportPropagatedLight(ColorF* altSource = NULL);
      void     exportDirectLight(ColorF* altSource = NULL);
      ColorF   calcLightColor(Point3F position);
      F32      getAttenuation(LightInfo* lightInfo, Point3F position);
      void     propagateLights(ColorF* source, ColorF* dest, bool sampleFromGeometry = false);

      // Static Functions.
      static void consoleInit();
      static void initPersistFields();

      // Network
      U32 packUpdate( NetConnection *, U32 mask, BitStream *stream );
      void unpackUpdate( NetConnection *, BitStream *stream );

      // SceneObject.
      virtual void buildSilhouette( const SceneCameraState& cameraState, Vector< Point3F >& outPoints );
      virtual void setTransform( const MatrixF& mat );

      // Editor Triggered Functions
      static bool _setRegenVolume( void *object, const char *index, const char *data );
      static bool _setInjectLights( void *object, const char *index, const char *data );
      static bool _setPropagateLights( void *object, const char *index, const char *data );
      static bool _setExportPropagated( void *object, const char *index, const char *data );
      static bool _setExportDirectLight( void *object, const char *index, const char *data );
      static bool _setSaveResults( void *object, const char *index, const char *data );
      static bool _setLoadResults( void *object, const char *index, const char *data );
};

#endif // !_OFFLINELPV_H_
