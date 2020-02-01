# MQ2SpawnDB macro and TLO usage examples

## Declare custom events
```js
#event SlowImmune		"Your target is immune to changes in its attack speed#*#"
#event IgSnareImmune		"Your target is immune to changes in its run speed#*#" 
#event MezImmune		"Your target cannot be mesmerized.#*#"
```
Then declare corresponding events

```js
Sub Event_SlowImmune
/spawndb immuneslow true "${Target.DisplayName}"
/return

Sub Event_IgSnareImmune
/spawndb immunesnare true "${Target.DisplayName}"
/return

Sub Event_MezImmune
/spawndb immunemez true "${Target.DisplayName}"
/return
```

## Accessing TLO members
### SpawnDB is a Top Level Object that has members you can access within your macros and holyshits/downshits.
Example: `${SpawnDB.Known[Fippy Darkpaw]}`

Spawns are stored by Target.DisplayName in the database. So always reference the DisplayName when trying to access spawn data.

**Strings:**
```
  Notes - what custom notes have i added
  Body - what was this npc's original body type for when i use body changing spells
```

**Bools:**
```
  Known - is this in the database
  Named - is this considered named
  Quest - is this considered a quest mob
  Ignore - should i ignore this entirely
  ImmunityCheck - should i check immunities on this mob
  Burn - should i burn this mob? default true if named
    
  ImmuneMez
  ImmuneCharm
  ImmuneSlow
  ImmuneSnare
  ImmuneStun

  ImmuneCold
  ImmuneDisease
  ImmuneFire
  ImmunePoison
  ImmuneMagic
```


**Integers: **
```
  Size - how many unique npcs do i know about this zone
  MinLevel - what is the minimum level I have seen this npc
  MaxLevel - what is maximum level i have seen this npc
```
