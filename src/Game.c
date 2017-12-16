#include "Game.h"

#include "Loader.h"
#include "System.h"
#include "Video.h"

#include "UI/Window.h"

#include "Data/State.h"
#include "UI/TopMenu.h"

#include "building/model.h"
#include "core/debug.h"
#include "core/lang.h"
#include "core/random.h"
#include "figure/type.h"
#include "game/settings.h"
#include "graphics/image.h"
#include "scenario/property.h"
#include "scenario/scenario.h"
#include "sound/system.h"

#include <string.h>

static void errlog(const char *msg)
{
    debug_log(msg, 0, 0);
}

static void loadDefaultNames()
{
    // TODO move out
    scenario_set_player_name(lang_get_string(9, 5));
}

int Game_preInit()
{
    settings_load();
    scenario_settings_init();
    Data_State.gamePaused = 0;
    UI_TopMenu_initFromSettings(); // TODO eliminate need for this

	if (!lang_load("c3.eng", "c3_mm.eng")) {
		errlog("ERR: 'c3.eng' or 'c3_mm.eng' files not found or too large.");
		return 0;
	}
	loadDefaultNames();
	random_init();
	return 1;
}

int Game_init()
{
	System_initCursors();
	if (!image_init()) {
		errlog("ERR: unable to init graphics");
		return 0;
	}
	
	if (!image_load_climate(CLIMATE_CENTRAL)) {
		errlog("ERR: unable to load main graphics");
		return 0;
	}
	if (!image_load_enemy(ENEMY_0_BARBARIAN)) {
		errlog("ERR: unable to load enemy graphics");
		return 0;
	}

	if (!model_load()) {
		errlog("ERR: unable to load c3_model.txt");
		return 0;
	}

	sound_system_init();
	Loader_GameState_init();
	UI_Window_goTo(Window_Logo);
	return 1;
}

void Game_exit()
{
    Video_shutdown();
    settings_save();
    sound_system_shutdown();
}
