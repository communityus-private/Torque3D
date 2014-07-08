$serializeActions = true;

function createAIManager()
{
	%AIManager = new ScriptObject(){class="AIManager";};
	MissionCleanup.add(%AIManager);
	%AIManager.targetList = new arrayobject();
	return %AIManager;
}

function AIManager::Start(%this)
{
	%this.think(0);
}

function AIManager::onDelete(%this, %manager)
{
	%manager.targetList.delete();
}

function AIManager::think(%this,%botnum)  
{
	if (isObject(%this.bots))  
	{
		%num = %this.bots.getCount();
		if (($serializeActions)&&(%num>0))
		{
			%botnum++;
			if (%botnum>%num-1) %botnum = 0;
			%bot = %this.bots.getObject(%botnum);
			if(isObject(%bot)) %bot.aiDecide();		
		}
		else
		{
			for(%botnum = 0; %botnum<%num;%botnum++)
			{
				%bot = %this.bots.getObject(%botnum);
				if(isObject(%bot)) %bot.aiDecide();
			}
		}
	}
	if ($serializeActions) %this.schedule(32, think ,%botnum);
	else %this.schedule(500, think ,%botnum);
}

function AIManager::addBot(%this,%bot)
{
	if (!isObject(%this.bots))
		%this.bots = new SimSet();
	%this.bots.add(%bot);
}

function AIManager::addTarget(%this,%targ)
{
	%this.targetList.add(%targ);
}

function AIManager::clearTargets(%this)
{
	%this.targetList.empty();
}