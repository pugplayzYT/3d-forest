OpenGL Procedural Forest Simulator
A 3D OpenGL simulation of a procedurally generated forest environment featuring walk/fly mode, basic physics, lighting effects (sun simulation), and collision with trees, houses, apartment towers, and balconies.

Features
🌲 Procedurally placed trees, bushes, houses, and apartment towers

🌇 A configurable sunlight cube with realistic positioning and shading

🧍 First-person camera with support for:

Walking and jumping (with collision and gravity)

Fly mode (no clip, no gravity)

🌆 Balconies on apartment towers with 3D railing geometry and collision

🖱️ Mouse look controls and adjustable FOV

🌐 Fullscreen toggle with F11

🧪 Seed-based world generation for consistent layouts

🪟 Windows-exclusive GUI dialog for seed entry and fly mode toggle

🧱 Object collisions including:

Cylinder-based collision (trees)

AABB-based collision (houses, towers, balconies)

🎨 Configurable colors, shader logic, and OpenGL setup

Controls
Key	Action
W / A / S / D	Move forward/left/back/right
SPACE	Jump (walk mode) or ascend (fly)
CTRL	Descend in fly mode
SHIFT	Sprint / increase fly speed
F11	Toggle fullscreen
ESC	Exit application
Mouse	Look around (first-person view)
Requirements
OpenGL 3.3 compatible GPU

C++17 compatible compiler

GLFW

GLAD

GLM

Windows (for seed dialog box), though the core simulation works cross-platform

Build Instructions
Make sure you have installed the required dependencies:

GLFW

GLAD

GLM

Compile the source code with your preferred C++ build system. Example using g++:

bash
Copy
Edit
g++ main.cpp -o ForestSim -lglfw -lGL -ldl -lX11 -lpthread -lXrandr -lXi
For Windows, make sure to link against Comctl32.lib and set up GLAD/GLFW/GLM properly in your Visual Studio project.

Notes
Fly mode disables all collision detection and gravity.

If you enter the seed 666, an easter egg activates turning the sky red.

Bushes currently do not have collision.

Known Limitations
No lighting/shadows beyond basic color shading.

Bushes are purely decorative and non-collidable.

No texture mapping; only solid colors.

No saving/loading of the generated world.
