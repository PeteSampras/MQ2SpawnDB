// MQ2SpawnDB.cpp : Defines the entry point for the DLL application.
//

// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup, do NOT do it in DllMain.

//#define MMOBUGS_LOADER

// notes: pqxx-7.0.dll and libpq.dll has to go in EQ directory.

#ifdef MMOBUGS_LOADER
#include "../MQ2Plugin.h"
#include "../MMOBugs.h"
#ifdef MMOBUGS_LOADER
#include "../MQ2MMOBugs/CSL.h"
#endif
#else
#include "../MQ2Plugin.h"
#endif

#pragma comment(lib,"libpq.lib")
#pragma comment(lib,"pqxx-7.0.lib")
#include <pqxx/pqxx>
#include <Shlwapi.h>
#include <vector>
#pragma comment(lib, "Shlwapi.lib")
#include <stdlib.h>

using namespace std;
using namespace pqxx;

#ifdef MMOBUGS
#define REDEFINE 1
#undef MMOBUGS
#endif

#define ISINDEX() (Index[0])
#define ISNUMBER() (IsNumber(Index))
#define GETNUMBER() (atoi(Index))
#define GETFIRST() Index

extern char MYPLUGIN_NAME[MAX_PATH] = "MQ2SpawnDB"; 
PreSetup("MQ2SpawnDB");
#ifdef REDEFINE
#define MMOBUGS 1
#endif

/*
Notes:
All caught up for basic usage.


*/



char dbname[100] = { 0 }, dbuser[100] = { 0 }, dbpassword[200] = { 0 }, dbhost[30] = { 0 }, dbport[10] = { 0 };
//char dbname[100] = "testdb", dbuser[100] = "test", dbpassword[200] = "password", dbhost[30] = "127.0.0.1", dbport[10] = "5432";
char myconnect[MAX_STRING] = { 0 }, INISection[MAX_STRING] = { 0 };
char NPC_Table_Columns[MAX_STRING] = "NAME, MAX_LEVEL, MIN_LEVEL, QUEST_NPC, NAMED, IGNORE, immuneSNARE, immuneSLOW, immuneCHARM, immuneMEZ, immuneFIRE, immuneCOLD, immunePOISON, immuneDISEASE, BODYTYPE, IMMUNITY_CHECK, BURN, NOTES, immuneSTUN, immuneMAGIC";
bool CurrentZoneTableExists = false;


// get environmental variables if people dont want to include db data in INI
int GetEnvVariables(PCHAR EnvVarName)
{
	/*
	PGDATABASE	(name of database; defaults to your user name)
	PGHOST		(database server; defaults to local machine)
	PGPORT		(TCP port to connect to; default is 5432)
	PGUSER		(your PostgreSQL user ID; defaults to your login name)
	PGPASSWORD	(your PostgreSQL password, if needed)
	*/
	try {
		char *buf = nullptr;
		size_t sz = 0;
		if (_dupenv_s(&buf, &sz, EnvVarName) == 0 && buf != nullptr)
		{
			if (!_stricmp(EnvVarName, "PGDATABASE"))
				sprintf_s(dbname, buf);
			if (!_stricmp(EnvVarName, "PGHOST"))
				sprintf_s(dbhost, buf);
			if (!_stricmp(EnvVarName, "PGPORT"))
				sprintf_s(dbport, buf);
			if (!_stricmp(EnvVarName, "PGUSER"))
				sprintf_s(dbuser, buf);
			if (!_stricmp(EnvVarName, "PGPASSWORD"))
				sprintf_s(dbpassword, buf);
			//WriteChatf("%s = %s", EnvVarName, buf);
			//sprintf_s((char*)setting, sizeof(setting), buf);
			//sprintf_s(setting, buf);
			//free(buf);
			return 1;
		}
		return 0;
		}
	catch (const std::exception & e) {
		WriteChatf("%s: %s", __FUNCTION__, e.what());
		return 0;
	}
}



struct _SpawnCheck {
	char displayname[64];
	bool Quest = false;
	bool named = false;
	char body[64] = { 0 };
	int maxlevel;
	int minlevel;
	bool known;
} SpawnCheck, PSpawnCheck;

struct _SpawnMaster {
	char	Name[64];
	int		MaxLevel;
	int		MinLevel;
	char	Notes[MAX_STRING];
	bool	Quest;
	bool	Named;
	bool	Ignore;
	bool	ImmuneCharm;
	bool	ImmuneSnare;
	bool	ImmuneSlow;
	bool	ImmuneMez;
	bool	ImmuneFire;
	bool	ImmuneCold;
	bool	ImmunePoison;
	bool	ImmuneDisease;
	char	BodyType[64];
	bool	ImmunityCheck;
	bool	Burn;
	bool	ImmuneStun;
	bool	ImmuneMagic;
} SpawnMaster, PSpawnMaster;

vector<_SpawnCheck> vSpawns;
vector<_SpawnMaster> vMaster;

enum TABLETYPE { SPAWNS = 1, PC = 2 };
enum SQL_FIELDS {	ID, 	NAME,	MAX_LEVEL,	MIN_LEVEL,	NOTES,	QUEST_NPC,	NAMED,	IGNOREMOB,	immuneSNARE,	
	immuneSLOW,	immuneCHARM,	immuneMEZ,	immuneFIRE,	immuneCOLD,	immunePOISON,	immuneDISEASE,	BODYTYPE,	
	IMMUNITY_CHECK,	BURN, immuneSTUN, immuneMAGIC};

void CreateTable(int TableType, PCHAR szLine);

inline bool InGameOK()
{
	return(GetGameState() == GAMESTATE_INGAME && GetCharInfo() && GetCharInfo()->pSpawn && GetCharInfo2());
}

int FindSpawnInMaster(PSPAWNINFO pSpawn)
{
	int vSize = vMaster.size();
	for (int i = 0; i < vSize; i++)
	{
		if (!_stricmp(vMaster[i].Name, pSpawn->DisplayedName))
		{
			return i + 1;
		}
	}
	return 0;
}


#pragma region TLO

class MQ2SpawnDBType* pSpawnDBTypes = nullptr;
class MQ2SpawnDBType : public MQ2Type
{
private:
	char Tempos[MAX_STRING] = { 0 };
public:
	enum SpawnDBMembers
	{
		Known = 1,
		MinLevel,
		MaxLevel,
		Named,
		Quest,
		Notes,
		Ignore,
		ImmuneCold,
		ImmuneDisease,
		ImmuneFire,
		ImmuneMez,
		ImmuneCharm,
		ImmunePoison,
		ImmuneSlow,
		ImmuneSnare,
		ImmunityCheck,
		Burn,
		Body,
		SpawnTime,
		Size,
		ImmuneStun,
		ImmuneMagic
	};
	MQ2SpawnDBType() :MQ2Type("SpawnDB")
	{
		TypeMember(Known);
		TypeMember(Named);
		TypeMember(Quest);
		TypeMember(Notes);
		TypeMember(Ignore);
		TypeMember(ImmuneCold);
		TypeMember(ImmuneDisease);
		TypeMember(ImmuneFire);
		TypeMember(ImmuneMez);
		TypeMember(ImmuneCharm);
		TypeMember(ImmunePoison);
		TypeMember(ImmuneSlow);
		TypeMember(ImmuneSnare);
		TypeMember(ImmunityCheck);
		TypeMember(Burn);
		TypeMember(Body);
		TypeMember(SpawnTime);
		TypeMember(Size);
		TypeMember(ImmuneStun);
		TypeMember(ImmuneMagic);
	};
	bool GetMember(MQ2VARPTR VarPtr, PCHAR Member, PCHAR Index, MQ2TYPEVAR& Dest)
	{
		PMQ2TYPEMEMBER pMember = MQ2SpawnDBType::FindMember(Member);
		if (pMember)
		{
			switch ((SpawnDBMembers)pMember->ID)
			{
			case Size:
				Dest.Type = pIntType;
				Dest.DWord = vMaster.size();
				return true;
			case Known: // return id in case we want to do something with that
				if (ISINDEX())
				{
					if (ISNUMBER())
					{
						DWORD id = GETNUMBER();
						if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(GETNUMBER()))
						{
							if (int found = FindSpawnInMaster(pSpawn))
							{
								Dest.Type = pIntType;
								Dest.DWord = pSpawn->SpawnID;
								return true;
							}
						}
						else
							return false;
					}
					else
					{
						if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByPartialName(GETFIRST()))
						{
							if (int found = FindSpawnInMaster(pSpawn))
							{
								Dest.Type = pIntType;
								Dest.DWord = pSpawn->SpawnID;
								return true;
							}
						}
						else
							return false;
					}
				}
				return false;
			case Named:
				Dest.Type = pBoolType;
				Dest.DWord = false;
				if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByPartialName(GETFIRST()))
				{
					if (int found = FindSpawnInMaster(pSpawn))
					{
						Dest.Type = pBoolType;
						Dest.DWord = vMaster[found - 1].Named;
						return true;
					}
				}
				return true;
			case Quest:
				Dest.Type = pBoolType;
				Dest.DWord = false;
				if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByPartialName(GETFIRST()))
				{
					if (int found = FindSpawnInMaster(pSpawn))
					{
						Dest.Type = pBoolType;
						Dest.DWord = vMaster[found - 1].Quest;
						return true;
					}
				}
				return true;
			case Ignore:
				Dest.Type = pBoolType;
				Dest.DWord = false;
				if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByPartialName(GETFIRST()))
				{
					if (int found = FindSpawnInMaster(pSpawn))
					{
						Dest.Type = pBoolType;
						Dest.DWord = vMaster[found - 1].Ignore;
						return true;
					}
				}
				return true;
			case ImmuneCold:
				Dest.Type = pBoolType;
				Dest.DWord = false;
				if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByPartialName(GETFIRST()))
				{
					if (int found = FindSpawnInMaster(pSpawn))
					{
						Dest.Type = pBoolType;
						Dest.DWord = vMaster[found - 1].ImmuneCold;
						return true;
					}
				}
				return true;
			case ImmuneDisease:
				Dest.Type = pBoolType;
				Dest.DWord = false;
				if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByPartialName(GETFIRST()))
				{
					if (int found = FindSpawnInMaster(pSpawn))
					{
						Dest.Type = pBoolType;
						Dest.DWord = vMaster[found - 1].ImmuneDisease;
						return true;
					}
				}
				return true;
			case ImmuneFire:
				Dest.Type = pBoolType;
				Dest.DWord = false;
				if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByPartialName(GETFIRST()))
				{
					if (int found = FindSpawnInMaster(pSpawn))
					{
						Dest.Type = pBoolType;
						Dest.DWord = vMaster[found - 1].ImmuneFire;
						return true;
					}
				}
				return true;
			case ImmuneMez:
				Dest.Type = pBoolType;
				Dest.DWord = false;
				if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByPartialName(GETFIRST()))
				{
					if (int found = FindSpawnInMaster(pSpawn))
					{
						Dest.Type = pBoolType;
						Dest.DWord = vMaster[found - 1].ImmuneMez;
						return true;
					}
				}
				return true;
			case ImmuneCharm:
				Dest.Type = pBoolType;
				Dest.DWord = false;
				if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByPartialName(GETFIRST()))
				{
					if (int found = FindSpawnInMaster(pSpawn))
					{
						Dest.Type = pBoolType;
						Dest.DWord = vMaster[found - 1].ImmuneCharm;
						return true;
					}
				}
				return true;
			case ImmunePoison:
				Dest.Type = pBoolType;
				Dest.DWord = false;
				if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByPartialName(GETFIRST()))
				{
					if (int found = FindSpawnInMaster(pSpawn))
					{
						Dest.Type = pBoolType;
						Dest.DWord = vMaster[found - 1].ImmunePoison;
						return true;
					}
				}
				return true;
			case ImmuneSlow:
				Dest.Type = pBoolType;
				Dest.DWord = false;
				if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByPartialName(GETFIRST()))
				{
					if (int found = FindSpawnInMaster(pSpawn))
					{
						Dest.Type = pBoolType;
						Dest.DWord = vMaster[found - 1].ImmuneSlow;
						return true;
					}
				}
				return true;
			case ImmuneSnare:
				Dest.Type = pBoolType;
				Dest.DWord = false;
				if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByPartialName(GETFIRST()))
				{
					if (int found = FindSpawnInMaster(pSpawn))
					{
						Dest.Type = pBoolType;
						Dest.DWord = vMaster[found - 1].ImmuneSnare;
						return true;
					}
				}
				return true;
			case ImmunityCheck:
				Dest.Type = pBoolType;
				Dest.DWord = false;
				if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByPartialName(GETFIRST()))
				{
					if (int found = FindSpawnInMaster(pSpawn))
					{
						Dest.Type = pBoolType;
						Dest.DWord = vMaster[found - 1].ImmunityCheck;
						return true;
					}
				}
				return true;
			case Burn:
				Dest.Type = pBoolType;
				Dest.DWord = false;
				if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByPartialName(GETFIRST()))
				{
					if (int found = FindSpawnInMaster(pSpawn))
					{
						Dest.Type = pBoolType;
						Dest.DWord = vMaster[found - 1].Burn;
						return true;
					}
				}
				return true;
			case Body:
				if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByPartialName(GETFIRST()))
				{
					if (int found = FindSpawnInMaster(pSpawn))
					{
						strcpy_s(DataTypeTemp, vMaster[found - 1].BodyType);
						Dest.Type = pStringType;
						Dest.Ptr = &DataTypeTemp[0];
						return true;
					}
				}
			case Notes:
				if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByPartialName(GETFIRST()))
				{
					if (int found = FindSpawnInMaster(pSpawn))
					{
						strcpy_s(DataTypeTemp, vMaster[found - 1].Notes);
						Dest.Type = pStringType;
						Dest.Ptr = &DataTypeTemp[0];
						return true;
					}
				}
			case ImmuneStun:
				Dest.Type = pBoolType;
				Dest.DWord = false;
				if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByPartialName(GETFIRST()))
				{
					if (int found = FindSpawnInMaster(pSpawn))
					{
						Dest.Type = pBoolType;
						Dest.DWord = vMaster[found - 1].ImmuneStun;
						return true;
					}
				}
				return true;
			case ImmuneMagic:
				Dest.Type = pBoolType;
				Dest.DWord = false;
				if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByPartialName(GETFIRST()))
				{
					if (int found = FindSpawnInMaster(pSpawn))
					{
						Dest.Type = pBoolType;
						Dest.DWord = vMaster[found - 1].ImmuneMagic;
						return true;
					}
				}
				return true;
			}
		}
		return false;
	}

	bool ToString(MQ2VARPTR VarPtr, PCHAR Destination)
	{
		strcpy_s(Destination, MAX_STRING, "TRUE");
		return true;
	}

	bool FromData(MQ2VARPTR& VarPtr, MQ2TYPEVAR& Source)
	{
		return false;
	}

	bool FromString(MQ2VARPTR& VarPtr, PCHAR Source)
	{
		return false;
	}

	~MQ2SpawnDBType()
	{
	}
};

BOOL DataSpawnDB(PCHAR Index, MQ2TYPEVAR& Dest)
{
	Dest.Type = pSpawnDBTypes;
	Dest.DWord = 1;
	return true;
}

/*
BOOL DataAdd(PCHAR Index, MQ2TYPEVAR &Dest)
{
Dest.Type=pAddTypes;
Dest.DWord=1;
return true;
}
*/

#pragma endregion TLO

void SetDatabaseInfo()
{
	sprintf_s(myconnect, "dbname = %s user = %s password = %s hostaddr = %s port = %s", dbname, dbuser, dbpassword, dbhost, dbport);
}

//set up some basic routines for CRUD and DB connect check
#pragma region API
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
		WriteChatf("%s: %s", __FUNCTION__, e.what());
		return 0;
	}
}

bool CheckvMaster(PCHAR name, SQL_FIELDS sql_field)
{
	int vSize = vMaster.size();
	for (int i = 0; i < vSize; i++)
	{
		if (!_stricmp(vMaster[i].Name, name))
			switch (sql_field)
			{
			case	ID:
				return true;
			case 	MAX_LEVEL:
			case	MIN_LEVEL:
			case	NOTES:
			case	QUEST_NPC:
				return (vMaster[i].Quest ? true : false);
			case	NAMED:
				return (vMaster[i].Named ? true : false);
			case	IGNOREMOB:
				return (vMaster[i].Ignore ? true : false);
			case	immuneSNARE:
				return (vMaster[i].ImmuneSnare ? true : false);
			case	immuneSLOW:
				return (vMaster[i].ImmuneSlow ? true : false);
			case	immuneCHARM:
				return (vMaster[i].ImmuneCharm ? true : false);
			case	immuneMEZ:
				return (vMaster[i].ImmuneMez ? true : false);
			case	immuneFIRE:
				return (vMaster[i].ImmuneFire ? true : false);
			case	immuneCOLD:
				return (vMaster[i].ImmuneCold ? true : false);
			case	immunePOISON:
				return (vMaster[i].ImmunePoison ? true : false);
			case	immuneDISEASE:
				return (vMaster[i].ImmuneDisease ? true : false);
			case	BODYTYPE:
			case	IMMUNITY_CHECK:
				return (vMaster[i].ImmunityCheck ? true : false);
			case	BURN:
				return (vMaster[i].Burn ? true : false);
			case	immuneSTUN:
				return (vMaster[i].ImmuneStun ? true : false);
			case	immuneMAGIC:
				return (vMaster[i].ImmuneMagic ? true : false);
			return false;
			}
			
	}
	return false;
}

char* CheckvMasterString(PCHAR name, SQL_FIELDS sql_field)
{
	static char Buffer[MAX_STRING];
	Buffer[0] = 0;
	int vSize = vMaster.size();
	for (int i = 0; i < vSize; i++)
	{
		if (!_stricmp(vMaster[i].Name, name))
		{
			switch (sql_field)
			{
			case	ID:
			case 	MAX_LEVEL:
			case	MIN_LEVEL:
			case	NOTES:
				sprintf_s(Buffer, vMaster[i].Notes);
				return Buffer;
			case	QUEST_NPC:
			case	NAMED:
			case	IGNOREMOB:
			case	immuneSNARE:
			case	immuneSLOW:
			case	immuneCHARM:
			case	immuneMEZ:
			case	immuneFIRE:
			case	immuneCOLD:
			case	immunePOISON:
			case	immuneDISEASE:
			case	BODYTYPE:
				sprintf_s(Buffer, vMaster[i].BodyType);
				return Buffer;
			case	IMMUNITY_CHECK:
			case	BURN:
			case	immuneSTUN:
			case	immuneMAGIC:
				return Buffer;
			}
		}
	}
	return Buffer;
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
	return (CheckvMaster(name, ID));
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
PLUGIN_API bool isImmuneStun(PCHAR name)
{
	return (CheckvMaster(name, immuneSTUN));
}
PLUGIN_API bool isImmuneMagic(PCHAR name)
{
	return (CheckvMaster(name, immuneMAGIC));
}

#pragma endregion API

int CheckIfTableExists(PCHAR &table_name)
{
	try { 
		/* Check Db connect*/
		connection C(myconnect);

		

		/* Create SQL statement */
		char sql[MAX_STRING] = { 0 };
		/*sprintf_s(sql, "SELECT EXISTS( 												\
				SELECT 1													\
				FROM   pg_catalog.pg_class c								\
				JOIN   pg_catalog.pg_namespace n ON n.oid = c.relnamespace	\
				WHERE  n.nspname = 'schema_name'							\
				AND    c.relname = '%s'								\
			);", table_name);*/
		
		sprintf_s(sql,"SELECT to_regclass('schema_name.%s');",table_name);
		/* Create a non-transactional object. */
		nontransaction N(C);

		/* Execute SQL query */
		result R(N.exec(sql));
		for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
			try {
				if (R.empty())
				{
					//WriteChatf("R.empty is true");
					return 0;
				}
				else
				{
					//WriteChatf("R.empty is false");
					return 1;
				}
			}
			catch (const std::exception & e) {
				WriteChatf("%s: %s", __FUNCTION__, e.what());
				return 0;
			}
		}
	}
	catch (const std::exception & e) {
		WriteChatf("%s: %s", __FUNCTION__, e.what());
		return 0;
	}
	return 0;
}

void CreateTable(int TableType, PCHAR szLine)
{
	/* Create SQL statement */
	char sql[MAX_STRING] = { 0 };
	sprintf_s(sql, "CREATE TABLE IF NOT EXISTS %s (  \
		ID SERIAL PRIMARY KEY			NOT NULL, \
		NAME			VARCHAR(64)		NOT NULL, \
		MAX_LEVEL		INT				NOT NULL, \
		MIN_LEVEL		INT				NOT NULL, \
		NOTES			VARCHAR(2048)	NOT NULL DEFAULT '',\
		QUEST_NPC		BOOL			NOT NULL, \
		NAMED			BOOL			NOT NULL, \
		IGNORE			BOOL			NOT NULL, \
		immuneSNARE	BOOL			NOT NULL, \
		immuneSLOW		BOOL			NOT NULL, \
		immuneCHARM	BOOL			NOT NULL, \
		immuneMEZ		BOOL			NOT NULL, \
		immuneFIRE		BOOL			NOT NULL, \
		immuneCOLD		BOOL			NOT NULL, \
		immunePOISON	BOOL			NOT NULL, \
		immuneDISEASE	BOOL			NOT NULL, \
		BODYTYPE		VARCHAR(64)		NOT NULL, \
		IMMUNITY_CHECK	BOOL			NOT NULL, \
		BURN			BOOL			NOT NULL, \
		immuneSTUN		BOOL			NOT NULL, \
		immuneMAGIC		BOOL			NOT NULL);",szLine);

	try {
		connection C(myconnect);
		if (C.is_open()) {
			/* Create a transactional object. */
			work W(C);

			/* Execute SQL query */
			W.exec(sql);
			W.commit();
			WriteChatf("Table \ag%s \awexists or has been created", szLine);
			C.close();
		}
	}
	catch (const std::exception & e) {
		WriteChatf("%s: %s", __FUNCTION__, e.what());
	}
	CurrentZoneTableExists = true;
}

void InsertSpawn(PSPAWNINFO pSpawn)
{
	if (!InGameOK()) // if i am not in a zone let's not build a zone
		return;
	// all spawns added to vSpawn, so let's insert each into table.
	if (!pZoneInfo)
		return;

	bool quest = false, named = false;
	char body[64] = { 0 };
	if (strlen(pSpawn->Lastname)) // is it a quest mob?
		quest = true;
	if (StrStrIA(pSpawn->Name, "#") && !StrStrIA(pSpawn->Name, "#a_") && !StrStrIA(pSpawn->Name, "#an_") && !pSpawn->MasterID && !strlen(pSpawn->Lastname) && pSpawn->Type == SPAWN_NPC) // TODO: do we need to add a Kael Drakkal check or other zone where there's a bunch of fake named
		named = true;
	if (GetBodyType(pSpawn))
		strcpy_s(body, GetBodyTypeDesc(GetBodyType(pSpawn)));

	/* Create SQL statement */
	char table_name[128];
	strcpy_s(table_name, ((PZONEINFO)pZoneInfo)->ShortName);
	char sql[MAX_STRING] = { 0 };
	try {
		connection C(myconnect);
		if (C.is_open()) {
			//  "NAME, MAX_LEVEL, MIN_LEVEL, QUEST_NPC, NAMED, IGNORE, immuneSNARE, immuneSLOW, immuneCHARM, immuneMEZ, immuneFIRE, immuneCOLD, immunePOISON, immuneDISEASE, BODYTYPE, IMMUNITY_CHECK, BURN, NOTES, immuneSTUN, immuneMAGIC";
			sprintf_s(sql, "INSERT INTO %s (%s) \
			VALUES \
			( '%s', '%d', '%d', '%s', '%s', '%s', 'FALSE', 'FALSE', 'FALSE', 'FALSE', 'FALSE', 'FALSE', 'FALSE', 'FALSE', '%s', 'TRUE', '%s', '', 'FALSE', 'FALSE');", \
				table_name, NPC_Table_Columns, pSpawn->DisplayedName, pSpawn->Level, pSpawn->Level, quest ? "TRUE" : "FALSE", named ? "TRUE" : "FALSE",quest ? "TRUE" : "FALSE", body, named ? "TRUE" : "FALSE");

			// build and add spawn to known mobs
			_SpawnMaster master;
			strcpy_s(master.Name, pSpawn->DisplayedName);
			master.MaxLevel = pSpawn->Level;
			master.MinLevel = pSpawn->Level;
			master.Quest = quest;
			master.Named = named;
			master.Ignore = quest;
			master.ImmuneSnare = false;
			master.ImmuneSlow = false;
			master.ImmuneMez = false;
			master.ImmuneFire = false;
			master.ImmuneCold = false;
			master.ImmuneStun = false;
			master.ImmuneMagic = false;
			master.ImmunePoison = false;
			master.ImmuneDisease = false;
			strcpy_s(master.BodyType, body);
			master.ImmunityCheck = true;
			master.Burn =named;
			vMaster.push_back(master);
			/* Create a transactional object. */
			work W(C);
			/* Execute SQL query */
			W.exec(sql);
			W.commit();
			//WriteChatf("298 \agSpawn Added: \aw%s", master.Name);
		}
		else {
			WriteChatf("Can't open database");
		}
		C.close();
	}
	catch (const std::exception & e) {
		WriteChatf("%s: %s", __FUNCTION__, e.what());
	}
}

void BuildNewZone()
{
	if (!InGameOK()) // if i am not in a zone let's not build a zone
		return;
	PSPAWNINFO pSpawn = NULL;
	bool quest, named;
	char body[64] = { 0 };
	int found = 0;
	vSpawns.clear();
	vMaster.clear();
	if (ppSpawnManager && pSpawnList) // again verifying we have spawns or else let's not worry about this
	{
		pSpawn = (PSPAWNINFO)pSpawnList;
	}
	try {
		while (pSpawn)
		{
			if (GetSpawnType(pSpawn) == NPC && !pSpawn->MasterID) // let's not add pets
			{
				found = 0;
				quest = false;
				named = false;
				if (strlen(pSpawn->Lastname)) // is it a quest mob?
					quest = true;
				if (StrStrIA(pSpawn->Name, "#") && !StrStrIA(pSpawn->Name, "#a_") && !StrStrIA(pSpawn->Name, "#an_") && !pSpawn->MasterID && !strlen(pSpawn->Lastname) && pSpawn->Type == SPAWN_NPC) // TODO: do we need to add a Kael Drakkal check or other zone where there's a bunch of fake named
					named = true;
				if (GetBodyType(pSpawn))
					strcpy_s(body, GetBodyTypeDesc(GetBodyType(pSpawn)));
				int vSize = vSpawns.size();
				for (int i = 0; i < vSize; i++)
				{
					if (_stricmp(pSpawn->DisplayedName, vSpawns[i].displayname)) // is this already known
					{
						// if it is, let's see if it is lower or higher level than known
						if (pSpawn->Level < vSpawns[i].minlevel)
							vSpawns[i].minlevel = pSpawn->Level;
						if (pSpawn->Level < vSpawns[i].maxlevel)
							vSpawns[i].maxlevel = pSpawn->Level;
						found++; // sure is
					}
				}
				if (!found) // not found, add it
				{
					_SpawnCheck myspawn;
					char temp[64] = { 0 };
					strcpy_s(myspawn.displayname, pSpawn->DisplayedName);
					myspawn.minlevel = pSpawn->Level;
					myspawn.maxlevel = pSpawn->Level;
					myspawn.Quest = quest;
					myspawn.named = named;
					strcpy_s(myspawn.body,body);
					vSpawns.push_back(myspawn);
				}
			}
			pSpawn = pSpawn->pNext;
		}
	}
	catch (const std::exception & e) {
		WriteChatf("%s: %s", __FUNCTION__, e.what());
	}
	// all spawns added to vSpawn, so let's insert each into table.
	if (!pZoneInfo)
		return;

	/* Create SQL statement */
	char table_name[128];
	strcpy_s(table_name, ((PZONEINFO)pZoneInfo)->ShortName);
	char sql[MAX_STRING] = { 0 };
	try {
		connection C(myconnect);
		if (C.is_open()) {
			int vSize = vSpawns.size();
			for (int i = 0; i < vSize; i++)
			{
				//  "NAME, MAX_LEVEL, MIN_LEVEL, QUEST_NPC, NAMED, IGNORE, immuneSNARE, immuneSLOW, immuneCHARM, immuneMEZ, immuneFIRE, immuneCOLD, immunePOISON, immuneDISEASE, BODYTYPE, IMMUNITY_CHECK, BURN, NOTES, immuneSTUN, immuneMAGIC";
				sprintf_s(sql, "INSERT INTO %s (%s) \
				VALUES \
				( '%s', '%d', '%d', '%s', '%s', '%s', 'FALSE', 'FALSE', 'FALSE', 'FALSE', 'FALSE', 'FALSE', 'FALSE', 'FALSE', '%s', 'TRUE', '%s', '', 'FALSE','FALSE');", \
				table_name, NPC_Table_Columns, vSpawns[i].displayname, vSpawns[i].maxlevel, vSpawns[i].minlevel, vSpawns[i].Quest ? "TRUE" : "FALSE", vSpawns[i].named ? "TRUE" : "FALSE", vSpawns[i].Quest ? "TRUE" : "FALSE", vSpawns[i].body, vSpawns[i].named ? "TRUE" : "FALSE");
			
				// build and add spawn to known mobs
				_SpawnMaster master;
				strcpy_s(master.Name, vSpawns[i].displayname);
				master.MaxLevel= vSpawns[i].maxlevel;
				master.MinLevel = vSpawns[i].minlevel;
				master.Quest = vSpawns[i].Quest;
				master.Named = vSpawns[i].named;
				master.Ignore= vSpawns[i].Quest;
				master.ImmuneSnare=false;
				master.ImmuneSlow=false;
				master.ImmuneMez=false;
				master.ImmuneFire=false;
				master.ImmuneCold = false;
				master.ImmuneStun = false;
				master.ImmuneMagic = false;
				master.ImmunePoison=false;
				master.ImmuneDisease = false;
				strcpy_s(master.BodyType, vSpawns[i].body);
				master.ImmunityCheck=true;
				master.Burn= vSpawns[i].named;
				//WriteChatf("inserting %s", master.Name);
				vMaster.push_back(master);
			}
			/* Create a transactional object. */
			work W(C);

			/* Execute SQL query */
			W.exec(sql);
			W.commit();
		}
		else {
			WriteChatf("Can't open database");
		}
		C.close();
	}
	catch (const std::exception & e) {
		WriteChatf("%s: %s", __FUNCTION__, e.what());
	}
	CurrentZoneTableExists = true;
	return;
}

void UpdateZone()
{
	if (!InGameOK()) // if i am not in a zone let's not build a zone
		return;
	vSpawns.clear();
	vector<_SpawnCheck> vUnknown;
	PSPAWNINFO pSpawn;
	char table_name[128];
	strcpy_s(table_name, ((PZONEINFO)pZoneInfo)->ShortName);
	if (ppSpawnManager && pSpawnList) // again verifying we have spawns or else let's not worry about this
	{
		pSpawn = (PSPAWNINFO)pSpawnList;
	}
	try {
		connection C(myconnect);
		if (C.is_open()) {
			/* Create SQL statement */
			char sql[MAX_STRING] = { 0 };
			sprintf_s(sql, "SELECT * FROM %s;", table_name);
			/* Create a non-transactional object. */
			nontransaction N(C);

			/* Execute SQL query */
			result R(N.exec(sql));
			int rSize = R.size();
			/* List down all the records */
			for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
				_SpawnCheck myspawn;
				strcpy_s(myspawn.displayname, c[NAME].c_str());
				myspawn.maxlevel = c[MAX_LEVEL].as<int>();
				myspawn.minlevel = c[MIN_LEVEL].as<int>();
				//WriteChatf("%s is in db already", myspawn.displayname);
				vSpawns.push_back(myspawn);
			}
			// spawn list is created, now let's compare known vs unknown and add any unknown to db and vSpawns
			bool quest, named;
			char body[64] = { 0 };
			int found = 0;
			try {
				while (pSpawn)
				{
					if (GetSpawnType(pSpawn) == NPC && !pSpawn->MasterID) // let's not add pets
					{
						found = 0;
						quest = false;
						named = false;
						int vSize = vSpawns.size();
						for (int i = 0; i < vSize; i++)
						{
							if (!_stricmp(pSpawn->DisplayedName, vSpawns[i].displayname)) // is this already known
							{
								//WriteChatf("%s is known as %s", pSpawn->DisplayedName, vSpawns[i].displayname);
								// if it is, let's see if it is lower or higher level than known
								if (pSpawn->Level < vSpawns[i].minlevel)
									vSpawns[i].minlevel = pSpawn->Level;
								if (pSpawn->Level > vSpawns[i].maxlevel)
									vSpawns[i].maxlevel = pSpawn->Level;
								found++;
							}
						}
						if (!found) // not found, add it
						{
							if (strlen(pSpawn->Lastname)) // is it a quest mob?
								quest = true;
							if (StrStrIA(pSpawn->Name, "#") && !StrStrIA(pSpawn->Name, "#a_") && !StrStrIA(pSpawn->Name, "#an_") && !pSpawn->MasterID && !strlen(pSpawn->Lastname) && pSpawn->Type == SPAWN_NPC) // TODO: do we need to add a Kael Drakkal check or other zone where there's a bunch of fake named
								named = true;
							if (GetBodyType(pSpawn))
								strcpy_s(body, GetBodyTypeDesc(GetBodyType(pSpawn)));
							//WriteChatf("%s is unknown", pSpawn->Name);
							_SpawnCheck myspawn;
							myspawn.minlevel = 1000;
							myspawn.maxlevel = 0;
							if (pSpawn->Level < myspawn.minlevel)
								myspawn.minlevel = pSpawn->Level;
							if (pSpawn->Level > myspawn.maxlevel)
								myspawn.maxlevel = pSpawn->Level;
							strcpy_s(myspawn.displayname, pSpawn->DisplayedName);
							myspawn.Quest = quest;
							myspawn.named = named;
							strcpy_s(myspawn.body, body);
							vSpawns.push_back(myspawn);
							vUnknown.push_back(myspawn); // only add unknowns
						}
					}
					pSpawn = pSpawn->pNext;
				}
			}
			catch (const std::exception & e) {
				WriteChatf("%s: %s", __FUNCTION__, e.what());
			}
			C.close();
		}
	}
	catch (const std::exception & e) {
		WriteChatf("%s: %s", __FUNCTION__, e.what());
	}
	// now let's add the unknowns
	// all spawns added to vSpawn, so let's insert each into table.
	if (!pZoneInfo)
		return;
	//WriteChatf("Trying to insert");
	//int usize = vUnknown.size();
	//for (int i = 0; i < usize; i++)
	//{
	//	WriteChatf("%s", vUnknown[i].displayname);
	//}
	/* Create SQL statement */
	char sql[MAX_STRING] = { 0 };
	try {
		connection C(myconnect);
		if (C.is_open()) {
			int vSize = vUnknown.size();
			for (int i = 0; i < vSize; i++)
			{
				//  "NAME, MAX_LEVEL, MIN_LEVEL, QUEST_NPC, NAMED, IGNORE, immuneSNARE, immuneSLOW, immuneCHARM, immuneMEZ, immuneFIRE, immuneCOLD, immunePOISON, immuneDISEASE, BODYTYPE, IMMUNITY_CHECK, BURN, NOTES, immuneSTUN, immuneMAGIC";
				sprintf_s(sql, "INSERT INTO %s (%s) \
				VALUES \
				( '%s', '%d', '%d', '%s', '%s', '%s', 'FALSE', 'FALSE', 'FALSE', 'FALSE', 'FALSE', 'FALSE', 'FALSE', 'FALSE', '%s', 'TRUE', '%s', '', 'FALSE','FALSE');", \
					table_name, NPC_Table_Columns, vUnknown[i].displayname, vUnknown[i].maxlevel, vUnknown[i].minlevel, vUnknown[i].Quest ? "TRUE" : "FALSE", vUnknown[i].named ? "TRUE" : "FALSE", vUnknown[i].Quest ? "TRUE" : "FALSE", vUnknown[i].body, vUnknown[i].named ? "TRUE" : "FALSE");

				/* Create a transactional object. */
				work W(C);

				/* Execute SQL query */
				W.exec(sql);
				W.commit();
			}
		}
		else {
			WriteChatf("Can't open database");
		}
		C.close();
	}
	catch (const std::exception & e) {
		WriteChatf("%s: %s", __FUNCTION__, e.what());
	}
	CurrentZoneTableExists = true;
}

void CheckMaster()
{
	if (!InGameOK()) // if i am not in a zone let's not build a zone
		return;
	// all spawns added to vSpawn, so let's insert each into table.
	if (!pZoneInfo)
		return;
	char table_name[128];
	strcpy_s(table_name, ((PZONEINFO)pZoneInfo)->ShortName);
	try {
		connection C(myconnect);
		if (C.is_open()) {
			/* Create SQL statement */
			char sql[MAX_STRING] = { 0 };
			sprintf_s(sql, "SELECT * FROM %s;", table_name);
			//WriteChatf("%s", sql);
			/* Create a non-transactional object. */
			nontransaction N(C);

			/* Execute SQL query */
			result R(N.exec(sql));
			int rSize = R.size();
			int found = 0;
			if (!R.empty())
			{
				for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
					found = 0;
					int vSize = vMaster.size();
					//WriteChatf("vSize: %d", vSize);
					for (int i = 0; i < vSize; i++)
					{
						if (StrStrIA(vMaster[i].Name, c[2].c_str()))
							found++; // TODO: i think i can add the max and min level checks here but it has to be transactional, not nontransactional
					}
					if (!found)
					{	// 1		2		  3			4		5		  6		  7			8		 	9			10				11			12			13			14				15				16			17			18		19			20		21
						//ID       NAME, MAX_LEVEL, MIN_LEVEL, NOTES, QUEST_NPC, NAMED, IGNORE, immuneSNARE, immuneSLOW, immuneCHARM, immuneMEZ, immuneFIRE, immuneCOLD, immunePOISON, immuneDISEASE, BODYTYPE, IMMUNITY_CHECK, BURN, known, immuneSTUN, immuneMagic
						if (strlen(c[NAME].c_str()) > 2)
						{
							_SpawnMaster master;
							strcpy_s(master.Name, c[NAME].c_str());
							//WriteChatf("%s",master.Name);
							master.MaxLevel = c[MAX_LEVEL].as<int>();
							master.MinLevel = c[MIN_LEVEL].as<int>();
							strcpy_s(master.Notes, c[NOTES].c_str());
							master.Quest = c[QUEST_NPC].as<bool>();
							master.Named = c[NAMED].as<bool>();
							master.Ignore = c[IGNOREMOB].as<bool>();
							master.ImmuneSnare = c[immuneSNARE].as<bool>();
							master.ImmuneSlow = c[immuneSLOW].as<bool>();
							master.ImmuneCharm = c[immuneCHARM].as<bool>();
							master.ImmuneMez = c[immuneMEZ].as<bool>();
							master.ImmuneFire = c[immuneFIRE].as<bool>();
							master.ImmuneCold = c[immuneCOLD].as<bool>();
							master.ImmuneStun = c[immuneSTUN].as<bool>();
							master.ImmuneMagic = c[immuneMAGIC].as<bool>();
							master.ImmunePoison = c[immunePOISON].as<bool>();
							master.ImmuneDisease = c[immuneDISEASE].as<bool>();
							strcpy_s(master.BodyType, c[BODYTYPE].c_str());
							master.ImmunityCheck = c[IMMUNITY_CHECK].as<bool>();
							master.Burn = c[BURN].as<bool>();
							//WriteChatf("%s added to vMaster", master.Name);
							vMaster.push_back(master);
						}
					}
				}
			}
		}
		C.close();
	}
	catch (const std::exception & e) {
		WriteChatf("%s: %s", __FUNCTION__, e.what());
	}
}

void CheckZone()
{
	try {
		// Pass the zone info and build table for NPC spawns if it doesnt exist
		PZONEINFO pthezone = (PZONEINFO)pZoneInfo;
		#define pZone ((PZONEINFO)pZoneInfo)
		PCHAR zone = (PCHAR)pZone->ShortName;
		if (int i = CheckIfTableExists(zone))
		{ // it exists so let's just update it
			//WriteChatf("Table \ag%s \awexists", pZone->ShortName);
			CreateTable(SPAWNS, pZone->ShortName);
			UpdateZone();
		}
		else
		{
			WriteChatf("Table \ar%s \aydoes not exist or is empty, \agbuilding...", pZone->ShortName);
			CreateTable(SPAWNS, pZone->ShortName);
			BuildNewZone();
		}
	}
	catch (const std::exception & e) {
		WriteChatf("%s: %s", __FUNCTION__, e.what());
	}
	// then either way, update our known spawns
	CheckMaster();
}

#pragma region commands

void SpawnDBCommand(PSPAWNINFO pChar, PCHAR szLine)
{
	if (!InGameOK())
		return;
	if (!pZoneInfo)
		return;
	PCHAR SpawnDBSettings[] = { "Named","Quest","Notes","Ignore","ImmuneCold","ImmuneDisease","ImmuneFire","ImmuneMez","ImmuneCharm","ImmunePoison","ImmuneSlow","ImmuneSnare","ImmunityCheck","Burn","ImmuneStun","ImmuneMagic",NULL };
	if (strlen(szLine) != 0)
	{
		try {
			char Arg1[MAX_STRING] = { 0 }, Arg2[MAX_STRING] = { 0 }, Arg3[MAX_STRING] = { 0 }; // 1 = SpawnDBSetting, 2 = value, 3 = NPC
			GetArg(Arg3, szLine, 3);
			GetArg(Arg2, szLine, 2);
			GetArg(Arg1, szLine, 1);
			//WriteChatf("%s", Arg1);
			for (unsigned int i = 0; SpawnDBSettings[i]; i++)
			{
				if (StrStrIA(SpawnDBSettings[i], Arg1)) // variable exists
				{
					bool bSetting;
					if (strlen(Arg2) != 0)
					{
						if (_stricmp(Arg1, "notes") && _stricmp(Arg2, "false") && _stricmp(Arg2, "true"))
						{	
							WriteChatf("Your Arg2 value of '%s' is not a valid option");
							return;  // setting does not exist so exit 

						}
						if (!_stricmp(Arg2, "false") || !_stricmp(Arg2, "0"))
							bSetting = false;
						if (!_stricmp(Arg2, "true") || !_stricmp(Arg2, "1"))
							bSetting = true;
						if (strlen(Arg3) != 0 && strlen(Arg3)<65)
						{
							int vSize = vMaster.size();
							for (int x = 0; x < vSize; x++) // search master to see if spawn exists
							{
								/*if(StrStrIA(vMaster[x].Name,Arg3))
									WriteChatf("%s partial matches %s", Arg3, vMaster[x].Name);*/
								if (!_stricmp(vMaster[x].Name, Arg3))
								{ // found match, let's update it
									WriteChatf("%s arg2=%s", Arg3,Arg2);
									/* Create SQL statement */
									char table_name[128];
									strcpy_s(table_name, ((PZONEINFO)pZoneInfo)->ShortName);
									char sql[MAX_STRING];
									_strlwr_s(Arg3);
									sprintf_s(sql, "UPDATE %s SET %s = '%s' WHERE LOWER(NAME) = '%s'", table_name, Arg1, Arg2, Arg3);
								
									connection C(myconnect);
									if (C.is_open()) {
										/* Create a transactional object. */
										work W(C);

										/* Execute SQL query */
										W.exec(sql);
										W.commit();
										WriteChatf("Spawn: \ag %s \awSetting: \ay %s \awNew Value : \ao%s", Arg3,Arg1,Arg2);
											C.close();
									}
									// also let's update vMaster
									if (StrStrIA(Arg1, "Named"))
										vMaster[x].Named = bSetting;
									if (StrStrIA(Arg1, "Quest"))
										vMaster[x].Quest = bSetting;
									if (StrStrIA(Arg1, "Notes"))
										strcpy_s(vMaster[x].Notes, Arg2);
									if (StrStrIA(Arg1, "Ignore"))
										vMaster[x].Ignore = bSetting;
									if (StrStrIA(Arg1, "ImmuneCold"))
										vMaster[x].ImmuneCold = bSetting;
									if (StrStrIA(Arg1, "ImmuneStun"))
										vMaster[x].ImmuneStun = bSetting;
									if (StrStrIA(Arg1, "ImmuneMagic"))
										vMaster[x].ImmuneMagic = bSetting;
									if (StrStrIA(Arg1, "ImmuneDisease"))
										vMaster[x].ImmuneDisease = bSetting;
									if (StrStrIA(Arg1, "ImmuneFire"))
										vMaster[x].ImmuneFire = bSetting;
									if (StrStrIA(Arg1, "ImmuneMez"))
										vMaster[x].ImmuneMez = bSetting;
									if (StrStrIA(Arg1, "ImmuneCharm"))
										vMaster[x].ImmuneCharm = bSetting;
									if (StrStrIA(Arg1, "ImmunePoison"))
										vMaster[x].ImmunePoison = bSetting;
									if (StrStrIA(Arg1, "ImmuneSlow"))
										vMaster[x].ImmuneSlow = bSetting;
									if (StrStrIA(Arg1, "ImmuneSnare"))
										vMaster[x].ImmuneSnare = bSetting;
									if (StrStrIA(Arg1, "ImmunityCheck"))
										vMaster[x].ImmunityCheck = bSetting;
									if (StrStrIA(Arg1, "Burn"))
										vMaster[x].Burn = bSetting;
								}
							}
						}
					}
				}
			}
		}
		catch (const std::exception & e) {
			WriteChatf("%s: %s", __FUNCTION__, e.what());
		}
	}
}

void TestCommand(PSPAWNINFO pChar, PCHAR szLine)
{
	if (int i = isDBConnected())
		WriteChatf("Tested DB connect");
}

void TestTable(PSPAWNINFO pChar, PCHAR szLine)
{
	if (int i = CheckIfTableExists(szLine))
		WriteChatf("Table \ag%s \awexists",szLine);
	else
	{
		WriteChatf("Table \ar%s \aydoes not exist or is empty, \agbuilding...", szLine);
		CreateTable(SPAWNS, szLine);
	}
}
#pragma endregion commands

// Load configuration settings
void Configure()
{
	if (!InGameOK())
		return;
	char tempINI[MAX_STRING] = { 0 };
	sprintf_s(INIFileName, "%s\\%s_%s.ini", gszINIPath, EQADDR_SERVERNAME, GetCharInfo()->Name);
	sprintf_s(INISection, "%s", PLUGIN_NAME);

	//char dbname[100] = "testdb", dbuser[100] = "test", dbpassword[100] = "password", dbhost[30] = "127.0.0.1", dbport[10] = "5432";

	if (!GetEnvVariables("PGDATABASE"))
	{
		GetPrivateProfileString(INISection, "DatabaseName", "postgres", dbname, sizeof(dbname), INIFileName);
		WritePrivateProfileString(INISection, "DatabaseName", dbname, INIFileName);
	}
	if (!GetEnvVariables("PGUSER"))
	{
		GetPrivateProfileString(INISection, "Username", "postgres", dbuser, sizeof(dbuser), INIFileName);
		WritePrivateProfileString(INISection, "Username", dbuser, INIFileName);
	}
	if (!GetEnvVariables("PGPASSWORD"))
	{
		GetPrivateProfileString(INISection, "Password", "NULL", dbpassword, sizeof(dbpassword), INIFileName);
		WritePrivateProfileString(INISection, "Password", dbpassword, INIFileName);
	}
	if (!GetEnvVariables("PGHOST"))
	{
		GetPrivateProfileString(INISection, "HostIP", "127.0.0.1", dbhost, sizeof(dbhost), INIFileName);
		WritePrivateProfileString(INISection, "HostIP", dbhost, INIFileName);
	}
	if (!GetEnvVariables("PGPORT"))
	{
		GetPrivateProfileString(INISection, "HostPort", "5432", dbport, sizeof(dbport), INIFileName);
		WritePrivateProfileString(INISection, "HostPort", dbport, INIFileName);
	}
}

void PluginOn()
{
	CheckZone();
	//Add commands, MQ2Data items, hooks, etc.
	//AddCommand("/mycommand",MyCommand);
	//AddXMLFile("MQUI_MyXMLFile.xml");
	//bmMyBenchmark=AddMQ2Benchmark("My Benchmark Name");
}

void PluginOff()
{

	//Remove commands, MQ2Data items, hooks, etc.
	//RemoveMQ2Benchmark(bmMyBenchmark);
	//RemoveCommand("/mycommand");
	//RemoveXMLFile("MQUI_MyXMLFile.xml");
}

// Called once, when the plugin is to initialize
PLUGIN_API VOID InitializePlugin(VOID)
{
	DebugSpewAlways("Initializing %s", PLUGIN_NAME);

#ifdef MMOBUGS_LOADER
	MMORequiredAccess = AL_Premium;
	if(LOK(MMORequiredAccess))
		PluginOn();
	else
		WriteChatf("\ar%s \aysubscription is required to use \ag%s", MMOAccessName[MMORequiredAccess], PLUGIN_NAME);
	InitLib(PLUGIN_NAME);
#else
	Configure();
	SetDatabaseInfo();
	AddCommand("/testdb", TestCommand);
	AddCommand("/testtable", TestTable);
	AddCommand("/spawndb", SpawnDBCommand);
	//Add commands, MQ2Data items, hooks, etc.
	//AddCommand("/mycommand",MyCommand);
	//AddXMLFile("MQUI_MyXMLFile.xml");
	//bmMyBenchmark=AddMQ2Benchmark("My Benchmark Name");
#endif
	pSpawnDBTypes = new MQ2SpawnDBType;
	AddMQ2Data("SpawnDB", DataSpawnDB);
	PluginOn();
}

// Called once, when the plugin is to shutdown
PLUGIN_API VOID ShutdownPlugin(VOID)
{
	DebugSpewAlways("Shutting down %s", PLUGIN_NAME);

#ifdef MMOBUGS_LOADER
	PluginOff();
	FreeLib(PLUGIN_NAME);
#else
	RemoveCommand("/testdb");
	RemoveCommand("/testtable");
	RemoveCommand("/spawndb");
	//Remove commands, MQ2Data items, hooks, etc.
	//RemoveMQ2Benchmark(bmMyBenchmark);
	//RemoveCommand("/mycommand");
	//RemoveXMLFile("MQUI_MyXMLFile.xml");
	RemoveMQ2Data("SpawnDB");
	delete pSpawnDBTypes;
	pSpawnDBTypes = NULL;
#endif
}

// Called after entering a new zone
PLUGIN_API VOID OnZoned(VOID)
{
	DebugSpewAlways("%s::OnZoned()", PLUGIN_NAME);
}

// Called once directly before shutdown of the new ui system, and also
// every time the game calls CDisplay::CleanGameUI()
PLUGIN_API VOID OnCleanUI(VOID)
{
	DebugSpewAlways("%s::OnCleanUI()", PLUGIN_NAME);
	// destroy custom windows, etc
}

// Called once directly after the game ui is reloaded, after issuing /loadskin
PLUGIN_API VOID OnReloadUI(VOID)
{
	DebugSpewAlways("%s::OnReloadUI()", PLUGIN_NAME);
	// recreate custom windows, etc
}

// Called every frame that the "HUD" is drawn -- e.g. net status / packet loss bar
PLUGIN_API VOID OnDrawHUD(VOID)
{
	// DONT leave in this debugspew, even if you leave in all the others
	//DebugSpewAlways("%s::OnDrawHUD()", PLUGIN_NAME);
}

// Called once directly after initialization, and then every time the gamestate changes
PLUGIN_API VOID SetGameState(DWORD GameState)
{
	DebugSpewAlways("%s::SetGameState()", PLUGIN_NAME);
	//if (GameState==GAMESTATE_INGAME)
	// create custom windows if theyre not set up, etc
}


// This is called every time MQ pulses
PLUGIN_API VOID OnPulse(VOID)
{
	// DONT leave in this debugspew, even if you leave in all the others
	//DebugSpewAlways("%s::OnPulse()", PLUGIN_NAME);
}

// This is called every time WriteChatColor is called by MQ2Main or any plugin,
// IGNORING FILTERS, IF YOU NEED THEM MAKE SURE TO IMPLEMENT THEM. IF YOU DONT
// CALL CEverQuest::dsp_chat MAKE SURE TO IMPLEMENT EVENTS HERE (for chat plugins)
PLUGIN_API DWORD OnWriteChatColor(PCHAR Line, DWORD Color, DWORD Filter)
{
	DebugSpewAlways("%s::OnWriteChatColor(%s)", PLUGIN_NAME, Line);
	return 0;
}

// This is called every time EQ shows a line of chat with CEverQuest::dsp_chat,
// but after MQ filters and chat events are taken care of.
PLUGIN_API DWORD OnIncomingChat(PCHAR Line, DWORD Color)
{
	DebugSpewAlways("%s::OnIncomingChat(%s)", PLUGIN_NAME, Line);
	return 0;
}

// This is called each time a spawn is added to a zone (inserted into EQ's list of spawns),
// or for each existing spawn when a plugin first initializes
// NOTE: When you zone, these will come BEFORE OnZoned
PLUGIN_API VOID OnAddSpawn(PSPAWNINFO pNewSpawn)
{
	DebugSpewAlways("%s::OnAddSpawn(%s)", PLUGIN_NAME, pNewSpawn->Name);
	if (CurrentZoneTableExists) // only add new spawns if the table has been created and updated.
	{
		// check if spawn is known
		int vSize = vMaster.size();
		int found = 0;
		for (int i = 0; i < vSize; i++)
		{
			if (_stricmp(vMaster[i].Name, pNewSpawn->DisplayedName))
			{
				found++; //found it but we should still check level to see if we need to update that
				if (pNewSpawn->Level < vMaster[i].MinLevel)
					vMaster[i].MinLevel = pNewSpawn->Level; //TODO: this doesnt actually update DB yet
				if (pNewSpawn->Level > vMaster[i].MaxLevel)
					vMaster[i].MaxLevel = pNewSpawn->Level; //TODO: this doesnt actually update DB yet
			}
		}
		if (!found)
		{
			//not found so let's add it
			InsertSpawn(pNewSpawn);
		}
	}
}

// This is called each time a spawn is removed from a zone (removed from EQ's list of spawns).
// It is NOT called for each existing spawn when a plugin shuts down.
PLUGIN_API VOID OnRemoveSpawn(PSPAWNINFO pSpawn)
{
	DebugSpewAlways("%s::OnRemoveSpawn(%s)", PLUGIN_NAME, pSpawn->Name);
}

// This is called each time a ground item is added to a zone
// or for each existing ground item when a plugin first initializes
// NOTE: When you zone, these will come BEFORE OnZoned
PLUGIN_API VOID OnAddGroundItem(PGROUNDITEM pNewGroundItem)
{
	DebugSpewAlways("%s::OnAddGroundItem(%d)", PLUGIN_NAME, pNewGroundItem->DropID);
}

// This is called each time a ground item is removed from a zone
// It is NOT called for each existing ground item when a plugin shuts down.
PLUGIN_API VOID OnRemoveGroundItem(PGROUNDITEM pGroundItem)
{
	DebugSpewAlways("%s::OnRemoveGroundItem(%d)", PLUGIN_NAME, pGroundItem->DropID);
}

// This is called when we receive the EQ_BEGIN_ZONE packet is received
PLUGIN_API VOID OnBeginZone(VOID)
{
	CurrentZoneTableExists = false;
	vMaster.clear();
	DebugSpewAlways("%s::OnBeginZone()", PLUGIN_NAME);
}

// This is called when we receive the EQ_END_ZONE packet is received
PLUGIN_API VOID OnEndZone(VOID)
{
	DebugSpewAlways("%s::OnEndZone()", PLUGIN_NAME);
	CheckZone();
}

// This is called when pChar!=pCharOld && We are NOT zoning
// honestly I have no idea if its better to use this one or EndZone (above)
PLUGIN_API VOID Zoned(VOID)
{
    DebugSpewAlways("%s::Zoned", PLUGIN_NAME);
}