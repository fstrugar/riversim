RiverSimulator, V0.91, 26 Feb 2009, (c) by Filip Strugar (filip.strugar@gmail.com)

******************************************************************************************************
MIT License

Copyright (c) 2010 Filip Strugar

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************************************

Requirements: 

 - WindowsXP/Vista
 - Latest directx (november 2008 and up!)
 - Any CPU and 512Mb of RAM should be enough.
 - Tested on NVidia 8xxx and ATI 4xxx, and should work on all above. Does not work on 7xxx - there are some rendering problems (simulation works fine), and it is not tested on anything else yet. Seems to work on ATI 3xxx-s.


How to run the demo:

 Start the RiverSim.exe, open the project .rsini file. 


Short description:

 This demo has two modes: Simulation and 3DPreview. 

 Simulation mode will use a GPU-based water flow simulation to form flowing water areas on the given terrain (defined by heightmap) using given water input (spring list or simple precipitation). Once the simulation is stabilized, water data can be saved for viewing/interacting with it in 3D.

 3DPreview mode will use Simulator-created data to render the water, and perform simple wave simulation on top of it. In this mode no actual water flow is simulated, but water properties (height/velocities/etc, exported by Simulator) are used to render flowing water and do the simple flowing surface-wave and foam simulation used only for rendering.



Description of project files:

 Project is loaded by loading .rsini project configuration file. Other than that, a project can/must have following additional files:

 Heightmap          - 16bit greyscale defining the terrain, in .tbmp format.
                     (always required, not modified by RiverSim).

 SimWatermap	    - contains current simulation state (water depths & velocities).
                      (if not existing, will be created when simulation mode exits. can be deleted to reset simulation)

 SpringList         - a text file that lists springs that will add water in simulation mode.
                      (not neccessary, not modified by RiverSim)

 RendWaterHeightmap - contains processed water heightmap (16bit grayscale), used by 3DPreview mode.
                      (created automatically every time simulation mode exits)

 RendWaterInfomap   - contains processed water velocities and other info, used by 3DPreview mode.
                      (created automatically every time simulation mode exits)
 


How to start a new project:


 New project requires:

  - Project configuration file (.rsini) with correct settings (paths and file names, world size, etc).
    For a quick start copy this file from any of the demo projects, and change mapDim to reflect your world size. It's best if one heightmap pixel is close to 10.0 x 10.0 in world space. 

  - A terrain heightmap (16bit grayscale in .tbmp format).
    Size is restricted by your GPU's maximum texture size. Make sure HeightmapPath in .rsini points to this file.

  - List of springs that will add water (or, alternatively, global precipitation that adds water everywhere equally can be set in .rsini config file) - see existing projects for examples. Coordinates are in relative [0,1] space - (0,0) is upper left corner and (1,1) is lower right corner of the heightmap. You can read these in simulation mode on the bottom of the screen.



 Once the RiverSim is started use 'F5' to switch to simulation mode, if not already in it. Wait for water quantity to stabilize itself based on water input (springs, global precipitation) and output (edge areas where water flows out of the simulation). This can take a while, so 'B' key can be used to temporarily boost spring output.
 RiverSim will save simulation state (to a file specified by SimWatermap) every time it is exited and reload it on load, so that simulation can be continued later.
 You can modify springs list at runtime and reload it (and parameters from .rsini configuration file) at runtime by clicking Ctrl+F11.

 When water is stabilized, switch to 3DPreview mode (F5) to see it in 3D.



History:

 v0.91 - various minor fixes

 v0.90 - initial version
