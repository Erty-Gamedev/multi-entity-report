# Multi Entity Report

CLI tool for performing 'entity report'-like search on all GoldSrc maps in a Steam directory.

Output is a list of BSP files containing the entities matching the search queries.<br>
Each BSP will have its own list of the matching entities by their classnames,
index in the entity lump, targetname if it has one, and which keys/values that
matched the queries.

## Usage

The first time you call the application it will ask you for your Steam directory.<br>
Call the application through a commandline interface with one or more arguments
defining the search query. You may optionally provide mod names to narrow the search
to those mods only, e.g. `cstrike` or `valve`.

Search queries are key=value pairs separated by spaces and implicitly or-chained.
Use the *AND* keyword to and-chain two queries.<br>
The value can be left out to only match the key or the key can be left out
to search for any matching value.

Keys with multiple values such as *origin* can be queried on a specific element
using square brackets index notation, e.g. `origin[1]` will query the second element.

Other operators can be used instead of `=` to affect the behavior of the query:

| Operator   | Description                               |
| ---------- | ----------------------------------------- |
| =          | Partial match key/value with these terms  |
| ==         | Match only exact key/value                |
| !=         | Match key/value that isn't these terms    |
| <          | Numeric less than comparison              |
| >          | Numeric greater than comparison           |
| <=         | Numeric less or equal to comparison       |
| >=         | Numeric greater or equal to comparison    |

To query an empty value use a percentage sign (`%`), e.g. `angles[1]=%`

### Example

```cli
mer.exe valve classname=monster_gman AND =argument
```

Will result in

```txt
Half-Life\valve\maps\c1a0.bsp: [
  monster_gman (index 55, targetname 'argumentg', classname=monster_gman AND targetname=argumentg)
]
```

### Wildcards

Asterisk (`*`) characters can be used as wildcards to change the
partial match operator (`=`) from its default *starts with* behavior
to checking if a value *ends with* the search term (asterisk at the 
end of the term, e.g. `=*rockgibs.mdl`) or *contains* the search term
(asterisk at beginning and end of the term, e.g. `=*duck*`).

### Spawnflags

A search query on the spawnflags key will check if *any* flag of the
search term is set when using the `=` operator (`spawnflags & query`),
if using the `==` operator *all* flags in the search term must be set to match
(`spawnflags & query == query`), and if using the `!=` operator
the query will only match if none of the flags in the search term matches
(`spawnflags & query == 0`).<br>
Comparison operators will do numeric comparisons as normal.

## Interactive mode

You may also run the applications without any arguments,
in which case it will start interactive mode.

## Special thanks

Many thanks goes out to RaptorSKA for helping out with testing
and providing valuable feedback!
