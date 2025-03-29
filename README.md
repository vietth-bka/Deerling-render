# lightwave

Welcome to _lightwave_, an educational framework for writing ray tracers that can render photo-realistic images!
Lightwave provides the boring boilerplate, so you can focus on writing the insightful parts.
It aims to be minimal enough to remain comprehensible, yet flexible enough to provide a solid foundation even for sophisticated rendering algorithms.

## Assignments
We want you to make this renderer truly your own. Our assignments provide ample opportunities to customize your renderer with features that you personally find interesting, and you are welcome to change each and every line of your renderer as you see fit, including all of its interfaces. The only **hard requirement** is that your renderer remains capable of reading our test scene files, and outputs the right images for those.

You are encouraged to publish your renderer (e.g., on Github) after the course ends, but during the course you are not allowed to share code with other groups. Copying code from other sources is equally prohibited, as blindly copying code defeats the purpose of this project:

We want you to have fun writing your very own ray tracer, producing renders that you can be proud of, and to learn and prepare you for the final exam!

## Overview


## What's Included
Out of the box, lightwave is unable to produce any images, as it lacks all necessary rendering functionality to do so.
It is your job to write the various components that make this possible: You will write camera models, intersect shapes, program their appearance, and orchestrate how rays are traced throughout the virtual scene.
Lightwave supports you in this endeavour by supplying tedious to implement boilerplate, including:

* Modularity
  * Modern APIs flexible enough for sophisticated algorithms
  * Shapes, materials, etc are implemented as plugins
  * Whatever you can think of, you can make a plugin for it
* Basic math library
  * Vector operations
  * Matrix operations
  * Transforms
* File I/O
  * An XML parser for the lightwave scene format
  * Reading and writing various image formats
  * Reading and representing triangle meshes
  * Streaming images to the [tev](https://github.com/Tom94/tev) image viewer
* Multi-threading
  * Rendering is parallelized across all available cores
  * Parallelized scene loading (image loading, BVH building, etc)
* BVH acceleration structure
  * Data-structure and traversal is supplied by us
  * Split-in-the-middle build is supplied as well
  * It's your job to implement more sophisticated building
* Useful utilities
  * Thread-safe logger functionality
  * Assert statements that provide extra context
  * An embedded profiler to identify bottlenecks of your code
  * Random number generators
* A Blender exporter
  * You can easily build and render your own scenes

## Contributors
Lightwave was written by [Alexander Rath](https://graphics.cg.uni-saarland.de/people/rath.html), with contributions from [Ömercan Yazici](https://graphics.cg.uni-saarland.de/people/yazici.html) and [Philippe Weier](https://graphics.cg.uni-saarland.de/people/weier.html).
Many of our design decisions were heavily inspired by [Nori](https://wjakob.github.io/nori/), a great educational renderer developed by Wenzel Jakob.
We would also like to thank the teams behind our dependencies: [ctpl](https://github.com/vit-vit/CTPL), [miniz](https://github.com/richgel999/miniz), [stb](https://github.com/nothings/stb), [tinyexr](https://github.com/syoyo/tinyexr), [tinyformat](https://github.com/c42f/tinyformat), [pcg32](https://github.com/wjakob/pcg32), and [catch2](https://github.com/catchorg/Catch2).
