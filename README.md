# tyler

> [!WARNING]
> Work in progress

Tyler is autotiling tool + exporter based on Godot's autotiling system. Tyler comes with a small accompanying library [ty.h](/src/ty.h) that enables you to easily implement autotiling in your game/engine. See [examples/example.c](/examples/example.c) for an example of how to use Tyler + ty.h in your project.

> [!NOTE]
> Tyler is a tool I'm making for my own game, and it has been put together quickly over a week. Expect bugs and missing features!

## Features

- [X] 3x3 simplified mode
- [X] Export masks as JSON or C Header
- [X] Accompanying library -- [ty.h](/src/ty.h)

## Preview

![Screenshot](/assets/screenshot.png)

## TODO

- [ ] 2x2 mode
- [ ] 3x3 mode (implemented but needs adding to menus)
- [ ] Finish map editor window
- [ ] Load + modify tile mask atlas from mask editor
- [ ] Import masks + maps
- [ ] Map editor select tiles (modify multiple tiles at once)
- [ ] Modal tile selection + editing
- [ ] Documentation + example for ty.h

## Credits

### Assets

- Default 2x2 + 3x3 textures were taking from a video by [SlushyGames](https://www.youtube.com/watch?v=ZDghWCd_1k8).

### Libraries

- [floooh/sokol](https://github.com/floooh/sokol) (zlib/libpng)
    - sokol_gfx.h
    - sokol_app.h
    - sokol_glue.h
    - sokol_imgui.h
    - sokol_log.h
    - sokol_time.h
- [edubart/sokol_gp](https://github.com/edubart/sokol_gp) (MIT-0)
    - sokol_gp.h
- [tsoding/jim](https://github.com/tsoding/jim) (MIT)
    - jim.h
- [ocornut/imgui](https://github.com/ocornut/imgui) (MIT)
    - deps/imgui
- [cimgui/cimgui](https://github.com/cimgui/cimgui) (MIT)
    - cimgui.h
    - cimgui.cpp
- [AndrewBelt/osdialog](https://github.com/AndrewBelt/osdialog) (CC0-1.0)
    - osdialog.h
    - osdialog*.c

## LICENSE
```
The MIT License (MIT)

Copyright (c) 2024 George Watson

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```
