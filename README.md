## Currently refactoring this project as I have added DX12 support to the hook. Repo will be updated in the near future.

This DirectX hook can take control of the DirectX 11/12 renderer in games and applications. The hook should work for most games and can be built for both 32-bit (DirectX 11 only) and 64-bit. **Still under development, but all the core parts work.** 

Below is a video of the hook hooking the renderer in Monster Hunter World.

[![Youtube Video](https://github.com/techiew/DirectX11Hook/blob/master/thumbnail.png)](https://youtu.be/kzF1YnqXKXY)

I'll be adding proper documentation at some point, working on other stuff.

Little note: if you want to compile, you'll need to download directxtk (the DirectX11 version), you can do this in Visual Studio through the package manager.

**Tested in and confirmed working for multiple games (both 32-bit and 64-bit):**

Monster Hunter World, Kenshi, Witcher 3, Divinity Original Sin 2, Metro 2033, and more. The hook should work for everything, but some games might contain edge cases in which the hook either crashes the game or causes graphical glitches.

