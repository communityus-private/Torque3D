function AssetBrowser::renameModule(%this)
{
   
}

function AssetBrowser::reloadModule(%this)
{
   ModuleDatabase.unregisterModule(AssetBrowser.SelectedModule, 1);
   ModuleDatabase.loadExplicit(AssetBrowser.SelectedModule);
}

function AssetBrowser::deleteModule(%this)
{
   
}