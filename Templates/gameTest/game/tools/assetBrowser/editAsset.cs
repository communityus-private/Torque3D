function AssetBrowser_editAsset::saveAsset(%this)
{
   %file = AssetDatabase.getAssetFilePath(%this.editedAssetId);
   %success = TamlWrite(AssetBrowser_editAsset.editedAsset, %file);
   
   AssetBrowser.loadFilters();

   Canvas.popDialog(AssetBrowser_editAsset);
}

function AssetNameField::onReturn(%this)
{
   //if the name is different to the asset's original name, rename it!
   %i = 0;
}

function AssetBrowser::editAsset(%this)
{
   //Find out what type it is
   %assetDef = AssetDatabase.acquireAsset(EditAssetPopup.assetId);
   %assetType = %assetDef.getClassName();
   
   if(%assetType $= "MaterialAsset")
   {
      Canvas.pushDialog(ShaderEditor); 
      ShaderEditorGraph.loadGraph(%assetDef.shaderGraph);
      $ShaderGen::targetShaderFile = filePath(%assetDef.shaderGraph) @"/"@fileBase(%assetDef.shaderGraph);
   }
   else if(%assetType $= "StateMachineAsset")
   {
      eval("AssetBrowser.tempAsset = new " @ %assetDef.getClassName() @ "();");
      AssetBrowser.tempAsset.assignFieldsFrom(%assetDef);
      
      SMAssetEditInspector.inspect(AssetBrowser.tempAsset);  
      AssetBrowser_editAsset.editedAssetId = EditAssetPopup.assetId;
      AssetBrowser_editAsset.editedAsset = AssetBrowser.tempAsset;
      
      //remove some of the groups we don't need:
      for(%i=0; %i < SMAssetEditInspector.getCount(); %i++)
      {
         %caption = SMAssetEditInspector.getObject(%i).caption;
         
         if(%caption $= "Ungrouped" || %caption $= "Object" || %caption $= "Editing" 
            || %caption $= "Persistence" || %caption $= "Dynamic Fields")
         {
            SMAssetEditInspector.remove(SMAssetEditInspector.getObject(%i));
            %i--;
         }
      }
   
      Canvas.pushDialog(StateMachineEditor);
      %file = AssetDatabase.acquireAsset(EditAssetPopup.assetId).stateMachineFile;
      StateMachineEditor.loadStateMachineFile(%file);
      StateMachineEditor-->Window.text = "State Machine Editor ("@EditAssetPopup.assetId@")";
   }
   else if(%assetType $= "ComponentAsset")
   {
      %assetDef = AssetDatabase.acquireAsset(EditAssetPopup.assetId);
      %scriptFile = %assetDef.scriptFile;
      
      EditorOpenFileInTorsion(makeFullPath(%scriptFile), 0);
   }
   else if(%assetType $= "GameObjectAsset")
   {
      %assetDef = AssetDatabase.acquireAsset(EditAssetPopup.assetId);
      %scriptFile = %assetDef.scriptFilePath;
      
      EditorOpenFileInTorsion(makeFullPath(%scriptFile), 0);
   }
   else if(%assetType $= "ShapeAsset")
   {
      %this.hideDialog();
      ShapeEditorPlugin.openShapeAsset(EditAssetPopup.assetId);  
   }
}

function AssetBrowser::editAssetInfo(%this)
{
   Canvas.pushDialog(AssetBrowser_editAsset); 
   
   %assetDef = AssetDatabase.acquireAsset(EditAssetPopup.assetId);
   
   eval("AssetBrowser.tempAsset = new " @ %assetDef.getClassName() @ "();");
   AssetBrowser.tempAsset.assignFieldsFrom(%assetDef);
   
   AssetEditInspector.inspect(AssetBrowser.tempAsset);  
   AssetBrowser_editAsset.editedAssetId = EditAssetPopup.assetId;
   AssetBrowser_editAsset.editedAsset = AssetBrowser.tempAsset;
   
   //remove some of the groups we don't need:
   for(%i=0; %i < AssetEditInspector.getCount(); %i++)
   {
      %caption = AssetEditInspector.getObject(%i).caption;
      
      if(%caption $= "Ungrouped" || %caption $= "Object" || %caption $= "Editing" 
         || %caption $= "Persistence" || %caption $= "Dynamic Fields")
      {
         AssetEditInspector.remove(AssetEditInspector.getObject(%i));
         %i--;
      }
   }
}

function AssetBrowser::renameAsset(%this)
{
   //Find out what type it is
   %assetDef = AssetDatabase.acquireAsset(EditAssetPopup.assetId);
   
   AssetBrowser.selectedAssetPreview-->AssetNameLabel.setFirstResponder();
}