singleton ShaderData( ClearGBufferShader )
{
   DXVertexShaderFile = "shaders/common/lighting/advanced/deferredClearGBufferV.hlsl";
   DXPixelShaderFile  = "shaders/common/lighting/advanced/deferredClearGBufferP.hlsl";

   OGLVertexShaderFile = "shaders/common/postFx/gl/postFxV.glsl";
   OGLPixelShaderFile  = "shaders/common/lighting/advanced/gl/deferredClearGBufferP.glsl";

   pixVersion = 2.0;   
};

singleton ShaderData( DeferredColorShader )
{
   DXVertexShaderFile = "shaders/common/postFx/postFxV.hlsl";
   DXPixelShaderFile  = "shaders/common/lighting/advanced/deferredColorShaderP.hlsl";
   
   OGLVertexShaderFile = "shaders/common/postFx/gl/postFxV.glsl";
   OGLPixelShaderFile  = "shaders/common/lighting/advanced/gl/deferredColorShaderP.glsl";

   pixVersion = 2.0;   
};

// Primary Deferred Shader
new GFXStateBlockData( AL_DeferredShadingState : PFX_DefaultStateBlock )
{  
   cullMode = GFXCullNone;
      
   blendDefined = true;
   blendEnable = true; 
   blendSrc = GFXBlendSrcAlpha;
   blendDest = GFXBlendInvSrcAlpha;

   samplersDefined = true;
   samplerStates[0] = SamplerWrapLinear;
   samplerStates[1] = SamplerWrapLinear;
   samplerStates[2] = SamplerWrapLinear;
   samplerStates[3] = SamplerWrapLinear;
   samplerStates[4] = SamplerWrapLinear;
};

new GFXStateBlockData( AL_DeferredCaptureState : PFX_DefaultStateBlock )
{        
   blendEnable = false; 
   
   separateAlphaBlendDefined = true;
   separateAlphaBlendEnable = true;
   separateAlphaBlendSrc = GFXBlendSrcAlpha;
   separateAlphaBlendDest = GFXBlendDestAlpha;
   separateAlphaBlendOp = GFXBlendOpMin;
   
   samplersDefined = true;
   samplerStates[0] = SamplerWrapLinear;
   samplerStates[1] = SamplerWrapLinear;
   samplerStates[2] = SamplerWrapLinear;
   samplerStates[3] = SamplerWrapLinear;
   samplerStates[4] = SamplerWrapLinear;
};

new ShaderData( AL_ProbeShader )
{
   DXVertexShaderFile = "shaders/common/postFx/postFxV.hlsl";
   DXPixelShaderFile  = "shaders/common/lighting/advanced/probeShadingP.hlsl";
   
   OGLVertexShaderFile = "shaders/common/postFx/gl/postFxV.glsl";
   OGLPixelShaderFile  = "shaders/common/lighting/advanced/gl/probeShadingP.glsl";

   samplerNames[0] = "colorBufferTex";
   samplerNames[1] = "diffuseLightingBuffer";
   samplerNames[2] = "matInfoTex";
   samplerNames[3] = "specularLightingBuffer";
   samplerNames[4] = "deferredTex";
   pixVersion = 2.0;
};

singleton PostEffect( AL_PreCapture )
{
   renderTime = "PFXBeforeBin";
   renderBin = "ProbeBin";
   shader = AL_ProbeShader;
   stateBlock = AL_DeferredCaptureState;
   texture[0] = "#color";
   texture[1] = "#diffuseLighting";
   texture[2] = "#matinfo";
   texture[3] = "#specularLighting";
   texture[4] = "#deferred";
   target = "$backBuffer";
   renderPriority = 10000;
   allowReflectPass = true;
};

new ShaderData( SSR_RaycastShader )
{
   DXVertexShaderFile = "shaders/common/postFx/postFxV.hlsl";
   DXPixelShaderFile  = "shaders/common/lighting/advanced/SSLRraycastResult.hlsl";
   
   OGLVertexShaderFile = "shaders/common/postFx/gl/postFxV.glsl";
   OGLPixelShaderFile  = "shaders/common/lighting/advanced/gl/SSLRraycastResult.glsl";
   samplerNames[0] = "deferredTex";
   pixVersion = 2.0;
};

new ShaderData( SSR_BlurShader )
{
   DXVertexShaderFile = "shaders/common/postFx/postFxV.hlsl";
   DXPixelShaderFile  = "shaders/common/lighting/advanced/SSLRblur.hlsl";
   
   OGLVertexShaderFile = "shaders/common/postFx/gl/postFxV.glsl";
   OGLPixelShaderFile  = "shaders/common/lighting/advanced/gl/SSLRblur.glsl";
   samplerNames[0] = "colorBufferTex";
   pixVersion = 2.0;
};

new ShaderData( SSR_ResultShader )
{
   DXVertexShaderFile = "shaders/common/postFx/postFxV.hlsl";
   DXPixelShaderFile  = "shaders/common/lighting/advanced/SSLR.hlsl";
   
   OGLVertexShaderFile = "shaders/common/postFx/gl/postFxV.glsl";
   OGLPixelShaderFile  = "shaders/common/lighting/advanced/gl/SSLR.glsl";

   samplerNames[0] = "colorBlur";
   samplerNames[1] = "matInfoTex";
   samplerNames[2] = "specularLightingBuffer";
   samplerNames[3] = "deferredTex";
   samplerNames[4] = "$rayTraceTex";
   pixVersion = 2.0;
};

new ShaderData( AL_DeferredShader )
{
   DXVertexShaderFile = "shaders/common/postFx/postFxV.hlsl";
   DXPixelShaderFile  = "shaders/common/lighting/advanced/deferredShadingP.hlsl";
   
   OGLVertexShaderFile = "shaders/common/postFx/gl/postFxV.glsl";
   OGLPixelShaderFile  = "shaders/common/lighting/advanced/gl/deferredShadingP.glsl";

   samplerNames[0] = "colorBufferTex";
   samplerNames[1] = "diffuseLightingBuffer";
   samplerNames[2] = "matInfoTex";
   samplerNames[3] = "ssrLighting";
   samplerNames[4] = "deferredTex";
   pixVersion = 2.0;
};


singleton PostEffect( AL_DeferredShading )
{
   renderTime = "PFXAfterBin";
   renderBin = "ProbeBin";
   
   renderPriority = 10000;
   allowReflectPass = true;   
   shader = SSR_RaycastShader;
   stateBlock = AL_DeferredShadingState;
   texture[0] = "#deferred";
   targetFormat = "GFXFormatR16G16B16A16F";
   target = "#rayTrace";
      
   new PostEffect()
   {
      internalName = "ssrColorBlurPass";
      shader = SSR_BlurShader;
      stateBlock = AL_DeferredShadingState;
      texture[0] = "#color";
      targetFormat = "GFXFormatR16G16B16A16F";
      target = "$outTex";
   };
      new PostEffect()
   {
      internalName = "ssrColorBlurPass2";
      shader = SSR_BlurShader;
      stateBlock = PFX_DefaultStateBlock;
      texture[0] = "$inTex";
      targetFormat = "GFXFormatR16G16B16A16F";
      target = "#colorBlur";
   };
   
   new PostEffect()
   {
      internalName = "ssrSpecularResultPass";
      shader = SSR_ResultShader;
      stateBlock = AL_DeferredShadingState;
      texture[0] = "#colorBlur";
      texture[1] = "#matinfo";
      texture[2] = "#specularLighting";
      texture[3] = "#deferred";
      texture[4] = "#rayTrace";
      targetFormat = "GFXFormatR16G16B16A16F";
      target = "#ssrLighting";
   };
   new PostEffect()
   {
      internalName = "finalCombinePass";
      shader = AL_DeferredShader;
      stateBlock = AL_DeferredShadingState;
      texture[0] = "#color";
      texture[1] = "#diffuseLighting";
      texture[2] = "#matinfo";
      texture[3] = "#ssrLighting";
      texture[4] = "#deferred";
      target = "$backBuffer";
   };
};

function AL_DeferredShading::setShaderConsts( %this )
{
   %this-->ssrColorBlurPass.setShaderConst( "blurDir", 0);
   %this-->ssrColorBlurPass2.setShaderConst( "blurDir", 1);

   %this-->ssrColorBlurPass.setShaderConst( "cb_numMips", 10 );//we'll actually want to look this one up from the input tex/resolution calc
   %this-->ssrSpecularResultPass.setShaderConst( "cb_numMips", 10 );
   %this-->finalCombinePass.setShaderConst( "cb_numMips", 10 );
   %this.setShaderConst( "cb_numMips", 10 );
   
   %startfade = theLevelInfo.visibleDistance*theLevelInfo.nearClip;
   %this-->ssrColorBlurPass.setShaderConst( "cb_fadeStart",  %startfade);
   %this-->ssrSpecularResultPass.setShaderConst( "cb_fadeStart",  %startfade);
   %this-->finalCombinePass.setShaderConst( "cb_fadeStart",  %startfade);
   %this.setShaderConst( "cb_fadeStart", %startfade );
   
   %this-->ssrColorBlurPass.setShaderConst( "cb_fadeEnd", theLevelInfo.visibleDistance );
   %this-->ssrSpecularResultPass.setShaderConst( "cb_fadeEnd", theLevelInfo.visibleDistance );
   %this-->finalCombinePass.setShaderConst( "cb_fadeEnd", theLevelInfo.visibleDistance );
   %this.setShaderConst( "cb_fadeEnd", theLevelInfo.visibleDistance );
   
   %res = getWords($pref::Video::mode, 0, 1);
   %oneOverRes = 1/getWord($pref::Video::mode, 0) SPC 1/getWord($pref::Video::mode, 1);
   %this-->ssrColorBlurPass.setShaderConst( "cb_windowSize",%res);
   %this-->ssrSpecularResultPass.setShaderConst( "cb_windowSize", %res);
   %this-->finalCombinePass.setShaderConst( "cb_windowSize", %res);
   %this.setShaderConst( "cb_windowSize", %res);
   
   %this-->ssrColorBlurPass.setShaderConst( "cb_oneOverwindowSize", %oneOverRes);
   %this-->ssrSpecularResultPass.setShaderConst( "cb_oneOverwindowSize", %oneOverRes);
   %this-->finalCombinePass.setShaderConst( "cb_oneOverwindowSize", %oneOverRes);
   %this.setShaderConst( "cb_oneOverwindowSize", %oneOverRes);
   
}

// Debug Shaders.
new ShaderData( AL_ColorBufferShader )
{
   DXVertexShaderFile = "shaders/common/postFx/postFxV.hlsl";
   DXPixelShaderFile  = "shaders/common/lighting/advanced/dbgColorBufferP.hlsl";
   
   OGLVertexShaderFile = "shaders/common/postFx/gl/postFxV.glsl";
   OGLPixelShaderFile  = "shaders/common/lighting/advanced/gl/dbgColorBufferP.glsl";

   samplerNames[0] = "colorBufferTex";
   pixVersion = 2.0;
};

singleton PostEffect( AL_ColorBufferVisualize )
{   
   shader = AL_ColorBufferShader;
   stateBlock = AL_DefaultVisualizeState;
   texture[0] = "#color";
   target = "$backBuffer";
   renderPriority = 9999;
};

/// Toggles the visualization of the AL lighting specular power buffer.
function toggleColorBufferViz( %enable )
{   
   if ( %enable $= "" )
   {
      $AL_ColorBufferShaderVar = AL_ColorBufferVisualize.isEnabled() ? false : true;
      AL_ColorBufferVisualize.toggle();
   }
   else if ( %enable )
   {
      AL_DeferredShading.disable();
      AL_ColorBufferVisualize.enable();
   }
   else if ( !%enable )
   {
      AL_ColorBufferVisualize.disable();    
      AL_DeferredShading.enable();
   }
}

//roughness map display (matinfo.b)
new ShaderData( AL_RoughMapShader )
{
   DXVertexShaderFile = "shaders/common/postFx/postFxV.hlsl";
   DXPixelShaderFile  = "shaders/common/lighting/advanced/dbgRoughMapVisualizeP.hlsl";

   OGLVertexShaderFile = "shaders/common/postFx/gl/postFxV.glsl";
   OGLPixelShaderFile  = "shaders/common/lighting/advanced/gl/dbgRoughMapVisualizeP.glsl";

   samplerNames[0] = "matinfoTex";
   pixVersion = 2.0;
};

singleton PostEffect( AL_RoughMapVisualize )
{   
   shader = AL_RoughMapShader;
   stateBlock = AL_DefaultVisualizeState;
   texture[0] = "#matinfo";
   target = "$backBuffer";
   renderPriority = 9999;
};

function toggleRoughMapViz( %enable )
{   
   if ( %enable $= "" )
   {
      $AL_RoughMapShaderVar = AL_RoughMapVisualize.isEnabled() ? false : true;
      AL_RoughMapVisualize.toggle();
   }
   else if ( %enable )
      AL_RoughMapVisualize.enable();
   else if ( !%enable )
      AL_RoughMapVisualize.disable();    
}

//metalness map display (matinfo.a)
new ShaderData( AL_MetalMapShader )
{
   DXVertexShaderFile = "shaders/common/postFx/postFxV.hlsl";
   DXPixelShaderFile  = "shaders/common/lighting/advanced/dbgMetalMapVisualizeP.hlsl";

   OGLVertexShaderFile = "shaders/common/postFx/gl/postFxV.glsl";
   OGLPixelShaderFile  = "shaders/common/lighting/advanced/gl/dbgMetalMapVisualizeP.glsl";

   samplerNames[0] = "matinfoTex";
   pixVersion = 2.0;
};

singleton PostEffect( AL_MetalMapVisualize )
{   
   shader = AL_MetalMapShader;
   stateBlock = AL_DefaultVisualizeState;
   texture[0] = "#matinfo";
   target = "$backBuffer";
   renderPriority = 9999;
};

function toggleMetalMapViz( %enable )
{   
   if ( %enable $= "" )
   {
      $AL_MetalMapShaderVar = AL_MetalMapVisualize.isEnabled() ? false : true;
      AL_MetalMapVisualize.toggle();
   }
   else if ( %enable )
      AL_MetalMapVisualize.enable();
   else if ( !%enable )
      AL_MetalMapVisualize.disable();    
}

//Light map display (indirectLighting)
new ShaderData( AL_LightMapShader )
{
   DXVertexShaderFile = "shaders/common/postFx/postFxV.hlsl";
   DXPixelShaderFile  = "shaders/common/lighting/advanced/dbgLightMapVisualizeP.hlsl";

   OGLVertexShaderFile = "shaders/common/postFx/gl/postFxV.glsl";
   OGLPixelShaderFile  = "shaders/common/lighting/advanced/gl/dbgLightMapVisualizeP.glsl";

   samplerNames[0] = "specularLightingBuffer";
   pixVersion = 2.0;
};

singleton PostEffect( AL_LightMapVisualize )
{   
   shader = AL_LightMapShader;
   stateBlock = AL_DefaultVisualizeState;
   texture[0] = "#ssrLighting";
   target = "$backBuffer";
   renderPriority = 9999;
};

function toggleLightMapViz( %enable )
{   
   if ( %enable $= "" )
   {
      $AL_LightMapShaderVar = AL_LightMapVisualize.isEnabled() ? false : true;
      AL_LightMapVisualize.toggle();
   }
   else if ( %enable )
      AL_LightMapVisualize.enable();
   else if ( !%enable )
      AL_LightMapVisualize.disable();    
}