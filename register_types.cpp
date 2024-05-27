/* register_types.cpp */

#include "register_types.h"

#include "core/object/class_db.h"
#include "summator.h"
#include "attribute_modifier.h"
#include "gameplay_tag.h"
#include "ability_system_client.h"

void initialize_godot_ability_system_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
			return;
	}
	ClassDB::register_class<Summator>();
	ClassDB::register_class<AbilitySystemClient>();
}

void uninitialize_godot_ability_system_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
			return;
	}
   // Nothing to do here in this example.
}
