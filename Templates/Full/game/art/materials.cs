singleton Material(CornellBox_cornell_red)
{
   mapTo = "cornell_red";
   diffuseColor[0] = "0.64 0.00124607 0 1";
   roughness[0] = "0";
   metalness[0] = "1";
   translucentBlendOp = "None";
   materialTag0 = "Miscellaneous";
};

singleton Material(CornellBox_cornell_grey)
{
   mapTo = "cornell_grey";
   diffuseColor[0] = "0.996078 0.996078 0.996078 1";
   roughness[0] = "1";
   metalness[0] = "1";
   translucentBlendOp = "None";
   materialTag0 = "Miscellaneous";
};

singleton Material(CornellBox_cornell_grey)
{
   mapTo = "cornell_grey";
   diffuseColor[0] = "0.996078 0.996078 0.996078 1";
   roughness[0] = "1";
   metalness[0] = "1";
   translucentBlendOp = "None";
   effectColor[1] = "InvisibleBlack";
   effectColor[0] = "0 0 0 0";
   materialTag0 = "Miscellaneous";
};

singleton Material(CornellBox_cornell_green)
{
   mapTo = "cornell_green";
   diffuseColor[0] = "0.00675477 0.64 0 1";
   roughness[0] = "1";
   metalness[0] = "0";
   translucentBlendOp = "None";
   materialTag0 = "Miscellaneous";
};
   
singleton Material(PBRstone_mat)
{
   mapTo = "PBRstone";
   diffuseMap[0] = "art/shapes/textures/Stone_A.dds";
   materialTag0 = "Miscellaneous";
   normalMap[0] = "art/shapes/textures/Stone_N.png";
   roughMap[0] = "art/shapes/textures/Stone_S.dds";
   metalMap[0] = "art/shapes/textures/Stone_S.dds";
   metalChan[0] = "3";
};
