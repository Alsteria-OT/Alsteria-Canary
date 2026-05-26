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
#include <optional>
#include <string>
#include <unordered_map>

#include "creatures/creatures_definitions.hpp"
#include "utils/utils_definitions.hpp"

struct CustomDamageType {
	std::string name;
	uint8_t textColor = 215;
	uint16_t combatEffect = 0;
	uint8_t shootType = 0;
};

struct CustomCondition {
	std::string name;
	bool isBuff = false;
};

// Registered via the "fields" array in data/custom/types.json. Lets
// items.xml use `<attribute key="field" value="<name>">` for custom
// magic fields (holy, ice, agony, etc.) without modifying the C++
// switch tables in item_parse.cpp.
struct CustomFieldType {
	std::string name;                          // lowercase, matches items.xml value
	ConditionType_t conditionType = CONDITION_NONE;
	CombatType_t combatType = COMBAT_NONE;
};

class CustomManager {
public:
	static CustomManager &getInstance();

	void load(const std::string &dataPath);

	bool isLoaded() const {
		return m_loaded;
	}

	std::optional<CustomDamageType> getDamageType(CombatType_t type) const;
	std::optional<CustomCondition> getCondition(ConditionType_t type) const;
	std::optional<CustomFieldType> getFieldType(const std::string &name) const;

	bool isDamageTypeActive(CombatType_t type) const;
	bool isConditionActive(ConditionType_t type) const;

	static CombatType_t damageSlotToEnum(int slot);
	static ConditionType_t conditionSlotToEnum(int slot);
	static CombatType_t stringToCombatType(const std::string &name);

private:
	CustomManager() = default;

	bool m_loaded = false;
	CustomDamageType m_damageTypes[8] = {};
	bool m_damageActive[8] = {};
	CustomCondition m_conditions[8] = {};
	bool m_conditionActive[8] = {};
	std::unordered_map<std::string, CustomFieldType> m_fieldTypes;
};

inline CustomManager &g_customManager() {
	return CustomManager::getInstance();
}
