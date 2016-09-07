

singleton Material(gr55)
{
   mapTo = "Radar_gr55";
   diffuseMap[0] = "art/radarMesh/gr55.tga";
   normalMap[0] = "art/radarMesh/materialLib/flatNormal.dds";
   specularMap[0] = "art/radarMesh/materialLib/gravel/gravelComposite.dds";
   pixelSpecular0 = "0";
};

singleton Material(grass_wip2_Material_003)
{
   mapTo = "Material.003";
   diffuseColor[0] = "0.0336221 0.0325533 0.0317185 1";
   smoothness[0] = "1";
   metalness[0] = "50";
   translucentBlendOp = "None";
};


singleton Material(grass_texture)
{
   mapTo = "grass_texture";
   diffuseMap[0] = "art/images/brightGrassTexture";
   normalMap[0] = "art/radarMesh/materialLib/flatNormal.dds";
   smoothness[0] = "1";
   metalness[0] = "50";
   invertSmoothness[0] = "1";
   specularMap[0] = "art/radarMesh/materialLib/grass/grassComposite.dds";
   useAnisotropic[0] = "1";
   doubleSided = "1";
   translucentBlendOp = "None";
   alphaTest = "1";
   alphaRef = "80";
   pixelSpecular0 = "0";
   castShadows = "0";
};