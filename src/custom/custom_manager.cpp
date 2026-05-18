/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019–present OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#include "custom/custom_manager.hpp"

#include <algorithm>
#include <fstream>

#include <nlohmann/json.hpp>

#include "utils/tools.hpp"

using json = nlohmann::json;

CustomManager &CustomManager::getInstance() {
	static CustomManager instance;
	return instance;
}

void CustomManager::load(const std::string &dataPath) {
	m_loaded = false;
	std::fill(std::begin(m_damageActive), std::end(m_damageActive), false);
	std::fill(std::begin(m_conditionActive), std::end(m_conditionActive), false);

	const std::string path = dataPath + "/custom/types.json";
	std::ifstream file(path);
	if (!file.is_open()) {
		m_loaded = true;
		return;
	}

	json root;
	try {
		file >> root;
	} catch (const json::parse_error &e) {
		g_logger().error("[CustomManager::load] Failed to parse {}: {}", path, e.what());
		return;
	}

	if (root.contains("damageTypes") && root["damageTypes"].is_array()) {
		for (const auto &entry : root["damageTypes"]) {
			int slot = entry.value("slot", 0);
			if (slot < 1 || slot > 8) {
				continue;
			}
			const int idx = slot - 1;
			m_damageTypes[idx].name = entry.value("name", "Custom " + std::to_string(slot));
			m_damageTypes[idx].textColor = static_cast<uint8_t>(entry.value("textColor", 215));
			m_damageTypes[idx].combatEffect = static_cast<uint16_t>(entry.value("combatEffect", 0));
			m_damageTypes[idx].shootType = static_cast<uint8_t>(entry.value("shootType", 0));
			m_damageActive[idx] = true;
		}
	}

	if (root.contains("conditions") && root["conditions"].is_array()) {
		for (const auto &entry : root["conditions"]) {
			int slot = entry.value("slot", 0);
			if (slot < 1 || slot > 8) {
				continue;
			}
			const int idx = slot - 1;
			m_conditions[idx].name = entry.value("name", "Custom " + std::to_string(slot));
			m_conditions[idx].isBuff = entry.value("isBuff", false);
			m_conditionActive[idx] = true;
		}
	}

	m_loaded = true;
	g_logger().info("[CustomManager] Loaded custom types from {}", path);
}

std::optional<CustomDamageType> CustomManager::getDamageType(CombatType_t type) const {
	if (type < COMBAT_CUSTOM_1 || type > COMBAT_CUSTOM_8) {
		return std::nullopt;
	}
	const int idx = static_cast<int>(type) - static_cast<int>(COMBAT_CUSTOM_1);
	if (!m_damageActive[idx]) {
		return std::nullopt;
	}
	return m_damageTypes[idx];
}

std::optional<CustomCondition> CustomManager::getCondition(ConditionType_t type) const {
	if (type < CONDITION_CUSTOM_1 || type > CONDITION_CUSTOM_8) {
		return std::nullopt;
	}
	const int idx = static_cast<int>(type) - static_cast<int>(CONDITION_CUSTOM_1);
	if (!m_conditionActive[idx]) {
		return std::nullopt;
	}
	return m_conditions[idx];
}

bool CustomManager::isDamageTypeActive(CombatType_t type) const {
	return getDamageType(type).has_value();
}

bool CustomManager::isConditionActive(ConditionType_t type) const {
	return getCondition(type).has_value();
}

CombatType_t CustomManager::damageSlotToEnum(int slot) {
	if (slot < 1 || slot > 8) {
		return COMBAT_NONE;
	}
	return static_cast<CombatType_t>(static_cast<int>(COMBAT_CUSTOM_1) + slot - 1);
}

ConditionType_t CustomManager::conditionSlotToEnum(int slot) {
	if (slot < 1 || slot > 8) {
		return CONDITION_NONE;
	}
	return static_cast<ConditionType_t>(static_cast<int>(CONDITION_CUSTOM_1) + slot - 1);
}
