
singleton TSShapeConstructor(Controlable_bridgeDae)
{
   baseShape = "./controlable_bridge.dae";
   lodType = "TrailingNumber";
   loadLights = "0";
};

function Controlable_bridgeDae::onLoad(%this)
{
   %this.setMeshSize("colBoxA 1", "-1");
   %this.setMeshSize("ColBoxB 1", "-1");
   %this.setMeshSize("Controlbox 2", "-2");
   %this.setMeshSize("Controlbox -2", "2");
   %this.setMeshSize("colBoxA 2", "-2");
   %this.setMeshSize("ColBoxB 2", "-2");
   %this.removeNode("Lamp");
   %this.removeNode("Camera");
}

singleton TSShapeConstructor(Controlable_bridgeDts)
{
   baseShape = "./controlable_bridge.dts";
};

function Controlable_bridgeDts::onLoad(%this)
{
   %this.setMeshSize("ColBoxB 1", "-1");
   %this.setMeshSize("colBoxA 1", "-1");
   %this.setMeshSize("colBoxA 2", "-2");
   %this.setMeshSize("ColBoxB 2", "-2");
   %this.setSequenceCyclic("ambient", "0");
}
