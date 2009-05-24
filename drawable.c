/* -*- c -*- */

/*
 * drawable.c
 *
 * MathMap
 *
 * Copyright (C) 2007-2009 Mark Probst
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <string.h>

#include "drawable.h"
#include "mathmap.h"

#define MAX_INPUT_DRAWABLES 64

static input_drawable_t input_drawables[MAX_INPUT_DRAWABLES];

input_drawable_t*
alloc_input_drawable (int kind, int width, int height)
{
    int i;
    input_drawable_t *drawable;

    for (i = 0; i < MAX_INPUT_DRAWABLES; ++i)
	if (!input_drawables[i].used)
	    break;
    if (i == MAX_INPUT_DRAWABLES)
	return 0;

    drawable = &input_drawables[i];

    g_assert(!drawable->used);

    drawable->used = TRUE;
    drawable->kind = kind;

    drawable->image.type = IMAGE_DRAWABLE;
    drawable->image.pixel_width = width;
    drawable->image.pixel_height = height;
    drawable->image.v.drawable = drawable;

    return drawable;
}

void
free_input_drawable (input_drawable_t *drawable)
{
    g_assert(drawable->used);

    switch (drawable->kind)
    {
#ifndef OPENSTEP
	case INPUT_DRAWABLE_GIMP :
	    if (drawable->v.gimp.tile != 0)
	    {
		gimp_tile_unref(drawable->v.gimp.tile, FALSE);
		drawable->v.gimp.tile = 0;
	    }
	    if (drawable->v.gimp.fast_image_source != 0)
	    {
		g_free(drawable->v.gimp.fast_image_source);
		drawable->v.gimp.fast_image_source = 0;
	    }
	    drawable->v.gimp.drawable = 0;
	    break;
#endif

        case INPUT_DRAWABLE_OPENSTEP:
	    break;

#ifdef MATHMAP_CMDLINE
	case INPUT_DRAWABLE_CMDLINE_IMAGE :
	    g_free(drawable->v.cmdline.image_filename);
	    g_free(drawable->v.cmdline.cache_entries);
	    break;
#endif

	default :
	    g_assert_not_reached();
    }

    drawable->used = FALSE;
}

void
for_each_input_drawable (void (*actor) (input_drawable_t *drawable))
{
    int i;

    for (i = 0; i < MAX_INPUT_DRAWABLES; ++i)
	if (input_drawables[i].used)
	    actor(&input_drawables[i]);
}

input_drawable_t*
copy_input_drawable (input_drawable_t *drawable)
{
    input_drawable_t *copy = 0;

    switch (drawable->kind)
    {
#ifndef OPENSTEP
	case INPUT_DRAWABLE_GIMP :
	    copy = alloc_gimp_input_drawable(drawable->v.gimp.drawable);
	    copy->v.gimp.has_selection = drawable->v.gimp.has_selection;
	    break;
#else
        case INPUT_DRAWABLE_OPENSTEP:
	    copy = alloc_input_drawable(INPUT_DRAWABLE_OPENSTEP,
					drawable->image.pixel_width, drawable->image.pixel_height);
	    copy->v.openstep.row_stride = drawable->v.openstep.row_stride;
	    copy->v.openstep.data = drawable->v.openstep.data;
	    break;
#endif

#ifdef MATHMAP_CMDLINE
	case INPUT_DRAWABLE_CMDLINE_IMAGE :
	    copy = alloc_cmdline_image_input_drawable(drawable->v.cmdline.image_filename);
	    break;

#ifdef MOVIES
	case INPUT_DRAWABLE_CMDLINE_MOVIE :
	    copy = alloc_cmdline_movie_input_drawable(drawable->v.cmdline.image_filename);
	    break;
#endif
#endif

	default :
	    g_assert_not_reached();
    }

    copy->scale_x = drawable->scale_x;
    copy->scale_y = drawable->scale_y;
    copy->middle_x = drawable->middle_x;
    copy->middle_y = drawable->middle_y;

    return copy;
}

#ifndef OPENSTEP
input_drawable_t*
get_default_input_drawable (void)
{
    int i;

    for (i = 0; i < MAX_INPUT_DRAWABLES; ++i)
	if (input_drawables[i].used
	    && input_drawables[i].kind == INPUT_DRAWABLE_GIMP
	    && input_drawables[i].v.gimp.has_selection)
	    return &input_drawables[i];
    /* No GIMP drawable with selection found, which means we're
       probably not in GIMP.  Just return the first used drawable. */
    for (i = 0; i < MAX_INPUT_DRAWABLES; ++i)
	if (input_drawables[i].used)
	    return &input_drawables[i];
    return 0;
}
#endif

int
get_num_input_drawables (void)
{
    int i;
    int n = 0;

    for (i = 0; i < MAX_INPUT_DRAWABLES; ++i)
	if (input_drawables[i].used)
	    ++n;

    return n;
}

input_drawable_t*
get_nth_input_drawable (int n)
{
    int i;

    for (i = 0; i < MAX_INPUT_DRAWABLES; ++i)
	if (input_drawables[i].used)
	{
	    if (n == 0)
		return &input_drawables[i];
	    --n;
	}

    g_assert_not_reached();
}

CALLBACK_SYMBOL
image_t*
make_resize_image (image_t *image, float x_factor, float y_factor, pools_t *pools)
{
    image_t *resize = pools_alloc(pools, sizeof(image_t));

    g_assert(image->type != IMAGE_RESIZE);

    resize->type = IMAGE_RESIZE;
    resize->pixel_width = image->pixel_width;
    resize->pixel_height = image->pixel_height;
    resize->v.resize.original = image;
    resize->v.resize.x_factor = x_factor;
    resize->v.resize.y_factor = y_factor;

    return resize;
}

image_t*
closure_image_alloc (mathfuncs_t *mathfuncs, filter_func_t filter_func,
		     int num_uservals, userval_t *uservals,
		     int pixel_width, int pixel_height)
{
    image_t *image = g_malloc0(sizeof(image_t) + num_uservals * sizeof(userval_t));

    image->type = IMAGE_CLOSURE;
    image->pixel_width = pixel_width;
    image->pixel_height = pixel_height;
    image->v.closure.funcs = mathfuncs;
    image->v.closure.func = filter_func;
    image->v.closure.num_args = num_uservals;
    image->v.closure.pools = g_new(pools_t, 1);
    init_pools(image->v.closure.pools);
    memcpy(image->v.closure.args, uservals, num_uservals * sizeof(userval_t));

    return image;
}

void
closure_image_free (image_t *closure)
{
    free_pools(closure->v.closure.pools);
    g_free(closure);
}
