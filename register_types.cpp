/* register_types.cpp */

#include "register_types.h"

#include "core/object/class_db.h"
#include "summator.h"

void initialize_godot_ability_system_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
			return;
	}
	ClassDB::register_class<Summator>();
}

void uninitialize_godot_ability_system_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
			return;
	}
   // Nothing to do here in this example.
}
