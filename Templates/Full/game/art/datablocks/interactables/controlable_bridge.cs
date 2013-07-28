datablock RigidShapeData( Controlable_Bridge )
{	
   category = "Interactable";
	
   shapeFile = "art/shapes/interactables/controlable_bridge.dts";
   //go ahead and use defaults. We're locking it physically anyway.
   named = false;
   isInvincible = true;
   UseCollisonLods = true;
};

function Controlable_Bridge::onAdd(%this,%obj)
{
	%obj.freezeSim(true);
	%this.createTrigger(%obj);
	%obj.isDown = true;
}

function serverCmdToggleObject(%client,%toggleState)
{
	%obj = %client.interactingWith;
	%obj.getDatablock().toggleObject(%obj,%toggleState);
}

function Controlable_Bridge::toggleObject(%this,%obj,%toggleState)
{
	if ((%obj.isDown)&&(!%toggleState)) %this.setUp(%obj);
	else  if ((!%obj.isDown)&&(%toggleState)) %this.setDown(%obj);
}

function Controlable_Bridge::setDown(%this,%obj)
{
	if(!isObject(%obj)) return;
	%obj.isDown = true;
	%obj.stopThread(0);
	%obj.playThread( 0, "ambient" );
	%obj.setThreadPosition(0,1);
	%obj.setThreadDir(0,false);
	%obj.schedule(3000,"setActiveCollision",0);
}

function Controlable_Bridge::setUp(%this,%obj)
{
	if(!isObject(%obj)) return;
	%obj.isDown = false;
	%obj.playthread(0,"ambient");
	%obj.schedule(3000,"setActiveCollision",1);

}

function Controlable_Bridge::createTrigger(%this,%obj)
{
	%mountxy = getWords(%obj.getSlotTransform(0),0,1); 
	%mountz = getWord(%obj.getSlotTransform(0),2);
	%mountrxyz = getWords(%obj.getSlotTransform(0),3,5);
	%mountrt = mRadToDeg(getWord(%obj.getSlotTransform(0),6));  
	%pos = %mountxy SPC %mountz-2;
	%rot = %mountrxyz SPC %mountrt;
	%obj.trigger = %this.createBridgeTrigger(%pos,%rot,%obj);
}

function Controlable_Bridge::createBridgeTrigger(%this,%pos,%rot,%owner)
{
	%trigger = new Trigger()
	{
		dataBlock = BridgeTriggerData;
		name = "BridgeTrigger";
		position = %pos;
		rotation = %rot;
		scale = "4 4 4";
		polyhedron = "-0.5000000 0.5000000 0.0000000 1.0000000 0.0000000 0.0000000 0.0000000 -1.0000000 0.0000000 0.0000000 0.0000000 1.0000000";
	};
	%trigger.owner = %owner;
	if (isObject(MissionCleanup))
		MissionCleanup.add(%trigger);
	else
		schedule(300, Game, "MissionCleanup.add", %trigger);
	return %trigger;
}

//------------------------------------------------------------------
// trigger info
//------------------------------------------------------------------

datablock TriggerData(BridgeTriggerData)
{
   tickPeriodMS = 100;
};

function BridgeTriggerData::raycheck(%this,%obj,%owner)
{
	if(!isObject(%obj)) return false;
	
	%origin = %obj.getEyePoint();
	%vec = %obj.getEyeVector();
	%targPos = vectorAdd(%origin,vectorScale(%vec,2));
	
	//things to get in the way 
	%mask = $TypeMasks::VehicleObjectType;
	// see if anything gets hit
	%collision = containerRayCast(%origin, %targPos, %mask);
	//if(!%collision) return true;  
	%hit = firstWord(%collision);
	if(%hit == %owner) return true;
	return false;
}

//Rod Trigger Logic
function BridgeTriggerData::onTickTrigger(%this,%trigger)
{
	if (!isObject(%trigger.owner))
	{
		%trigger.delete();
		return;
	}
	%count = %trigger.getNumObjects();
	for (%i=0;%i<%count;%i++)
	{
		%obj = %trigger.getObject(%i);
		if ((%obj != %trigger.owner))
		{
			if (%this.raycheck(%obj,%trigger.owner))
			{
				if (!%obj.prompted)
				{
					commandToClient(%obj.client,'toggleInteractPrompt', 1);
					commandtoClient(%obj.client,'setInteractCommand',"toggleSwitchGui");
					%obj.client.interactingWith = %trigger.owner;
					commandToClient(%obj.client,'setSwitchState', %trigger.owner.isDown);
					%obj.prompted = true;
				}
			}
			else
			{
				commandToClient(%obj.client,'toggleInteractPrompt', 0);
				commandtoClient(%obj.client,'setInteractCommand',"placebo");
				commandtoClient(%obj.client,'HideSwitchGui');
				%obj.client.interactingWith = "";
				%obj.prompted = false;
			}
		}
	}
}