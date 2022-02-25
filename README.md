## DirectXHook + Overlay Framework
This is a DirectX hook that works with DirectX 11 and DirectX 12. A simple but primitive overlay framework is included.

### Overlay examples
#### A mod for Monster Hunter Rise, [RiseDpsMeter found on NexusMods](https://www.nexusmods.com/monsterhunterrise/mods/289)

-add video-

#### Test triangle

-add video-

### How to create an overlay
First, check the [wiki page](https://github.com/techiew/DirectXHook/wiki/How-to-set-up-the-Visual-Studio-solution) on how to quickly set up the Visual Studio solution.

When the project is built, "dxgi.dll" will be generated in the project folder. This can be copied next to a game executable which uses DirectX 11 or 12. The game will load the .dll automatically on startup and will render what you told it to.

#### Create files
Create a .cpp and .h file in the Overlays folder (optionally put these inside a parent folder):

-add image-

Create a class that inherits from the IRenderCallback interface and includes "OverlayFramework.h":

-add image-

Define the Setup() and Render() functions in the .cpp file:

-add image-

**Note: Setup() is called once and Render() is called every frame. InitFramework() must be called on the very first line in Setup().**

Make the hook render your stuff by adding these lines in DllMain.cpp:

-add image-

Now we need some stuff to render.

#### Boxes
All rendering with the overlay framework is done using Boxes:

-add image-

Boxes are a simple struct with data that the framework manages.

Create some boxes and render them:

-add image-

Result:

-add image-

Boxes can be rendered with either textures or colors:

-add image-

**Note: textures must be loaded in Setup().**

Result:

-add image-

Text can be rendered inside Boxes:

-add image-

**Note: a font must be set before rendering text.**

Result:

-add image-

### Contributions
Feel free to create issues or contribute code to the repo.

### License
Feel free to use this code for anything and however you like, but if you create something with it then it would be cool if you could show me what you made :)

