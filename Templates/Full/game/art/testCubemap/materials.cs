singleton CubemapData( TestCubemap )
{
   cubeFace[0] = "art/testCubemap/skybox_1";
   cubeFace[1] = "art/testCubemap/skybox_2";
   cubeFace[2] = "art/testCubemap/skybox_3";
   cubeFace[3] = "art/testCubemap/skybox_4";
   cubeFace[4] = "art/testCubemap/skybox_5";
   cubeFace[5] = "art/testCubemap/skybox_6";
};

singleton Material( TestCubemapMat )
{
   cubemap = TestCubemap;
};