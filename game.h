
#ifndef Game_H
#define Game_H

#include <stddef.h>

enum color {
	Color_Empty,

	Color_Cyan,
	Color_Blue,
	Color_Orange,
	Color_Yellow,
	Color_Green,
	Color_Purple,
	Color_Red,
};

struct input {
	int		is_left;
	int		is_right;
	int		is_sprint_up;
	int		is_sprint_down;
	int		is_rotate_left;
	int		is_rotate_right;
	int		is_play;
};

struct game {
	int		is_play;

	int		is_holding;
	int		holding_x;
	int		holding_y;

	size_t	time_start;
	size_t	time_last_move;

	int		holding_rotate;
	int		next_holding_index;
	int		current_holding_index;

	int		is_sprint;
	size_t	sprint_freq;
	size_t	freq; // miliseconds per move

	unsigned char	holding[4][4];
	unsigned char	field[15][10];
};

void	init_game (struct game *);
int		run_game (struct game *, struct input *);

extern unsigned char	s_holdings[][4][4][4];

#endif /* Game_H */
