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

class CustomManager {
public:
	static CustomManager &getInstance();

	void load(const std::string &dataPath);

	bool isLoaded() const {
		return m_loaded;
	}

	std::optional<CustomDamageType> getDamageType(CombatType_t type) const;
	std::optional<CustomCondition> getCondition(ConditionType_t type) const;

	bool isDamageTypeActive(CombatType_t type) const;
	bool isConditionActive(ConditionType_t type) const;

	static CombatType_t damageSlotToEnum(int slot);
	static ConditionType_t conditionSlotToEnum(int slot);

private:
	CustomManager() = default;

	bool m_loaded = false;
	CustomDamageType m_damageTypes[8] = {};
	bool m_damageActive[8] = {};
	CustomCondition m_conditions[8] = {};
	bool m_conditionActive[8] = {};
};

inline CustomManager &g_customManager() {
	return CustomManager::getInstance();
}
