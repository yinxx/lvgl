/**
 * @file lv_draw_line.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <stdbool.h>
#include "lv_draw.h"
#include "../lv_misc/lv_math.h"

/*********************
 *      DEFINES
 *********************/
#define LINE_WIDTH_CORR_BASE  64
#define LINE_WIDTH_CORR_SHIFT 6
#if LV_COMPILER_VLA_SUPPORTED == 0
#define LINE_MAX_WIDTH		  64
#endif

/**********************
 *      TYPEDEFS
 **********************/

typedef struct
{
	lv_point_t p1;
	lv_point_t p2;
	lv_point_t p_act;
	lv_coord_t dx;
	lv_coord_t sx;		/*-1: x1 < x2; 1: x2 >= x1*/
	lv_coord_t dy;
	lv_coord_t sy;		/*-1: y1 < y2; 1: y2 >= y1*/
	lv_coord_t err;
	lv_coord_t e2;
	bool hor;	/*Rather horizontal or vertical*/
}line_draw_t;

typedef struct {
	lv_coord_t width;
	lv_coord_t width_1;
	lv_coord_t width_half;
}line_width_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void line_draw_hor(line_draw_t * main_line, const lv_area_t * mask, const lv_style_t * style);
static void line_draw_ver(line_draw_t * main_line, const lv_area_t * mask, const lv_style_t * style);
static void line_draw_skew(line_draw_t * main_line, const lv_area_t * mask, const lv_style_t * style);
static void line_init(line_draw_t * line, const lv_point_t * p1, const lv_point_t * p2);
static bool line_next(line_draw_t * line);
static bool line_next_y(line_draw_t * line);
static bool line_next_x(line_draw_t * line);
static void line_ver_aa(lv_coord_t x, lv_coord_t y, lv_coord_t length, const lv_area_t * mask, lv_color_t color, lv_opa_t opa);
static void line_hor_aa(lv_coord_t x, lv_coord_t y, lv_coord_t length, const lv_area_t * mask, lv_color_t color, lv_opa_t opa);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Draw a line
 * @param p1 first point of the line
 * @param p2 second point of the line
 * @param maskthe line will be drawn only on this area
 * @param lines_p pointer to a line style
 */
void lv_draw_line(const lv_point_t * point1, const lv_point_t * point2, const lv_area_t * mask,
                  const lv_style_t * style)
{

    if(style->line.width == 0) return;
    if(point1->x == point2->x && point1->y == point2->y) return;

    line_draw_t main_line;
    lv_point_t p1;
    lv_point_t p2;

    /*If the line if rather vertical then be sure y1 < y2 else x1 < x2*/

    if(LV_MATH_ABS(point1->x-point2->x) > LV_MATH_ABS(point1->y-point2->y)) {

    	/*Steps less in y then x -> rather horizontal*/
    	if(point1->x < point2->x) {
			p1.x = point1->x;
			p1.y = point1->y;
			p2.x = point2->x;
			p2.y = point2->y;
		} else {
			p1.x = point2->x;
			p1.y = point2->y;
			p2.x = point1->x;
			p2.y = point1->y;
		}
    } else {
    	/*Steps less in x then y -> rather vertical*/
		if(point1->y < point2->y) {
			p1.x = point1->x;
			p1.y = point1->y;
			p2.x = point2->x;
			p2.y = point2->y;
		} else {
			p1.x = point2->x;
			p1.y = point2->y;
			p2.x = point1->x;
			p2.y = point1->y;
		}
    }

    line_init(&main_line, &p1, &p2);


    /*Special case draw a horizontal line*/
    if(main_line.p1.y == main_line.p2.y ) {
    	line_draw_hor(&main_line, mask, style);
    }
    /*Special case draw a vertical line*/
    else if(main_line.p1.x == main_line.p2.x ) {
    	line_draw_ver(&main_line, mask, style);
    }
    /*Arbitrary skew line*/
    else {
    	line_draw_skew(&main_line, mask, style);
    }
}


/**********************
 *   STATIC FUNCTIONS
 **********************/


static void line_draw_hor(line_draw_t * line, const lv_area_t * mask, const lv_style_t * style)
{
	lv_coord_t width = style->line.width - 1;
	lv_coord_t width_half = width >> 1;
	lv_coord_t width_1 = width & 0x1;

    lv_area_t act_area;
    act_area.x1 = line->p1.x;
    act_area.x2 = line->p2.x;
    act_area.y1 = line->p1.y - width_half - width_1;
    act_area.y2 = line->p2.y + width_half ;

    lv_area_t draw_area;
    draw_area.x1 = LV_MATH_MIN(act_area.x1, act_area.x2);
    draw_area.x2 = LV_MATH_MAX(act_area.x1, act_area.x2);
    draw_area.y1 = LV_MATH_MIN(act_area.y1, act_area.y2);
    draw_area.y2 = LV_MATH_MAX(act_area.y1, act_area.y2);
    fill_fp(&draw_area, mask, style->line.color, style->line.opa);
}

static void line_draw_ver(line_draw_t * line, const lv_area_t * mask, const lv_style_t * style)
{
	lv_coord_t width = style->line.width - 1;
	lv_coord_t width_half = width >> 1;
	lv_coord_t width_1 = width & 0x1;
    lv_area_t act_area;

    act_area.x1 = line->p1.x - width_half;
    act_area.x2 = line->p2.x + width_half + width_1;
    act_area.y1 = line->p1.y;
    act_area.y2 = line->p2.y;

    lv_area_t draw_area;
    draw_area.x1 = LV_MATH_MIN(act_area.x1, act_area.x2);
    draw_area.x2 = LV_MATH_MAX(act_area.x1, act_area.x2);
    draw_area.y1 = LV_MATH_MIN(act_area.y1, act_area.y2);
    draw_area.y2 = LV_MATH_MAX(act_area.y1, act_area.y2);
    fill_fp(&draw_area, mask, style->line.color, style->line.opa);
}

static void line_draw_skew(line_draw_t * main_line, const lv_area_t * mask, const lv_style_t * style)
{

	lv_coord_t width;
	width = style->line.width;
	lv_coord_t width_safe = width;

#if LV_ANTIALIAS
	width--;
	if(width == 0) width_safe = 1;
#endif


	static const uint8_t width_corr_array[] = {
			64, 64, 64, 64, 64, 64, 64, 64, 64, 65, 65, 65, 65, 65, 66, 66, 66, 66, 66,
			67, 67, 67, 68, 68, 68, 69, 69, 69, 70, 70, 71, 71, 72, 72, 72, 73, 73, 74,
			74, 75, 75, 76, 77, 77, 78, 78, 79, 79, 80, 81, 81, 82, 82, 83, 84, 84, 85,
			86, 86, 87, 88, 88, 89, 90, 91,
	};

	uint16_t wcor;
	if(main_line->hor == false) {
		wcor = (main_line->dx * LINE_WIDTH_CORR_BASE) / main_line->dy;
	} else  {
		wcor = (main_line->dy * LINE_WIDTH_CORR_BASE) / main_line->dx;
	}

	/*Make the correction on line width*/
	if(width > 0) {
		width = (width * width_corr_array[wcor]);
		width = width >> LINE_WIDTH_CORR_SHIFT;
	}
	width_safe = width;

	lv_point_t vect_main, vect_norm;
	vect_main.x = main_line->p2.x - main_line->p1.x;
	vect_main.y = main_line->p2.y - main_line->p1.y;

	if(main_line->hor) {
		if(main_line->p1.y < main_line->p2.y) {
			vect_norm.x = - vect_main.y;
			vect_norm.y = vect_main.x;
		} else {
			vect_norm.x = vect_main.y;
			vect_norm.y = -vect_main.x;
		}
	} else {
		if(main_line->p1.x < main_line->p2.x) {
			vect_norm.x = vect_main.y;
			vect_norm.y = - vect_main.x;
		} else {
			vect_norm.x = - vect_main.y;
			vect_norm.y = vect_main.x;
		}
	}

	printf("width %d\n", width);
#if LV_COMPILER_VLA_SUPPORTED
	lv_point_t pattern[width_safe];
#else
	lv_point_t pattern[LINE_MAX_WIDTH];
#endif
	int i = 0;

	/*Create a perpendicular pattern (a small line)*/
	if(width != 0) {
		line_draw_t pattern_line;
		lv_point_t p0 = {0,0};
		line_init(&pattern_line, &p0, &vect_norm);

		for(i = 0; i < width; i ++) {
			pattern[i].x = pattern_line.p_act.x;
			pattern[i].y = pattern_line.p_act.y;

			line_next(&pattern_line);
		}
	} else {
		pattern[0].x = 0;
		pattern[0].y = 0;
	}

#if LV_ANTIALIAS
	lv_coord_t aa_last_corner;
	aa_last_corner = 0;
#endif

	/* Make the coordinates relative to the center */
	for(i = 0; i < width; i++) {
		pattern[i].x -= pattern[width - 1].x / 2;
		pattern[i].y -= pattern[width - 1].y / 2;
#if LV_ANTIALIAS
		if(i != 0) {
			if(main_line->hor) {
				if(pattern[i - 1].x != pattern[i].x) {
					lv_coord_t seg_w = pattern[i].y - pattern[aa_last_corner].y;
					if(main_line->sy < 0) {
						line_ver_aa(main_line->p1.x + pattern[aa_last_corner].x - 1, main_line->p1.y + pattern[aa_last_corner].y + seg_w + 1,
									seg_w, mask, LV_COLOR_RED, LV_OPA_50);

						line_ver_aa(main_line->p2.x + pattern[aa_last_corner].x + 1, main_line->p2.y + pattern[aa_last_corner].y + seg_w + 1,
									-seg_w, mask, LV_COLOR_RED, LV_OPA_50);
					} else {
						line_ver_aa(main_line->p1.x + pattern[aa_last_corner].x - 1, main_line->p1.y + pattern[aa_last_corner].y,
									seg_w, mask, LV_COLOR_RED, LV_OPA_50);

						line_ver_aa(main_line->p2.x + pattern[aa_last_corner].x + 1, main_line->p2.y + pattern[aa_last_corner].y,
									-seg_w, mask, LV_COLOR_RED, LV_OPA_50);
					}
					aa_last_corner = i;
				}
			} else {
				if(pattern[i - 1].y != pattern[i].y) {
					lv_coord_t seg_w = pattern[i].x - pattern[aa_last_corner].x;
					if(main_line->sx < 0) {
						line_hor_aa(main_line->p1.x + pattern[aa_last_corner].x + seg_w + 1, main_line->p1.y + pattern[aa_last_corner].y - 1,
									seg_w, mask, LV_COLOR_RED, LV_OPA_50);

						line_hor_aa(main_line->p2.x + pattern[aa_last_corner].x + seg_w + 1, main_line->p2.y + pattern[aa_last_corner].y + 1,
									-seg_w, mask, LV_COLOR_RED, LV_OPA_50);
					} else {
						line_hor_aa(main_line->p1.x + pattern[aa_last_corner].x, main_line->p1.y + pattern[aa_last_corner].y - 1,
									seg_w, mask, LV_COLOR_RED, LV_OPA_50);

						line_hor_aa(main_line->p2.x + pattern[aa_last_corner].x, main_line->p2.y + pattern[aa_last_corner].y + 1,
									-seg_w, mask, LV_COLOR_RED, LV_OPA_50);
					}
					aa_last_corner = i;
				}
			}

		}
#endif
	}


#if LV_ANTIALIAS
	/*Add the last part of anti-aliasing for the perpendicular ending*/
	if(main_line->hor) {
		lv_coord_t seg_w = pattern[width_safe - 1].y - pattern[aa_last_corner].y;
		if(main_line->sy < 0) {
			line_ver_aa(main_line->p1.x + pattern[aa_last_corner].x - 1, main_line->p1.y + pattern[aa_last_corner].y + seg_w,
						seg_w + main_line->sy, mask, LV_COLOR_RED, LV_OPA_50);

			line_ver_aa(main_line->p2.x + pattern[aa_last_corner].x + 1, main_line->p2.y + pattern[aa_last_corner].y + seg_w,
						-(seg_w + main_line->sy), mask, LV_COLOR_RED, LV_OPA_50);

		} else {
			line_ver_aa(main_line->p1.x + pattern[aa_last_corner].x - 1, main_line->p1.y + pattern[aa_last_corner].y,
						seg_w + main_line->sy, mask, LV_COLOR_RED, LV_OPA_50);

			line_ver_aa(main_line->p2.x + pattern[aa_last_corner].x + 1, main_line->p2.y + pattern[aa_last_corner].y,
						-(seg_w + main_line->sy), mask, LV_COLOR_RED, LV_OPA_50);
		}
	} else {
		lv_coord_t seg_w = pattern[width_safe - 1].x - pattern[aa_last_corner].x;
		if(main_line->sy < 0) {
			line_hor_aa(main_line->p1.x + pattern[aa_last_corner].x + seg_w, main_line->p1.y + pattern[aa_last_corner].y - 1,
						seg_w + main_line->sx, mask, LV_COLOR_RED, LV_OPA_50);

			line_hor_aa(main_line->p2.x + pattern[aa_last_corner].x + seg_w, main_line->p2.y + pattern[aa_last_corner].y + 1,
						-(seg_w + main_line->sx), mask, LV_COLOR_RED, LV_OPA_50);

		} else {
			line_hor_aa(main_line->p1.x + pattern[aa_last_corner].x, main_line->p1.y + pattern[aa_last_corner].y - 1,
						seg_w + main_line->sx, mask, LV_COLOR_RED, LV_OPA_50);

			line_hor_aa(main_line->p2.x + pattern[aa_last_corner].x , main_line->p2.y + pattern[aa_last_corner].y + 1,
						-(seg_w + main_line->sx), mask, LV_COLOR_RED, LV_OPA_50);
		}

	}
#endif

#if LV_ANTIALIAS

	/*With with value shift the anti aliasing on the edges (-1, 1 or 0 (zero only in case width == 0))*/
	lv_coord_t aa_shift1;
	lv_coord_t aa_shift2;
	aa_shift1 = main_line->hor ? main_line->sy : main_line->sx;
	aa_shift2 = width == 0 ? 0 : aa_shift1;
#endif


	volatile lv_point_t prev_p;
	prev_p.x = main_line->p1.x;
	prev_p.y = main_line->p1.y;
	lv_area_t draw_area;
	bool first_run = true;

	if(main_line->hor) {
		while(line_next_y(main_line)) {
			for(i = 0; i < width; i++) {
				draw_area.x1 = prev_p.x + pattern[i].x;
				draw_area.y1 = prev_p.y + pattern[i].y;
				draw_area.x2 = draw_area.x1 + main_line->p_act.x - prev_p.x - 1;
				draw_area.y2 = draw_area.y1;
				fill_fp(&draw_area, mask, style->line.color, style->line.opa);

				/* Fill the gaps
				 * When stepping in y one pixel remains empty on every corner (don't do this on the first segment ) */
				if(i != 0 && pattern[i].x != pattern[i - 1].x && !first_run) {
					px_fp(draw_area.x1 , draw_area.y1 - main_line->sy, mask, style->line.color, style->line.opa);
				}
			}

#if LV_ANTIALIAS
			line_hor_aa(prev_p.x + pattern[0].x, prev_p.y + pattern[0].y - aa_shift1,
					    -(main_line->p_act.x - prev_p.x), mask, style->line.color, style->line.opa);
			line_hor_aa(prev_p.x + pattern[width_safe - 1].x, prev_p.y + pattern[width_safe - 1].y + aa_shift2,
					    main_line->p_act.x - prev_p.x, mask, style->line.color, style->line.opa);
#endif

			first_run = false;

			prev_p.x = main_line->p_act.x;
			prev_p.y = main_line->p_act.y;
		}

		for(i = 0; i < width; i++) {
			draw_area.x1 = prev_p.x + pattern[i].x;
			draw_area.y1 = prev_p.y + pattern[i].y;
			draw_area.x2 = draw_area.x1 + main_line->p_act.x - prev_p.x;
			draw_area.y2 = draw_area.y1;
			fill_fp(&draw_area, mask, style->line.color , style->line.opa);

			/* Fill the gaps
			 * When stepping in y one pixel remains empty on every corner */
			if(i != 0 && pattern[i].x != pattern[i - 1].x && !first_run) {
				px_fp(draw_area.x1, draw_area.y1 - main_line->sy, mask, style->line.color, style->line.opa);
			}
		}

#if LV_ANTIALIAS
			line_hor_aa(prev_p.x + pattern[0].x, prev_p.y + pattern[0].y - aa_shift1,
					    -(main_line->p_act.x - prev_p.x + 1), mask, style->line.color, style->line.opa);
			line_hor_aa(prev_p.x + pattern[width_safe - 1].x, prev_p.y + pattern[width_safe - 1].y + aa_shift2,
					    main_line->p_act.x - prev_p.x + 1, mask, style->line.color, style->line.opa);
#endif
	}
	/*Rather a vertical line*/
	else {
		while(line_next_x(main_line)) {
			for(i = 0; i < width; i++) {
				draw_area.x1 = prev_p.x + pattern[i].x;
				draw_area.y1 = prev_p.y + pattern[i].y;
				draw_area.x2 = draw_area.x1;
				draw_area.y2 = draw_area.y1 + main_line->p_act.y - prev_p.y - 1;

				fill_fp(&draw_area, mask, style->line.color, style->line.opa);

				/* Fill the gaps
				 * When stepping in x one pixel remains empty on every corner (don't do this on the first segment ) */
				if(i != 0 && pattern[i].y != pattern[i - 1].y && !first_run) {
					px_fp(draw_area.x1 - main_line->sx, draw_area.y1, mask, style->line.color, style->line.opa);
				}
			}

#if LV_ANTIALIAS
			line_ver_aa(prev_p.x + pattern[0].x - aa_shift1, prev_p.y + pattern[0].y,
					    -(main_line->p_act.y - prev_p.y), mask, style->line.color, style->line.opa);
			line_ver_aa(prev_p.x + pattern[width_safe - 1].x + aa_shift2, prev_p.y + pattern[width_safe - 1].y,
					    main_line->p_act.y - prev_p.y, mask, style->line.color, style->line.opa);
#endif

			first_run = false;

			prev_p.x = main_line->p_act.x;
			prev_p.y = main_line->p_act.y;
		}

		/*Draw the last part*/
		for(i = 0; i < width; i++) {
			draw_area.x1 = prev_p.x + pattern[i].x;
			draw_area.y1 = prev_p.y + pattern[i].y;
			draw_area.x2 = draw_area.x1;
			draw_area.y2 = draw_area.y1 + main_line->p_act.y - prev_p.y;

			fill_fp(&draw_area, mask, style->line.color, style->line.opa);

			/* Fill the gaps
			 * When stepping in x one pixel remains empty on every corner */
			if(i != 0 && pattern[i].y != pattern[i - 1].y && !first_run) {
				px_fp(draw_area.x1 - main_line->sx, draw_area.y1, mask, style->line.color, style->line.opa);
			}
		}

#if LV_ANTIALIAS
		line_ver_aa(prev_p.x + pattern[0].x - aa_shift1, prev_p.y + pattern[0].y,
					-(main_line->p_act.y - prev_p.y + 1), mask, style->line.color, style->line.opa);
		line_ver_aa(prev_p.x + pattern[width_safe - 1].x + aa_shift2, prev_p.y + pattern[width_safe - 1].y,
					main_line->p_act.y - prev_p.y + 1, mask, style->line.color, style->line.opa);
#endif
	}
}


static void line_init(line_draw_t * line, const lv_point_t * p1, const lv_point_t * p2)
{
	line->p1.x = p1->x;
	line->p1.y = p1->y;
	line->p2.x = p2->x;
	line->p2.y = p2->y;

	line->dx = LV_MATH_ABS(line->p2.x - line->p1.x);
	line->sx = line->p1.x < line->p2.x ? 1 : -1;
	line->dy = LV_MATH_ABS(line->p2.y - line->p1.y);
	line->sy = line->p1.y < line->p2.y ? 1 : -1;
	line->err = (line->dx > line->dy ? line->dx : -line->dy) / 2;
	line->e2 = 0;
	line->hor = line->dx > line->dy ? true : false;	/*Rather horizontal or vertical*/

    line->p_act.x = line->p1.x;
    line->p_act.y = line->p1.y;
}

static bool line_next(line_draw_t * line)
{
	if (line->p_act.x == line->p2.x && line->p_act.y == line->p2.y) return false;
	line->e2 = line->err;
	if (line->e2 >-line->dx) {
		line->err -= line->dy;
		line->p_act.x += line->sx;
	}
	if (line->e2 < line->dy) {
		line->err += line->dx;
		line->p_act.y += line->sy;
	}
	return true;
}

/**
 * Iterate until step one in y direction.
 * @param line
 * @return
 */
static bool line_next_y(line_draw_t * line)
{
	lv_coord_t last_y = line->p_act.y;

	do {
		if(!line_next(line)) return false;
	} while(last_y == line->p_act.y);

	return true;

}

/**
 * Iterate until step one in x direction.
 * @param line
 * @return
 */
static bool line_next_x(line_draw_t * line)
{
	lv_coord_t last_x = line->p_act.x;

	do {
		if(!line_next(line)) return false;
	} while(last_x == line->p_act.x);

	return true;

}

/**
 * Add a vertical  anti-aliasing segment (pixels with decreasing opacity)
 * @param x start point x coordinate
 * @param y start point y coordinate
 * @param length length of segment (negative value to start from 0 opacity)
 * @param mask draw only in this area
 * @param color color of pixels
 * @param opa maximum opacity
 */
static void line_ver_aa(lv_coord_t x, lv_coord_t y, lv_coord_t length, const lv_area_t * mask, lv_color_t color, lv_opa_t opa)
{
	bool aa_inv = false;
	if(length < 0) {
		aa_inv = true;
		length = -length;
	}

	lv_coord_t i;
	for(i = 0; i < length; i++) {
		lv_opa_t px_opa = antialias_get_opa(length, i, opa);
		if(aa_inv) px_opa = opa - px_opa;
		px_fp(x, y + i, mask, color, px_opa);
	}
}

/**
 * Add a horizontal anti-aliasing segment (pixels with decreasing opacity)
 * @param x start point x coordinate
 * @param y start point y coordinate
 * @param length length of segment (negative value to start from 0 opacity)
 * @param mask draw only in this area
 * @param color color of pixels
 * @param opa maximum opacity
 */
static void line_hor_aa(lv_coord_t x, lv_coord_t y, lv_coord_t length, const lv_area_t * mask, lv_color_t color, lv_opa_t opa)
{
	bool aa_inv = false;
	if(length < 0) {
		aa_inv = true;
		length = -length;
	}

	lv_coord_t i;
	for(i = 0; i < length; i++) {
		lv_opa_t px_opa = antialias_get_opa(length, i, opa);
		if(aa_inv) px_opa = opa - px_opa;
		px_fp(x + i, y, mask, color, px_opa);
	}
}
