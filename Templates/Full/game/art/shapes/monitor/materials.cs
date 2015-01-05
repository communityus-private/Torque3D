new GuiTextureCanvas( MonitorCanvas )   
{  
   guiControl = "optionsDlg";
};  

singleton Material(ScreensMaterial)
{
	mapTo = "Screen1";
   diffuseMap[0] = "core/art/white.jpg";
	diffuseMap[1] = "$guiMonitorCanvas";
   emissive[0] = "1";
   materialTag0 = "GUIs";
   emissive[1] = "1";
   diffuseColor[0] = "0.368627 0.368627 0.368627 1";
};

singleton Material(monitorMaterial)
{
   mapTo = "monitor_mat";
   diffuseColor[0] = "0.512 0.512 0.512 1";
   specular[0] = "0.25 0.25 0.25 1";
   diffuseMap[0] = "core/art/white.jpg";
   pixelSpecular[0] = "1";
};

singleton Material(monitor_frame)
{
   mapTo = "monitor_frame";
   diffuseColor[0] = "0.512 0.512 0.512 1";
   specular[0] = "0.125 0.125 0.125 1";
   specularPower[0] = "50";
   translucentBlendOp = "None";
};
