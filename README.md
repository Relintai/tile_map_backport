# RTileMap

Godot 4.0's TileMap backported to 3.x as an engine module, with (eventually) a few smaller features added.

The tilemap classes will be prefixed with R, so it compiles cleanly with the built in TileMap class.

It needs https://github.com/godotengine/godot/pull/48395 to work.

Deprecated. It turns out I need something else.

# Building

1. Get the source code for the engine.

```git clone -b 3.x https://github.com/godotengine/godot.git godot```

2. Go into Godot's modules directory.

```
cd ./godot/modules/
```

3. Clone this repository

```
git clone https://github.com/Relintai/tile_map_backport.git rtile_map
```

The folder needs to be named `rtile_map`!

4. Build Godot. [Tutorial](https://docs.godotengine.org/en/latest/development/compiling/index.html)
