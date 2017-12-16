#ifndef SCENARIO_PROPERTY_H
#define SCENARIO_PROPERTY_H

#include <stdint.h>

typedef enum {
    CLIMATE_CENTRAL = 0,
    CLIMATE_NORTHERN = 1,
    CLIMATE_DESERT = 2
} scenario_climate;

int scenario_is_custom();
void scenario_set_custom(int custom);

int scenario_campaign_rank();
void scenario_set_campaign_rank(int rank);
int scenario_campaign_mission();
void scenario_set_campaign_mission(int mission);
int scenario_is_tutorial_1();
int scenario_is_tutorial_2();
int scenario_is_tutorial_3();

int scenario_starting_favor();
int scenario_starting_personal_savings();

const uint8_t *scenario_name();
void scenario_set_name(const uint8_t *name);

const uint8_t *scenario_player_name();
void scenario_set_player_name(const uint8_t *name);

int scenario_is_open_play();

int scenario_open_play_id();

scenario_climate scenario_property_climate();

int scenario_property_start_year();

int scenario_property_rome_supplies_wheat();

int scenario_property_enemy();

int scenario_property_player_rank();

int scenario_image_id();

const uint8_t *scenario_brief_description();

int scenario_initial_funds();

int scenario_rescue_loan();

#endif // SCENARIO_PROPERTY_H
