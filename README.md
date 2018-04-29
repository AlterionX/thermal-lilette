# Thermal Lilette

Explosion Particle System

## Overall Structure
### CPU
* Particle info: lifetime (expiration time)
* Render setup
  * Scene creation: generate meshes
* Control: through IOM
  * Camera
  * Keys
  
### GPU
* Render
* Particle simulation
  * State: position, velocity, presure, tempurature
  * Model: changes of states
  
### Pipeline
1. [CPU] initialize + start render loop
2. [CPU -> GPU] transfer uniform
3. [CPU] update particle info
4. [GPU] update particle model
5. [GPU] update particle state
6. [GPU] render particle & objects
7. [GPU -> CPU] transfer result

## Game Plan
### Particle system
- [ ] Instances rendering
- [ ] Particle generation
- [ ] Constant acceleration field (e.g. earth gravity, simple buoyancy)

**Test 1: water fountain, bubbles**

### Tools
- [ ] Design interface for scene creations
  - [ ] Click/drag interaction: create particle at positions
- [x] Visualization of 3D texture: alpha blending projection plane
  - [ ] Adaptive plane projection

**Test 2: water vapour in turbulence**

### Gas simulation
- [x] Navier-Strokes Equation with u', x'

### Explosion simulation
- [ ] Feldman's model I
  - [ ] Fluid/Gas model: u', p', T'
  - [ ] Particulate model: x'', Y'
  
**Test 3.1: set phi/H'/f constant -> gas portal**

- [ ] Feldman's model II: Combustion
  - [ ] Velocity div: d phi
  - [ ] Heat transfer: H'
  - [ ] External force: f
  - [ ] Soot: s (maybe?)
  
**Test 3.2: explosion!**

### Interaction with objects (???)

## Reference
- http://graphics.berkeley.edu/papers/Feldman-ASP-2003-08/Feldman-ASP-2003-08.pdf

## Useful Links
- https://www.3dgep.com/simulating-particle-effects-using-opengl/
