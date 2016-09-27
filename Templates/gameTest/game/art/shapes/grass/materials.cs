
singleton Material(grass_replace_me)
{
   mapTo = "grass_replace_me";
   diffuseMap[0] = "art/shapes/grass/images/Grass_Texture";
   smoothness[0] = "1";
   metalness[0] = "50";
   translucent = "1";
   normalMap[0] = "art/shapes/grass/images/Grass_Texture_Normal.png";
   specularMap[0] = "art/shapes/grass/images/Grass_Texture_Composite.png";
   doubleSided = "1";
   alphaRef = "73";
   pixelSpecular0 = "0";
   castShadows = "0";
};

singleton Material(grass_texture)
{
   mapTo = "Grass_Texture";
   diffuseMap[0] = "art/shapes/grass/images/Grass_Texture";
   normalMap[0] = "art/shapes/grass/images/Grass_Texture_Normal.png";
   smoothness[0] = "1";
   metalness[0] = "50";
   invertSmoothness[0] = "0";
   specularMap[0] = "art/shapes/grass/images/Grass_Texture_Composite.png";
   useAnisotropic[0] = "0";
   doubleSided = "1";
   castShadows = "0";
   translucentBlendOp = "LerpAlpha";
   alphaTest = "1";
   alphaRef = "107";
   pixelSpecular0 = "0";
};


singleton Material(MedGrass5m_Grass_Texture)
{
   mapTo = "unmapped_mat";
   diffuseMap[0] = "art/shapes/grass/images/Grass_Texture";
   smoothness[0] = "1";
   metalness[0] = "50";
   translucent = "1";
   pixelSpecular0 = "0";
   materialTag0 = "Miscellaneous";
};
