#include "house_population.h"

#include "building/building.h"
#include "building/list.h"
#include "building/model.h"
#include "city/labor.h"
#include "city/message.h"
#include "city/migration.h"
#include "city/population.h"
#include "core/calc.h"
#include "figuretype/migrant.h"

int house_population_add_to_city(int num_people)
{
    int added = 0;
    int building_id = city_population_last_used_house_add();
    for (int i = 1; i < MAX_BUILDINGS && added < num_people; i++) {
        if (++building_id >= MAX_BUILDINGS) {
            building_id = 1;
        }
        building *b = building_get(building_id);
        if (b->state == BUILDING_STATE_IN_USE && b->house_size && b->distance_from_entry > 0 && b->house_population > 0) {
            city_population_set_last_used_house_add(building_id);
            int max_people = model_get_house(b->subtype.house_level)->max_people;
            if (b->house_is_merged) {
                max_people *= 4;
            }
            if (b->house_population < max_people) {
                ++added;
                ++b->house_population;
                b->house_population_room = max_people - b->house_population;
            }
        }
    }
    return added;
}

int house_population_remove_from_city(int num_people)
{
    int removed = 0;
    int building_id = city_population_last_used_house_remove();
    for (int i = 1; i < 4 * MAX_BUILDINGS && removed < num_people; i++) {
        if (++building_id >= MAX_BUILDINGS) {
            building_id = 1;
        }
        building *b = building_get(building_id);
        if (b->state == BUILDING_STATE_IN_USE && b->house_size) {
            city_population_set_last_used_house_remove(building_id);
            if (b->house_population > 0) {
                ++removed;
                --b->house_population;
            }
        }
    }
    return removed;
}

static void fill_building_list_with_houses(void)
{
    building_list_large_clear(0);
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        building *b = building_get(i);
        if (b->state == BUILDING_STATE_IN_USE && b->house_size) {
            building_list_large_add(i);
        }
    }
}

void house_population_update_room(void)
{
    city_population_clear_capacity();

    fill_building_list_with_houses();
    int total_houses = building_list_large_size();
    const int *houses = building_list_large_items();
    for (int i = 0; i < total_houses; i++) {
        building *b = building_get(houses[i]);
        b->house_population_room = 0;
        if (b->distance_from_entry > 0) {
            int max_pop = model_get_house(b->subtype.house_level)->max_people;
            if (b->house_is_merged) {
                max_pop *= 4;
            }
            city_population_add_capacity(b->house_population, max_pop);
            b->house_population_room = max_pop - b->house_population;
            if (b->house_population > b->house_highest_population) {
                b->house_highest_population = b->house_population;
            }
        } else if (b->house_population) {
            // not connected to Rome, mark people for eviction
            b->house_population_room = -b->house_population;
        }
    }
}

int house_population_create_immigrants(int num_people)
{
    int total_houses = building_list_large_size();
    const int *houses = building_list_large_items();
    int to_immigrate = num_people;
    // clean up any dead immigrants
    for (int i = 0; i < total_houses; i++) {
        building *b = building_get(houses[i]);
        if (b->immigrant_figure_id && figure_get(b->immigrant_figure_id)->state != FIGURE_STATE_ALIVE) {
            b->immigrant_figure_id = 0;
        }
    }
    // houses with plenty of room
    for (int i = 0; i < total_houses && to_immigrate > 0; i++) {
        building *b = building_get(houses[i]);
        if (b->distance_from_entry > 0 && b->house_population_room >= 8 && !b->immigrant_figure_id) {
            if (to_immigrate <= 4) {
                figure_create_immigrant(b, to_immigrate);
                to_immigrate = 0;
            } else {
                figure_create_immigrant(b, 4);
                to_immigrate -= 4;
            }
        }
    }
    // houses with less room
    for (int i = 0; i < total_houses && to_immigrate > 0; i++) {
        building *b = building_get(houses[i]);
        if (b->distance_from_entry > 0 && b->house_population_room > 0 && !b->immigrant_figure_id) {
            if (to_immigrate <= b->house_population_room) {
                figure_create_immigrant(b, to_immigrate);
                to_immigrate = 0;
            } else {
                figure_create_immigrant(b, b->house_population_room);
                to_immigrate -= b->house_population_room;
            }
        }
    }
    return num_people - to_immigrate;
}

int house_population_create_emigrants(int num_people)
{
    int total_houses = building_list_large_size();
    const int *houses = building_list_large_items();
    int to_emigrate = num_people;
    for (int level = HOUSE_SMALL_TENT; level < HOUSE_LARGE_INSULA && to_emigrate > 0; level++) {
        for (int i = 0; i < total_houses && to_emigrate > 0; i++) {
            building *b = building_get(houses[i]);
            if (b->house_population > 0 && b->subtype.house_level == level) {
                int current_people;
                if (b->house_population >= 4) {
                    current_people = 4;
                } else {
                    current_people = b->house_population;
                }
                if (to_emigrate <= current_people) {
                    figure_create_emigrant(b, to_emigrate);
                    to_emigrate = 0;
                } else {
                    figure_create_emigrant(b, current_people);
                    to_emigrate -= current_people;
                }
            }
        }
    }
    return num_people - to_emigrate;
}

static void calculate_working_population(void)
{
    int num_plebs = 0;
    int num_patricians = 0;
    int total_houses = building_list_large_size();
    const int *houses = building_list_large_items();
    for (int i = 0; i < total_houses; i++) {
        building *b = building_get(houses[i]);
        if (b->house_population > 0) {
            if (b->subtype.house_level >= HOUSE_SMALL_VILLA) {
                num_patricians += b->house_population;
            } else {
                num_plebs += b->house_population;
            }
        }
    }
    city_labor_calculate_workers(num_plebs, num_patricians);
}

void house_population_update_migration(void)
{
    city_migration_update();

    city_population_yearly_update();
    calculate_working_population();
    // population messages
    int population = city_population();
    if (population >= 500 && city_message_mark_population_shown(500)) {
        city_message_post(1, MESSAGE_POPULATION_500, 0, 0);
    }
    if (population >= 1000 && city_message_mark_population_shown(1000)) {
        city_message_post(1, MESSAGE_POPULATION_1000, 0, 0);
    }
    if (population >= 2000 && city_message_mark_population_shown(2000)) {
        city_message_post(1, MESSAGE_POPULATION_2000, 0, 0);
    }
    if (population >= 3000 && city_message_mark_population_shown(3000)) {
        city_message_post(1, MESSAGE_POPULATION_3000, 0, 0);
    }
    if (population >= 5000 && city_message_mark_population_shown(5000)) {
        city_message_post(1, MESSAGE_POPULATION_5000, 0, 0);
    }
    if (population >= 10000 && city_message_mark_population_shown(10000)) {
        city_message_post(1, MESSAGE_POPULATION_10000, 0, 0);
    }
    if (population >= 15000 && city_message_mark_population_shown(15000)) {
        city_message_post(1, MESSAGE_POPULATION_15000, 0, 0);
    }
    if (population >= 20000 && city_message_mark_population_shown(20000)) {
        city_message_post(1, MESSAGE_POPULATION_20000, 0, 0);
    }
    if (population >= 25000 && city_message_mark_population_shown(25000)) {
        city_message_post(1, MESSAGE_POPULATION_25000, 0, 0);
    }
}

void house_population_evict_overcrowded(void)
{
    int size = building_list_large_size();
    const int *items = building_list_large_items();
    for (int i = 0; i < size; i++) {
        building *b = building_get(items[i]);
        if (b->house_population_room < 0) {
            int num_people_to_evict = -b->house_population_room;
            figure_create_homeless(b->x, b->y, num_people_to_evict);
            if (num_people_to_evict < b->house_population) {
                b->house_population -= num_people_to_evict;
            } else {
                // house has been removed
                b->state = BUILDING_STATE_UNDO;
            }
        }
    }
}
