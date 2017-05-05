/*singleton Material(ReflectProbePreviewMat)
{
   mapTo = "ReflectProbePreviewMat";
   diffuseColor[0] = "1 1 1 1";
   smoothness[0] = "1";
   metalness[0] = "1";
   emissive[0] = 1;
   translucentBlendOp = "None";
};*/

//Reflection Probe Diffuse
new ShaderData( ReflectionProbePreviewShader )
{
   DXVertexShaderFile = "./reflectionProbePreviewV.hlsl";
   DXPixelShaderFile  = "./reflectionProbePreviewP.hlsl";

   //OGLVertexShaderFile = "shaders/common/lighting/advanced/gl/convexGeometryV.glsl";
   //OGLPixelShaderFile  = "shaders/common/lighting/advanced/gl/pointLightP.glsl";

   samplerNames[0] = "$deferredBuffer";
   samplerNames[1] = "$cubemap";
   
   pixVersion = 3.0;
};

new CustomMaterial( ReflectProbePreviewMat )
{
   mapTo = "ReflectProbePreviewMat";
   shader = ReflectionProbePreviewShader;
   
   sampler["deferredBuffer"] = "#deferred";
   
   target = "directLighting";
   
   pixVersion = 3.0;
};

