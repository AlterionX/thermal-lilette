#ifndef CONFIG_H
#define CONFIG_H

/*
 * Global variables go here.
 */

const float kCylinderRadius = 0.25;
const int kMaxBones = 128;
/*
 * Extra credit: what would happen if you set kNear to 1e-5? How to solve it?
 */
const float kNear = 0.1f;
const float kFar = 10000.0f;
const float kFov = 45.0f;

// Floor info.
const float kFloorEps = 0.5 * (0.025 + 0.0175);
const float kFloorXMin = -128.0f;
const float kFloorXMax = 128.0f;
const float kFloorXSize = kFloorXMax - kFloorXMin;

const float kFloorYMin = -224.0f;
const float kFloorYMax = 32.0f;
const float kFloorYSize = kFloorYMax - kFloorYMin;

const float kFloorZMin = -128.0f;
const float kFloorZMax = 128.0f;
const float kFloorZSize = kFloorZMax - kFloorZMin;

const float kFloorY = -0.75617 - kFloorEps;

const float kSkyY = 100;
const float kSkyXMin = -10000.0f;
const float kSkyXMax = -kSkyXMin;
const float kSkyXSize = kSkyXMax - kSkyXMin;
const float kSkyZMin = -10000.0f;
const float kSkyZMax = -kSkyZMin;
const float kSkyZSize = kSkyZMax - kSkyZMin;

const unsigned int kChunkSz = 16;
const unsigned int kChunkH = 256;
const float kScrollSpeed = 64.0f;


#endif
