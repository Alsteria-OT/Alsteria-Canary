#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include "creatures/creatures_definitions.hpp"
#include "utils/utils_definitions.hpp"

struct CustomDamageType {
	std::string name;
	uint8_t textColor = 215;      // TEXTCOLOR_WHITE_EXP default
	uint16_t combatEffect = 0;    // MagicEffectClasses value, 0 = none
	uint8_t shootType = 0;        // ShootType_t value, 0 = none
};

struct CustomCondition {
	std::string name;
	bool isBuff = false;
};

class CustomManager {
public:
	static CustomManager &getInstance();

	// Call once at server startup — reads data/custom/types.json
	void load(const std::string &dataPath);

	bool isLoaded() const { return m_loaded; }

	// Returns the display name for a custom combat type slot (1-8), or empty if unset
	std::optional<CustomDamageType> getDamageType(CombatType_t type) const;

	// Returns the display name for a custom condition slot (1-8), or empty if unset
	std::optional<CustomCondition> getCondition(ConditionType_t type) const;

	// True if the slot has been configured
	bool isDamageTypeActive(CombatType_t type) const;
	bool isConditionActive(ConditionType_t type) const;

	// Helpers to convert slot index (1-8) to enum value
	static CombatType_t damageSlotToEnum(int slot);
	static ConditionType_t conditionSlotToEnum(int slot);

private:
	CustomManager() = default;

	bool m_loaded = false;
	CustomDamageType m_damageTypes[8] = {};   // slots 1-8
	bool m_damageActive[8] = {};
	CustomCondition m_conditions[8] = {};     // slots 1-8
	bool m_conditionActive[8] = {};
};

inline CustomManager &g_customManager() {
	return CustomManager::getInstance();
}
