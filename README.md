# Script Viewer

![Screenshot](https://imgur.com/BdEslxf.png)

Script Viewer lets you easily view and modify (pause and kill) scripts in Grand Theft Auto V, along with the ability to easily start new ones in an easy to understand interface.

Toggle it in-game by pressing CTRL + O.

## Compiling

You need Visual Studio 2019 along with the most recent build tools.

1. Clone the repo `git clone https://github.com/pongo1231/V_ScriptViewer.git`

2. Go into the cloned directory

3. Initialize all submodules

```
git submodule init
git submodule update --recursive
```

4. Open `vendor/minhook/build/VC16/MinHookVC16.sln` in Visual Studio

5. Compile libMinHook as x64 Release build

6. Open `ScriptViewer.sln` (located in the project root) in Visual Studio
