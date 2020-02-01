# MQ2SpawnDB - A Macroquest Plugin

This is the MQ2SpawnDB plugin for macroquest2. This plugin is designed to be used to interface with a PostgreSQL database. The plugin will automatically log all observed unique spawns by DisplayName into the database and take a best guess if it is a named, quest related,  and if you should burn available skills. You can log spell immunities, ignore mobs entirely, add notes, or note spell resistances/immunities. This capabilities mimics what other macros/plugins have accomplished with INIs but instead uses a PostgreSQL database to allow multiple users to simultaenously access and write data locally or remotely.

Steps:
1. Install PostgreSQL/pgAdmin (remember your password and if you use any custom host or ports)
2. Setup a blank database or use default postgres
3. Setup a new user or default admin (you should set a new user)
4. Setup INI or it will autobuild an invalid ini for you on first load. **PASSWORD CHANGE MANDATORY**
5. Put libpq.dll and pqxx-7.0.dll in your Everquest folder
6. Load MQ2SpawnDB

## INI options
### Environmental Variables
- You may set up environmental variables so that your real database info is never stored in an INI. These are the environmental variables that Postgres looks for:
```
PGDATABASE	(name of database; defaults to your user name)
PGHOST		(database server; defaults to local machine)
PGPORT		(TCP port to connect to; default is 5432)
PGUSER		(your PostgreSQL user ID; defaults to your login name)
PGPASSWORD	(your PostgreSQL password, if needed)
```

### server_character.ini
You may also set your variables in your character's ini. the Password will cause plugin to fail loading if you do not change it.
```
[MQ2SpawnDB]
DatabaseName=postgres
Username=postgres
Password=NULL
HostIP=127.0.0.1
HostPort=5432
```

## Commands

```
/spawndb setting value "npc name"
```
Use double quotes for any values that have a space, ie: /spawndb notes "my super awesome notes" "Fippy Darkpaw"
All settings other than notes are simple "true" or "false" flags. /spawndb named false "Fippy Darkpaw"

## Setting Options

```
These options are NOT case sensitive:
"Named","Quest","Notes","Ignore","ImmuneCold","ImmuneDisease",
"ImmuneFire","ImmuneMez","ImmuneCharm","ImmunePoison",
"ImmuneSlow","ImmuneSnare","ImmunityCheck","Burn"
```

# Setup
Setup is a little more complicated than a traditional plugin because this plugins requires you to install PostgreSQL, create a database, and add dynamic link libraries (.dll) to your EverQuest folder.

## DLLs (these go in EverQuest folder)
**libpq.dll** The libpq.dll is a PostgresSQL file that is available in your own local PostgreSQL lib folder. You can use the posted copy or use your own.
**pqxx-7.0.dll** This dll can be built manually from ([pqxx][pqxx]) or you can just use the posted version.

## PostgreSQL installation
- [Download][download] the appropriate version of PostgreSQL from  (64bit and 32bit version 10 and higher work).
- Install Postgres: ([Video tutorial][tutorial])
- Free online Postgres Sandbox: [PGAdmin panel][try]

## Create a database/user/password
- [Create database][tutorial] using the same install tutorial listed above.
- [Create user][user]

## PostgreSQL as a service
If you want to automatically run Postgres each time your computer boots up you open command prompt/terminal and navigate to your Postgres bin directory and execute the following command (changing the directory to whatever your actual directory is):
```
pg_ctl.exe register -N "PostgreSQL" -U "NT AUTHORITY\NetworkService" -D "C:/Program Files/postgresql/pgsql/bin/pgsql/data" -w
```

## Project Board:
- [MQ2SpawnDB Roadmap Tracker][GLO_Board]

## Contact/Donations 
- You can [email][email] me if you have questions and want to contribute. I am also in the MMOBugs discord.
- If you like the project and just want to [Donate][donate], that is always appreciated.

[user]: https://www.youtube.com/watch?v=zzvloWiKsEc
[pqxx]: https://github.com/jtv/libpqxx
[download]: https://www.postgresql.org/download/
[try]: https://www.pgadmin.org/try/
[tutorial]: https://www.youtube.com/watch?v=_qUpvRTqK0Y
[GLO_Board]: https://app.gitkraken.com/glo/board/XjTJOxhLpwAQLX7Z
[email]: petesampras.mmobugs@gmail.com
[donate]: https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=PeteSampras%2eMMOBugs%40Gmail%2ecom&lc=US&item_name=PeteSampras&no_note=0&currency_code=USD&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHostedGuest

## Contributing guidelines

I’d love for you to help us improve this project. To help us keep this collection
high quality, I request that contributions adhere to the following guidelines.

- **Provide links to documentation** supporting the change you’re making when available.
  Current, canonical documentation mentioning the data changes are useful so developers can understand changes more easily.
  Posts from EQMule showing code/method changes are also useful.
  If documentation isn’t available to support your change, do the best you can to explain the changes.

- **Explain why you’re making a change**. Even if it seems self-evident, please
  take a sentence or two to tell us why your change or addition should happen.
  It’s especially helpful to articulate why this change applies to _everyone_
  who uses the plugin, rather than just you or your team.

- **Please consider the scope of your change**. If your change is specific to a
  certain function please remember that it may impact other functions you might not use.
  Please try to check any other functions that use the same data as your changed function
  to ensure they still work correctly.

- **Please only modify _one major change_ per pull request**. This helps keep pull
  requests and feedback focused on a specific issue.

In general, the more you can do to help us understand the change you’re making,
the more likely we’ll be to accept your contribution quickly.
