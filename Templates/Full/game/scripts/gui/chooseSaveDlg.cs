//----------------------------------------
function ChooseSaveDlg::onWake( %this )
{
	ChooseSaveDlg.refreshSavesList();
	newcharOptions.setVisible(false);
	newcharBtn.setVisible(true);
}

function ChooseSaveDlg::refreshSavesList()
{
	ChooseSaveWindow->SaveGamePreviews.clear();
	%i=0;
	for( %file = findFirstFile( getPrefpath() @"/saves/*.sav" ); %file !$= ""; %file = findNextFile() )
	{
		%name = fileBase(%file);
		readSave(%name);
		
		%count = $saveRecord.count();
		%type = getword($saveRecord.getKey(0),0);
		%lastlev = $saveRecord.getKey(%count-1);
		%levscore = $saveRecord.getValue(%count-1);
		%line = %type @","@ %lastlev @","@ %levscore;
		//error(%count SPC %line);
		
		%cmd = "chooseSaveDlg.setCharachterVars("@ fileBase(%file) @","@ %line @");";
		%cmd2 = "chooseSaveDlg.killCharachter("@ fileBase(%file) @");";
		%preview = new GuiContainer() {
			internalName = "guiSavegameGroup" @ %i;
			Extent = "280 32";
		};
		ChooseSaveWindow->SaveGamePreviews.add(%preview);
		%pvName = new guiButtonCtrl() {
			internalName = "guiSavegameName" @ %i;
			Extent = "200 14";
			text = fileBase(%file);
			command = %cmd;
		};
		%preview.add(%pvName);
		%pvtype = new guiBitmapCtrl() {
			internalName = "guiSavegameType" @ %i;
			Position = "0 16";
			Extent = "16 16";
			bitmap = "art/gui/charIcons/ico" @ %type;
		};
		%preview.add(%pvtype);
		%weapTier = getword($saveRecord.getKey(0),1);
		%mainweap = new guiBitmapCtrl() {
			internalName = "guiSavegameType" @ %i;
			Position = "16 16";
			Extent = "16 16";
			bitmap = "art/gui/charIcons/" @ %type @"/weapIco" @ 0 @ %weapTier;
		};
		%preview.add(%mainweap);
		%depWeapTier = getword($saveRecord.getKey(0),1);
		%depWeap = new guiBitmapCtrl() {
			internalName = "guiSavegameType" @ %i;
			Position = "32 16";
			Extent = "16 16";
			bitmap = "art/gui/charIcons/" @ %type @"/weapIco" @ 1 @ %depWeapTier;
		};
		%preview.add(%depWeap);
		
		%refnum = 2;
		for(%y = 0;%y<3;%y++)
		{
			for(%x = 0;%x<3;%x++)
			{
				%refnum++;
				%xyEntry = getword($saveRecord.getKey(0),%refnum);
				%offset =  %refnum*16 SPC 16;
				%tech = new guiBitmapCtrl() {
					internalName = "guiSavegameTech" @ %x @ %y @ %i;
					Position = %offset;
					Extent = "16 16";
					bitmap = "art/gui/charIcons/" @ %type @"/techIco" @ %x @ %y @ %xyEntry;
				};
				%preview.add(%tech);
			}
		}
		%pvScore = new guiTextCtrl() {
			internalName = "guiSavegameScore" @ %i;
			Position = "200 0";
			Extent = "80 16";
			text = %levscore;
			profile = "GuiprogressTextProfile";
		};
		%preview.add(%pvScore);
		%pvLevel = new guiTextCtrl() {
			internalName = "guiSavegameLevel" @ %i;
			Position = "280 0";
			Extent = "60 16";
			text = %lastlev;
			profile = "GuiprogressTextProfile";
		};
		%preview.add(%pvLevel);
		%pvDelSave = new guiButtonCtrl() {
			internalName = "guiKillSave" @ %i;
			Position = "348 0";
			Extent = "32 32";
			text = "X";
			command = %cmd2;
		};
		%preview.add(%pvDelSave);
		%i++;
	}
	if (%i > 5)
	{
		scrollSavesDownbtn.setVisible(true);
		scrollSavesUpbtn.setVisible(true);
	}
	ChooseSaveDlg.refreshSavesListPosition();
}
function ChooseSaveDlg::refreshSavesListPosition()
{
	ChooseSaveWindow->SaveGamePreviews.firstVisible = -1;
	ChooseSaveWindow->SaveGamePreviews.lastVisible = -1;
	if (ChooseSaveWindow->SaveGamePreviews.getCount() > 0)
	{
		ChooseSaveWindow->SaveGamePreviews.firstVisible = 0;

		if (ChooseSaveWindow->SaveGamePreviews.getCount() < 6)
			ChooseSaveWindow->SaveGamePreviews.lastVisible = ChooseSaveWindow->SaveGamePreviews.getCount() - 1;
		else
		ChooseSaveWindow->SaveGamePreviews.lastVisible = 4;
	}
}

function ChooseSaveWindow::previousPreviews(%this)
{
   %prevHiddenIdx = %this->SaveGamePreviews.firstVisible - 1;

   if (%prevHiddenIdx < 0)
      return;

   %lastVisibleIdx = %this->SaveGamePreviews.lastVisible;

   if (%lastVisibleIdx >= %this->SaveGamePreviews.getCount())
      return;

   %prevHiddenObj  = %this->SaveGamePreviews.getObject(%prevHiddenIdx);
   %lastVisibleObj = %this->SaveGamePreviews.getObject(%lastVisibleIdx);

   if (isObject(%prevHiddenObj) && isObject(%lastVisibleObj))
   {
      %this->SaveGamePreviews.firstVisible--;
      %this->SaveGamePreviews.lastVisible--;

      %prevHiddenObj.setVisible(true);
      %lastVisibleObj.setVisible(false);
   }
}

function ChooseSaveWindow::nextPreviews(%this)
{
   %firstVisibleIdx = %this->SaveGamePreviews.firstVisible;

   if (%firstVisibleIdx < 0)
      return;

   %firstHiddenIdx = %this->SaveGamePreviews.lastVisible + 1;

   if (%firstHiddenIdx >= %this->SaveGamePreviews.getCount())
      return;

   %firstVisibleObj = %this->SaveGamePreviews.getObject(%firstVisibleIdx);
   %firstHiddenObj  = %this->SaveGamePreviews.getObject(%firstHiddenIdx);

   if (isObject(%firstVisibleObj) && isObject(%firstHiddenObj))
   {
      %this->SaveGamePreviews.firstVisible++;
      %this->SaveGamePreviews.lastVisible++;

      %firstVisibleObj.setVisible(false);
      %firstHiddenObj.setVisible(true);
   }
}

function ChooseSaveDlg::makeNewChar()
{
	newcharOptions.setVisible(true);
	newcharBtn.setVisible(false);
}

function ChooseSaveDlg::cancelNewChar()
{
	newcharOptions.setVisible(false);
	newcharBtn.setVisible(true);
}

function ChooseSaveDlg::createNewChar()
{
	%name = newCharName.getText();
	$pref::Player::Name = %name;
	
	PlayAsBtn.setText($pref::Player::Name);
	if (readSave($pref::Player::Name))
	{
		MessageBoxYesNo( "Existing Character", "Overwrite?", "overwriteCharfile();", "");
		return;
	}
	ChooseSaveDlg.writeEmptyCharFile(%name);
	ChooseSaveDlg.refreshSavesList();
	newcharOptions.setVisible(false);
	newcharBtn.setVisible(true);
}

function overwriteCharfile()
{
	%name = $pref::Player::Name;
	ChooseSaveDlg.writeEmptyCharFile(%name);
	ChooseSaveDlg.refreshSavesList();
	newcharOptions.setVisible(false);
	newcharBtn.setVisible(true);
}

function ChooseSaveDlg::writeEmptyCharFile(%this,%name)
{
	%file = new FileObject();
	if(%file.openForWrite(getPrefpath()@ "/saves/" @ %name @ ".sav"))
	{
		%file.writeLine($PlayerType SPC 0 SPC 0 SPC 0 SPC 0 SPC 0 SPC 0 SPC 0 SPC 0  SPC 0 SPC 0 SPC 0 );
		%levelname = "CityScape";
		%line = %levelname SPC "0";
		%file.writeLine(%line);
		%file.close();
	}
	else
	{
		error( "Failed to open " @ %name @ " for writing" );
	}
	%file.delete();
}

function ChooseSaveDlg::setCharachterVars(%this,%name,%type,%lastlevel,%score)
{
	$pref::Player::Name = %name;
	$PlayerType = %type;
	PlayAsBtn.setText($pref::Player::Name);
	readSave($pref::Player::Name);
	$connectionData = $saveRecord.getKey(0);
	Canvas.popDialog(ChooseSaveDlg);
}

function ChooseSaveDlg::killCharachter(%this,%name)
{
	%yesString = "yesKillSave("@ %name @");";
	MessageBoxYesNo("Retire Character?", "Are you sure you want to send this character to the Scrap Heap?", %yesString, "");
}

function yesKillSave(%name)
{
	error("MOIDA HIIIIM! /points at "@ %name);
	%fileName = getPrefpath() @"/saves/"@ %name @".sav";
	fileDelete(%fileName);
	ChooseSaveDlg.refreshSavesList();
}

function closeSaveDlg()
{
	Canvas.popDialog(ChooseSaveDlg);
	readSave($pref::Player::Name);
	$connectionData = $saveRecord.getKey(0);
	Canvas.popDialog(ChooseSaveDlg);
}

function chooseCharType(%val)
{
	$PlayerType = %val;
	for (%i=0;%i<4;%i++)
	{
		%guielement = "chartype" @ %i;
		if (%i == %val)
		{
			%guielement.setbitmap("art/gui/charIcons/ico" @ %i);
		}
		else
		{
			%guielement.setbitmap("art/gui/charIcons/ico" @ %i @ "_grey");
		
		}
	}
}