
singleton Material(GreyGrid)
{
   mapTo = "Material";
   diffuseColor[0] = "0.996078 0.988235 0.988235 1";
   smoothness[0] = "1";
   metalness[0] = "50";
   translucentBlendOp = "None";
   diffuseMap[0] = "core/art/grids/512_grey.png";
   specularMap[0] = "core/art/grids/512_green.png";
   pixelSpecular0 = "0";
};

singleton Material(Red)
{
   mapTo = "Red";
   diffuseColor[0] = "0.996078 0.988235 0.988235 1";
   smoothness[0] = "1";
   metalness[0] = "50";
   translucentBlendOp = "None";
   diffuseMap[0] = "modules/Characters/Images/SkinAlbedo.png";
   normalMap[0] = "modules/Characters/Images/FlatNormal.png";
   specularMap[0] = "modules/Characters/Images/SkinComposite.png";
   pixelSpecular0 = "0";
   castDynamicShadows = "1";
};
