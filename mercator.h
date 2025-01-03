/* $Id: mercator.h,v 1.13 2003/02/22 02:34:48 dyll Exp $ */
/***********************************************************************
 * Copyright (C) 2003 Ingo Schwarze
 * Copyright (C) 2002 Roger Butenuth, Marko Schulz
 *
 * This file is part of MERCATOR.
 *
 * MERCATOR is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING in the main
 * distribution directory.  If you do not find the License, have
 * a look at the GNU website http://www.gnu.org/ or write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA.
 **********************************************************************/

#ifndef MERCATOR_H
#define MERCATOR_H

#include <stdlib.h>
#include <string.h>
#include "map.h"

typedef struct html_struct {
    map_t *map;
    int   tsx, tsy;		/* Größe der Teilkarten */
    int   min_x, min_y;		/* Gesamtgröße, visuelle Koordinaten */
    int   max_x, max_y;
    int   x_lap;		/* Überlappung zwischen Kartenteilen */
    int   y_lap;
    locale_e locale;      /* Spracheinstellung */
} html_t;

typedef struct wmap_struct {
    int min_vx;
    int min_vy;
    int max_vx;
    int max_vy;
} wmap_t;

typedef struct burgi_struct {
    int  groesse;        /* Groesse in Steinen */
    int  lohn;           /* Bauernlohn in der Region */
    int  anteil;        /* in Prozent */
    const char *typ;
} burgi_t;

typedef struct helfe_info_struct {
    helfe_e status;
    const char *text;
} helfe_info_t;

/* html.c */
void hex2vis(int hx, int hy, int *vx, int *vy);
void vis2hex(int vx, int vy, int *hx, int *hy);
int valid_vis(int vx, int vy);
int region_filter(map_entry_t *e);
void fill_html(map_t *map, int tsx, int tsy, int x_lap, int y_lap,
               ebene_t *eb, html_t *h);
void write_html_map(map_t *map, const char *dir, int tsx, int tsy);

/* language.c */
void set_global_names (locale_e locale, game_e game);
const char *translate(const char *deutsch, locale_e sprache);

/* map.c */
void write_cr_map(map_t *map, const char *file);

/* parser/parser.c */
partei_t *finde_partei(map_t *map, int nummer);
char *finde_gruppe(map_t *map, int pnr, int gnr);
void set_region_typ(map_t *map, int x, int y, char *typ);

/* parser/parser.h */
char *get_value(meldung_t *m, char *tag);

/* image.c */
int write_world_map(html_t *h, const char *prefix, const char *name,
		    int ebene, int image_map);
int write_image_map(html_t *h, wmap_t *w, int ebene,
		    const char *prefix, const char *name, FILE *fp,
		    int image_map);
int  write_png(int *argc, char *argv[], map_t *map);
void init_image(void);
void fini_image(void);

#endif
