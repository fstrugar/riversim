# riversim
GPU-based water flow over heightmap terrain sim, combined with a secondary realtime surface wave sim

This was a personal learning / proof of concept project and it is no longer being updated but the source code is available in case anyone finds it useful.

The first part of the simulation is the offline part, based on the pipe flow model on a heightmap (O'Brien and Hodgins 1995 - http://graphics.berkeley.edu/papers/Obrien-DSS-1995-04/Obrien-DSS-1995-04.pdf, as well as http://old.cescg.org/CESCG-2011/papers/TUBudapest-Jako-Balazs.pdf and http://www-ljk.imag.fr/Publications/Basilic/com.lmc.publi.PUBLI_Inproceedings@117681e94b6_fff75c/FastErosion_PG07.pdf)
This part of simulation outputs water depth map and flow (velocity) field map, which is then used by the next part for real-time visualization.

Second part is a realtime surface wave simulation, based on a simple Finite Difference Method based surface simulator (for ex., https://uk.mathworks.com/matlabcentral/fileexchange/46579-water-surface-simulation-energy-conservation), advected by the velocity field from the previous step (not energy preserving).




Videos available here:

https://www.youtube.com/watch?v=-VjRq8YagdQ

https://www.youtube.com/watch?v=8eH-KMNxPW4

https://www.youtube.com/watch?v=jrhjxudnMNg
