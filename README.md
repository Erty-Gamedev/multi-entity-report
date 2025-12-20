# Multi Entity Report

CLI tool for performing 'entity report'-like search on all GoldSrc maps in a Steam directory.

Output is a list of BSP files containing the entities matching the search query.<br>
Each BSP will have its own list of the matching entities by their classnames,
index in the entity lump, and targetname if it has one.

## Usage

The first time you call the application it will ask you for your Steam directory.<br>
Call the application through a commandline interface with one or more arguments
defining the search query. You may use the `--mod <mod_name>` option to narrow the
search to within a specific mod folder, e.g. *cstrike* or *valve*.

The following arguments are available for building a search query.
Using them more than once builds a list of possible terms to match against,
i.e. `-k targetname -v my_entity1 -v my_entity2` will match any entity whose targetname is *either* my_entity1 or my_entity2.

By default a partial match is used for classnames, keys and values,
only the beginning of each field need to match the query
(meaning the classname query "monster_alien" will match "monster_alien_controller", "monster_alien_grunt" and "monster_alien_slave").<br>
One can change this by using `--exact` for whole term matches only.

| Argument         | Description                        |
| ---------------- | ---------------------------------- |
| -m, --mod        | Mod folder to search within (leave out for global search) |
| -c, --classname  | Classnames that must match         |
| -k, --key        | Keys that must match               |
| -v, --value      | Values that must match             |
| -f, --flags      | Spawnflags that must match (ALL must match unless --flags_or is used) |
| -o, --flags_or   | Change spawnflag check mode to ANY |
| -e, --exact      | Matches must be exact (whole term) |
| -s, --case       | Make matches case sensitive        |

### Example

```cli
mer.exe --mod valve -c monster_gman -v argument
```

Will result in

```txt
Half-Life\valve\maps\c1a0.bsp: [
  monster_gman (index 54, targetname 'argumentg')
]
```

### Spawnflags

The `--flags` argument will by default match entities that have all the
specified spawnflags enabled, i.e. `-f 2 -f 8` will only match entities that have
both the second (2) and fourth (8) spawnflags enabled.

All `--flags` arguments are bitwise OR'd together,
which means `-f 1 -f 4` is equivalent to `-f 5`.

Using the `--flags_or` argument changes the spawnflags check to ANY mode.
This means only one of the specified spawnflags must be enabled to match.
