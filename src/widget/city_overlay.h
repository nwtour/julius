#ifndef WIDGET_CITY_OVERLAY_H
#define WIDGET_CITY_OVERLAY_H

#include "building/building.h"
#include "figure/figure.h"
#include "graphics/tooltip.h"

#define NO_COLUMN -1

enum {
    COLUMN_TYPE_RISK,
    COLUMN_TYPE_ACCESS
};

typedef struct {
    int type;
    int column_type;
    int (*show_building)(const building *b);
    int (*show_figure)(const figure *f);
    int (*get_column_height)(const building *b);
    int (*get_tooltip_for_grid_offset)(tooltip_context *c, int grid_offset);
    int (*get_tooltip_for_building)(tooltip_context *c, const building *b);
    void (*draw_custom_footprint)(int x, int y, int grid_offset);
    void (*draw_custom_top)(int x, int y, int grid_offset);
} city_overlay;

void city_with_overlay_draw_building_footprint(int x, int y, int grid_offset, int image_offset);

void city_with_overlay_draw_building_top(int x, int y, int grid_offset);

#endif // WIDGET_CITY_OVERLAY_H
