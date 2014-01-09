$debug_FileIO = true;

//save record register value
$saveRecord = new arrayobject();

//tells the client to save the file it'sself.
//first re-saves the ConnectionData they selected prior to connection
//then inserts any new entries
function clientCmdwriteSaveRemote(%name,%metaKey,%metaVal)
{
	getPrefpath();
	readSave(%name);
	%file = new FileObject();
	if(%file.openForWrite($prefpath @"/saves/" @ %name @ ".sav"))
	{
		%file.writeLine($saveRecord.getKey(0));
		insertSaveline(%metaKey,%metaVal);
		SetMetaData(%file);
		%file.close();
	}
	else
	{
		if ($debug_FileIO) error( "Failed to open " @ %name @ " for writing" );
	}
	%file.delete();
}

//writes a save file based on a connected client
function writeSave(%client)
{
	getPrefpath();
	%name = %client.saveName;
	readSave(%name);
	%file = new FileObject();
	if(%file.openForWrite($prefpath @"/saves/" @ %name @ ".sav"))
	{
		SetConnectionData(%file,%client);
		
		for (%metaKey = 0; %metaKey < %client.MetaInfoCount; %metaKey++)
		{
			%metaVal = $metaVal;
			insertSaveline(%client.MetaInfo[%metaKey],%metaVal);
		}
		SetMetaData(%file);
		%file.close();
	}
	else
	{
		if ($debug_FileIO) error( "Failed to open " @ %name @ " for writing" );
	}
	%file.delete();
}

//reads a save file based on either a save file name, or connected client
function readSave(%name,%client)
{
	$saveRecord.empty();
	%file = new FileObject();
	if(%file.openForRead(getPrefpath() @"/saves/" @ %name @ ".sav"))
	{
		GetConnectionData(%file,%client);
		GetMetaData(%file,%client);
		return true;
	}
	else
	{
		return false;
	}
}

//connection-specific storage. writes it as a single line header entry. 
//Used for client selectable options.
function SetConnectionData(%file,%client)
{
	%line = %client.connectionData;
	%file.writeLine(%line);
}

//connection-specific storage. reads it as a single line header entry, 
//and stores it 2 places for manipulation: a $saveRecord, and if provided, on a client connection
//Used for client selectable options.
function GetConnectionData(%file,%client)
{
	if (!%file.isEof()) %header = %file.readLine();
	$saveRecord.add(%header);
	if (%client !$= "") %client.connectionData = %header;
}

//inserts a single key = value entry into the current $saveRecord
function insertSaveline(%metaKey,%metaVal)
{
	if (%metaKey $= "") return;
	%lineID = $saveRecord.getIndexFromKey(%metaKey);
	if (%lineID != -1)
	{
		if ($saveRecord.getValue(%lineID) !$= %metaVal)
			$saveRecord.setValue(%metaVal,%lineID);
	}
	else $saveRecord.add(%metaKey,%metaVal);
	if ($debug_FileIO) error("KEY, VAL:" SPC %metaKey SPC %metaVal);
}

//writes all data from a $saveRecord register to a file
function SetMetaData(%file)
{
	%count = $saveRecord.count();
	for(%i=1;%i<%count;%i++)
	{
		%line = $saveRecord.getKey(%i) SPC $saveRecord.getValue(%i);
		%file.writeLine(%line);
	}
}

//connection-agnostic storage retrieval. reads it as a series of entries, 
//and if provided, sets it to a client connection
//in the form MetaInfo[entryKey] = entryValue
//in addition to the $saveRecord array register
function GetMetaData(%file,%client)
{
	%metaKey = -1;
	while (!%file.isEof())
	{
		%temp = %file.readLine();
		$saveRecord.add(getword(%temp,0),getword(%temp,1));
		if (%client !$= "")
		{
			%client.MetaInfo[getword(%temp,0)] = getword(%temp,1);
			%metakey++;
		}
	}
	if (%client !$= "") %client.MetaInfoCount = %metakey;
}