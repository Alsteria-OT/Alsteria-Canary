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
#include <unordered_map>

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
	m_fieldTypes.clear();

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

	// "fields" — register custom magic-field types. Each entry binds an
	// items.xml `field="<name>"` value to a ConditionType (slot 1..8 OR a
	// builtin like "CONDITION_FIRE") + a CombatType (a slot 1..8 OR a
	// builtin like "COMBAT_HOLYDAMAGE"). The two parsers in item_parse.cpp
	// fall through to this registry when a field name isn't built-in, so
	// users can add new field types via JSON without any source change.
	if (root.contains("fields") && root["fields"].is_array()) {
		for (const auto &entry : root["fields"]) {
			std::string name = asLowerCaseString(entry.value("name", std::string {}));
			if (name.empty()) {
				continue;
			}
			CustomFieldType ft;
			ft.name = name;

			// Condition: prefer numeric slot (1..8). Falls back to a string
			// like "CONDITION_FIRE" so users can re-skin existing conditions
			// (rare but allowed for parity with vanilla field behavior).
			int conditionSlot = entry.value("conditionSlot", 0);
			if (conditionSlot >= 1 && conditionSlot <= 8) {
				ft.conditionType = conditionSlotToEnum(conditionSlot);
			}

			// Combat type: prefer numeric custom slot, else map a builtin
			// name string. If both are missing the field still parses, just
			// with COMBAT_NONE — protects against typos breaking server boot.
			int damageSlot = entry.value("damageSlot", 0);
			if (damageSlot >= 1 && damageSlot <= 8) {
				ft.combatType = damageSlotToEnum(damageSlot);
			} else if (entry.contains("combatType") && entry["combatType"].is_string()) {
				ft.combatType = stringToCombatType(entry["combatType"].get<std::string>());
			}

			m_fieldTypes[name] = ft;
		}
	}

	m_loaded = true;
	g_logger().info("[CustomManager] Loaded custom types from {} ({} fields)", path, m_fieldTypes.size());
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

std::optional<CustomFieldType> CustomManager::getFieldType(const std::string &name) const {
	auto it = m_fieldTypes.find(asLowerCaseString(name));
	if (it == m_fieldTypes.end()) {
		return std::nullopt;
	}
	return it->second;
}

CombatType_t CustomManager::getCombatTypeForCondition(ConditionType_t conditionType) const {
	// Iterate registered fields looking for the one bound to this condition
	// slot. The registry is small (≤ 8 entries) so linear scan is fine.
	for (const auto &[name, ft] : m_fieldTypes) {
		if (ft.conditionType == conditionType) {
			return ft.combatType;
		}
	}
	return COMBAT_NONE;
}

CombatType_t CustomManager::stringToCombatType(const std::string &name) {
	// Static map of canonical CombatType_t names → enum values. Matches the
	// strings the OT Forge UI emits in types.json so the round-trip stays
	// readable. Anything missing returns COMBAT_NONE which the field parser
	// treats as "no damage on tick" — server still boots cleanly.
	static const std::unordered_map<std::string, CombatType_t> map = {
		{ "COMBAT_PHYSICALDAMAGE", COMBAT_PHYSICALDAMAGE },
		{ "COMBAT_ENERGYDAMAGE", COMBAT_ENERGYDAMAGE },
		{ "COMBAT_EARTHDAMAGE", COMBAT_EARTHDAMAGE },
		{ "COMBAT_FIREDAMAGE", COMBAT_FIREDAMAGE },
		{ "COMBAT_LIFEDRAIN", COMBAT_LIFEDRAIN },
		{ "COMBAT_MANADRAIN", COMBAT_MANADRAIN },
		{ "COMBAT_HEALING", COMBAT_HEALING },
		{ "COMBAT_DROWNDAMAGE", COMBAT_DROWNDAMAGE },
		{ "COMBAT_ICEDAMAGE", COMBAT_ICEDAMAGE },
		{ "COMBAT_HOLYDAMAGE", COMBAT_HOLYDAMAGE },
		{ "COMBAT_DEATHDAMAGE", COMBAT_DEATHDAMAGE },
		{ "COMBAT_AGONYDAMAGE", COMBAT_AGONYDAMAGE },
		{ "COMBAT_NEUTRALDAMAGE", COMBAT_NEUTRALDAMAGE },
		{ "COMBAT_CUSTOM_1", COMBAT_CUSTOM_1 },
		{ "COMBAT_CUSTOM_2", COMBAT_CUSTOM_2 },
		{ "COMBAT_CUSTOM_3", COMBAT_CUSTOM_3 },
		{ "COMBAT_CUSTOM_4", COMBAT_CUSTOM_4 },
		{ "COMBAT_CUSTOM_5", COMBAT_CUSTOM_5 },
		{ "COMBAT_CUSTOM_6", COMBAT_CUSTOM_6 },
		{ "COMBAT_CUSTOM_7", COMBAT_CUSTOM_7 },
		{ "COMBAT_CUSTOM_8", COMBAT_CUSTOM_8 },
	};
	auto it = map.find(name);
	return it == map.end() ? COMBAT_NONE : it->second;
}
