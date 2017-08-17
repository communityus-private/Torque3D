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
#include "lighting/advanced/hlsl/advancedLightingFeaturesHLSL.h"

#include "lighting/advanced/advancedLightBinManager.h"
#include "shaderGen/langElement.h"
#include "shaderGen/shaderOp.h"
#include "shaderGen/conditionerFeature.h"
#include "renderInstance/renderDeferredMgr.h"
#include "materials/processedMaterial.h"
#include "materials/materialFeatureTypes.h"


void DeferredRTLightingFeatHLSL::processPixMacros( Vector<GFXShaderMacro> &macros, 
                                                   const MaterialFeatureData &fd  )
{
   // Skip deferred features, and use forward shading instead
   if ( !fd.features[MFT_isDeferred] )
   {
      Parent::processPixMacros( macros, fd );
      return;
   }

   // Pull in the uncondition method for the light info buffer
   NamedTexTarget *texTarget = NamedTexTarget::find( AdvancedLightBinManager::smBufferName );
   if ( texTarget && texTarget->getConditioner() )
   {
      ConditionerMethodDependency *unconditionMethod = texTarget->getConditioner()->getConditionerMethodDependency(ConditionerFeature::UnconditionMethod);
      unconditionMethod->createMethodMacro( String::ToLower( AdvancedLightBinManager::smBufferName ) + "Uncondition", macros );
      addDependency(unconditionMethod);
   }
}

void DeferredRTLightingFeatHLSL::processVert(   Vector<ShaderComponent*> &componentList, 
                                                const MaterialFeatureData &fd )
{
   // Skip deferred features, and use forward shading instead
   if ( !fd.features[MFT_isDeferred] )
   {
      Parent::processVert( componentList, fd );
      return;
   }

   // Pass screen space position to pixel shader to compute a full screen buffer uv
   ShaderConnector *connectComp = dynamic_cast<ShaderConnector *>( componentList[C_CONNECTOR] );
   Var *ssPos = connectComp->getElement( RT_TEXCOORD );
   ssPos->setName( "screenspacePos" );
   ssPos->setStructName( "OUT" );
   ssPos->setType( "float4" );

   Var *outPosition = (Var*) LangElement::find( "hpos" );
   AssertFatal( outPosition, "No hpos, ohnoes." );

   output = new GenOp( "   @ = @;\r\n", ssPos, outPosition );
}

void DeferredRTLightingFeatHLSL::processPix( Vector<ShaderComponent*> &componentList,
                                             const MaterialFeatureData &fd )
{
   // Skip deferred features, and use forward shading instead
   if ( !fd.features[MFT_isDeferred] )
   {
      Parent::processPix( componentList, fd );
      return;
   }

   MultiLine *meta = new MultiLine;

   ShaderConnector *connectComp = dynamic_cast<ShaderConnector *>( componentList[C_CONNECTOR] );
   Var *ssPos = connectComp->getElement( RT_TEXCOORD );
   ssPos->setName( "screenspacePos" );
   ssPos->setStructName( "IN" );
   ssPos->setType( "float4" );

   Var *uvScene = new Var;
   uvScene->setType( "float2" );
   uvScene->setName( "uvScene" );
   LangElement *uvSceneDecl = new DecOp( uvScene );

   String rtParamName = String::ToString("rtParams%s", "directLightingBuffer");
   Var *rtParams = (Var*)LangElement::find(rtParamName);
   if (!rtParams)
   {
      rtParams = new Var;
      rtParams->setType( "float4" );
      rtParams->setName( rtParamName );
      rtParams->uniform = true;
      rtParams->constSortPos = cspPass;
   }

   meta->addStatement( new GenOp( "   @ = @.xy / @.w;\r\n", uvSceneDecl, ssPos, ssPos ) ); // get the screen coord... its -1 to +1
   meta->addStatement( new GenOp( "   @ = ( @ + 1.0 ) / 2.0;\r\n", uvScene, uvScene ) ); // get the screen coord to 0 to 1
   meta->addStatement( new GenOp( "   @.y = 1.0 - @.y;\r\n", uvScene, uvScene ) ); // flip the y axis 
   meta->addStatement( new GenOp( "   @ = ( @ * @.zw ) + @.xy;\r\n", uvScene, uvScene, rtParams, rtParams) ); // scale it down and offset it to the rt size

   Var *lightInfoSamp = new Var;
   lightInfoSamp->setType( "float4" );
   lightInfoSamp->setName( "lightInfoSample" );

   // create texture var
   Var *lightInfoBuffer = new Var;
   lightInfoBuffer->setType( "SamplerState" );
   lightInfoBuffer->setName( "lightInfoBuffer" );
   lightInfoBuffer->uniform = true;
   lightInfoBuffer->sampler = true;
   lightInfoBuffer->constNum = Var::getTexUnitNum();     // used as texture unit num here

   Var* lightBufferTex =  new Var;
   lightBufferTex->setName("lightInfoBufferTex");
   lightBufferTex->setType("Texture2D");
   lightBufferTex->uniform = true;
   lightBufferTex->texture = true;
   lightBufferTex->constNum = lightInfoBuffer->constNum;

   // Declare the RTLighting variables in this feature, they will either be assigned
   // in this feature, or in the tonemap/lightmap feature
   Var *d_lightcolor = new Var( "d_lightcolor", "float3" );
   meta->addStatement( new GenOp( "   @;\r\n", new DecOp( d_lightcolor ) ) );

   Var *d_NL_Att = new Var( "d_NL_Att", "float" );
   meta->addStatement( new GenOp( "   @;\r\n", new DecOp( d_NL_Att ) ) );

   Var *d_specular = new Var( "d_specular", "float" );
   meta->addStatement( new GenOp( "   @;\r\n", new DecOp( d_specular ) ) );
   

   // Perform the uncondition here.
   String unconditionLightInfo = String::ToLower( AdvancedLightBinManager::smBufferName ) + "Uncondition";
   meta->addStatement(new GenOp(avar("   %s(@.Sample(@, @), @, @, @);\r\n",
      unconditionLightInfo.c_str()), lightBufferTex, lightInfoBuffer, uvScene, d_lightcolor, d_NL_Att, d_specular));

   // If this has an interlaced pre-pass, do averaging here
   if( fd.features[MFT_InterlacedDeferred] )
   {
      Var *oneOverTargetSize = (Var*) LangElement::find( "oneOverTargetSize" );
      if( !oneOverTargetSize )
      {
         oneOverTargetSize = new Var;
         oneOverTargetSize->setType( "float2" );
         oneOverTargetSize->setName( "oneOverTargetSize" );
         oneOverTargetSize->uniform = true;
         oneOverTargetSize->constSortPos = cspPass;
      }

      meta->addStatement( new GenOp( "   float id_NL_Att, id_specular;\r\n   float3 id_lightcolor;\r\n" ) );
      meta->addStatement(new GenOp(avar("   %s(@.Sample(@, @ + float2(0.0, @.y)), id_lightcolor, id_NL_Att, id_specular);\r\n",
         unconditionLightInfo.c_str()), lightBufferTex, lightInfoBuffer, uvScene, oneOverTargetSize));

      meta->addStatement( new GenOp("   @ = lerp(@, id_lightcolor, 0.5);\r\n", d_lightcolor, d_lightcolor ) );
      meta->addStatement( new GenOp("   @ = lerp(@, id_NL_Att, 0.5);\r\n", d_NL_Att, d_NL_Att ) );
      meta->addStatement( new GenOp("   @ = lerp(@, id_specular, 0.5);\r\n", d_specular, d_specular ) );
   }

   // This is kind of weak sauce
   if( !fd.features[MFT_VertLit] && !fd.features[MFT_ToneMap] && !fd.features[MFT_LightMap] && !fd.features[MFT_SubSurface] )
      meta->addStatement( new GenOp( "   @;\r\n", assignColor( new GenOp( "float4(@, 1.0)", d_lightcolor ), Material::Mul ) ) );

   output = meta;
}

ShaderFeature::Resources DeferredRTLightingFeatHLSL::getResources( const MaterialFeatureData &fd )
{
   // Skip deferred features, and use forward shading instead
   if ( !fd.features[MFT_isDeferred] )
      return Parent::getResources( fd );

   // HACK: See DeferredRTLightingFeatHLSL::setTexData.
   mLastTexIndex = 0;

   Resources res; 
   res.numTex = 1;
   res.numTexReg = 1;
   return res;
}

void DeferredRTLightingFeatHLSL::setTexData( Material::StageData &stageDat,
                                             const MaterialFeatureData &fd, 
                                             RenderPassData &passData, 
                                             U32 &texIndex )
{
   // Skip deferred features, and use forward shading instead
   if ( !fd.features[MFT_isDeferred] )
   {
      Parent::setTexData( stageDat, fd, passData, texIndex );
      return;
   }

   NamedTexTarget *texTarget = NamedTexTarget::find( AdvancedLightBinManager::smBufferName );
   if( texTarget )
   {
      // HACK: We store this for use in DeferredRTLightingFeatHLSL::processPix()
      // which cannot deduce the texture unit itself.
      mLastTexIndex = texIndex;

      passData.mTexType[ texIndex ] = Material::TexTarget;
      passData.mSamplerNames[ texIndex ]= "directLightingBuffer";
      passData.mTexSlot[ texIndex++ ].texTarget = texTarget;
   }
}


void DeferredBumpFeatHLSL::processVert(   Vector<ShaderComponent*> &componentList, 
                                          const MaterialFeatureData &fd )
{
   if( fd.features[MFT_DeferredConditioner] )
   {
      // There is an output conditioner active, so we need to supply a transform
      // to the pixel shader. 
      MultiLine *meta = new MultiLine;

      // We need the view to tangent space transform in the pixel shader.
      getOutViewToTangent( componentList, meta, fd );

      const bool useTexAnim = fd.features[MFT_TexAnim];
      // Make sure there are texcoords
      if( !fd.features[MFT_Parallax] && !fd.features[MFT_DiffuseMap])
      {

         getOutTexCoord(   "texCoord", 
                           "float2", 
                           useTexAnim, 
                           meta, 
                           componentList );
      }

      if ( fd.features.hasFeature( MFT_DetailNormalMap ) )
            addOutDetailTexCoord( componentList, 
                                  meta,
                                  useTexAnim );

      output = meta;
   }
   else if (   fd.materialFeatures[MFT_NormalsOut] || 
               !fd.features[MFT_isDeferred] || 
               !fd.features[MFT_RTLighting] )
   {
      Parent::processVert( componentList, fd );
      return;
   }
   else
   {
      output = NULL;
   }
}

void DeferredBumpFeatHLSL::processPix( Vector<ShaderComponent*> &componentList, 
                                       const MaterialFeatureData &fd )
{
   // NULL output in case nothing gets handled
   output = NULL;

   if( fd.features[MFT_DeferredConditioner] )
   {
      MultiLine *meta = new MultiLine;

      Var *viewToTangent = getInViewToTangent( componentList );

      // create texture var
      Var *bumpMap = getNormalMapTex();
      Var *texCoord = getInTexCoord("texCoord", "float2", componentList);

      Var *bumpMapTex = (Var*)LangElement::find("bumpMapTex");
      LangElement *texOp = new GenOp("@.Sample(@, @)", bumpMapTex, bumpMap, texCoord);

      // create bump normal
      Var *bumpNorm = new Var;
      bumpNorm->setName( "bumpNormal" );
      bumpNorm->setType( "float4" );

      LangElement *bumpNormDecl = new DecOp( bumpNorm );
      meta->addStatement( expandNormalMap( texOp, bumpNormDecl, bumpNorm, fd ) );

      // If we have a detail normal map we add the xy coords of
      // it to the base normal map.  This gives us the effect we
      // want with few instructions and minial artifacts.
      if ( fd.features.hasFeature( MFT_DetailNormalMap ) )
      {
         bumpMap = new Var;
         bumpMap->setType( "SamplerState" );
         bumpMap->setName( "detailBumpMap" );
         bumpMap->uniform = true;
         bumpMap->sampler = true;
         bumpMap->constNum = Var::getTexUnitNum();

         Var* detailNormalTex = new Var;
         detailNormalTex->setName("detailBumpMapTex");
         detailNormalTex->setType("Texture2D");
         detailNormalTex->uniform = true;
         detailNormalTex->texture = true;
         detailNormalTex->constNum = bumpMap->constNum;
         texCoord = getInTexCoord("detCoord", "float2", componentList);
         texOp = new GenOp("@.Sample(@, @)", detailNormalTex, bumpMap, texCoord);

         Var *detailBump = new Var;
         detailBump->setName( "detailBump" );
         detailBump->setType( "float4" );
         meta->addStatement( expandNormalMap( texOp, new DecOp( detailBump ), detailBump, fd ) );

         Var *detailBumpScale = new Var;
         detailBumpScale->setType( "float" );
         detailBumpScale->setName( "detailBumpStrength" );
         detailBumpScale->uniform = true;
         detailBumpScale->constSortPos = cspPass;
         meta->addStatement( new GenOp( "   @.xy += @.xy * @;\r\n", bumpNorm, detailBump, detailBumpScale ) );
      }
      if (fd.features.hasFeature(MFT_NormalDamage))
      {
         bumpMap = new Var;
         bumpMap->setName("normalDamageMap");
         bumpMap->setType("SamplerState");
         bumpMap->uniform = true;
         bumpMap->sampler = true;
         bumpMap->constNum = Var::getTexUnitNum();
         bumpMap->setType("SamplerState");

         Var* damageBumpTex = new Var;
         damageBumpTex->setName("detailBumpTex");
         damageBumpTex->setType("Texture2D");
         damageBumpTex->uniform = true;
         damageBumpTex->texture = true;
         damageBumpTex->constNum = bumpMap->constNum;

         texCoord = getInTexCoord("texCoord", "float2", componentList);
         texOp = new GenOp("@.Sample(@, @)", damageBumpTex, bumpMap, texCoord);

         Var *damageBump = new Var;
         damageBump->setName("damageBump");
         damageBump->setType("float4");
         meta->addStatement(expandNormalMap(texOp, new DecOp(damageBump), damageBump, fd));

         Var *damage = (Var*)LangElement::find("materialDamage");
         if (!damage){
            damage = new Var("materialDamage", "float");
            damage->uniform = true;
            damage->constSortPos = cspPrimitive;
         }
         Var *floor = (Var*)LangElement::find("materialDamageMin");
         if (!floor){
            floor = new Var("materialDamageMin", "float");
            floor->uniform = true;
            floor->constSortPos = cspPrimitive;
         }

         Var *damageResult = (Var*)LangElement::find("damageResult");
         if (!damageResult){
            damageResult = new Var("damageResult", "float");
            meta->addStatement(new GenOp("   @ = max(@,@);\r\n", new DecOp(damageResult), floor, damage));
         }
         else
            meta->addStatement(new GenOp("   @ = max(@,@);\r\n", damageResult, floor, damage));

         meta->addStatement(new GenOp("   @.xyz = lerp(@.xyz, @.xyz, @);\r\n", bumpNorm, bumpNorm, damageBump, damageResult));
      }

      // This var is read from GBufferConditionerHLSL and 
      // used in the deferred output.
      //
      // By using the 'half' type here we get a bunch of partial
      // precision optimized code on further operations on the normal
      // which helps alot on older Geforce cards.
      //
      Var *gbNormal = new Var;
      gbNormal->setName( "gbNormal" );
      gbNormal->setType( "half3" );
      LangElement *gbNormalDecl = new DecOp( gbNormal );

      // Normalize is done later... 
      // Note: The reverse mul order is intentional. Affine matrix.
      meta->addStatement( new GenOp( "   @ = (half3)mul( @.xyz, @ );\r\n", gbNormalDecl, bumpNorm, viewToTangent ) );

      output = meta;
      return;
   }
   else if (fd.features[MFT_AccuMap]) 
   {
      Var *bumpSample = (Var *)LangElement::find( "bumpSample" );
      if (bumpSample == NULL)
      {
         MultiLine *meta = new MultiLine;

         Var *texCoord = getInTexCoord("texCoord", "float2", componentList);

         Var *bumpMap = getNormalMapTex();

         bumpSample = new Var;
         bumpSample->setType("float4");
         bumpSample->setName("bumpSample");
         LangElement *bumpSampleDecl = new DecOp(bumpSample);

         Var *bumpMapTex = (Var *)LangElement::find("bumpMapTex");
         output = new GenOp("   @ = @.Sample(@, @);\r\n", bumpSampleDecl, bumpMapTex, bumpMap, texCoord);

         if ( fd.features.hasFeature( MFT_DetailNormalMap ) )
         {
            Var *bumpMap = (Var*)LangElement::find( "detailBumpMap" );
            if ( !bumpMap )
            {
               bumpMap = new Var;
               bumpMap->setType( "sampler2D" );
               bumpMap->setName( "detailBumpMap" );
               bumpMap->uniform = true;
               bumpMap->sampler = true;
               bumpMap->constNum = Var::getTexUnitNum();
            }

            Var* bumpMapTex = (Var*)LangElement::find("detailBumpMap");
            if (!bumpMapTex)
            {
               bumpMap->setType("SamplerState");
               bumpMapTex = new Var;
               bumpMapTex->setName("detailBumpMapTex");
               bumpMapTex->setType("Texture2D");
               bumpMapTex->uniform = true;
               bumpMapTex->texture = true;
               bumpMapTex->constNum = bumpMap->constNum;
            }

            texCoord = getInTexCoord( "detCoord", "float2", componentList );
            LangElement *texOp = new GenOp("@.Sample(@, @)", bumpMap, bumpMapTex, texCoord);

            Var *detailBump = new Var;
            detailBump->setName( "detailBump" );
            detailBump->setType( "float4" );
            meta->addStatement( expandNormalMap( texOp, new DecOp( detailBump ), detailBump, fd ) );

            Var *detailBumpScale = new Var;
            detailBumpScale->setType( "float" );
            detailBumpScale->setName( "detailBumpStrength" );
            detailBumpScale->uniform = true;
            detailBumpScale->constSortPos = cspPass;
            meta->addStatement( new GenOp( "   @.xy += @.xy * @;\r\n", bumpSample, detailBump, detailBumpScale ) );
         }

         output = meta;

         return;
      }
   } 
   else if (   fd.materialFeatures[MFT_NormalsOut] || 
               !fd.features[MFT_isDeferred] || 
               !fd.features[MFT_RTLighting] )
   {
      Parent::processPix( componentList, fd );
      return;
   }
   else if ( fd.features[MFT_PixSpecular] && !fd.features[MFT_SpecularMap] )
   {
      Var *bumpSample = (Var *)LangElement::find( "bumpSample" );
      if( bumpSample == NULL )
      {
         Var *texCoord = getInTexCoord( "texCoord", "float2", componentList );

         Var *bumpMap = getNormalMapTex();

         bumpSample = new Var;
         bumpSample->setType( "float4" );
         bumpSample->setName( "bumpSample" );
         LangElement *bumpSampleDecl = new DecOp( bumpSample );

         Var *bumpMapTex = (Var *)LangElement::find("bumpMapTex");
         output = new GenOp("   @ = @.Sample(@, @);\r\n", bumpSampleDecl, bumpMapTex, bumpMap, texCoord);
         return;
      }
   }

   output = NULL;
}

ShaderFeature::Resources DeferredBumpFeatHLSL::getResources( const MaterialFeatureData &fd )
{
   if (  fd.materialFeatures[MFT_NormalsOut] || 
         !fd.features[MFT_isDeferred] || 
         fd.features[MFT_Parallax] ||
         !fd.features[MFT_RTLighting] )
      return Parent::getResources( fd );

   Resources res; 
   if(!fd.features[MFT_SpecularMap])
   {
      res.numTex = 1;
      res.numTexReg = 1;

      if (  fd.features[MFT_DeferredConditioner] &&
            fd.features.hasFeature( MFT_DetailNormalMap ) )
      {
         res.numTex += 1;
         if ( !fd.features.hasFeature( MFT_DetailMap ) )
            res.numTexReg += 1;
      }
   }

   return res;
}

void DeferredBumpFeatHLSL::setTexData( Material::StageData &stageDat,
                                       const MaterialFeatureData &fd, 
                                       RenderPassData &passData, 
                                       U32 &texIndex )
{
   if (  fd.materialFeatures[MFT_NormalsOut] || 
         !fd.features[MFT_isDeferred] || 
         !fd.features[MFT_RTLighting] )
   {
      Parent::setTexData( stageDat, fd, passData, texIndex );
      return;
   }

   if (  !fd.features[MFT_DeferredConditioner] && fd.features[MFT_AccuMap] )
   {
      passData.mTexType[ texIndex ] = Material::Bump;
      passData.mSamplerNames[ texIndex ] = "bumpMap";
      passData.mTexSlot[ texIndex++ ].texObject = stageDat.getTex( MFT_NormalMap );

      if (  fd.features.hasFeature( MFT_DetailNormalMap ) )
      {
         passData.mTexType[ texIndex ] = Material::DetailBump;
         passData.mSamplerNames[texIndex] = "detailBumpMap";
         passData.mTexSlot[ texIndex++ ].texObject = stageDat.getTex( MFT_DetailNormalMap );
      }
      
      if (  fd.features.hasFeature( MFT_NormalDamage ) )
      {
         passData.mTexType[texIndex] = Material::Bump;
         passData.mSamplerNames[texIndex] = "normalDamageMap";
         passData.mTexSlot[texIndex++].texObject = stageDat.getTex(MFT_NormalDamage);
      }
   }
   else if (  !fd.features[MFT_Parallax] && !fd.features[MFT_SpecularMap] &&
         ( fd.features[MFT_DeferredConditioner] ||
           fd.features[MFT_PixSpecular] ) )
   {
      passData.mTexType[ texIndex ] = Material::Bump;
      passData.mSamplerNames[ texIndex ] = "bumpMap";
      passData.mTexSlot[ texIndex++ ].texObject = stageDat.getTex( MFT_NormalMap );

      if (  fd.features[MFT_DeferredConditioner] &&
            fd.features.hasFeature( MFT_DetailNormalMap ) )
      {
         passData.mTexType[ texIndex ] = Material::DetailBump;
         passData.mSamplerNames[ texIndex ] = "detailBumpMap";
         passData.mTexSlot[ texIndex++ ].texObject = stageDat.getTex( MFT_DetailNormalMap );
      }

      if (fd.features[MFT_DeferredConditioner] &&
         fd.features.hasFeature(MFT_NormalDamage))
      {
         passData.mTexType[texIndex] = Material::Bump;
         passData.mSamplerNames[texIndex] = "normalDamageMap";
         passData.mTexSlot[texIndex++].texObject = stageDat.getTex(MFT_NormalDamage);
      }
   }
}


void DeferredPixelSpecularHLSL::processVert( Vector<ShaderComponent*> &componentList, 
                                             const MaterialFeatureData &fd )
{
   if( !fd.features[MFT_isDeferred] || !fd.features[MFT_RTLighting] )
   {
      Parent::processVert( componentList, fd );
      return;
   }
   output = NULL;
}

void DeferredPixelSpecularHLSL::processPix(  Vector<ShaderComponent*> &componentList, 
                                             const MaterialFeatureData &fd )
{
   if( !fd.features[MFT_isDeferred] || !fd.features[MFT_RTLighting] )
   {
      Parent::processPix( componentList, fd );
      return;
   }

   MultiLine *meta = new MultiLine;

   Var *specular = new Var;
   specular->setType( "float" );
   specular->setName( "specular" );
   LangElement * specDecl = new DecOp( specular );

   Var *specCol = (Var*)LangElement::find( "specularColor" );
   if(specCol == NULL)
   {
      specCol = new Var;
      specCol->setType( "float4" );
      specCol->setName( "specularColor" );
      specCol->uniform = true;
      specCol->constSortPos = cspPotentialPrimitive;
   }

   Var *smoothness = (Var*)LangElement::find("smoothness");
   if (!smoothness)
   {
      smoothness = new Var("smoothness", "float");

      // If the gloss map flag is set, than the specular power is in the alpha
      // channel of the specular map
      if (fd.features[MFT_GlossMap])
         meta->addStatement(new GenOp("   @ = @.a;\r\n", new DecOp(smoothness), specCol));
      else
      {
         smoothness->uniform = true;
         smoothness->constSortPos = cspPotentialPrimitive;
      }
   }

   Var *metalness = (Var*)LangElement::find("metalness");
   if (!metalness)
   {
       metalness = new Var("metalness", "float");
       metalness->uniform = true;
       metalness->constSortPos = cspPotentialPrimitive;
   }

   Var *lightInfoSamp = (Var *)LangElement::find( "lightInfoSample" );
   Var *d_specular = (Var*)LangElement::find( "d_specular" );
   Var *d_NL_Att = (Var*)LangElement::find( "d_NL_Att" );

   AssertFatal( lightInfoSamp && d_specular && d_NL_Att,
      "DeferredPixelSpecularHLSL::processPix - Something hosed the deferred features!" );

   if (fd.features[ MFT_AccuMap ])
   {
      // change specularity where the accu texture is applied
      Var *accuPlc = (Var*) LangElement::find( "plc" );
      Var *accuSpecular = (Var*)LangElement::find( "accuSpecular" );
      if(accuPlc != NULL && accuSpecular != NULL)
         //d_specular = clamp(lerp( d_specular, accuSpecular * d_specular, plc.a), 0, 1)
         meta->addStatement( new GenOp( "   @ = clamp( lerp( @, @ * @, @.a), 0, 1);\r\n", d_specular, d_specular, accuSpecular, d_specular, accuPlc ) );
   }
	  
   // (a^m)^n = a^(m*n)
   meta->addStatement( new GenOp( "   @ = pow( abs(@), max((@ / AL_ConstantSpecularPower),1.0f)) * @;\r\n", 
       specDecl, d_specular, smoothness, metalness));

   LangElement *specMul = new GenOp( "float4( @.rgb, 0 ) * @", specCol, specular );
   LangElement *final = specMul;

   // We we have a normal map then mask the specular 
   if( !fd.features[MFT_SpecularMap] && fd.features[MFT_NormalMap] )
   {
      Var *bumpSample = (Var*)LangElement::find( "bumpSample" );
      final = new GenOp( "@ * @.a", final, bumpSample );
   }

   // add to color
   meta->addStatement( new GenOp( "   @;\r\n", assignColor( final, Material::Add ) ) );

   output = meta;
}

ShaderFeature::Resources DeferredPixelSpecularHLSL::getResources( const MaterialFeatureData &fd )
{
   if( !fd.features[MFT_isDeferred] || !fd.features[MFT_RTLighting] )
      return Parent::getResources( fd );

   Resources res; 
   return res;
}


ShaderFeature::Resources DeferredMinnaertHLSL::getResources( const MaterialFeatureData &fd )
{
   Resources res;
   if( fd.features[MFT_isDeferred] && fd.features[MFT_RTLighting] )
   {
      res.numTex = 1;
      res.numTexReg = 1;
   }
   return res;
}

void DeferredMinnaertHLSL::setTexData( Material::StageData &stageDat,
                                       const MaterialFeatureData &fd, 
                                       RenderPassData &passData, 
                                       U32 &texIndex )
{
   if( fd.features[MFT_isDeferred] && fd.features[MFT_RTLighting] )
   {
      NamedTexTarget *texTarget = NamedTexTarget::find(RenderDeferredMgr::BufferName);
      if ( texTarget )
      {
         passData.mTexType[texIndex] = Material::TexTarget;
         passData.mSamplerNames[texIndex] = "deferredBuffer";
         passData.mTexSlot[ texIndex++ ].texTarget = texTarget;
      }
   }
}

void DeferredMinnaertHLSL::processPixMacros( Vector<GFXShaderMacro> &macros, 
                                             const MaterialFeatureData &fd  )
{
   if( fd.features[MFT_isDeferred] && fd.features[MFT_RTLighting] )
   {
      // Pull in the uncondition method for the g buffer
      NamedTexTarget *texTarget = NamedTexTarget::find( RenderDeferredMgr::BufferName );
      if ( texTarget && texTarget->getConditioner() )
      {
         ConditionerMethodDependency *unconditionMethod = texTarget->getConditioner()->getConditionerMethodDependency(ConditionerFeature::UnconditionMethod);
         unconditionMethod->createMethodMacro( String::ToLower(RenderDeferredMgr::BufferName) + "Uncondition", macros );
         addDependency(unconditionMethod);
      }
   }
}

void DeferredMinnaertHLSL::processVert(   Vector<ShaderComponent*> &componentList,
                                          const MaterialFeatureData &fd )
{
   // If there is no deferred information, bail on this feature
   if( !fd.features[MFT_isDeferred] || !fd.features[MFT_RTLighting] )
   {
      output = NULL;
      return;
   }

   // Make sure we pass the world space position to the
   // pixel shader so we can calculate a view vector.
   MultiLine *meta = new MultiLine;
   addOutWsPosition( componentList, fd.features[MFT_UseInstancing], meta );
   output = meta;
}

void DeferredMinnaertHLSL::processPix( Vector<ShaderComponent*> &componentList, 
                                       const MaterialFeatureData &fd )
{
   // If there is no deferred information, bail on this feature
   if( !fd.features[MFT_isDeferred] || !fd.features[MFT_RTLighting] )
   {
      output = NULL;
      return;
   }

   Var *minnaertConstant = new Var;
   minnaertConstant->setType( "float" );
   minnaertConstant->setName( "minnaertConstant" );
   minnaertConstant->uniform = true;
   minnaertConstant->constSortPos = cspPotentialPrimitive;

   // create texture var
   Var *deferredBuffer = new Var;
   deferredBuffer->setType( "SamplerState" );
   deferredBuffer->setName( "deferredBuffer" );
   deferredBuffer->uniform = true;
   deferredBuffer->sampler = true;
   deferredBuffer->constNum = Var::getTexUnitNum();     // used as texture unit num here


   Var* deferredTex = new Var;
   deferredTex->setName("deferredTex");
   deferredTex->setType("Texture2D");
   deferredTex->uniform = true;
   deferredTex->texture = true;
   deferredTex->constNum = deferredBuffer->constNum;

   // Texture coord
   Var *uvScene = (Var*) LangElement::find( "uvScene" );
   AssertFatal(uvScene != NULL, "Unable to find UVScene, no RTLighting feature?");

   MultiLine *meta = new MultiLine;

   // Get the world space view vector.
   Var *wsViewVec = getWsView( getInWsPosition( componentList ), meta );

   String unconditionDeferredMethod = String::ToLower(RenderDeferredMgr::BufferName) + "Uncondition";

   Var *d_NL_Att = (Var*)LangElement::find( "d_NL_Att" );

   meta->addStatement(new GenOp(avar("   float4 normalDepth = %s(@, ,@, @);\r\n", unconditionDeferredMethod.c_str()), deferredBuffer, deferredTex, uvScene));

   meta->addStatement( new GenOp( "   float vDotN = dot(normalDepth.xyz, @);\r\n", wsViewVec ) );
   meta->addStatement( new GenOp( "   float Minnaert = pow( @, @) * pow(vDotN, 1.0 - @);\r\n", d_NL_Att, minnaertConstant, minnaertConstant ) );
   meta->addStatement( new GenOp( "   @;\r\n", assignColor( new GenOp( "float4(Minnaert, Minnaert, Minnaert, 1.0)" ), Material::Mul ) ) );

   output = meta;
}


void DeferredSubSurfaceHLSL::processPix(  Vector<ShaderComponent*> &componentList, 
                                          const MaterialFeatureData &fd )
{

   Var *subSurfaceParams = new Var;
   subSurfaceParams->setType( "float4" );
   subSurfaceParams->setName( "subSurfaceParams" );
   subSurfaceParams->uniform = true;
   subSurfaceParams->constSortPos = cspPotentialPrimitive;

   Var *d_lightcolor = (Var*)LangElement::find( "d_lightcolor" );
   Var *d_NL_Att = (Var*)LangElement::find( "d_NL_Att" );

   MultiLine *meta = new MultiLine;
   if (fd.features[MFT_isDeferred])
   {
      Var* targ = (Var*)LangElement::find(getOutputTargetVarName(ShaderFeature::RenderTarget3));
      meta->addStatement(new GenOp("   @.rgb += @.rgb*@.a;\r\n", targ, subSurfaceParams, subSurfaceParams));
      output = meta;
      return;
   }

   output = meta;
}
