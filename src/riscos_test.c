/*
 * riscos_test.c : A "game" which can be built and linked for the
 * purpose of testing the graphics code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#ifdef NO_TGMATH_H
#  include <math.h>
#else
#  include <tgmath.h>
#endif

#include "core/puzzles.h"

enum {
	COL_BACKGROUND,
	COL_BLACK,
	COL_RED,
	COL_GREEN,
	COL_BLUE,
	COL_YELLOW,
	COL_MAGENTA,
	COL_CYAN,
	COL_WHITE,
	NCOLOURS
};

struct game_params {
	/* The number of tiles in the X and Y dimensions. */
	int size;
};

struct game_state {
    int FIXME;
};

static game_params *default_params(void)
{
	game_params *ret = snew(game_params);

	ret->size = 20;

	return ret;
}

static bool game_fetch_preset(int i, char **name, game_params **params)
{
    return false;
}

static void free_params(game_params *params)
{
    sfree(params);
}

static game_params *dup_params(const game_params *params)
{
    game_params *ret = snew(game_params);
    *ret = *params;		       /* structure copy */
    return ret;
}

static void decode_params(game_params *params, char const *string)
{
}

static char *encode_params(const game_params *params, bool full)
{
    return dupstr("FIXME");
}

static const char *validate_params(const game_params *params, bool full)
{
    return NULL;
}

static char *new_game_desc(const game_params *params, random_state *rs,
			   char **aux, bool interactive)
{
    return dupstr("FIXME");
}

static const char *validate_desc(const game_params *params, const char *desc)
{
    return NULL;
}

static game_state *new_game(midend *me, const game_params *params,
                            const char *desc)
{
    game_state *state = snew(game_state);

    state->FIXME = 0;

    return state;
}

static game_state *dup_game(const game_state *state)
{
    game_state *ret = snew(game_state);

    ret->FIXME = state->FIXME;

    return ret;
}

static void free_game(game_state *state)
{
    sfree(state);
}

static game_ui *new_ui(const game_state *state)
{
    return NULL;
}

static void free_ui(game_ui *ui)
{
}

static void game_changed_state(game_ui *ui, const game_state *oldstate,
                               const game_state *newstate)
{
}

struct game_drawstate {
	int tilesize;
	int w;
	int h;

    blitter *bl1;
    blitter *bl2;
};

static char *interpret_move(const game_state *state, game_ui *ui,
                            const game_drawstate *ds,
                            int x, int y, int button)
{
    return NULL;
}

static game_state *execute_move(const game_state *state, const char *move)
{
    return NULL;
}

/* ----------------------------------------------------------------------
 * Drawing routines.
 */

static void game_compute_size(const game_params *params, int tilesize,
                              const game_ui *ui, int *x, int *y)
{
	*x = *y = params->size * tilesize;
}

static void game_set_size(drawing *dr, game_drawstate *ds,
                          const game_params *params, int tilesize)
{
	ds->tilesize = tilesize;
	ds->w = ds->h = params->size * tilesize;

    ds->bl1 = blitter_new(dr, tilesize, tilesize);
    ds->bl2 = blitter_new(dr, tilesize, tilesize);
}

static float *game_colours(frontend *fe, int *ncolours)
{
    float *ret = snewn(3 * NCOLOURS, float);

    frontend_default_colour(fe, &ret[COL_BACKGROUND * 3]);

    ret[COL_BLACK * 3 + 0] = 0.0F;
    ret[COL_BLACK * 3 + 1] = 0.0F;
    ret[COL_BLACK * 3 + 2] = 0.0F;

    ret[COL_RED * 3 + 0] = 1.0F;
    ret[COL_RED * 3 + 1] = 0.0F;
    ret[COL_RED * 3 + 2] = 0.0F;

    ret[COL_GREEN * 3 + 0] = 0.0F;
    ret[COL_GREEN * 3 + 1] = 1.0F;
    ret[COL_GREEN * 3 + 2] = 0.0F;

    ret[COL_BLUE * 3 + 0] = 0.0F;
    ret[COL_BLUE * 3 + 1] = 0.0F;
    ret[COL_BLUE * 3 + 2] = 1.0F;

    ret[COL_YELLOW * 3 + 0] = 1.0F;
    ret[COL_YELLOW * 3 + 1] = 1.0F;
    ret[COL_YELLOW * 3 + 2] = 0.0F;

    ret[COL_MAGENTA * 3 + 0] = 1.0F;
    ret[COL_MAGENTA * 3 + 1] = 0.0F;
    ret[COL_MAGENTA * 3 + 2] = 1.0F;

    ret[COL_CYAN * 3 + 0] = 0.0F;
    ret[COL_CYAN * 3 + 1] = 1.0F;
    ret[COL_CYAN * 3 + 2] = 1.0F;

    ret[COL_WHITE * 3 + 0] = 1.0F;
    ret[COL_WHITE * 3 + 1] = 1.0F;
    ret[COL_WHITE * 3 + 2] = 1.0F;

    *ncolours = NCOLOURS;
    return ret;
}

static game_drawstate *game_new_drawstate(drawing *dr, const game_state *state)
{
    struct game_drawstate *ds = snew(struct game_drawstate);

    ds->tilesize = 0;
    ds->w = 0;
    ds->h = 0;

    ds->bl1 = NULL;
    ds->bl2 = NULL;

    return ds;
}

static void game_free_drawstate(drawing *dr, game_drawstate *ds)
{
    if (ds->bl1 != NULL)
        blitter_free(dr, ds->bl1);
    if (ds->bl2 != NULL)
        blitter_free(dr, ds->bl2);
    sfree(ds);
}

static void game_redraw(drawing *dr, game_drawstate *ds,
                        const game_state *oldstate, const game_state *state,
                        int dir, const game_ui *ui,
                        float animtime, float flashtime)
{
	/* A red and black band around the outside of the canvas. */

	draw_rect_outline(dr, 0, 0, ds->w, ds->h, COL_BLACK);
	draw_rect_outline(dr, 1, 1, ds->w - 2, ds->h -2, COL_RED);

	/* Draw a line around the inside of the red rectangle, missing the corner pixels. */

	draw_line(dr, 3, 2, ds->w - 4, 2, COL_MAGENTA);
	draw_line(dr, 3, ds->h - 3, ds->w - 4, ds->h - 3, COL_MAGENTA);
	draw_line(dr, 2, 3, 2, ds->h - 4, COL_MAGENTA);
	draw_line(dr, ds->w - 3, 3, ds->w - 3, ds->h - 4, COL_MAGENTA);

	/* Fill a rectangle in the space left. */

	draw_rect(dr, 3, 3, ds->w - 6, ds->h - 6, COL_YELLOW);

	clip(dr, 5, 5, ds->w - 10, ds->h - 10);

	draw_rect(dr, 0, 0, ds->w, ds->h, COL_CYAN);

	unclip(dr);

    draw_rect(dr, 31, 31, ds->tilesize-2, ds->tilesize-2, COL_YELLOW);
    draw_rect_outline(dr, 31, 31, ds->tilesize-2, ds->tilesize-2, COL_BLACK);

    blitter_save(dr, ds->bl1, 0, 0);
    blitter_save(dr, ds->bl2, 3, 3);

    blitter_load(dr, ds->bl1, 60, 60);

    blitter_load(dr, ds->bl2, 60, 90);
}

static float game_anim_length(const game_state *oldstate,
                              const game_state *newstate, int dir, game_ui *ui)
{
    return 0.0F;
}

static float game_flash_length(const game_state *oldstate,
                               const game_state *newstate, int dir, game_ui *ui)
{
    return 0.0F;
}

static void game_get_cursor_location(const game_ui *ui,
                                     const game_drawstate *ds,
                                     const game_state *state,
                                     const game_params *params,
                                     int *x, int *y, int *w, int *h)
{
}

static int game_status(const game_state *state)
{
    return 0;
}

#ifdef COMBINED
#define thegame riscostest
#endif

const struct game thegame = {
    "RISC OS Test", NULL, NULL,
    default_params,
    game_fetch_preset, NULL,
    decode_params,
    encode_params,
    free_params,
    dup_params,
    false, NULL, NULL, /* configure, custom_params */
    validate_params,
    new_game_desc,
    validate_desc,
    new_game,
    dup_game,
    free_game,
    false, NULL, /* solve */
    false, NULL, NULL, /* can_format_as_text_now, text_format */
    NULL, NULL, /* get_prefs, set_prefs */
    new_ui,
    free_ui,
    NULL, /* encode_ui */
    NULL, /* decode_ui */
    NULL, /* game_request_keys */
    game_changed_state,
    NULL, /* current_key_label */
    interpret_move,
    execute_move,
    20 /* FIXME */, game_compute_size, game_set_size,
    game_colours,
    game_new_drawstate,
    game_free_drawstate,
    game_redraw,
    game_anim_length,
    game_flash_length,
    game_get_cursor_location,
    game_status,
    false, false, NULL, NULL,          /* print_size, print */
    false,			       /* wants_statusbar */
    false, NULL,                       /* timing_state */
    0,				       /* flags */
};
