This is a simple DX11.1 render engine created to study modern graphics technologies. Particularly Voxel Cone Tracing implementation according to "Interactive Indirect Illumination Using Voxel Cone Tracing" by Crassin et al. (Cyril Crassin, Fabrice Neyret, Miguel Saintz, Simon Green and Elmar Eisemann) https://research.nvidia.com/sites/default/files/publications/GIVoxels-pg2011-authors.pdf
It's easy to use. It has a basic obj/mtl loader, supports static scene, uses effect11 framework and precompiled fx-shaders.
I have added abstracting comments and draw several schemes to simplify codes reading.
If you'd like to program any technique you can start from DefaultShader and RenderTick.

Require: Microsoft Redistributable 2013, d3dcompiler_47.dll, DX11.1 compatible adapter (or at least DX10.0 compatible adapter to run application).

Demo: https://youtu.be/gK837_HTfNU

Author: Dontsov Valentin
Email: dont.val@yandex.ru