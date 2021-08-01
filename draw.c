
#include "draw.h"
#include "game.h"
#include "settings.h"

#include <string.h>

static const unsigned	s_colors[] = {
	0xFF000000,

	0xFFFFFF00,
	0xFFFF0000,
	0xFF507FFF,
	0xFF00FFFF,
	0xFF00FF00,
	0xFFFF00FF,
	0xFF0000FF,
};

struct vars {
	int	field_offset_x;
	int	field_offset_y;
	int	tile_width;
};

static void	draw_field (struct framebuffer *buffer, struct game *game, struct vars *vars);

void	draw_game (struct framebuffer *buffer, struct game *game) {
	struct vars vars;

	vars.field_offset_x = 20;
	vars.field_offset_y = 20;

	const int	field_height = sizeof game->field / sizeof game->field[0];
	const int	field_width = sizeof game->field[0] / sizeof game->field[0][0];

	const int	tile_width_y = (buffer->height - 2 * vars.field_offset_y) / field_height;
	const int	tile_width_x = (buffer->width - 2 * vars.field_offset_x) / field_width;

	vars.tile_width = tile_width_y < tile_width_x ? tile_width_y : tile_width_x;

	vars.field_offset_x = (buffer->width - field_width * vars.tile_width) / 2;

	draw_field (buffer, game, &vars);

}

static unsigned	set_color_brightness (unsigned color, float brightness) {
	int			r = color & 0x000000FF;
	int			g = (color & 0x0000FF00) >> 8;
	int			b = (color & 0x00FF0000) >> 16;

	color = ((int) (r * brightness) & 0xFF);
	color += ((int) (g * brightness) & 0xFF) << 8;
	color += ((int) (b * brightness) & 0xFF) << 16;
	color += 0xFF000000;
	return (color);
}

static void	draw_field (struct framebuffer *buffer, struct game *game, struct vars *vars) {

	const float	color_brightness = Tile_Brightness;

	const int	field_height = sizeof game->field / sizeof game->field[0];
	const int	field_width = sizeof game->field[0] / sizeof game->field[0][0];

	const int	holding_height = sizeof game->holding / sizeof game->holding[0];
	const int	holding_width = sizeof game->holding[0] / sizeof game->holding[0][0];

	for (int y = 0; y < field_height; y += 1) {
		for (int x = 0; x < field_width; x += 1) {

			unsigned char	*ptr = buffer->data
				+ (y * vars->tile_width + vars->field_offset_y) * buffer->stride
				+ (x * vars->tile_width + vars->field_offset_x) * buffer->channels;

			for (int tile_y = 0; tile_y < vars->tile_width; tile_y += 1) {
				for (int tile_x = 0; tile_x < vars->tile_width; tile_x += 1) {
					unsigned	*out = (unsigned *)(ptr + tile_y * buffer->stride + tile_x * buffer->channels);

					*out = set_color_brightness (s_colors[game->field[y][x]], color_brightness);
				}
			}

		}

		if (game->is_holding && y >= game->holding_y && y < game->holding_y + holding_height) {
			for (int x = 0; x < holding_width; x += 1) {
				int pos_x = x + game->holding_x;
				if (pos_x < 0) {
					continue ;
				}
				if (pos_x >= field_width) {
					break ;
				}

				unsigned char	*ptr = buffer->data
					+ (y * vars->tile_width + vars->field_offset_y) * buffer->stride
					+ ((x + game->holding_x) * vars->tile_width + vars->field_offset_x) * buffer->channels;

				for (int tile_y = 0; tile_y < vars->tile_width; tile_y += 1) {
					for (int tile_x = 0; tile_x < vars->tile_width; tile_x += 1) {
						unsigned	*out = (unsigned *)(ptr + tile_y * buffer->stride + tile_x * buffer->channels);

						if (game->holding[y - game->holding_y][x] != Color_Empty) {
							*out = set_color_brightness (s_colors[game->holding[y - game->holding_y][x]], color_brightness);
						}
					}
				}

			}
		}
	}
}















