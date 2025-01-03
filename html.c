/* $Id: html.c,v 1.32 2006/02/08 01:40:56 schwarze Exp $ */
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
#include <ctype.h>
#include <math.h>
#include "mercator.h"

#define EINZELKARTEN 0

typedef struct hex_koor_struct {
    int x;
    int y;
    int z;
} hex_koor_t;


/* externe Variablen - aus language.c */
extern const char * const kampfstatus[];
extern const char * const zauberart[];
extern const char * const zaubermod[];
extern const char * const kuestenname[];
extern const char * const monatsnamen[];

/* Achtung, der Name der Burg wird in language.c gesetzt! */
burgi_t burgen[8] = {
    {    0, 11,  0, "" },
    {    1, 11,  0, "" },
    {    2, 11,  0, "" },
    {   10, 12,  6, "" },
    {   50, 13, 12, "" },
    {  250, 14, 18, "" },
    { 1250, 15, 24, "" },
    { 6250, 16, 30, "" }
};

static int max_bauern[] = {
    0,                /* unbekannt */
    0,                /* nichts */
    0,                /* Ozean */
    0,                /* Land */
    10000,            /* Ebene */
    10000,            /* Wald */
    2000,             /* Sumpf */
    500,              /* Wüste */
    4000,             /* Hochland */
    1000,             /* Berge */
    1000,             /* Vulkan */
    1000,             /* Aktiver Vulkan */
    100,              /* Gletscher */
    0,                /* Eisberg */
    0,                /* Feuerwand */
    0,                /* Weltenrand */
    0,                /* Nebel */
    0                 /* Dichter Nebel */
};

helfe_info_t helfe_info[] = {
    { helfe_silber, "" },
    { helfe_kaempfe, "" },
    { helfe_wahrnehmung, "" },
    { helfe_gib, "" },
    { helfe_bewache, "" },
    { helfe_partei, "" },
    { 0, 0 }
};

static void put36(FILE *fp, int number)
{
    char buf[16];
    int  sign;
    int  c, i;

    if (number < 0) {
    sign = 1;
    number = - number;
    } else
    sign = 0;

    i = 16;
    buf[--i] = 0;
    do {
    c = number % 36;
    number /= 36;
    if (c <= 9)
        buf[--i] = c + '0';
    else
        buf[--i] = c - 10 + 'a';
    if (buf[i] == 'l')
        buf[i] = 'L';
    } while (number != 0);
    if (sign)
    buf[--i] = '-';
    fputs(buf + i, fp);
}

static int get36(const char *s)
{
    int c;
    int sign;

    if (*s == '-') {
      s++;
      sign = 1;
    } else
    sign = 0;
    c = 0;
    while (*s) {
    if ('0' <= *s && *s <= '9')
        c = 36 * c + *s - '0';
    else if ('a' <= *s && *s <= 'z')
        c = 36 * c + *s - 'a' + 10;
    else if ('A' <= *s && *s <= 'Z')
        c = 36 * c + *s - 'A' + 10;
    else
        break;
    s++;
    }
    if (sign)
      return -c;
    else
      return c;
}

static burgi_t *get_burg_info(burg_t *b)
{
    int i;

    i = 6;
    while (i >= 0 && (strcmp(b->typ, burgen[i].typ) != 0))
      i--;

    if (i<0)
      return NULL;        

    i = 6;
    while (i >= 0 && b->groesse < burgen[i].groesse)
      i--;

    return &burgen[i];
}


static int get_bauernlohn(map_entry_t *e, map_t *map)
{
    burg_t    *b;
    burgi_t   *bi;
    int       max_lohn = 11;

  if (map->game == game_eressea) {
    for (b = e->first_burg; b != NULL; b = b->next) {
    bi = get_burg_info(b);
    if (bi != NULL && bi->lohn > max_lohn)
        max_lohn = bi->lohn;
    }

    /* ab CR V59 wurden die Burgen abgewertet */
    if (map->version < 59)
      max_lohn++;
  } else {
    max_lohn = e->lohn;
  }

    return max_lohn;
}

void hex2vis(int hx, int hy, int *vx, int *vy)
{
    *vx = 2 * hx + hy;
    *vy = -hy;
}

void vis2hex(int vx, int vy, int *hx, int *hy)
{
    assert((vx + vy) % 2 == 0);
    *hx = (vx + vy) / 2;
    *hy = -vy;
}

int valid_vis(int vx, int vy)
{
    return (vx + vy) % 2 == 0;
}

/*
 * 1: Region wird ausgegeben
 * 0: Region wird nicht ausgegeben
 */
int region_filter(map_entry_t *e)
{
    extern config_t config;
    int code = 0;

    if (e->first_einheit) code += 1;
    if (e->first_burg) code += 2;
    if (e->first_schiff) code += 4;
    if (e->first_grenze) code += 8;
    if (e->first_text || e->first_botschaft || e->first_ereignis ||
        e->first_kommentare || e->first_effect) code += 16;
    if (e->first_durchreise || e->first_durchschiffung) code += 32;
    if (e->kraut || e->baeume || e->schoesslinge || e->bauern ||
        e->silber || e->eisen || e->laen || e->pferde ||
        e->first_resource || e->first_biete || e->first_kaufe) code += 64;
    if (e->beschr) code += 128;
    if (e->name) code += 256;
    if (code & config.region_flags)
      return 1;
    else
      return 0;
}

static char *number_to_filename(const char *prefix, int ch, int x, int y, int z,
                const char *postfix)
{
    char *name;

    name = xmalloc(strlen(prefix) + 1 + 10 + 10 + strlen(postfix) + 1);
    sprintf(name, "%s%c%c%d%c%de%d%s", prefix, ch,
        x >= 0 ? 'p' : 'm', x >= 0 ? x : -x,
        y >= 0 ? 'p' : 'm', y >= 0 ? y : -y,
        z, postfix);
    
    return name;
}


static char *region_to_filename(map_entry_t *r)
{
    char *name = xmalloc(20);

    assert(r!=NULL);
    sprintf(name, "r%c%d%c%de%d.html",
        (r->x >= 0) ? 'p' : 'm',
        (r->x >= 0) ? r->x : -(r->x),
        (r->y >= 0) ? 'p' : 'm',
        (r->y >= 0) ? r->y : -(r->y),
         r->z);
    return name;
}

#if EINZELKARTEN
/*
 * Berechne aus Eressea-Koordinaten (x, y, z) den Dateinamen, in dem die
 * Karte für diese Region steht.
 */
static char *region_to_map(html_t *h, int x, int y, int z)
{
    int dx, dy;

    hex2vis(x, y, &x, &y);
    x &= ~1;
    dx = x - h->min_x;
    dy = y - h->min_y;
    
    dx -= dx % (h->tsx - h->x_lap);
    dy -= dy % (h->tsy - h->y_lap);
    
    return number_to_filename("", 'k', h->min_x + dx, h->min_y + dy, z,
                  ".html");
}
#endif

/*
 * Schreibe Index für eine Region.
 */
static void write_index(html_t *h, const char *prefix)
{
    FILE *fp;
    char *file;

    file = xmalloc(strlen(prefix) + strlen("index.html") + 1);
    strcpy(file, prefix);
    strcat(file, "index.html");
    fp = xfopen(file, "w");
    xfree(file);

    fprintf(fp, "<html>\n<head>\n");
    fprintf(fp, "<meta name=\"GENERATOR\" "
        "content=\"Mercator Eressea Map Generator\">\n");
    if (h->locale == en)
      fprintf(fp, "<title>Eressea Map</title>\n");
    else
      fprintf(fp, "<title>Eressea Karte</title>\n");
    fprintf(fp, "</head>\n");

#if EINZELKARTEN
    fprintf(fp, "<frameset cols=\"300,*\">\n");
    file = region_to_map(h, 0, 0, 0);
    fprintf(fp, "<frame src=\"%s\" name=\"karte\" marginwidth=\"1\" ", file);
    xfree(file);
#else
    fprintf(fp, "<frameset cols=\"450,*\">\n");
    fprintf(fp, "<frame src=\"welt_0.html\" name=\"karte\" marginwidth=\"1\" ");
#endif
    fprintf(fp, "marginheight=\"1\" scrolling=\"auto\">\n");
    fprintf(fp, "<frame src=\"uebersicht_%i.html\" ", h->map->partei);
    fprintf(fp, "name=\"region\" marginwidth=\"1\" ");
    fprintf(fp, "marginheight=\"1\" scrolling=\"auto\">\n");
    fprintf(fp, "</frameset>\n");
    if (h->locale == en)
      fprintf(fp, "<noframes><body>You will need Frames for this...</body>");
    else
      fprintf(fp, "<noframes><body>Frames braucht man schon...</body>");
    fprintf(fp, "</noframes>\n</html>\n");
    
    fclose(fp);
}


static void write_nav_table(html_t *h, int ebene, int argc, char **argv, FILE *fp)
{
    char *file;
    int  i;
    int  vx, vy;

    fprintf(fp, "<center>\n");
    fprintf(fp,
      "<table border=\"2\" cellpadding=\"3\" cellspacing=\"0\"><tr>\n");
    fprintf(fp, "<td><a href=\"uebersicht_%i.html\">%s</a></td>\n",
                 h->map->partei,
                 translate("&Uuml;bersicht", h->locale));
    hex2vis(0, 0, &vx, &vy);
#if EINZELKARTEN
    file = region_to_map(h, vx, vy, ebene);
    fprintf(fp, "<td><a target=\"karte\" href=\"%s\">Ursprung</a></td>\n",
        file);
    xfree(file);
#endif
    (void)file;
    fprintf(fp, "<td><a href=\"parteien.html\">%s</a></td>\n", 
                translate("Parteien", h->locale));
#if EINZELKARTEN
    fprintf(fp, "<td><a href=\"welt_%d.html\">Weltkarte</a></td>\n", ebene);
#endif

    for (i = 0; i < argc; i++) {
      fprintf(fp, "<td>%s</td>\n", argv[i]);
    }
    fprintf(fp, "</tr></table>\n</center>\n");
}

/* Regionsmeldungen schreiben */
void write_region_meldungen(map_t *map, lpmeldung_t *c, FILE *fp)
{
  map_entry_t *r;
  char link_name[256];
 
  if (c == NULL)
    return;

  fprintf(fp, "<h2>%s</h2>\n", translate("Regionsmeldungen", map->locale));
  fprintf(fp, "<p>\n");
  for ( ; c != NULL; c = c->next)
  {
    r = mp(map, c->x, c->y, c->z); /* richtige Region suchen */
    assert(r!=NULL);

    sprintf(link_name, "r%c%d%c%de%d.html",
    (c->x >= 0) ? 'p' : 'm',
    (c->x >= 0) ? c->x : -(c->x),
    (c->y >= 0) ? 'p' : 'm',
    (c->y >= 0) ? c->y : -(c->y),
     c->z);

    fprintf(fp, "<a href=\"%s\">", link_name);
    if (r->name != NULL)
      fprintf(fp, "%s</a>: ", r->name);
    else
      fprintf(fp, "%s</a>: ",
              translate(region_typ_name[r->typ], map->locale));
    fprintf(fp, "%s<br>", c->pmeldung->zeile);
  }
  fprintf(fp, "</p>\n");
}


/* Zu einem Block zugeordnete Meldungen schreiben */
void write_meldungen(meldung_t *c, FILE *fp)
{
  if (c != NULL)
  {
      fprintf(fp, "<i>");
      for ( ; c != NULL; c = c->next)
        fprintf(fp, "%s<br>", c->zeile);
      fprintf(fp, "</i>");
  }        
}

void write_lpmeldungen(lpmeldung_t *c, FILE *fp)
{
  if (c != NULL)
  {
      fprintf(fp, "<i>");
      for ( ; c != NULL; c = c->next)
        fprintf(fp, "%s<br>", c->pmeldung->zeile);
      fprintf(fp, "</i>");
  }        
}


static void write_text_list(html_t *h, const char *title,
                int level, meldung_t *m, FILE *fp)
{
    extern config_t config;
    einheit_t *ei;
    int  komma_found;
    char *s, *p;
    int  x, y, z, enr;
    char *file;

    if (m == NULL)        /* keine Meldungen... */
    return;
    if (title != NULL) {
        fprintf(fp, "<h%d>%s</h%d>\n", level, title, level);
        fprintf(fp, "<p>\n");
    }
    while (m != NULL) {
    for (s = m->zeile; *s != 0; s++) {
        if (*s != '(')
          fputc(*s, fp);
        else {
          komma_found = 0;
        for (p = s; *p != 0 && *p != ')'; p++)
            komma_found |= (*p == ',');
        if (*p == ')') {
            if (komma_found) {
            sscanf(s, "(%d, %d)", &x, &y);
            fprintf(fp, "(%d, %d)", x, y);
            } else {
            enr = get36(s + 1); /* (...) */
            ei = ep(h->map, enr);
            if (ei == NULL) {
                fprintf(fp, "(");
                put36(fp, enr);
                fprintf(fp, ")");
            } else {
                x = ei->region->x;
                y = ei->region->y;
                z = ei->region->z;
                file = number_to_filename("", 'r', x, y, z, ".html");
                fprintf(fp, "(<a target=\"region\" href=\"%s\">",
                    file);
                put36(fp, enr);
                fprintf(fp, "</a>)");
                xfree(file);
            }
            }
            s = p; /* auf ")", s++ in "for" geht dahinter */
        } else {
          if (config.verbose > 0) {
            fprintf(stderr, "Warnung in write_text_list: Klammerfehler.\n");
            fprintf(fp, "%s", s);
          }
          break;
        }
        }
    }
    fprintf(fp, "<br>\n");
    m = m->next;
    }
    fprintf(fp, "</p>\n");
}


/* Kaempfe schreiben */
static void write_kampf_list(html_t *h, int level, battle_t *b, FILE *fp)
{
    if (b == NULL) return;
    while (b != NULL) {
    if (b->region->name == NULL)
      fprintf(fp, "<h%d>%s (%d %d %d)</h%d>\n", level, translate("Kampf in", h->locale),
          b->region->x, b->region->y, b->region->z, level);
    else
    {
      char *filename = region_to_filename(b->region);
      fprintf(fp, "<h%d>%s <a href=%s>%s</a></h%d>\n", level, translate("Kampf in", h->locale),
          filename, b->region->name, level);
      xfree(filename);
    }
    fprintf(fp, "<p>\n");
      write_text_list(h, NULL, 2, b->details, fp);
      b = b->next;
    }
}


static void write_zauber_list(html_t *h, const char *title,
                int level, zauber_t *z, FILE *fp)
{
    komponenten_t *g;

    if (z == NULL)        /* keine Zauber... */
      return;
    if (title != NULL) {
        fprintf(fp, "<h%d>%s</h%d>\n", level, title, level);
        fprintf(fp, "<p>\n");
    }
    while (z != NULL) {
      fprintf(fp, "%s: <b>%s</b>", translate("Name", h->locale), z->spruch);
      if (z->art != -1)
        fprintf(fp, "<br>%s", zauberart[z->art]);
      fprintf(fp, "<br>%s: ", translate("Komponenten", h->locale));

      for (g = z->komp; g != NULL; g = g->next) {
        fprintf(fp, "%d&nbsp;%s", g->anzahl, g->name);
        if (g->mult == 1)
        {
          fprintf(fp, " * %s", translate("Stufe", h->locale));
        }
        if (g->next != NULL)
          fprintf(fp, ", ");
      }

      if (z->mod != 0)
      {
        if (h->locale == en)
          fprintf(fp, "<br>modifications: ");
        else
          fprintf(fp, "<br>Modifikationen: ");
        if ((z->mod & 1) != 0)
          fprintf(fp, "%s ", zaubermod[0]);
        if ((z->mod & 2) != 0)
          fprintf(fp, "%s ", zaubermod[1]);
      }


      if (h->locale == en)
        fprintf(fp, "<br>level: %d<br>", z->stufe);
      else
        fprintf(fp, "<br>Stufe: %d<br>", z->stufe);
      if (z->rang != -1)
      {
        if (h->locale == en)
          fprintf(fp, "rank: %d<br>", z->rang);
        else
          fprintf(fp, "Rang: %d<br>", z->rang);
      }


      fprintf(fp, "<i>%s</i>", z->beschr);
      fprintf(fp, "<br><br>\n");
      z = z->next;
    }
    fprintf(fp, "</p>\n");
}

static void write_trank_list(html_t *h, const char *title,
                int level, trank_t *z, FILE *fp)
{
    gegenstand_t *g;

    if (z == NULL)        /* keine Traenke... */
      return;
    if (title != NULL) {
        fprintf(fp, "<h%d>%s</h%d>\n", level, title, level);
        fprintf(fp, "<p>\n");
    }
    while (z != NULL) {
      if (h->locale == en)
      {
        fprintf(fp, "name: %s<br>", z->name);
        fprintf(fp, "<br>ingredients: ");
      }
      else
      {
        fprintf(fp, "Name: %s<br>", z->name);
        fprintf(fp, "<br>Zutaten: ");
      }

      for (g = z->komp; g != NULL; g = g->next) {
        fprintf(fp, "%s", g->name);
        if (g->next != NULL)
          fprintf(fp, ", ");
      }

      if (h->locale == en)
        fprintf(fp, "<br>level: %d<br>", z->stufe);
      else
        fprintf(fp, "<br>Stufe: %d<br>", z->stufe);

      fprintf(fp, "%s", z->beschr);
      fprintf(fp, "<br><br><br>\n");
      z = z->next;
    }
    fprintf(fp, "</p>\n");
}


static void write_uebersicht(html_t *h, partei_t *p, const char *prefix)
{
    extern char *option_info[];
    char     file[256];
    FILE     *fp;
    int      iopt, idopt, ropt;
    char     *name;
    partei_t *mapp;
    float    prozent;
   
    mapp = finde_partei(h->map, h->map->partei);
    if (p != NULL && p->name != NULL)
    name = p->name;
    else
    name = "<unbekannt>";

    snprintf(file, sizeof(file), "%suebersicht_%i.html", prefix, p->nummer);
    fp = xfopen(file, "w");

    fprintf(fp, "<html>\n<head>\n<title>%s</title>\n</head>\n",
            translate("&Uuml;bersicht", h->locale));
    fprintf(fp, "<body>\n");
    write_nav_table(h, 0, 0, NULL, fp);
    fprintf(fp, "<h1>%s (", name);
    put36(fp, p->nummer);
    fprintf(fp, ")<BR>\n");

    fprintf(fp, "%s %d", translate("Runde", h->locale), h->map->runde);
    if (h->map->game == game_eressea && h->map->runde >= 184)
      fprintf(fp, ", %d/%s/%d", (h->map->runde-22)%27%3+1,
              monatsnamen[(h->map->runde-22)%27/3],
              (h->map->runde-22)/27-5);
    fprintf(fp, "</h1>\n");
    if (p->punktedurchschnitt != 0) {
      prozent = roundf(100.0 * (float)p->punkte / (float)p->punktedurchschnitt * 100.0) / 100.0;
      fprintf(fp, "%s: %d, %s: %d (%.2f%%)<br>\n", translate("Punkte", h->locale),
      p->punkte, translate("Punktedurchschnitt", h->locale),
      p->punktedurchschnitt, prozent);
    }
    if (p->schatz > 0)
      fprintf(fp, "%s: %d %s<br>\n", translate("Reichsschatz", h->locale),
              p->schatz, translate("Silber", h->locale));
    if (p->optionen > 0) {
      fprintf(fp, "<p>%s: ", translate("Optionen", h->locale));
      ropt = p->optionen;
      idopt = 1;
      iopt = 0;
      while (option_info[iopt]) {
        if (ropt & idopt) {
          fprintf(fp, "%s", option_info[iopt]);
          ropt -= idopt;
          if (ropt > 0) fprintf(fp, ", ");
        }
        idopt *= 2;
        iopt++;
      }
      if (ropt > 0)
        fprintf(fp, "%s: %d", translate("unbekannt", h->locale), ropt);
      fprintf(fp, "</p>\n");
    }

    write_kampf_list(h, 2, p->battle, fp);
    write_text_list(h, translate("K&auml;mpfe", h->locale), 2, p->kaempfe, fp);
   if (p == mapp) {
    write_zauber_list(h, translate("Zauber", h->locale), 2,
      h->map->zauber, fp);
    write_trank_list(h, translate("Tr&auml;nke", h->locale), 2,
      h->map->trank, fp);   /* neues Format */
   }
    write_text_list(h, translate("Tr&auml;nke", h->locale), 2,
      p->traenke, fp);   /* altes Format */
    write_text_list(h, translate("Meldungen und Ereignisse", h->locale),
                    2, p->meldungen, fp);
    write_text_list(h, translate("Warnungen und Fehler", h->locale),
                    2, p->fehler, fp);
    write_text_list(h, translate("Wirtschaft und Handel", h->locale),
                    2, p->handel, fp);
    write_text_list(h, translate("Rohstoffe und Produktion", h->locale),
                    2, p->produktion, fp);
    write_text_list(h, translate("Reisen und Bewegung", h->locale),
                    2, p->bewegungen, fp);
    write_text_list(h, translate("Lehren und Lernen", h->locale),
                    2, p->lernen, fp);
    write_text_list(h, translate("Magie und Artefakte", h->locale),
                    2, p->magie, fp);
    write_text_list(h, translate("Einkommen", h->locale), 2, p->einkommen, fp);

    if (p == mapp)
    write_region_meldungen(h->map, h->map->regionen_msg, fp);


    fprintf(fp, "</body>\n</html>\n");

    fclose(fp);
}

static void write_allianz(FILE *fp, allianz_t *a, locale_e locale)
{
   unsigned int i, j;

   if (a == NULL)
     return;

   fprintf(fp, "<h4>%s</h4>\n", translate("Allianzen", locale));
   fprintf(fp, "<table>\n");

   for ( ; a != NULL; a = a->next)
   {
       fprintf(fp, "<tr><td>%s  (<a href=parteien.html#%d>", a->partei->name, a->partei->nummer);
       put36(fp, a->partei->nummer);
       fprintf(fp, ")</a></td><td>");
       if (a->status == 0)
       {
         fprintf(fp, "</td></tr>\n");
         continue;
       }
       else
         fprintf(fp, "(");

       for (i = 0, j = 0; helfe_info[i].status != 0; i++) {
         if (helfe_info[i].status & a->status) {
             if (j != 0)
               fprintf(fp, ", ");
             fprintf(fp, "%s", helfe_info[i].text);
             j++;
         }
       }
       fprintf(fp, ")</td></tr>\n");
     }
   fprintf(fp, "</table>\n");
}

/* Ausgabe der Gruppen in der Parteistatistik*/
static void  write_gruppen(FILE *fp, partei_t *p, locale_e locale)
{
   gruppe_t *grp = p->first_gruppe;
   char rasse[256]; /* Rasse mit angeben, falls ein Typpraefix gesetzt ist */
   
   if (grp != NULL) {
     fprintf(fp, "<blockquote>");
   
     while (grp != NULL) {
        fprintf(fp, "<a name=\"%dgrp%d\"><h3>%s",
            p->nummer, grp->nummer, grp->name);
        if (grp->typprefix != NULL)
        {
          strcpy(rasse, grp->typprefix);
          strcat(rasse, p->typus);
          rasse[strlen(grp->typprefix)] = tolower(rasse[strlen(grp->typprefix)]);
          fprintf(fp, ", %s", rasse);
        }
        fprintf(fp, "</h3></a>");
         write_allianz(fp, grp->alli, locale);
         grp = grp->next;
     }
     fprintf(fp, "</blockquote>\n");
   }   
}


/* Auflistungen aller Parteien
  (das, was sich spaeter hinter dem Link "Parteien" verbirgt) */
static void write_parteien(html_t *h, const char *prefix)
{
    char      *file;
    FILE      *fp;
    partei_t  *p;
    einheit_t *ei;
    int       i, j;        /* für Status */
    int       ec, pc;        /* Einheiten, Personen */
    char       *unbekannt; /* entweder "unbekannt" oder "unknown" */

    struct reg {
    map_entry_t *map;
    struct reg  *next;
    } *rf, *rl, *r;

    file = xmalloc(strlen(prefix) + strlen("parteien.html") + 1);
    strcpy(file, prefix);
    strcat(file, "parteien.html");
    fp = xfopen(file, "w");
    xfree(file);
    fprintf(fp, "<html>\n<head>\n<title>%s</title>\n</head>\n",
                 translate("Parteien", h->locale));
    fprintf(fp, "<body>\n");
    write_nav_table(h, 0, 0, NULL, fp);
    if (h->locale == en)
    {
      fprintf(fp, "<h1>list of all factions</h1>\n");
      unbekannt = "(unknown)";
    }
    else
    {
      fprintf(fp, "<h1>Liste aller Parteien</h1>\n");
      unbekannt = "(unbekannt)";
    }

    for (p = h->map->first_partei; p != NULL; p = p->next) {
      ec = 0;
      pc = 0;
      /* Einheiten und Personen zaehlen */
      for (ei = p->first_einheit; ei != NULL; ei = ei->next_partei) {
        if (ei->verraeter == 0) /* Verraeter nicht mitzaehlen */
        {
          ec++;
          pc += ei->anzahl;
        }
      }
      
      fprintf(fp, "<a name=\"%d\"><h2>", p->nummer);
      if ( p->battle != NULL ||
           p->kaempfe != NULL ||
           p->meldungen != NULL ||
           p->fehler != NULL ||
           p->handel != NULL ||
           p->produktion != NULL ||
           p->bewegungen != NULL ||
           p->lernen != NULL ||
           p->magie != NULL ||
           p->einkommen != NULL ) {
        fprintf(fp, "<a href=\"uebersicht_%i.html\">%s</a> (",
                p->nummer, (p->name != NULL ? p->name : unbekannt));
        write_uebersicht(h, p, prefix);
      } else
        fprintf(fp, "%s (", (p->name != NULL ? p->name : unbekannt));
      put36(fp, p->nummer);
      if (p->typus != NULL && p->typus[0] != 0) {
	if (p->typprefix != NULL && p->typprefix[0] != 0)
          fprintf(fp, "), %s%c%s</h2></a>\n<p>", p->typprefix,
                  tolower(p->typus[0]), p->typus+1);
        else
          fprintf(fp, "), %s</h2></a>\n<p>", p->typus);
      } else {
          fprintf(fp, ")</h2></a>\n<p>");
      }

      if (p->email != NULL)
          fprintf(fp, "email: %s<br>\n", p->email);
      if (p->banner != NULL && p->banner[0] != 0)
          fprintf(fp, "Banner: %s<br>\n", p->banner);
      /*
       * Status ausgeben. "j" zählt die bereits ausgegebenen "helfes",
       * damit das "," richtig gesetzt werden kann.
       */
      if (p->status != 0) {
          fprintf(fp, "%s: ", translate("Status", h->locale));
          for (i = 0, j = 0; helfe_info[i].status != 0; i++) {
            if (helfe_info[i].status & p->status) {
                if (j != 0)
                fprintf(fp, ", ");
                fprintf(fp, "%s", helfe_info[i].text);
                j++;
            }
          }
          fprintf(fp, "<br>\n");
      }

      if (p->age != 0)
        fprintf(fp, "%s: %d %s<br>\n",
                translate("Alter", h->locale), p->age,
                translate("Wochen", h->locale));
      if (p->punktedurchschnitt != 0)
        fprintf(fp, "%s: %d, %s: %d (%d%%)<br>\n",
        translate("Punkte", h->locale), p->punkte,
        translate("Punktedurchschnitt", h->locale),
        p->punktedurchschnitt, p->punkte*100/p->punktedurchschnitt);
      if (ec != 0 || pc != 0) {
        fprintf(fp, "%s: %d, %s: %d\n",
                translate("Einheiten", h->locale), ec,
                translate("Personen", h->locale), pc);
        if (p->heroes != 0 || p->max_heroes != 0)
          fprintf(fp, ", %s: %d %s %d<BR>\n",
                  translate("Helden", h->locale), p->heroes,
                  translate("von max.", h->locale), p->max_heroes);
      }
      fprintf(fp, "</p><p>");
      rf = rl = NULL;
      for (ei = p->first_einheit; ei != NULL; ei = ei->next_partei) {
          for (r = rf; r != NULL; r = r->next) {
            if (r->map == ei->region)
              break;
          }
          if (r == NULL) {
          r = xmalloc(sizeof(struct reg));
          r->next = NULL;
          r->map  = ei->region;
          if (rf == NULL) {
              rf = r;
              rl = r;
          } else {
              rl->next = r;
              rl = r;
          }
          }
      }
      while (rf != NULL) {
          char *n = number_to_filename("", 'r',
                       rf->map->x, rf->map->y, rf->map->z,
                       ".html");
          if (rf->map->name != NULL)
          fprintf(fp, "<a target=\"region\" HREF=\"%s\">%s</a>",
              n, rf->map->name);
          else
          fprintf(fp, "<a target=\"region\" HREF=\"%s\">(%d, %d)</a>",
              n, rf->map->x, rf->map->y);
          xfree(n);
          if (rf->next != NULL)
          fprintf(fp, ", ");
          r = rf;
          rf = rf->next;
          xfree(r);
      }
      fprintf(fp, "</p>\n");
      write_allianz(fp, p->alli, h->locale);
      write_gruppen(fp, p, h->locale);
    }
    fprintf(fp, "</body>\n</html>\n");

    fclose(fp);
}

/* Gibt Anzahl der gesuchten Resource zurueck */
static resource_t *find_resource(resource_t *rs, char *resource, locale_e locale)
{
  for ( ; rs != NULL; rs = rs->next)
    if (strcmp(translate(rs->type, locale), resource) == 0)
      return rs;
  return NULL;
}


static void write_resource(FILE *fp, map_entry_t *e, locale_e locale)
{
  const resource_t *rs = e->first_resource;
  resource_t *old=NULL; /* Zeiger auf alte Resourcenliste */
  resource_t *resource_alt; /* Menge der Resourcen im vorigen Report */
  
  assert(e!=NULL);
  
  if (e->prev_round != NULL)
    old = e->prev_round->first_resource;

  for ( ; rs != NULL; rs = rs->next) /* Schleife ueber Resourcen */
  {
    fprintf(fp, "<tr><td>%s:</td><td>%d", translate(rs->type, locale),
                                            rs->number);
    resource_alt = find_resource(old, rs->type, locale);

    if (rs->skill > 0)
      fprintf(fp, "/T%d", rs->skill);
    fprintf(fp, "</td><td>");

    if (resource_alt != NULL)
    {
      if ((resource_alt->skill == rs->skill) &&
          (rs->number != resource_alt->number))
        fprintf(fp, " %+d", rs->number - resource_alt->number);

      if ((rs->skill != 0) && (resource_alt->skill != rs->skill))
          fprintf(fp, "T%+d", rs->skill-resource_alt->skill);
    }
    fprintf(fp, "</td></tr>\n");
  }
}

/* eine Zeile im "Statistik" Teil der Regionsuebersicht schreiben */
void write_stat_entry(FILE *efp, char *item, int old_count, int new_count, locale_e locale)
{
  if ((new_count > -1) && (old_count != new_count))
    fprintf(efp, "<tr><td>%s:</td><td>%d</td><td>&nbsp;%+d</td></tr>\n", 
            translate(item, locale), old_count, old_count-new_count);
  else
    fprintf(efp, "<tr><td>%s:</td><td>%d</td><td></td></tr>\n", 
            translate(item, locale), old_count);
}

static void write_html_umgebung(html_t *h, hex_koor_t *r,
                const char *prefix, FILE *efp)
{
    map_entry_t *e;
    resource_t *resource;
    preis_t     *p;
    int         vx, vy;
    wmap_t      w;
    int         err;
    char        *name;
    int         lohn;
    int         plaetze; /* Menge Arbeitsplaetze */
    int         old_value = -1;
    map_entry_t *e_old; /* Zeiger auf Report von letzter Runde */

    e = mp(h->map, r->x, r->y, r->z);
    assert(e != NULL);
    hex2vis(r->x, r->y, &vx, &vy);
    e_old = e->prev_round;

    plaetze = max_bauern[e->typ];
    if (h->map->game == game_eressea)
      plaetze -= 8*e->baeume;
    else
      plaetze -= 10*e->baeume;

    resource=find_resource(e->first_resource, "Bäume", h->locale);
    if (resource == NULL)
      resource=find_resource(e->first_resource, "Mallorn", h->locale);      
    if (resource != NULL)
      plaetze -= 8*resource->number;

    resource=find_resource(e->first_resource, "Schößlinge", h->locale);
    if (resource == NULL)
      resource=find_resource(e->first_resource, "Mallornschößlinge", h->locale);
    if (resource != NULL)
      plaetze -= 4*resource->number;

    fprintf(efp, "<table border=\"2\" cellpadding=\"3\" cellspacing=\"0\">\n");
    fprintf(efp, "<tr>\n");
    fprintf(efp, "<td><h3>%s</h3></td>\n",  translate("Statistik", h->locale));
    fprintf(efp, "<td><h3>%s</h3></td>\n",  translate("Markt", h->locale));
    fprintf(efp, "<td><h3>%s</h3></td>\n",  translate("Umgebung", h->locale));
    fprintf(efp, "</tr>\n");
    fprintf(efp, "<tr>\n");

    fprintf(efp, "<td valign=\"top\">\n");

    /*
     * Statistik & Veraenderung zur Vorwoche
     */
    fprintf(efp, "<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\">\n");
    if (e->strasse != 0)
      fprintf(efp, "<td>%s: %d %%</td><td></td>\n",
              translate("Straße", h->locale), e->strasse);
    if (e->baeume != 0) {
      if (e->prev_round != NULL)
        old_value = e_old->baeume;
      else
        old_value = -1;

      if (e->mallorn)
        write_stat_entry(efp, "Mallorn", e->baeume, old_value, h->locale);
      else
        write_stat_entry(efp, "B&auml;ume", e->baeume, old_value, h->locale);
    }
    if (e->schoesslinge != 0)
      fprintf(efp, "%s: %d<br>\n",
            translate("<tr><td>Sch&ouml;&szlig;linge</td><td></td></tr>", h->locale), e->schoesslinge);
    if (e->bauern != 0) {
      if (e->prev_round != NULL)
        old_value = e_old->bauern;
      else
        old_value = -1;

      write_stat_entry(efp, "Bauern", e->bauern, old_value, h->locale);

      /* zu viele Bauern, also rot */
      if (plaetze < e->bauern)
        fprintf(efp, "<tr><td>%s:</td><td><font color=#F02020>-%d</font></td><td></td></tr>\n",
                translate("Arbeitspl&auml;tze", h->locale), e->bauern - plaetze);
      else
        fprintf(efp, "<tr><td>%s:</td><td>%d</td><td></td></tr>\n", 
                translate("Arbeitspl&auml;tze", h->locale), plaetze - e->bauern);
    }
    write_resource(efp, e, h->locale);
    if (e->silber != 0) {
      if (e->prev_round != NULL)
        old_value = e_old->silber;
      else
        old_value = -1;

      write_stat_entry(efp, "Silber", e->silber, old_value, h->locale);
    }
    if (e->eisen != 0) {
      if (e->prev_round != NULL)
        old_value = e_old->eisen;
      else
        old_value = -1;

      write_stat_entry(efp, "Eisen", e->eisen, old_value, h->locale);
    }
    if (e->laen != 0) {
      if (e->prev_round != NULL)
        old_value = e_old->laen;
      else
        old_value = -1;

      write_stat_entry(efp, "Laen", e->laen, old_value, h->locale);
    }
    if (e->pferde != 0) {
      if (e->prev_round != NULL)
        old_value = e_old->pferde;
      else
        old_value = -1;

      write_stat_entry(efp, "Pferde", e->pferde, old_value, h->locale);
    }
    if (e->rekrutierung != 0) {
      if (e->prev_round != NULL)
        old_value = e_old->rekrutierung;
      else
        old_value = -1;

      write_stat_entry(efp, "Rekrutierung", e->rekrutierung, old_value, h->locale);
    }
    if (e->luxusgueter != 0) {
      if (e->prev_round != NULL)
        old_value = e_old->luxusgueter;
      else
        old_value = -1;

      write_stat_entry(efp, "Luxusg&uuml;ter", e->luxusgueter, old_value, h->locale);
    }
    if (h->map->game == game_eressea) {
      lohn = e->lohn;
      if (e->prev_round != NULL)
        old_value = e_old->lohn;
      else
        old_value = -1;
    } else {
      lohn = 0;
    }
    if (lohn) write_stat_entry(efp, "Lohn", lohn, old_value, h->locale);
    if (e->bauern != 0) {
      lohn = get_bauernlohn(e, h->map);
      write_stat_entry(efp, "Bauernlohn", lohn, -1, h->locale);
      write_stat_entry(efp, "&Uuml;berschu&szlig;", 
	(e->bauern<plaetze ? e->bauern : plaetze)*lohn - e->bauern*10,
        -1, h->locale);
    }
    if (e->unterhaltung != 0) {
      if (e->prev_round != NULL)
        old_value = e_old->unterhaltung;
      else
        old_value = -1;

      write_stat_entry(efp, "Unterhaltung", e->unterhaltung,
                       old_value, h->locale);
    }
    if (e->kraut != NULL)
      fprintf(efp, "<tr><td>%s: %s</td><td></td></tr>\n", translate("Kraut", h->locale), e->kraut);
    if (e->runde != 0 && e->runde != h->map->runde) {
      fprintf(efp, "<tr></tr><tr><td>%s %d</td><td>(%d)</td></tr>\n",
              translate("Runde", h->locale), e->runde,
              e->runde - h->map->runde);
    }
    fprintf(efp, "</table></td>\n");
    /*
     * Markt
     */
    fprintf(efp, "<td valign=\"top\">\n");
    if (e->first_kaufe == NULL)
    fprintf(efp, "&nbsp;\n");
    else {
    for (p = e->first_kaufe; p != NULL; p = p->next) {
        fprintf(efp, "%s: %d<br>\n", p->produkt, p->preis);
    }
    }
    fprintf(efp, "&nbsp;<br>\n");
    if (e->first_biete == NULL)
    fprintf(efp, "&nbsp;\n");
    else {
    for (p = e->first_biete; p != NULL; p = p->next) {
        fprintf(efp, "%s: %d<br>\n", p->produkt, -p->preis);
    }
    }
    fprintf(efp, "</td>\n");
    /*
     * Umgebung
     */
    fprintf(efp, "<td>\n");
    w.min_vx = vx - 2;
    w.max_vx = vx + 2;
    w.min_vy = vy - 1;
    w.max_vy = vy + 1;
    name = xmalloc(40);
    sprintf(name, "rm%p", (void *)e);
    err = write_image_map(h, &w, r->z, prefix, name, efp, 1);
    if (err) {
      fprintf(stderr,
        "Fehler in write_html_umgebung beim Schreiben der Image-Map.\n");
      exit(1);
    }
    xfree(name);
    if (e->insel != NULL) {
    fprintf(efp, "<center>%s</center>", e->insel);
    }
    fprintf(efp, "</td>\n");

    fprintf(efp, "</tr>\n");
    fprintf(efp, "</table>\n");

}


static void write_einheiten_nummer(map_t *map, einheit_t *ei, FILE *fp,
                   const char *prefix)
{
#ifdef GOTOLINE
    if (ei->partei == map->partei) {
    FILE *fp2;
    char *file;
    fprintf(fp, "<a href=\"%d.ere\">", ei->nummer);
    put36(fp, ei->nummer);
    fprintf(fp, "</a>");

    file = xmalloc(strlen(prefix) + 100);
    sprintf(file, "%s/%d.ere", prefix, ei->nummer);
    fp2 = xfopen(file, "w");
    put36(fp2, ei->nummer);
    putc('\n', fp2);
    fclose(fp2);
    xfree (file);
    } else {
#endif
    put36(fp, ei->nummer);
#ifdef GOTOLINE
    }
#endif
}

static void write_einheiten_liste(map_t *map, map_entry_t *e, FILE *fp,
                  int partei, int auf_schiff, int in_burg,
                  int einheit, const char *prefix)
{ 
  extern config_t config;
  einheit_t    *ei;
  talent_t     *t;
  gegenstand_t *g;
  meldung_t    *effect;
  int          d = 0;
  int          gib_aus;

  /* erstmal feststellen, welche Einheiten ausgegeben werden muessen */  
  for (ei = e->first_einheit; ei != NULL; ei = ei->next) {
    if (ei->ausgegeben || (einheit != -1 && ei->nummer != einheit))
      gib_aus = 0;
     else if (auf_schiff >= 0 || in_burg >= 0) {
        if (auf_schiff >= 0 && ei->ist_in == auf_schiff)
          gib_aus = 1;
        else if (in_burg >= 0 && ei->ist_in == in_burg)
          gib_aus = 1;
        else
          gib_aus = 0;
    } else {
        if (partei == -1)
          gib_aus = 1;
        else if (map->partei == ei->partei)
          gib_aus = 1;
        else
          gib_aus = 0;
    }

    if (gib_aus) {
        ei->ausgegeben = 1;
        if (ei->partei != map->partei) {
          partei_t *p = finde_partei(map, ei->partei);
        
          if (p == NULL || p->status == 0)
            fprintf(fp, "<font color=#800000>");
          else if ((p->status & config.helfe_alles) == config.helfe_alles)
            fprintf(fp, "<font color=#008000>");
          else
            fprintf(fp, "<font color=#808000>");
        } else if (ei->verraeter == 1) {
            fprintf(fp, "<font color=#00D0D0>");
        }
        fprintf(fp, "<strong>");
        write_einheiten_nummer(map, ei, fp, prefix);
            if (ei->alias != 0) {
                fprintf(fp, " (alias ");
                put36(fp, ei->alias);
                fprintf(fp, ")");
            }
            else if (ei->temp != -1) {
                fprintf(fp, " (alias TEMP ");
                put36(fp, ei->temp);
                fprintf(fp, ")");
            }
        fprintf(fp, ", ");
        if ((ei->partei == map->partei) && (ei->verraeter == 0)){
          fprintf(fp, "%d", ei->anzahl);
          if (ei->wahrer_typ != NULL)
            fprintf(fp, " <font color=#800000> %s</font>",
                ei->wahrer_typ);
        } else if (ei->typprefix != NULL) {
          char *typ = xstrdup(ei->typ);
          typ[0] = tolower(typ[0]);
          fprintf(fp, "%s%s, %d", ei->typprefix, typ, ei->anzahl);
          xfree(typ);
        } else {
          fprintf(fp, "%s, %d", ei->typ, ei->anzahl);
        }
        if (ei->name != NULL)
          fprintf(fp, " %s", ei->name);
        if (ei->partei != map->partei && ei->partei != 0) {
          fprintf(fp, " (%s <a href=\"parteien.html#%d\">",
            translate("Partei", map->locale), ei->partei);
          put36(fp, ei->partei);
          fprintf(fp, "</a>)");
        } else if (ei->verraeter == 1)
        {
          fprintf(fp, ", <font color=#FF0000><strong>%s!</strong></font>", translate("Spion", map->locale));
        } else if (ei->gruppe != 0) { /* Gruppenzugehoerigkeit schreiben */
          fprintf(fp, " (%s <a href=\"parteien.html#%dgrp%d\">\"%s\"</a>)",
                  translate("Gruppe", map->locale),
                  ei->partei, ei->gruppe,
                  finde_gruppe(map, ei->partei, ei->gruppe));          
        }
        if (ei->hero != 0)
          fprintf(fp, ", %s",
                  translate((ei->anzahl>1 ? "Helden" : "Held"), map->locale));
        if (ei->belagert != 0)
          fprintf(fp, " (%s %d)", translate("belagert", map->locale), ei->belagert);
        if (ei->folgt != 0) {
          fprintf(fp, " (folgt ");
          put36(fp, ei->folgt);
          fprintf(fp, ")");
        }
        if (ei->hunger != -1)
          fprintf(fp, " (<font color=#f50000>%s!</font>)", translate("hungert", map->locale));
        if (ei->hp != NULL)
          fprintf(fp, " (%s)", ei->hp);
        if (ei->ks != 0) /* Kampfstatus schreiben */
          fprintf(fp, " (%s)", kampfstatus[ei->ks]);
        if (ei->unaided != 0)
          fprintf(fp, " (%s)", translate("bekommt keine Hilfe", map->locale));
        if (einheit != -1) {
          if (in_burg > 0)
            fprintf(fp, " (%s)", translate("Burgherr", map->locale));
          else
            fprintf(fp, " (%s)", translate("Kapit&auml;n", map->locale));
        }
        if (ei->auramax != -1 || ei->aura != -1) {
          fprintf(fp, " (%d / %d Aura)", ei->aura, ei->auramax);
        }
        fprintf(fp, "</strong>");
        if (ei->beschr)
          fprintf(fp, " %s", ei->beschr);
        if (ei->privat)
          fprintf(fp, " (Bem: %s)", ei->privat);
        if (ei->silber != 0)
          fprintf(fp, " [$%d]", ei->silber);
        if (ei->weight != 0)
          fprintf(fp, " [%.2f GE]", 0.01 * ei->weight);
        if (ei->auramax != -1) {/* bei Magiern die aktiven Kampfzauber schreiben */
          fprintf(fp, "<BR>%s: ", translate("Kampfzauber", map->locale));
          if (ei->praezauber != NULL)
            fprintf(fp, "%s (%d) / ", ei->praezauber->name, ei->praezauber->level);
          else
            fprintf(fp, "%s / ", translate("keiner", map->locale));
          if (ei->normalzauber != NULL)
            fprintf(fp, "%s (%d) / ", ei->normalzauber->name, ei->normalzauber->level);
          else
            fprintf(fp, "%s / ", translate("keiner", map->locale));
          if (ei->postzauber != NULL)
            fprintf(fp, "%s (%d)", ei->postzauber->name, ei->postzauber->level);
          else
            fprintf(fp, "%s",  translate("keiner", map->locale));
        }
        for (effect = ei->first_effect; effect != NULL; effect = effect->next)
          fprintf(fp, "<BR><tt>%s</tt>", effect->zeile);

        fprintf(fp, "<BR>\n");
        d = 0;
        if (ei->bewacht != 0) {
          fprintf(fp, "%s %s", translate("bewacht", map->locale), translate("Region", map->locale));
          d = 1;
        }
        if (ei->parteitarnung || ei->andere_partei) {
          if (d) fprintf(fp, ", ");
          fprintf(fp, "%s", translate("parteigetarnt", map->locale));
          if (ei->andere_partei) {
            fprintf(fp, " (%s <a href=\"parteien.html#%d\">",
              translate("Partei", map->locale), ei->andere_partei);
            put36(fp, ei->andere_partei);
            fprintf(fp, "</a>)");
          }
          d = 1;
        }
        if (d)
          fprintf(fp, "<br>\n");

        write_lpmeldungen(ei->meldungen, fp);

        for (t = ei->first_talent; t != NULL; t = t->next) {
          if (map->noskillpoints == 0) {
            if (ei->anzahl > 1)
               fprintf(fp, "%d %s (T%d) %s (%d %s)",
              t->tage / ei->anzahl, translate("Tage", map->locale),
              t->stufe, t->name, t->tage,
              translate("Tage insgesamt", map->locale));
            else
               fprintf(fp, "%d %s (T%d) %s", t->tage, 
                       translate("Tage", map->locale), t->stufe, t->name);
          } else {
             fprintf(fp, "T %d %s", t->stufe, t->name);
          }
          if (!strcmp(t->name, translate("Unterhaltung", map->locale))
           || !strcmp(t->name, translate("Steuereintreiben", map->locale)))
                    fprintf(fp, " [%d]", t->stufe * ei->anzahl * 20);
          if (!strcmp(t->name, "Tarnung") && ei->tarnung >= 0)
                fprintf(fp, " [getarnt %d]", ei->tarnung);
          if (!strcmp(t->name, "Magie")) /* Lernkosten fuer naechste Stufe */
          {
            int talent;
             /* aktuelles Talent ohne Rassenbonus berechnen */
            for ( talent=0; ((talent+1)*talent*15) <= t->tage; talent++)
              ;

            fprintf(fp, " [%d$]", 50+25*(1+talent)*talent);
          }

          /* max. GE schreiben */
          if (!strcmp(t->name, "Reiten") && (ei->first_gegenstand != NULL))
          {
            int eins=-1, zwei=-1; /* GE fuer eine bzw. zwei Regionen bewegen */
            gegenstand_t *pferd = ei->first_gegenstand;
            gegenstand_t *wagen = ei->first_gegenstand;

            /* Pferde suchen */
            while ((strcmp(pferd->name, "Pferd") != 0) && (pferd != ei->last_gegenstand))
              pferd = pferd->next;
            /* Wagen suchen */
            while ((strcmp(wagen->name, "Wagen") != 0) && (wagen != ei->last_gegenstand))
              wagen = wagen->next;

            if (!strcmp(pferd->name, "Pferd"))
            {
              int menge = 0; /* Menge der Wagen */
              int rest  = 0;  /* Wagen, die nicht gezogen werden */
              int pferde = ei->anzahl * t->stufe * 2; /* Pferde, die geritten werden koennen */

              if (!strcmp(wagen->name, "Wagen"))
              {
                rest =  wagen->anzahl - pferd->anzahl/2;
                rest = (rest>0)?rest:0;
                menge = wagen->anzahl - rest;
              }
              zwei = eins = menge * 140 + (pferd->anzahl - 2*menge) * 20 - rest*40;
              if (strcmp(ei->typ, "Trolle") == 0)
              {
                zwei -= ei->anzahl * 20;
                eins += ei->anzahl * 10.8;
              }
              else
              {
                zwei -= ei->anzahl * 10;
                eins += ei->anzahl * 5.4;
              }

              if (pferd->anzahl > pferde) /* max. eine Region gehen */ 
              {
                zwei = -1;
                pferde = pferde*2+1; /* Pferde, die max. gefuehrt werden koennen */
                if (pferd->anzahl > pferde) /* ueberladen */
                  eins = -1;
              }
            }
            if (zwei >= 0)
              fprintf(fp, " [%d/%d GE]", zwei, eins);
            else if (eins >=0)
              fprintf(fp, " [%d GE]", eins);
          }

          fprintf(fp, "<br>\n");
        }
        for (g = ei->first_gegenstand; g != NULL; g = g->next) {
        fprintf(fp, "%d&nbsp;%s", g->anzahl, g->name);
        if (g->next != NULL)
            fprintf(fp, ", ");
        else
            fprintf(fp, "<br>\n");
        }
        
        if (ei->commands != NULL) {
            meldung_t *c;
            fprintf(fp, "<font face=\"courier, courier new\">\n");
            fprintf(fp, "<!--cmdstart ");
            put36(fp, ei->nummer);
            fprintf(fp, " -->\n");
            for (c = ei->commands; c != NULL; c = c->next) {
                fprintf(fp, "&nbsp;&nbsp;&nbsp;%s<br>\n", c->zeile);
            }
            fprintf(fp, "<!--cmdend-->\n");
            fprintf(fp, "</font>");
        }
        if ((ei->partei != map->partei) || (ei->verraeter == 1))
          fprintf(fp, "</font>");
    }
    }
}


static void write_schiffe(map_t *map, map_entry_t *e, FILE *fp, int partei,
              const char *prefix)
{
    schiff_t  *sch;
    einheit_t *ei;

    for (sch = e->first_schiff; sch != NULL; sch = sch->next) {
      if (sch->ausgegeben)
          continue;
      ei = ep(map, sch->kapitaen);
      if (partei == -1 ||
          (ei != NULL && ei->partei == partei)) {
          sch->ausgegeben = 1;
          fprintf(fp, "<strong>%s %s</strong> (",
              sch->typ, sch->name != NULL ? sch->name : "Namenlos");
              put36(fp, sch->nummer);
              fprintf(fp, ")");
          /* Kompatibilitaet! Schiff im Bau im alten Format */
          if ((sch->prozent != 0) && (sch->groesse == 0))
          {
            fprintf(fp, " %d%%", sch->prozent);
          }
          else if (sch->groesse != sch->max_groesse)
              fprintf(fp, " %s: %d/%d  (=%d%%)", translate("Fertigstellung", map->locale), sch->groesse, 
                 sch->max_groesse, sch->prozent);

          if (sch->kueste != -1)
          {
            if (map->locale == en)
              fprintf(fp, ", on the %s\n", kuestenname[sch->kueste + 1]);
            else
              fprintf(fp, ", an der %s\n", kuestenname[sch->kueste + 1]);              
          }
          if (sch->cargo != 0 || sch->capacity != 0) {
            fprintf(fp, ", (%.2f/%.2f)",
                    0.01 * sch->cargo, 0.01 * sch->capacity);
          }
          else if (sch->ladung != 0 || sch->max_ladung != 0)
            fprintf(fp, ", (%d/%d)", sch->ladung, sch->max_ladung);
          if (sch->schaden != 0)
            fprintf(fp, ", %d%% %s", sch->schaden, translate("Schaden", map->locale));
	  if (sch->anzahl > 0)
            fprintf(fp, ", %s: %d", translate("Anzahl", map->locale), sch->anzahl);
	  if (sch->speed > 0)
            fprintf(fp, ", %s: %d", translate("Reichweite", map->locale), sch->speed);
          fprintf(fp, "\n<br>\n");
          if (sch->beschr != NULL)
            fprintf(fp, "<tt>%s</tt><BR>", sch->beschr);
          if (sch->first_effect != NULL)
          {
            fprintf(fp, "<BR>");
            write_meldungen(sch->first_effect, fp);
          }
          fprintf(fp, "<blockquote>\n");
          write_einheiten_liste(map, e, fp, partei, sch->nummer, -1,
                    sch->kapitaen, prefix);
          write_einheiten_liste(map, e, fp, partei, sch->nummer, -1,
                    -1, prefix);
          fprintf(fp, "</blockquote>\n");
      }
    }
}

/*
 * Genaugenommen schreibt diese Funktion Gebäude, die Art steht in
 * b->typ.
 */
static void write_burgen(map_t *map, map_entry_t *e, FILE *fp, int partei,
             const char *prefix)
{
    burg_t    *b;
    einheit_t *ei;

    for (b = e->first_burg; b != NULL; b = b->next) {
      if (b->ausgegeben)
          continue;

      ei = ep(map, b->besitzer);
      if (partei == -1 ||
          (ei != NULL && ei->partei == partei)) {
          b->ausgegeben = 1;
          fprintf(fp, "<dl>\n");
          fprintf(fp, "<dt><strong>%s %s</strong> (",
              b->typ, b->name != NULL ? b->name : "Namenlos");
          put36(fp, b->nummer);
          fprintf(fp, "), ");
          fprintf(fp, "%s %d\n\n", 
                  translate("Gr&ouml;&szlig;e", map->locale), b->groesse);
          if (b->beschr != NULL)
            fprintf(fp, "<BR><tt>%s</tt>", b->beschr);
          
          if (b->first_effect != NULL)
          {
            fprintf(fp, "<BR>");
            write_meldungen(b->first_effect, fp);
          }
          if (b->meldungen != NULL)
          {
            fprintf(fp, "<BR>");
            write_lpmeldungen(b->meldungen, fp);
          }

          fprintf(fp, "<dd>\n");
          write_einheiten_liste(map, e, fp, partei, -1, b->nummer,
                    b->besitzer, prefix);
          write_einheiten_liste(map, e, fp, partei, -1, b->nummer, -1,
                    prefix);
          fprintf(fp, "</dl>\n");
      }
    }
}


void write_html_statistik(map_t *map, map_entry_t *e, FILE *efp)
{
    gegenstand_t *g_first = NULL, *g_last = NULL;
    gegenstand_t *g, *s;
    einheit_t    *ei;

    for (ei = e->first_einheit; ei != NULL; ei = ei->next) {
    if ((ei->partei == map->partei) && (ei->verraeter ==0)) {
        for (g = ei->first_gegenstand; g != NULL; g = g->next) {
        for (s = g_first;
             s != NULL && strcmp(s->name, g->name);
             s = s->next)
            ;
        /*
         * Wenn der Gegenstand gefunden ist, so wird nur addiert,
         * ansonsten wird ein neuer Gegenstand angelegt.
         */
        if (s != NULL ) {
            s->anzahl += g->anzahl;
        } else {
            s = xmalloc(sizeof(gegenstand_t));
            s->anzahl = g->anzahl;
            s->name   = g->name;
            s->next   = NULL;
            if (g_first == NULL) {
            g_first = s;
            g_last  = s;
            } else {
            g_last->next = s;
            g_last = s;
            }
        }
        }
    } /* if (ist eigene einheit) */
    } /* for einheit */

    for (g = g_first; g != NULL; g = s) {
    fprintf(efp, "%d&nbsp;%s", g->anzahl, g->name);
    s = g->next;
    if (s != NULL)
        fprintf(efp, ", ");
    else
        fprintf(efp, "\n");
    xfree(g);
    }
}


void write_html_reg_parteien(map_t *map, map_entry_t *e, FILE *fp)
{
    extern config_t config;
    struct plist {
      struct plist *next;
      partei_t     *p;
      short          bewacht;
      int          personen; /* Anzahl der Personen einer Partei */
    } *p_list, *next_p, *first_p = NULL, *last_p = NULL;
    einheit_t *ei = NULL;
    int verraeter = 0; /* Anbzahl der Varraeter in der Region */

    /*
     * Alle Einheiten durchlaufen, um alle Parteien in der Region zu
     * ermitteln.
     */
    for (ei = e->first_einheit; ei != NULL; ei = ei->next) {

      /* passende Partei zur aktuellen Einheit finden */
      for (p_list = first_p; p_list != NULL; p_list = p_list->next)
        if (p_list->p->nummer == ei->partei)
          break;

      if (p_list == NULL) {
        p_list = xmalloc(sizeof(struct plist));
        p_list->p = finde_partei(map, ei->partei);
        assert(p_list->p != NULL);
        p_list->next    = NULL;
        p_list->bewacht = 0;
        p_list->personen = 0;
        if (first_p == NULL) {
          first_p = p_list;
          last_p  = p_list;
        } else {
          last_p->next = p_list;
          last_p = p_list;
        }
      }
      p_list->bewacht |= ei->bewacht;
      p_list->personen += ei->anzahl;
      if (ei->verraeter == 1)
        verraeter += ei->anzahl;
    }

    for (p_list = first_p; p_list != NULL; p_list = next_p) {
      next_p = p_list->next;
      if (p_list->p->nummer != map->partei) {
        if (p_list->p->status == 0) /* keine Allianz */
              fprintf(fp, "<font color=#800000>");
        else if ((p_list->p->status & config.helfe_alles) ==
                 config.helfe_alles)
          fprintf(fp, "<font color=#008000>");
        else
          fprintf(fp, "<font color=#808000>");
        fprintf(fp, " (%s <a href=\"parteien.html#%d\">",
        translate("Partei", map->locale), p_list->p->nummer);
        put36(fp, p_list->p->nummer);
        fprintf(fp, "</a>) ");
        if (p_list->p->name != NULL && p_list->p->name[0])
          fprintf(fp, "%s", p_list->p->name);
        else
          fprintf(fp, "(%s)", translate("unbekannt", map->locale));

        fprintf(fp, " (%d) ", p_list->personen);

        if (p_list->bewacht)
          fprintf(fp, " (%s)", translate("bewacht", map->locale));
        fprintf(fp, "</font>");
        fprintf(fp, "<br>\n");
      }
      xfree(p_list);
    }
    
    if (verraeter !=0)
    {
      fprintf(fp, "<font color=#00D0D0>%s (%d)</font>", 
              translate("Verr&auml;ter", map->locale),  verraeter);
    }

    
}

static void write_html_einheiten(html_t *h, map_t *map, map_entry_t *e,
                 const char *prefix, FILE *fp)
{
    einheit_t *ei = NULL;
    schiff_t  *sch;
    burg_t    *b;
    int       silber, personen;

    for (ei = e->first_einheit; ei != NULL; ei = ei->next)
      ei->ausgegeben = 0;
    for (sch = e->first_schiff; sch != NULL; sch = sch->next)
      sch->ausgegeben = 0;
    for (b = e->first_burg; b != NULL; b = b->next)
      b->ausgegeben = 0;

    silber = 0;
    personen = 0;
    for (ei = e->first_einheit; ei != NULL; ei = ei->next) {
      if ((ei->partei == map->partei) && (ei->verraeter != 1)) {
          silber += ei->silber;
          personen += ei->anzahl;
      }
    }

  if (e->first_einheit != NULL) {
    fprintf(fp, "<table border=\"2\" cellpadding=\"2\" "
        "cellspacing=\"0\">\n");
    fprintf(fp, "<tr>\n");
    fprintf(fp, "<td valign=\"top\" width=\"50%%\"><h3>%s "
        "(%d %s, %d %s)</h3>", translate("Statistik", map->locale), 
             silber, translate("Silber", map->locale), personen, 
             translate("Pers.", map->locale));
    write_html_statistik(map, e, fp);
    fprintf(fp, "</td>\n");

    fprintf(fp, "<td valign=\"top\"width=\"50%%\">"
        "<h3>%s</h3>\n", translate("fremde Parteien", map->locale));
    write_html_reg_parteien(map, e, fp);
    fprintf(fp, "</td></tr></table>\n");
  }

    /*
     * Effekte, Botschaften und Ereignisse
     */
    write_text_list(h, translate("Regionszauber", h->locale),
                    3, e->first_effect, fp);
    write_text_list(h, translate("Botschaften", h->locale),
                    3, e->first_botschaft, fp);
    write_text_list(h, translate("Ereignisse", h->locale),
                    3, e->first_ereignis, fp);
    write_text_list(h, translate("Durchreise", h->locale),
                    3, e->first_durchreise, fp);
    write_text_list(h, translate("Durchschiffung", h->locale),
                    3, e->first_durchschiffung, fp);
    write_text_list(h, translate("Kommentare", h->locale),
                    3, e->first_kommentare, fp);
    write_text_list(h, translate("Sonstiges", h->locale),
                    3, e->first_text, fp);

    /*
     * Reihenfolge:
     *  - eigene Gebäude
     *  - fremde Gebäude
     *  - eigene Schiffe
     *  - fremde Schiffe
     *  - eigene Einheiten
     *  - fremde Einheiten
     */
    if (e->first_einheit == NULL) {
      write_burgen(map, e, fp, -1, prefix);
      write_schiffe(map, e, fp, -1, prefix);
    } else {
      write_burgen(map, e, fp, map->partei, prefix);
      write_burgen(map, e, fp, -1, prefix);
      write_schiffe(map, e, fp, map->partei, prefix);
      write_schiffe(map, e, fp, -1, prefix);
      write_einheiten_liste(map, e, fp, map->partei, -1, -1, -1, prefix);
      write_einheiten_liste(map, e, fp, -1, -1, -1, -1, prefix);
    }
}


/*
 * x und y sind Hex-Koordinaten.
 * tsx und tsy die Größe der Kartenausschnitte.
 */ 
static void write_html_region(html_t *h, hex_koor_t *r, const char *prefix)
{
    char        link_name[500], file_name[500];
    FILE        *efp;
    map_entry_t *e;
#if EINZELKARTEN
    char        *string, *file;
#endif

    e = mp(h->map, r->x, r->y, r->z);
    if (e == NULL)
    return;
    if (region_filter(e) == 0)
    return;

    sprintf(link_name, "r%c%d%c%de%d.html",
        (r->x >= 0) ? 'p' : 'm',
        (r->x >= 0) ? r->x : -r->x,
        (r->y >= 0) ? 'p' : 'm',
        (r->y >= 0) ? r->y : -r->y,
        r->z);
    sprintf(file_name, "%s%s", prefix, link_name);
    efp = xfopen(file_name, "w");
    /*
     * Html-header
     */
    fprintf(efp, "<html>\n<head>\n<title>%s %d %d</title>\n</head>\n",
            translate("Region", h->locale), r->x, r->y);
    fprintf(efp, "<body>\n");
    /*
     * Navigationstabelle und Regionsname
     */
    fprintf(efp, "<table><tr><td>\n");
    if (e->name != NULL)
    fprintf(efp, "<b><font size=\"5\">%s (%d, %d), %s</font></b>\n",
        e->name, r->x, r->y, region_typ_name[e->typ]);
    else
    fprintf(efp, "<b><font size=\"5\">(%d, %d), %s</font></b>\n",
        r->x, r->y, region_typ_name[e->typ]);

    fprintf(efp, "</td><td>\n");

#if EINZELKARTEN
    string = xmalloc(400);
    file = region_to_map(h, r->x, r->y, r->z);
    sprintf(string, "<a target=\"karte\" href=\"%s\">Karte</a>", file);
    xfree(file);
    write_nav_table(h, r->z, 1, &string, efp);
    xfree(string);
#else
    write_nav_table(h, r->z, 0, NULL, efp);
#endif
    fprintf(efp, "</td></tr></table>\n");
    /*
     * Regionsbeschreibung
     */
    if (e->beschr != NULL)
      fprintf(efp, "<p><tt>%s</tt></p>\n", e->beschr);
    /*
     * Regionsstatistik
     */
    write_html_umgebung(h, r, prefix, efp);
    /*
     * Einheiten in der Region
     */
    write_html_einheiten(h, h->map, e, prefix, efp);

    fprintf(efp, "</body>\n</html>\n");
    fclose(efp);
}


#if EINZELKARTEN
/*
 * Koordinaten sind visuelle Koordinaten, nicht Hex!
 * `m' ist NULL oder enthält Hexkoordinaten einer zu markierenden Region.
 */
static void write_html_map_tile(html_t *h, int xl, int xr, int yo, int yu, int z,
                const char *prefix)
{
    FILE   *fp;
    char   *file;
    wmap_t w;
    int    dx, dy;        /* Größe */

    dx = xr - xl + 1;
    dy = yu - yo + 1;

    file = number_to_filename(prefix, 'k', xl, yo, z, ".html");
    fp = xfopen(file, "w");
    xfree(file);

    fprintf(fp, "<html>\n<head>\n<title>Karte</title>\n</head>\n");
    fprintf(fp, "<body>\n");
    fprintf(fp, "<center>\n");
    fprintf(fp, "Ausschnitt von %d Grad %s Länge bis %d Grad %s Länge ",
        xl > 0 ? xl : -xl, xl < 0 ? "westlicher" : "östlicher",
        xr > 0 ? xr : -xr, xr < 0 ? "westlicher" : "östlicher");
    fprintf(fp, "und %d Grad %s Breite bis %d Grad %s Breite.<br>\n",
        yo > 0 ? yo : -yo, yo > 0 ? "südlicher" : "nördlicher",
        yu > 0 ? yu : -yu, yu > 0 ? "südlicher" : "nördlicher");
    fprintf(fp, "<table border=\"0\">\n");

    fprintf(fp, "<tr>\n");        /* Titelzeile */
    fprintf(fp, "<td>&nbsp;</td>\n");
    if (h->min_y < yo) {
    file = number_to_filename("", 'k', xl, yo - dy + h->y_lap, z, ".html");
    fprintf(fp, "<td align=\"center\">");
    fprintf(fp, "<a target=\"karte\" href=\"%s\">NORDEN</a>", file);
    fprintf(fp, "</td>\n");
    xfree(file);
    } else {
    fprintf(fp, "<td align=\"center\"></td>\n");
    }
    fprintf(fp, "<td>&nbsp;</td>\n");
    fprintf(fp, "</tr>\n");        /* Ende Titelzeile */
    
    fprintf(fp, "<tr>\n");    /* Anfang Hauptzeile */
    if (h->min_x < xl) {
    file = number_to_filename("", 'k', xl - dx + h->x_lap, yo, z,
                  ".html");
    fprintf(fp, "<td align=\"center\">");
    fprintf(fp, "<a target=\"karte\" href=\"%s\">W</a>", file);
    fprintf(fp, "</td>\n");
    xfree(file);
    } else {
    fprintf(fp, "<td align=\"center\">" "&nbsp;</td>\n");
    }
    fprintf(fp, "<td>\n");
    w.min_vx = xl;
    w.max_vx = xr;
    w.min_vy = yo;
    w.max_vy = yu;
    file = number_to_filename("", 'k', xl, yo, z, "");
    write_image_map(h, &w, z, prefix, file, fp, 1);
    xfree(file);
    fprintf(fp, "</td>\n");

    if (h->max_x > xr) {
    file = number_to_filename("", 'k', xl + dx - h->x_lap, yo, z,
                  ".html");
    fprintf(fp, "<td align=\"center\">");
    fprintf(fp, "<a target=\"karte\" href=\"%s\">O</a>", file);
    fprintf(fp, "</td>\n");
    xfree(file);
    } else {
    fprintf(fp, "<td align=\"center\">&nbsp;</td>\n");
    }
    fprintf(fp, "</tr>\n");    /* Ende Hauptzeile */

    fprintf(fp, "<tr>\n");        /* Nachspann */
    fprintf(fp, "<td>&nbsp;</td>\n");    /* untere linke Ecke */
    if (h->max_y > yu) {
    file = number_to_filename("", 'k', xl, yo + dy - h->y_lap, z, ".html");
    fprintf(fp, "<td align=\"center\">");
    fprintf(fp, "<a target=\"karte\" href=\"%s\">SÜDEN</a>", file);
    fprintf(fp, "</td>\n");
    xfree(file);
    } else {
    fprintf(fp, "<td align=\"center\">&nbsp;</td>\n");
    }
    fprintf(fp, "<td>&nbsp</td>\n");    /* untere rechte Ecke */
    fprintf(fp, "</tr>\n");        /* Ende Nachspann */
    
    fprintf(fp, "</table>\n");
    fprintf(fp, "</center>\n");
    fprintf(fp, "</body>\n");
    fprintf(fp, "</html>\n");
    fclose(fp);
}
#endif

static void write_unbekannt(const char *prefix)
{
    FILE *fp;
    char *file;

    file = xmalloc(strlen(prefix) + strlen("unbekannt.html") + 1);
    strcpy(file, prefix);
    strcat(file, "unbekannt.html");
    fp = xfopen(file, "w");
    xfree(file);

    fprintf(fp, "<html>\n<head>\n");
    fprintf(fp, "<title>Unbekannte Region</title>\n");
    fprintf(fp, "</head>\n");
    fprintf(fp, "<body>\n");
    fprintf(fp, "Sorry, keine Informationen für diese Region vorhanden.\n");
    fprintf(fp, "</body>\n");
    fprintf(fp, "</html>\n");

    fclose(fp);
}

void fill_html(map_t *map, int tsx, int tsy, int x_lap, int y_lap, ebene_t *eb, html_t *h)
{
    int min_vx, min_vy, max_vx, max_vy;
    int hx, hy;
    int vx, vy;

    h->map = map;
    h->tsx = tsx;
    h->tsy = tsy;
    h->x_lap = x_lap;
    h->y_lap = y_lap;

    min_vx = min_vy = INT_MAX;
    max_vx = max_vy = INT_MIN;
    for (hy = eb->min_y; hy <= eb->max_y; hy++) {
    for (hx = eb->min_x; hx <= eb->max_x; hx++) {
        if (mp(map, hx, hy, eb->koord) != NULL) {
        hex2vis(hx, hy, &vx, &vy);
        if (vx > max_vx) max_vx = vx;
        if (vx < min_vx) min_vx = vx;
        if (vy > max_vy) max_vy = vy;
        if (vy < min_vy) min_vy = vy;
        }
    }
    }
    h->min_x = min_vx;
    h->max_x = max_vx;
    h->min_y = min_vy;
    h->max_y = max_vy;
}

void write_html_map(map_t *map, const char *dir, int tsx, int tsy)
{
    char        *prefix=NULL; /* Pfad der Datei */
    char        welt[32];   /* Name der Weltkarte --> welt_<ebene>*/
    int         hx, hy, vx, vy;
    html_t      h;
    hex_koor_t  r;
    ebene_t     *eb;

    h.locale = map->locale;
    h.map = map;
    set_global_names(h.locale, map->game);

    prefix = xmalloc(strlen(dir) + 1 + 1);
    strcpy(prefix, dir);
    if (prefix[strlen(prefix) - 1] != '/') 
      strcat(prefix, "/");

    for (eb = map->ebenen; eb != NULL; eb=eb->next)
    {
      sprintf(welt, "welt_%d", eb->koord);
      fill_html(map, tsx, tsy, 6, 3, eb, &h);

      init_image();
      write_world_map(&h, prefix, welt, eb->koord, 1);
      hex2vis(0, 0, &vx, &vy);
  #if EINZELKARTEN
      for (vy = h.min_y; vy + h.y_lap <= h.max_y; vy += tsy - h.y_lap) {
      for (vx = h.min_x; vx + h.x_lap <= h.max_x; vx += tsx - h.x_lap) {
          write_html_map_tile(&h, vx, vx + tsx - 1, vy, vy + tsy - 1, 
                              eb->koord, prefix);
      }
      }
  #endif
      for (hy = eb->min_y; hy <= eb->max_y; hy++) {
      for (hx = eb->min_x; hx <= eb->max_x; hx++) {
          r.x = hx;
          r.y = hy;
          r.z = eb->koord;
          write_html_region(&h, &r, prefix);
      }
      }
      fini_image();
   }
   write_index(&h, prefix);
   write_unbekannt(prefix);
   write_parteien(&h, prefix);
   xfree(prefix);
}
