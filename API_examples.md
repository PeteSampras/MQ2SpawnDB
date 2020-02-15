# MQ2SpawnDB - How to link into using a plugin
Accessing the API is fairly straight forward. You just create a function to call to one of the exposed API functions and now you can access the data.

## Example API connection function
```CPP
bool DBConnected() {
	using fIsDBConnected = WORD(*)();
	PMQPLUGIN pLook = pPlugins;
	while (pLook && _strnicmp(pLook->szFilename, "mq2spawndb", 8)) pLook = pLook->pNext;
	if (pLook)
		if (fIsDBConnected checkf = (fIsDBConnected)GetProcAddress(pLook->hModule, "isDBConnected"))
			if (checkf()) return true;
	return false;
}
```
Now you can create a simple check to see if you can connect to the db:
```CPP
if (DBConnected())
    /do_something

```
## Exposed APIs

```CPP
PLUGIN_API bool isDBConnected()
{
	try {
		connection C(myconnect);
		if (C.is_open()) {
			C.close();
			//WriteChatf("Opened database successfully: %s", C.dbname());
			return true;
		}
		else {
			//WriteChatf("Can't open database");
			return false;
		}
		C.close();
	}
	catch (const std::exception & e) {
		WriteChatf("%s",e.what());
		return false;
	}
}

PLUGIN_API bool isNamed(PCHAR name)
{
	return (CheckvMaster(name, NAMED));
}

PLUGIN_API bool isQuestNPC(PCHAR name)
{
	return (CheckvMaster(name, QUEST_NPC));
}

PLUGIN_API bool isKnown(PCHAR name)
{
	return (CheckvMaster(name, KNOWN));
}

PLUGIN_API bool isIgnored(PCHAR name)
{
	return (CheckvMaster(name, IGNOREMOB));
}

PLUGIN_API bool isImmuneSnare(PCHAR name)
{
	return (CheckvMaster(name, immuneSNARE));
}

PLUGIN_API bool isImmuneSlow(PCHAR name)
{
	return (CheckvMaster(name, immuneSLOW));
}
PLUGIN_API bool isImmuneStun(PCHAR name)
{
	return (CheckvMaster(name, immuneSTUN));
}
PLUGIN_API bool isImmuneMez(PCHAR name)
{
	return (CheckvMaster(name, immuneMEZ));
}
PLUGIN_API bool isImmuneCharm(PCHAR name)
{
	return (CheckvMaster(name, immuneCHARM));
}
PLUGIN_API bool isImmuneFire(PCHAR name)
{
	return (CheckvMaster(name, immuneFIRE));
}
PLUGIN_API bool isImmuneCold(PCHAR name)
{
	return (CheckvMaster(name, immuneCOLD));
}
PLUGIN_API bool isImmunePoison(PCHAR name)
{
	return (CheckvMaster(name, immunePOISON));
}
PLUGIN_API bool isImmuneDisease(PCHAR name)
{
	return (CheckvMaster(name, immuneDISEASE));
}
PLUGIN_API bool isImmuneMagic(PCHAR name)
{
	return (CheckvMaster(name, immuneMAGIC));
}
PLUGIN_API bool Burn(PCHAR name)
{
	return (CheckvMaster(name, BURN));
}
PLUGIN_API bool ImmunityCheck(PCHAR name)
{
	return (CheckvMaster(name, IMMUNITY_CHECK));
}
PLUGIN_API char* Notes(PCHAR name)
{
	return (CheckvMasterString(name, NOTES));
}
PLUGIN_API char* BodyType(PCHAR name)
{
	return (CheckvMasterString(name, BODYTYPE));
}
```
