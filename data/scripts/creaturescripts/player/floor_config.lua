-- Per-map 3-band floor model sync.
-- Sends the server's active floor model (config.lua defaults, possibly overridden
-- per-map by the OTBM header) to the OTClient so the client mirrors the exact band
-- boundaries used when streaming the map. Without this, a map that embeds a custom
-- floor config in its OTBM header would desync the client's floor rendering.
-- Must match the client handler: ExtendedIds.FloorConfig (8), payload "maxZ,seaFloor,skyFloor".
local FLOOR_CONFIG_OPCODE = 8

local floorConfigLogin = CreatureEvent("FloorConfigLogin")

function floorConfigLogin.onLogin(player)
	if not player then
		return true
	end

	if player:isUsingOtClient() then
		local model = Game.getMapFloorModel()
		local buffer = string.format("%d,%d,%d", model.maxZ, model.seaFloor, model.skyFloor)
		player:sendExtendedOpcode(FLOOR_CONFIG_OPCODE, buffer)
	end

	return true
end

floorConfigLogin:register()
