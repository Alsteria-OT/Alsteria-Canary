/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019–present OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#pragma once

#include <cstdint>

static constexpr int32_t MAP_MAX_CLIENT_VIEW_PORT_X = 8;
static constexpr int32_t MAP_MAX_CLIENT_VIEW_PORT_Y = 6;
static constexpr int32_t MAP_MAX_VIEW_PORT_X = MAP_MAX_CLIENT_VIEW_PORT_X + 3; // min value: maxClientViewportX + 1
static constexpr int32_t MAP_MAX_VIEW_PORT_Y = MAP_MAX_CLIENT_VIEW_PORT_Y + 5; // min value: maxClientViewportY + 1

// 100-floor world. z grows DOWNWARD: 0 = top of the sky, MAP_MAX_LAYERS-1 = deepest.
static constexpr int8_t MAP_MAX_LAYERS = 100;
static constexpr int8_t MAP_INIT_SURFACE_LAYER = 69; // playable ground floor
static constexpr int8_t MAP_SKY_LAYER = 39; // floors with z < this are "sky" (windowed view)
static constexpr int8_t MAP_LAYER_VIEW_LIMIT = 2;

// ───────────────────────────────────────────────────────────────────────────
// Floor view model — three bands, each deciding which floors are streamed/
// visible from a given z:
//   Underground (z > MAP_INIT_SURFACE_LAYER): windowed, z ± MAP_LAYER_VIEW_LIMIT
//   Surface     (MAP_SKY_LAYER <= z <= MAP_INIT_SURFACE_LAYER): the whole stack
//   Sky         (z < MAP_SKY_LAYER): windowed, z ± MAP_LAYER_VIEW_LIMIT
// The OTClient side MUST mirror this exactly (gameconfig sea-floor/sky-floor +
// the matching logic in protocolgameparse/map) or the streamed map desyncs.
enum MapViewBand : uint8_t {
	MAP_BAND_SKY = 0,
	MAP_BAND_SURFACE = 1,
	MAP_BAND_UNDERGROUND = 2,
};

inline MapViewBand mapViewBand(int32_t z) {
	if (z > MAP_INIT_SURFACE_LAYER) {
		return MAP_BAND_UNDERGROUND;
	}
	if (z >= MAP_SKY_LAYER) {
		return MAP_BAND_SURFACE;
	}
	return MAP_BAND_SKY;
}

// Inclusive [minZ, maxZ] set of floors visible from `z`.
inline void getFloorViewRange(int32_t z, int32_t &minZ, int32_t &maxZ) {
	if (mapViewBand(z) == MAP_BAND_SURFACE) {
		minZ = MAP_SKY_LAYER;
		maxZ = MAP_INIT_SURFACE_LAYER;
	} else {
		minZ = z - MAP_LAYER_VIEW_LIMIT;
		maxZ = z + MAP_LAYER_VIEW_LIMIT;
	}
	if (minZ < 0) {
		minZ = 0;
	}
	if (maxZ > MAP_MAX_LAYERS - 1) {
		maxZ = MAP_MAX_LAYERS - 1;
	}
}

// Floor iteration order for streaming/parsing. Surface renders ground→sky
// (zstep -1); windowed bands render top→bottom (zstep +1). Must match the client.
inline void getFloorIterationRange(int32_t z, int32_t &startz, int32_t &endz, int32_t &zstep) {
	int32_t minZ, maxZ;
	getFloorViewRange(z, minZ, maxZ);
	if (mapViewBand(z) == MAP_BAND_SURFACE) {
		startz = maxZ;
		endz = minZ;
		zstep = -1;
	} else {
		startz = minZ;
		endz = maxZ;
		zstep = 1;
	}
}

inline bool mapSameViewBand(int32_t a, int32_t b) {
	return mapViewBand(a) == mapViewBand(b);
}

// SECTOR_SIZE must be power of 2 value
// The bigger the SECTOR_SIZE is the less hash map collision there should be but it'll consume more memory
static constexpr int32_t SECTOR_SIZE = 16;
static constexpr int32_t SECTOR_MASK = SECTOR_SIZE - 1;
