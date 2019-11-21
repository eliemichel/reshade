ReShade
=======

*Forked from https://github.com/crosire/reshade*

This is a generic post-processing injector for games and video software. It exposes an automated way to access both frame color and depth information and a custom shader language called ReShade FX to write effects like ambient occlusion, depth of field, color correction and more which work everywhere.

## Remote Control Branch

[![Remote Control Branch Demo](https://img.youtube.com/vi/IJwWgDVXk2g/0.jpg)](https://www.youtube.com/watch?v=IJwWgDVXk2g)

This branch introduces a remote control mechanism. The injected process listen on ports 36150, 36151, etc. (one for each runtime). You can send basic commands to it over TCP. The only command supported at the moment is "reload", that triggers the reload of shaders.

In my tests, the runtime I had to send commands to was the one listening to port 36152, but this may vary depending on the injected application.

**NB** This work is intended to be used for live performances, so aims at being working, not at being user friendly, cause I have no time to make it user friendly. Hence this is likely not going to be merged upstream.

You can use anything to send reload message. For commodity, the `ReShade Remote.exe` utility program is provided:

	"ReShade Remote.exe" -n 127.0.0.1 -p 36152 -d reload

You can also use the Sublime Text package in `sublime`

## Building

You'll need Visual Studio 2017 or higher to build ReShade and Python for the `gl3w` dependency. The [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#windows) is required if building with Vulkan support.

1. Clone this repository including all Git submodules
2. Open the Visual Studio solution
3. Select either the "32-bit" or "64-bit" target platform and build the solution (this will build ReShade and all dependencies).

After the first build, a `version.h` file will show up in the [res](/res) directory. Change the `VERSION_FULL` definition inside to something matching the current release version and rebuild so that shaders from the official repository at https://github.com/crosire/reshade-shaders won't cause a version mismatch error during compilation.

## Contributing

Any contributions to the project are welcomed, it's recommended to use GitHub [pull requests](https://help.github.com/articles/using-pull-requests/).

## License

All source code in this repository is licensed under a [BSD 3-clause license](LICENSE.md).
