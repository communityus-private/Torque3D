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

#include "platform/platform.h"
#include "lighting/advanced/hlsl/deferredShadingFeaturesHLSL.h"

#include "lighting/advanced/advancedLightBinManager.h"
#include "shaderGen/langElement.h"
#include "shaderGen/shaderOp.h"
#include "shaderGen/conditionerFeature.h"
#include "renderInstance/renderPrePassMgr.h"
#include "materials/processedMaterial.h"
#include "materials/materialFeatureTypes.h"


//****************************************************************************
// Deferred Shading Features
//****************************************************************************

// Specular Map -> Blue of Material Buffer ( greyscaled )
// Gloss Map (Alpha Channel of Specular Map) -> Alpha ( Spec Power ) of Material Info Buffer.
void DeferredSpecMapHLSL::processPix( Vector<ShaderComponent*> &componentList, const MaterialFeatureData &fd )
{
   // Get the texture coord.
   Var *texCoord = getInTexCoord( "texCoord", "float2", true, componentList );

   // search for color var
   Var *material = (Var*) LangElement::find( getOutputTargetVarName(ShaderFeature::RenderTarget2) );
   MultiLine * meta = new MultiLine;
   if ( !material )
   {
      // create color var
      material = new Var;
      material->setType( "fragout" );
      material->setName( getOutputTargetVarName(ShaderFeature::RenderTarget2) );
      material->setStructName( "OUT" );
   }

   // create texture var
   Var *specularMap = new Var;
   specularMap->setType( "sampler2D" );
   specularMap->setName( "specularMap" );
   specularMap->uniform = true;
   specularMap->sampler = true;
   specularMap->constNum = Var::getTexUnitNum();
   LangElement *texOp = new GenOp( "tex2D(@, @)", specularMap, texCoord );

   meta->addStatement(new GenOp("   @.b = dot(tex2D(@, @).rgb, float3(0.3, 0.59, 0.11));\r\n", material, specularMap, texCoord));
   meta->addStatement(new GenOp("   @.a = tex2D(@, @).a;\r\n", material, specularMap, texCoord));
   output = meta;
}

ShaderFeature::Resources DeferredSpecMapHLSL::getResources( const MaterialFeatureData &fd )
{
   Resources res; 
   res.numTex = 1;
   res.numTexReg = 1;

   return res;
}

void DeferredSpecMapHLSL::setTexData(   Material::StageData &stageDat,
                                       const MaterialFeatureData &fd,
                                       RenderPassData &passData,
                                       U32 &texIndex )
{
   GFXTextureObject *tex = stageDat.getTex( MFT_SpecularMap );
   if ( tex )
   {
      passData.mTexType[ texIndex ] = Material::Standard;
      passData.mSamplerNames[ texIndex ] = "specularMap";
      passData.mTexSlot[ texIndex++ ].texObject = tex;
   }
}

void DeferredSpecMapHLSL::processVert( Vector<ShaderComponent*> &componentList, 
                                       const MaterialFeatureData &fd )
{
   MultiLine *meta = new MultiLine;
   getOutTexCoord(   "texCoord", 
                     "float2", 
                     true, 
                     fd.features[MFT_TexAnim], 
                     meta, 
                     componentList );
   output = meta;
}

// Material Info Flags -> Red ( Flags ) of Material Info Buffer.
void DeferredMatInfoFlagsHLSL::processPix( Vector<ShaderComponent*> &componentList, const MaterialFeatureData &fd )
{
   // search for material var
   Var *material = (Var*) LangElement::find( getOutputTargetVarName(ShaderFeature::RenderTarget2) );
   if ( !material )
   {
      // create material var
      material = new Var;
      material->setType( "fragout" );
      material->setName( getOutputTargetVarName(ShaderFeature::RenderTarget2) );
      material->setStructName( "OUT" );
   }

   Var *matInfoFlags = new Var;
   matInfoFlags->setType( "float" );
   matInfoFlags->setName( "matInfoFlags" );
   matInfoFlags->uniform = true;
   matInfoFlags->constSortPos = cspPotentialPrimitive;

   output = new GenOp( "   @.r = @;\r\n", material, matInfoFlags );
}

// Spec Strength -> Blue Channel of Material Info Buffer.
void DeferredSpecStrengthHLSL::processPix( Vector<ShaderComponent*> &componentList, const MaterialFeatureData &fd )
{
   // search for material var
   Var *material = (Var*) LangElement::find( getOutputTargetVarName(ShaderFeature::RenderTarget2) );
   if ( !material )
   {
      // create material var
      material = new Var;
      material->setType( "fragout" );
      material->setName( getOutputTargetVarName(ShaderFeature::RenderTarget2) );
      material->setStructName( "OUT" );
   }

   Var *specStrength = new Var;
   specStrength->setType( "float" );
   specStrength->setName( "specularStrength" );
   specStrength->uniform = true;
   specStrength->constSortPos = cspPotentialPrimitive;

   output = new GenOp( "   @.b = @/128;\r\n", material, specStrength );
}

// Spec Power -> Alpha Channel ( of Material Info Buffer.
void DeferredSpecPowerHLSL::processPix( Vector<ShaderComponent*> &componentList, const MaterialFeatureData &fd )
{
   // search for material var
   Var *material = (Var*) LangElement::find( getOutputTargetVarName(ShaderFeature::RenderTarget2) );
   if ( !material )
   {
      // create material var
      material = new Var;
      material->setType( "fragout" );
      material->setName( getOutputTargetVarName(ShaderFeature::RenderTarget2) );
      material->setStructName( "OUT" );
   }

   Var *specPower = new Var;
   specPower->setType( "float" );
   specPower->setName( "specularPower" );
   specPower->uniform = true;
   specPower->constSortPos = cspPotentialPrimitive;
   output = new GenOp( "   @.a = @/5;\r\n", material, specPower );

}

// Black -> Blue and Alpha of Color Buffer (representing no specular)
void DeferredEmptySpecHLSL::processPix( Vector<ShaderComponent*> &componentList, const MaterialFeatureData &fd )
{
   // search for color var
   Var *color = (Var*) LangElement::find( getOutputTargetVarName(ShaderFeature::RenderTarget2) );
   if ( !color )
   {
       // create color var
      color = new Var;
      color->setType( "fragout" );
      color->setName( getOutputTargetVarName(ShaderFeature::RenderTarget2) );
      color->setStructName( "OUT" );
   }
   
   output = new GenOp( "   @.ba = 0.0;\r\n", color );
}

// Tranlucency -> Green of Material Info Buffer.
void DeferredTranslucencyMapHLSL::processPix( Vector<ShaderComponent*> &componentList, const MaterialFeatureData &fd )
{
   // Get the texture coord.
   Var *texCoord = getInTexCoord( "texCoord", "float2", true, componentList );

   // search for color var
   Var *material = (Var*) LangElement::find( getOutputTargetVarName(ShaderFeature::RenderTarget2) );
   if ( !material )
   {
      // create color var
      material = new Var;
      material->setType( "fragout" );
      material->setName( getOutputTargetVarName(ShaderFeature::RenderTarget2) );
      material->setStructName( "OUT" );
   }

   // create texture var
   Var *translucencyMap = new Var;
   translucencyMap->setType( "sampler2D" );
   translucencyMap->setName( "translucencyMap" );
   translucencyMap->uniform = true;
   translucencyMap->sampler = true;
   translucencyMap->constNum = Var::getTexUnitNum();

   output = new GenOp( "   @.g = dot(tex2D(@, @).rgb, float3(0.3, 0.59, 0.11));\r\n", material, translucencyMap, texCoord );
   
}

ShaderFeature::Resources DeferredTranslucencyMapHLSL::getResources( const MaterialFeatureData &fd )
{
   Resources res; 
   res.numTex = 1;
   res.numTexReg = 1;

   return res;
}

void DeferredTranslucencyMapHLSL::setTexData(   Material::StageData &stageDat,
                                       const MaterialFeatureData &fd,
                                       RenderPassData &passData,
                                       U32 &texIndex )
{
   GFXTextureObject *tex = stageDat.getTex( MFT_TranslucencyMap );
   if ( tex )
   {
      passData.mTexType[ texIndex ] = Material::Standard;
      passData.mSamplerNames[ texIndex ] = "translucencyMap";
      passData.mTexSlot[ texIndex++ ].texObject = tex;
   }
}

// Tranlucency -> Green of Material Info Buffer.
void DeferredTranslucencyEmptyHLSL::processPix( Vector<ShaderComponent*> &componentList, const MaterialFeatureData &fd )
{
   // search for material var
   Var *material = (Var*) LangElement::find( getOutputTargetVarName(ShaderFeature::RenderTarget2) );
   if ( !material )
   {
      // create color var
      material = new Var;
      material->setType( "fragout" );
      material->setName( getOutputTargetVarName(ShaderFeature::RenderTarget2) );
      material->setStructName( "OUT" );
   }
   output = new GenOp( "   @.g = 0.0;\r\n", material );
   
}