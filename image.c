/* $Id: image.c,v 1.13 2006/02/08 01:42:47 schwarze Exp $ */
/***********************************************************************
 * Copyright (C) 2003-2006 Ingo Schwarze
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <png.h>
#include "mercator.h"


typedef struct pixel_struct {
    unsigned char r, g, b;
    unsigned char alpha;	/* 0 = transparent, 255 = solid */
} pixel_t;

typedef struct picture_struct {
    unsigned  x, y;     /* size */
    pixel_t **p;		/* pixels */
} picture_t;

typedef struct offset_struct { /* alle Angaben in Pixels */
    unsigned int tx;   /* Gesamtbreite eines Regions-Icons */
    unsigned int ty;   /* Gesamthoehe eines Regions-Icons */
    unsigned int ox;   /* Abstand linker Rand - Mitte */
    unsigned int oy;   /* Abstand oberer Rand - Mitte */
    /* Ausdehnung einer Einheit in vis-Koordinaten */
    unsigned int dx;   /* halbe Breite des Regions-Sechseckes */
    unsigned int dy;   /* eineinhalbfache Seitenlaenge des Sechseckes */
} offset_t;

static const char *names[] = {
    "Unbekannt.png",    /*  0 */
    "Nichts.png",       /*  1 */
    "Ozean.png",        /*  2 */
    "Land.png",         /*  3 */
    "Ebene.png",        /*  4 */
    "Wald.png",	        /*  5 */
    "Sumpf.png",        /*  6 */
    "Wueste.png",       /*  7 */
    "Hochland.png",     /*  8 */
    "Berge.png",        /*  9 */
    "Vulkan.png",       /* 10 */
    "Aktiv.png",        /* 11 */
    "Gletscher.png",    /* 12 */
    "Eisberg.png",      /* 13 */
    "Feuer.png",        /* 14 */
    "Rand.png",         /* 15 */
    "Nebel.png",        /* 16 */
    "Dicht.png"         /* 17 */
};

static picture_t *pictures[NR_REGIONS];
static picture_t *strassen[6];
static picture_t *burg;
static picture_t *schiff;
static offset_t offsets;

picture_t *load_picture(const char *name)
{
    png_structp  png_ptr = NULL;
    png_infop    info_ptr = NULL, end_info = NULL;
    png_uint_32  width, height;
    int          bit_depth, color_type, interlace_type;
    FILE         *fp = NULL;
    picture_t    *pic = NULL;
    unsigned     y;
    double       gamma;

    pic = xmalloc(sizeof(picture_t));
    pic->p = NULL;

    fp = config_open(name);

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL)
	goto error_exit;
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
	goto error_exit;
    end_info = png_create_info_struct(png_ptr);
    if (end_info == NULL)
	goto error_exit;
    /*
     * Set error handling if you are using the setjmp/longjmp method (this is
     * the normal method of doing things with libpng).  REQUIRED unless you
     * set up your own error handlers in the png_create_read_struct() earlier.
     */
#if 0    
    if (setjmp(png_ptr->jmpbuf)) {
	goto error_exit;
    }
#endif

    png_init_io(png_ptr, fp);
    /*
     * The call to png_read_info() gives us all of the information from the
     * PNG file before the first IDAT (image data chunk).  REQUIRED
     */
    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
		 &interlace_type, NULL, NULL);

    png_set_strip_16(png_ptr);
    png_set_packing(png_ptr);
    if (color_type == PNG_COLOR_TYPE_PALETTE)
	png_set_expand(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY) {
	fprintf(stderr,
          "Fehler in load_picture: greyscale-png nicht unterstuetzt.\n");
	exit(1);
    }
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
	png_set_expand(png_ptr);
    
    if (png_get_gAMA(png_ptr, info_ptr, &gamma))
	png_set_gamma(png_ptr, 2.0, gamma);
    else
	png_set_gamma(png_ptr, 2.0, 0.45455);

    png_read_update_info(png_ptr, info_ptr);

    pic->x = width;
    pic->y = height;
    pic->p = xmalloc(pic->y * sizeof(pixel_t*));
    for (y = 0; y < pic->y; y++)
	pic->p[y] = xmalloc(pic->x * sizeof(pixel_t));
    png_read_image(png_ptr, (png_bytep*)pic->p);
    /* read rest of file, and get additional chunks in info_ptr - REQUIRED */
    png_read_end(png_ptr, end_info);
    /* clean up after the read, and free any memory allocated - REQUIRED */
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL /* &end_info*/);
    fclose(fp);
  
    return pic;
 error_exit:
    fprintf(stderr,
      "Fehler in load_picture: Fehler in png-Bibliotheksfunktion.\n");
    if (fp) fclose(fp);
    if (pic) {
	if (pic->p) {
	    for (y = 0; y < pic->y; y++)
		xfree(pic->p[y]);
	    xfree(pic->p);
	}
	xfree(pic);
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    return NULL;
}

picture_t *create_picture(int sx, int sy)
{
    picture_t *pic;
    unsigned   x, y;

    assert(sx > 0);
    assert(sy > 0);

    pic = xmalloc(sizeof(picture_t));
    
    pic->x = sx;
    pic->y = sy;

    pic->p = xmalloc(pic->y * sizeof(pixel_t*));

    for (y = 0; y < pic->y; y++) {
	pic->p[y] = xmalloc(pic->x * sizeof(pixel_t));
	for (x = 0; x < pic->x; x++) {
	    pic->p[y][x].r = 255;
	    pic->p[y][x].g = 255;
	    pic->p[y][x].b = 255;
	    pic->p[y][x].alpha = 0;
	}
    }
  
    return pic;
}

int save_picture(picture_t *pic, const char *name)
{
    FILE          *fp;
    png_structp   png_ptr;
    png_infop     info_ptr;
    png_color_8   sig_bit;

    fp = xfopen(name, "wb");

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
				      NULL, NULL, NULL);
    if (png_ptr == NULL) {
	fclose(fp);
	return -1;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
      fclose(fp);
      png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
      return -1;
    }
    /*
     * Set error handling.  REQUIRED if you aren't supplying your own
     * error hadnling functions in the png_create_write_struct() call.
     */
#if 0
    if (setjmp(png_ptr->jmpbuf)) {
	/* If we get here, we had a problem writing the file */
	fclose(fp);
	png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
	return -1;
    }
#endif
    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, pic->x, pic->y, 8,
		 PNG_COLOR_TYPE_RGB,
		 PNG_INTERLACE_NONE,
		 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    sig_bit.red   = 8;
    sig_bit.green = 8;
    sig_bit.blue  = 8;
    png_set_sBIT(png_ptr, info_ptr, &sig_bit);
    png_set_gAMA(png_ptr, info_ptr, 0.6);
    /*
     * Hier könnte man Text-Chunks schreiben.
     * Bild-Info in Datei schreiben.
     */
    png_write_info(png_ptr, info_ptr);
    /*
     * Jetzt wird das Bild geschrieben:
     */
    png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);
    png_write_image(png_ptr, (png_bytep*)pic->p);

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

    fclose(fp);
  
    return 0;
}

void free_picture(picture_t *pic)
{
    unsigned i;

    for (i = 0; i < pic->y; i++)
	xfree(pic->p[i]);
    xfree(pic->p);
    xfree(pic);
}

void pic_copy(const picture_t *src, picture_t *dst, int dx, int dy)
{
  static unsigned x, y; /* static wegen haeufiger Aufrufe */
  static int r, g, b, a;
  static int sa;

  assert(dx >= 0);
  assert(dy >= 0);
  assert(src->x + dx <= dst->x);
  assert(src->y + dy <= dst->y);

  for (y = 0; y < src->y; y++) {
	for (x = 0; x < src->x; x++) {
      sa = src->p[y][x].alpha;
	  if (sa != 0) {
        r = dst->p[dy + y][dx + x].r = src->p[y][x].r;
        g = dst->p[dy + y][dx + x].g = src->p[y][x].g;
        b = dst->p[dy + y][dx + x].b = src->p[y][x].b;
        a = dst->p[dy + y][dx + x].alpha = 255;

        r = (src->p[y][x].r * sa + r * (255 - sa)) / 255;
        g = (src->p[y][x].g * sa + g * (255 - sa)) / 255;
        b = (src->p[y][x].b * sa + b * (255 - sa)) / 255;
        a = (src->p[y][x].alpha * sa + a * (255 - sa)) / 255;
 
        dst->p[dy + y][dx + x].r = r;
        dst->p[dy + y][dx + x].g = g;
        dst->p[dy + y][dx + x].b = b;
        dst->p[dy + y][dx + x].alpha = a;
	  }
	}
  }
}

/*
 * Verkleinere Bild um den Faktor `scale'.
 */
void scale_picture(picture_t **pic, unsigned scale)
{
    picture_t *old, *new;
    unsigned  r, g, b, alpha;
    unsigned  q_scale;
    unsigned  x, y;         /* Zielbild */
    unsigned  xs, ys;       /* Quellbild */

    if (scale <= 1)
	return;
    q_scale = scale * scale;

    old = *pic;
    new = create_picture(old->x / scale, old->y / scale);
    for (y = 0; y < new->y; y++) {
	for (x = 0; x < new->x; x++) {
	    r = g = b = alpha = 0;
	    for (ys = y * scale; ys < y * scale + scale; ys++) {
		for (xs = x * scale; xs < x * scale + scale; xs++) {
		    r += old->p[ys][xs].r;
		    g += old->p[ys][xs].g;
		    b += old->p[ys][xs].b;
		    alpha += old->p[ys][xs].alpha;
		}
	    }
	    new->p[y][x].r = r / q_scale;
	    new->p[y][x].g = g / q_scale;
	    new->p[y][x].b = b / q_scale;
	    new->p[y][x].alpha = alpha / q_scale;
#if 1
	    /*
	     * Reduziere auf 3 * 3 = 9 Bit pro Pixel. Reduziert die
	     * Dateigröße auf ca. die Hälfte.
	     */
	    new->p[y][x].r &= 0xe0;
	    new->p[y][x].g &= 0xe0;
	    new->p[y][x].b &= 0xe0;
#endif
	}
    }
    free_picture(old);

    *pic = new;
}

void set_color_dot(picture_t *dst, int r, int g, int b, int dx, int dy)
{
    int x, y;

    assert(dx > 0);
    assert(dy > 0);

    for (y = -1; y < 1; y++) {
	for (x = -1; x < 1; x++) {
	    dst->p[dy + y][dx + x].r = r;
	    dst->p[dy + y][dx + x].g = g;
	    dst->p[dy + y][dx + x].b = b;
	}
    }
}

const picture_t *region_pic(map_entry_t *e)
{
    picture_t *pic;

    if (e == NULL)
	pic = pictures[T_UNBEKANNT];
    else
        pic = pictures[e->typ];
    return pic;
}

void init_image(void)
{
    extern config_t config;
    FILE *fp;
    int i;
    char bildname[13];  /* Name der Bilddatei mit den Strassenbildern */

    fp = config_open("cellgeometry.txt");
    if (fscanf(fp, "tx=%u\n", &offsets.tx) != 1) {
      fprintf(stderr, "init_image(): Error parsing tx of %s\n", bildname);
      exit(1);
    }
    if (fscanf(fp, "ty=%u\n", &offsets.ty) != 1) {
      fprintf(stderr, "init_image(): Error parsing ty of %s\n", bildname);
      exit(1);
    }
    if (fscanf(fp, "ox=%u\n", &offsets.ox) != 1) {
      fprintf(stderr, "init_image(): Error parsing ox of %s\n", bildname);
      exit(1);
    }
    if (fscanf(fp, "oy=%u\n", &offsets.oy) != 1) {
      fprintf(stderr, "init_image(): Error parsing oy of %s\n", bildname);
      exit(1);
    }
    if (fscanf(fp, "dx=%u\n", &offsets.dx) != 1) {
      fprintf(stderr, "init_image(): Error parsing dx of %s\n", bildname);
      exit(1);
    }
    if (fscanf(fp, "dy=%u\n", &offsets.dy) != 1) {
      fprintf(stderr, "init_image(): Error parsing dy of %s\n", bildname);
      exit(1);
    }
    fclose(fp);

    offsets.tx /= config.scale;
    offsets.ty /= config.scale;
    offsets.ox /= config.scale;
    offsets.oy /= config.scale;
    offsets.dx /= config.scale;
    offsets.dy /= config.scale;

    strcpy(bildname, "Strasse0.png");

    for (i = 0; i < NR_REGIONS; i++) {
      assert(pictures[i] == NULL);
      pictures[i] = load_picture(names[i]);
      if (pictures[i] == NULL) {
        fprintf(stderr,
          "Fehler in load_picture: Kann %s nicht laden.\n", names[i]);
        exit(1);
      }
      scale_picture(&pictures[i], config.scale);
    }
   
    for (i = 0; i < 6; i++) {
      bildname[7] = '0' + i; /* Die Zahl im Strassennahmen durch i ersetzen */
      strassen[i] = load_picture(bildname);
      if (strassen[i] == NULL) {
        fprintf(stderr,
          "Fehler in load_picture: Kann %s nicht laden.\n", bildname);
        exit(1);
      }
      scale_picture(&strassen[i], config.scale);
    }

    burg = load_picture("Burg.png");
    if (burg == NULL) {
	fprintf(stderr,
          "Fehler in load_picture: Kann Burg.png nicht laden.\n");
	exit(1);
    }
    scale_picture(&burg, config.scale);
    schiff = load_picture("Schiff.png");
    if (schiff == NULL) {
	fprintf(stderr,
          "Fehler in load_picture: Kann Schiff.png nicht laden.\n");
	exit(1);
    }
    scale_picture(&schiff, config.scale);
}

void fini_image(void)
{
    int i;

    for (i = 0; i < NR_REGIONS; i++) {
	free_picture(pictures[i]);
	pictures[i] = NULL;
    }
    for (i = 0; i < 6; i++) {
	free_picture(strassen[i]);
	strassen[i] = NULL;
    }
    free_picture(burg);
    free_picture(schiff);
}

void vis2corner(wmap_t *w, int vx, int vy, int *cx, int *cy)
{
    *cx = (vx - w->min_vx) * offsets.dx;
    *cy = (vy - w->min_vy) * offsets.dy;
}

void vis2center(wmap_t *w, int vx, int vy, int *cx, int *cy)
{
    vis2corner(w, vx, vy, cx, cy);
    *cx += offsets.ox;
    *cy += offsets.oy;
}

int write_image_map(html_t *h, wmap_t *w, int ebene,
		    const char *prefix, const char *name, FILE *fp,
		    int image_map)
{
    map_t       *map = h->map;
    map_entry_t *e;
    picture_t   *dst;
    const picture_t *src;
    const char      *reg_typ;
    char            link_name[100];
    char            *alt_string;
    char            *fname;  /* Name der Bilddatei */
    int vx, vy;			/* visuelle Koordinaten */
    int cx, cy;			/* Pixelkoordinaten */
    int hx, hy;			/* Hex-Koordinaten */

    if ( ( w->max_vx - w->min_vx > ROW_MULT ) ||
         ( w->max_vy - w->min_vy > ROW_MULT )    ) {
        fprintf(stderr,
            "Fehler in write_image_map: Karte zu gross: %i x %i Regionen.\n",
            w->max_vx - w->min_vx + 1, w->max_vy - w->min_vy + 1 );
        return 1;
    }

    if (image_map) {
	fprintf(fp, "<img border=0 src=\"%s.png\" usemap=\"#karte\">\n",
		name);
	fprintf(fp, "<map name=\"karte\">\n");
    }
    
    dst = create_picture(
            (w->max_vx - w->min_vx) * offsets.dx + offsets.tx,
            (w->max_vy - w->min_vy) * offsets.dy + offsets.ty );

    for (vy = w->min_vy; vy <= w->max_vy; vy++) {
	for (vx = w->min_vx; vx <= w->max_vx; vx++) {
	    if (valid_vis(vx, vy)) {
		vis2hex(vx, vy, &hx, &hy);
		e = mp(map, hx, hy, ebene);
		src = region_pic(e);
		vis2corner(w, vx, vy, &cx, &cy);
		pic_copy(src, dst, cx, cy);
		vis2center(w, vx, vy, &cx, &cy);
		if (e != NULL) {
		    if (e->first_burg != NULL)
			pic_copy(burg, dst, cx - burg->x/2,
                                            cy - burg->y/2 );
		    if (e->first_schiff != NULL)
			pic_copy(schiff, dst, cx - schiff->x/2,
                                              cy - schiff->y/2 );
		    if (e->first_einheit != NULL ||
                        e->first_text != NULL ||
                        e->first_botschaft != NULL ||
                        e->first_ereignis != NULL ||
                        e->first_durchreise != NULL ||
                        e->first_durchschiffung != NULL ||
                        e->first_kommentare != NULL ||
                        e->first_effect != NULL ) {
                       set_color_dot(dst, 255, 0, 0,
                         cx, cy-offsets.dy/2);
		    }			
            if (e->first_grenze != NULL)
            {
              grenze_t *gr;
              int i;
              for (gr = e->first_grenze; gr != NULL; gr = gr->next)
              {
	       if (gr->prozent == 100) {
                i = gr->richtung;
                pic_copy(strassen[i], dst, 
                         cx - strassen[i]->x/2, cy - strassen[i]->y/2);
	       }
              }
            }
                    if (image_map && region_filter(e)) {
			reg_typ = region_typ_name[e->typ];
			if (e->name != NULL) {
			  if (e->insel) {
			    alt_string = xmalloc(strlen(reg_typ) +
						 strlen(e->name) + 
						 strlen(e->insel) + 100);
			    sprintf(alt_string, "%s %s (%d, %d), %s",
				    reg_typ, e->name, hx, hy, e->insel);
			  } else {
			    alt_string = xmalloc(strlen(reg_typ) +
						 strlen(e->name) + 100);
			    sprintf(alt_string, "%s %s (%d, %d)",
				    reg_typ, e->name, hx, hy);
			  }
			} else {
			  if (e->insel) {
			    alt_string = xmalloc(strlen(reg_typ) +
						 strlen(e->insel) + 100);
			    sprintf(alt_string, "%s (%d, %d), %s",
				    reg_typ, hx, hy, e->insel);
			  } else {
			    alt_string = xmalloc(strlen(reg_typ) + 100);
			    sprintf(alt_string, "%s (%d, %d)",
				    reg_typ, hx, hy);
			  }
			}
			if (region_filter(e))
               sprintf(link_name, "r%c%d%c%de%d.html",
               (hx >= 0) ? 'p' : 'm',
               (hx >= 0) ? hx : -hx,
               (hy >= 0) ? 'p' : 'm',
               (hy >= 0) ? hy : -hy,
               ebene);
			else
			    strcpy(link_name, "unbekannt.html");
			fprintf(fp,
				"<area shape=\"circle\" coords=\"%d,%d,%d\" "
				" target=\"region\" href=\"%s\" "
                " alt=\"%s\">\n",
				cx, cy, offsets.dx, link_name, alt_string);
			xfree(alt_string);
		    }
		}  /* end if (e != NULL) */
	    }  /* end if (valid_vis(vx, vy)) */
     }
    }

    fname = xmalloc(strlen(prefix) + strlen(name) + strlen(".png") + 1);
    strcpy(fname, prefix);
    strcat(fname, name);
    strcat(fname, ".png");
    save_picture(dst, fname);
    free_picture(dst);
    xfree(fname);

    if (image_map)
	  fprintf(fp, "</map>\n");

    return 0;
}

int write_world_map(html_t *h, const char *prefix, const char *name,
		    int ebene, int image_map)
{
    wmap_t      w;
    ebene_t *eb; /* die verschiedenen Ebenen */
    char        *full_name;
    FILE        *fp = NULL;

    w.min_vx = h->min_x;
    w.min_vy = h->min_y;
    w.max_vx = h->max_x;
    w.max_vy = h->max_y;

    if (image_map) {
	full_name = xmalloc(strlen(prefix) + strlen(name) + strlen(".html")+1);
	strcpy(full_name, prefix);
	strcat(full_name, name);
	strcat(full_name, ".html");
	fp = xfopen(full_name, "wb");
	xfree(full_name);
	fprintf(fp, "<html>\n<head>\n<title>%s</title>\n</head>\n",
                translate("Weltkarte", h->locale));
	fprintf(fp, "<body bgcolor=\"white\">\n");
	fprintf(fp, "<center>\n");

    /* wenn es mehr als eine Ebene gibt, erscheint die Auflistung ueber der Karte */
    if ((h->map->ebenen != NULL) && (h->map->ebenen->next != NULL))
    {
      fprintf(fp, "<table border=\"2\" cellpadding=\"3\" cellspacing=\"0\">\n");

      for (eb=h->map->ebenen; eb != NULL; eb=eb->next)
        if (eb->koord != ebene) /* die aktuelle Ebene nicht mit anzeigen */
          fprintf(fp, "<td><a target=\"karte\" href=\"welt_%d.html\">Ebene %d</a></td>\n", eb->koord, eb->koord);
      fprintf(fp, "</table>");
    }
    }

    write_image_map(h, &w, ebene, prefix, name, fp, image_map);

    if (image_map) {
	fprintf(fp, "</center>\n");
	fprintf(fp, "</body>\n");
	fprintf(fp, "</html>\n");
    }
    
    return 0;
}

int write_png(int *argc, char *argv[], map_t *map)
{
    extern config_t config;
    int    i = *argc;
    int    j;
    char   *name;
    html_t h;
    ebene_t *eb;

    i++;			/* skip "write-png" */
    
    if (argv[i] == NULL) {
	fprintf(stderr, "Fehler bei write-png: Dateiname fehlt.\n");
	exit(1);
    }

    for (eb = map->ebenen; eb != NULL; eb = eb->next)
    {
      name = xmalloc(strlen(argv[i]) + 21);
      strcpy(name, argv[i]);
      j = strlen(argv[i])-1;

      while (j > 0 && name[j] != '.')
	  j--;
      if (j > 0)
        name[j] = '\0';		/* schneide ".sonstwas" ab */
      sprintf(name, "%s_%d", name, eb->koord);
      if (config.verbose > 1) fprintf(stdout,
        "Info in write_png: Reserviere Speicher fuer '%s'.\n", name);
      fill_html(map, 0, 0, 0, 0, eb, &h);
      init_image();
      if (config.verbose > 1) fprintf(stdout,
        "Info in write_png: Schreibe '%s.png'...\n", name);
      write_world_map(&h, "", name, eb->koord, 0);
      fini_image();

      xfree(name);
    }
    i++;
    *argc = i;
    return 0;
}
