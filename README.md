## DirectXHook + Overlay Framework
This is a DirectX hook that works with DirectX 11 and DirectX 12. A straightforward but primitive overlay framework is included.

### Overlay examples
#### A mod for Monster Hunter Rise, [RiseDpsMeter found on NexusMods](https://www.nexusmods.com/monsterhunterrise/mods/289)

-add video-

#### Test triangle

-add video-

## How to create an overlay
First, check the [wiki page](https://github.com/techiew/DirectXHook/wiki/How-to-set-up-the-Visual-Studio-solution) on how to quickly set up the Visual Studio solution.

When the project is built, "dxgi.dll" will be generated in the project folder. This can be copied next to a game executable which uses DirectX 11 or 12. The game will load the .dll automatically on startup and will render what you told it to.

### Create files
Create a .cpp and .h file in the Overlays folder (optionally put these inside a parent folder):

![create_files](https://github.com/techiew/DirectXHook/blob/master/pictures/create_files.png)

Create a class that inherits from the IRenderCallback interface and includes "OverlayFramework.h":
![example_header](https://github.com/techiew/DirectXHook/blob/master/pictures/example_header.png)

Define the Setup() and Render() functions in the .cpp file:
![example_source](https://github.com/techiew/DirectXHook/blob/master/pictures/example_source.png)
**Note: Setup() is called once and Render() is called every frame. InitFramework() must be called on the very first line in Setup().**

Make the hook render your stuff by adding these lines in DllMain.cpp:
![dllmain](https://github.com/techiew/DirectXHook/blob/master/pictures/dllmain.png)

### Boxes
All rendering with the overlay framework is done using Boxes:

![box_struct](https://github.com/techiew/DirectXHook/blob/master/pictures/box_struct.png)

Boxes are a simple struct with data that the framework manages.

Create some boxes and render them:

-add image-

Result:

![rgb_boxes](https://github.com/techiew/DirectXHook/blob/master/pictures/rgb_boxes.png)

Boxes can be rendered with either textures or colors:

-add image-

**Note: textures must be loaded in Setup().**

Result:

![textures](https://github.com/techiew/DirectXHook/blob/master/pictures/textures.png)

Text can be rendered inside Boxes:

-add image-

**Note: a font must be set before rendering text.**

Result:

![text](https://github.com/techiew/DirectXHook/blob/master/pictures/text.png)

### Contributions
Feel free to create issues or contribute code to the repo.

### License
Feel free to use this code for anything and however you like, but if you create something with it then it would be cool if you could show me what you made :)

