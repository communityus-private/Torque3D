
singleton TSShapeConstructor(MonitorDae)
{
   baseShape = "./monitor.dae";
   lodType = "TrailingNumber";
   loadLights = "0";
};

function MonitorDae::onLoad(%this)
{
   %this.setMeshSize("ColA-1 2", "-1");
   %this.renameObject("ColA-1", "ColA");
}
