# OpenGLRenderer
> A small project showcasing an OpenGL renderer I made whilst learning graphics programming.

## About

This is my project I made whilst learning graphics programming using OpenGL. This repository contains the source files and a standalone demonstration build of the project.

## Features

* Deferred Shading with point and directional lighting.
* MeshRenderer class to easily batch meshes for drawing with instancing.
* StaticMeshRenderer class to easily combine meshes into a static mesh.
* Material class that can draw all objects using it with minimal state changes.
* Skyboxes and Cube Mapping.

## Images

Images taken with some basic primitives and lighting.

<a href="https://imgur.com/7hasjSZ"><img src="https://i.imgur.com/k2RIITL.png" title="source: imgur.com" /></a>
<a href="https://imgur.com/7hasjSZ"><img src="https://i.imgur.com/FssHbmC.png" title="source: imgur.com" /></a>
<a href="https://imgur.com/7hasjSZ"><img src="https://i.imgur.com/AzM9MQi.png" title="source: imgur.com" /></a>

The following images were taken using materials containing diffuse, normal and specular maps. Rendered with deferred PBR shaders.

<a href="https://imgur.com/7hasjSZ"><img src="https://i.imgur.com/7hasjSZ.png" title="source: imgur.com" /></a>
<a href="https://imgur.com/s60Pf6c"><img src="https://i.imgur.com/s60Pf6c.png" title="source: imgur.com" /></a>

## Libraries

The following libraries made this renderer possible.

* OpenGL
* GLFW3
* stb_image.h from https://github.com/nothings/stb
* TinyObjLoader - https://github.com/syoyo/tinyobjloader
