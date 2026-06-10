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
// MAP_MAX_LAYERS is a COMPILE-TIME capacity (it sizes the per-sector floors[]
// array). The ACTIVE per-map model below may use a smaller maxZ and different
// band boundaries — these are loaded from the map and broadcast to the client.
static constexpr int8_t MAP_MAX_LAYERS = 100;
static constexpr int8_t MAP_INIT_SURFACE_LAYER = 69; // default playable ground floor
static constexpr int8_t MAP_SKY_LAYER = 39; // default: floors with z < this are "sky"
static constexpr int8_t MAP_LAYER_VIEW_LIMIT = 2;

// Per-map floor model — the single runtime source of truth for band boundaries
// and the deepest streamable floor. Defaults to the constants above; the map
// loader (iomap) overrides it from the map's header, and protocolgame sends the
// values to the client so both sides agree without hand-syncing setup.otml.
struct MapFloorModel {
	int32_t maxZ = MAP_MAX_LAYERS - 1;          // deepest floor that exists/streams
	int32_t surfaceLayer = MAP_INIT_SURFACE_LAYER; // surface band bottom (sea floor)
	int32_t skyLayer = MAP_SKY_LAYER;           // surface band top (sky floor)
};
inline MapFloorModel& g_mapFloorModel() {
	static MapFloorModel model;
	return model;
}

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
	const auto &m = g_mapFloorModel();
	if (z > m.surfaceLayer) {
		return MAP_BAND_UNDERGROUND;
	}
	if (z >= m.skyLayer) {
		return MAP_BAND_SURFACE;
	}
	return MAP_BAND_SKY;
}

// Inclusive [minZ, maxZ] set of floors visible from `z`. Each band streams its
// FULL range of floors (mirrors the client gameconfig getFloorViewMinZ/MaxZ):
//   sky        -> [0 .. MAP_SKY_LAYER-1]
//   surface    -> [MAP_SKY_LAYER .. MAP_INIT_SURFACE_LAYER]
//   underground-> [MAP_INIT_SURFACE_LAYER+1 .. MAP_MAX_LAYERS-1]
inline void getFloorViewRange(int32_t z, int32_t &minZ, int32_t &maxZ) {
	const auto &m = g_mapFloorModel();
	switch (mapViewBand(z)) {
		case MAP_BAND_SKY:
			minZ = 0;
			maxZ = m.skyLayer - 1;
			break;
		case MAP_BAND_SURFACE:
			minZ = m.skyLayer;
			maxZ = m.surfaceLayer;
			break;
		case MAP_BAND_UNDERGROUND:
		default:
			minZ = m.surfaceLayer + 1;
			maxZ = m.maxZ;
			break;
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
