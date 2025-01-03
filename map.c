/* $Id: map.c,v 1.24 2006/02/08 01:42:47 schwarze Exp $ */
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
#include "map.h"

#define X_OFF 1000
#define Y_OFF 1000

const char *region_typ_name[] = {
    "Unbekannt", "Nichts", "Ozean", "Festland",
    "Ebene", "Wald", "Sumpf", "Wüste", "Hochland",
    "Berge", "Vulkan", "Aktiver Vulkan", "Gletscher", "Eisberg",
    "Feuerwand", "Weltenrand", "Nebel", "Dichter Nebel", NULL
};

const char *sprache[] = {"de", "en"};

static int total_alloc = 0;
static int total_alloc_count = 0;

#if MALLOC_DEBUG
#  undef xmalloc
#  undef xstrdup
#  undef xfree

struct mlist {
    struct mlist *next;
    const char   *file;
    int          line;
    size_t       size;
} *ml;

void show_some_allocs(void);

void *xdmalloc(size_t size, char *f, int l)
{
    struct mlist *m;

    m = malloc(sizeof(struct mlist) + size);
    if (m == NULL) {
      fprintf(stderr, "Fehler in xdmalloc: Speicher voll bei %s:%d.\n", f, l);
      fprintf(stdout,
        "Druecke ENTER fuer Speicherliste, Ctrl-C fuer Abbruch.\n");
      (void)getchar();
      show_some_allocs();
      exit(1);
    }
    total_alloc += size;
    total_alloc_count++;
    m->size = size;
    m->line = l;
    m->file = f;
    m->next = ml;
    ml = m;
    memset(ml + 1, 0, size);

    return ml + 1;
}


void *xdstrdup(const char *str, char *f, int l)
{
    char *str2;
    size_t size = strlen(str) + 1;

    str2 = xdmalloc(size, f, l);
    strcpy((char*)str2, str);

    return str2;
}

void xdfree(void *ptr, char *f, int l)
{
    struct mlist **mlp, *m;

    m = ptr;
    m--;
    for (mlp = &ml; *mlp != NULL; mlp = &(*mlp)->next)
	if (*mlp == m)
	    break;
    if (*mlp != m) {
        fprintf(stderr,
          "Fehler in xdfree: Speicherblock ist gar nicht reserviert.\n");
        fflush(stderr);
	return;
    }
    total_alloc -= m->size;
    total_alloc_count--;
    memset(ptr, 0, m->size);

    *mlp = m->next;
    free(m);
}

void show_all_allocs(void)
{
    struct mlist *m;
    int ibl;
#if 0
    char *p;
    int  i;
#endif

    fprintf(stdout, "Info in show_all_allocs: Reservierte Speicherblocks:\n");
    ibl = 0;
    for (m = ml; m != NULL; m = m->next) {
	fprintf(stdout, "%i. %s:%d: size = %d\n",
            ibl, m->file, m->line, m->size);
#if 0
	p = (char*)m + sizeof(struct mlist);
	for (i = 0; i < m->size; i++)
	    putchar(p[i]);
	putchar('\n');
#endif
        ibl++;
    }
}

void show_some_allocs(void)
{
    struct mlist *m;
    int ibl;

    fprintf(stdout, "Info in show_some_allocs: 10 letzte Blocks:\n");
    ibl = 0;
    for (m = ml; m != NULL; m = m->next) {
        if (ibl < 10 || m->next == NULL) {
            fprintf(stdout, "%i. %s:%d: size = %d\n",
                ibl, m->file, m->line, m->size);
        } else if ( ibl == 10 ) {
            fprintf(stdout, "[...]\n");
        }
        ibl++;
    }
}

#  define xmalloc(s) xdmalloc(s, __FILE__, __LINE__)
#  define xstrdup(s) xdstrdup(s, __FILE__, __LINE__)
#  define xfree(p)   xdfree(p, __FILE__, __LINE__)
#else
void *xmalloc(size_t size)
{
    size_t *sp;

    sp = malloc(sizeof(size_t*) + size);
    assert(sp != NULL);
    total_alloc += size;
    total_alloc_count++;
    *sp = size;
    sp++;
    memset(sp, 0, size);

    return sp;
}


void *xstrdup(const char *str)
{
    size_t *sp;
    size_t size = strlen(str) + 1;

    sp = malloc(sizeof(size_t*) + size);
    assert(sp != NULL);
    total_alloc += size;
    total_alloc_count++;
    *sp = size;
    sp++;
    strcpy((char*)sp, str);

    return sp;
}

void xfree(void *ptr)
{
    size_t *sp;

    sp = ptr;
    sp--;
    total_alloc -= *sp;
    total_alloc_count--;
    memset(ptr, 0, *sp);
    free(sp);
}
#endif

void check_mem()
{
#ifndef MALLOC_DEBUG
    if (total_alloc != 0 || total_alloc_count != 0)
#endif
    fprintf(stderr, "Info in check_mem: %d Reservierungen, %d Worte\n",
      total_alloc_count, total_alloc);
#ifdef MALLOC_DEBUG
    show_all_allocs();
#endif
}

FILE *xfopen(const char *path, const char *mode)
{
    FILE *fp;

    fp = fopen(path, mode);
    if (fp == NULL) {
      fprintf(stderr, "Fehler in xfopen: Kann %s nicht zum %s öffnen.\n",
		path, *mode == 'r' ? "Lesen" : "Schreiben");
      exit(1);
    } else {
      return fp;
    }
}

FILE *config_open(const char *name)
{
    extern config_t config;
    FILE *fp;
    char *fname;
    unsigned flen = 0;

    if (config.path != NULL) flen = strlen(config.path) + 1;
    if (config.style != NULL) flen += strlen(config.style) + 1;
    flen += strlen(name) + 1;
    fname = xmalloc(flen);
    if (config.path != NULL) {
        strcpy(fname, config.path);
        if ( fname[strlen(fname)-1] != '/' ) strcat(fname, "/");
    }
    if (config.style != NULL) {
        strcat(fname, config.style);
        if ( fname[strlen(fname)-1] != '/' ) strcat(fname, "/");
    }
    strcat(fname, name);
    fp = xfopen(fname, "rb");
    xfree(fname);
    return fp;
}

int region_equiv(region_t r1, region_t r2)
{
    if ( r1 == r2 ) return 1;
    if ( r1 == T_UNBEKANNT ) return 1;
    if ( r1 == T_NICHTS &&
         ( r2 == T_OZEAN || r2 == T_FEUER || r2 == T_RAND ) ) return 1;
    if ( r1 == T_LAND && r2 > T_LAND && r2 < T_FEUER ) return 1;
    if ( r1 == T_EBENE  && r2 == T_WALD   ) return 1;
    if ( r1 == T_WALD   && r2 == T_EBENE  ) return 1;
    if ( r1 == T_VULKAN && r2 == T_AKTIV  ) return 1;
    if ( r1 == T_AKTIV  && r2 == T_VULKAN ) return 1;
    return 0;
}

map_t *make_map(int region_size, int row_mult, int einheit_size)
{
    map_t *map;
    int   i;

    map = xmalloc(sizeof(map_t));

    map->region_size = region_size;
    map->row_mult    = row_mult;
    map->tbl = xmalloc(sizeof(map_entry_t*) * region_size);
    for (i = 0; i < region_size; i++)
	map->tbl[i] = NULL;

    map->einheit_size = einheit_size;
    map->ei_hash = xmalloc(sizeof(einheit_t*) * einheit_size);
    for (i = 0; i < einheit_size; i++)
	map->ei_hash[i] = NULL;

    return map;
}

void destroy_tags(tags_t *z)
{
  int i;
  tags_t *next;

  while (z != NULL) {
    next = z->next;
    for (i=0; i<z->argc; i++)
      xfree(z->argv[i]);
    xfree(z->argv);
    xfree(z);
    z = next;
  }

}

void destroy_preis_liste(preis_t *p)
{
    preis_t *next;

    while (p) {
	next = p->next;
	if (p->produkt)
	    xfree(p->produkt);
	xfree(p);
	p = next;
    }
}

void destroy_grenze_liste(grenze_t *gr)
{
    grenze_t *next;

    while (gr) {
	next = gr->next;
	if (gr->typ)
	    xfree(gr->typ);
	xfree(gr);
	gr = next;
    }

}

void destroy_resource_liste(resource_t *rs)
{
    resource_t *next;

    while (rs != NULL) {
	next = rs->next;
	if (rs->type)
	    xfree(rs->type);
	xfree(rs);
	rs = next;
    }

}


void destroy_lpmeldung(lpmeldung_t **m)
{
  lpmeldung_t *last_msg;
  for (last_msg = *m; last_msg != NULL; last_msg = *m)
  {
    *m = last_msg->next;
    xfree(last_msg);
  }
}



void destroy_meldungs_liste(meldung_t *m)
{
    meldung_t *next;

    while (m) {
	next = m->next;
	if (m->zeile)
	    xfree(m->zeile);
    if (m->werte)
      destroy_tags(m->werte);
	xfree(m);
        m = next;
    }
}

void destroy_battle_liste(battle_t *m)
{
    battle_t *next;

    while (m) {
      next = m->next;
	  if (m->details)
	    destroy_meldungs_liste(m->details);
	  xfree(m);
      m = next;
    }
}
 
void destroy_talent_liste(talent_t *t)
{
    talent_t *next;

    while (t) {
	next = t->next;
	if (t->name);
	    xfree(t->name);
	xfree(t);
        t = next;
    }
}


void destroy_gegenstands_liste(gegenstand_t *g)
{
    gegenstand_t *next;

    while (g) {
	next = g->next;
	if (g->name)
	    xfree(g->name);
	xfree(g);
        g = next;
    }
}

void destroy_komponenten_liste(komponenten_t *g)
{
    komponenten_t *next;

    while (g) {
      next = g->next;
     if (g->name != NULL)
	    xfree(g->name);
      xfree(g);
      g = next;
    }
}

void destroy_zauber_liste(zauber_t *z)
{
    zauber_t *next;

    while (z) {
	next = z->next;
	if (z->spruch)
	    xfree(z->spruch);
	if (z->beschr)
	    xfree(z->beschr);
    destroy_komponenten_liste(z->komp);
	xfree(z);
	z = next;
    }
}

void destroy_trank_liste(trank_t *z)
{
    trank_t *next;

    while (z) {
	next = z->next;
	if (z->name != NULL)
	    xfree(z->name);
	if (z->beschr != NULL)
	    xfree(z->beschr);
    destroy_gegenstands_liste(z->komp);
	xfree(z);
	z = next;
    }
}


void destroy_einheiten_liste(einheit_t *ei)
{
    einheit_t *next;

    while (ei) {
      next = ei->next;
      if (ei->name) xfree(ei->name);
      if (ei->beschr) xfree(ei->beschr);
      if (ei->privat) xfree(ei->privat);
      if (ei->typ) xfree(ei->typ);
      if (ei->hp) xfree(ei->hp);
      if (ei->wahrer_typ) xfree(ei->wahrer_typ);
      if (ei->praezauber) {
        xfree(ei->praezauber->name);
        xfree(ei->praezauber);
      }
      if (ei->normalzauber) {
        xfree(ei->normalzauber->name);
        xfree(ei->normalzauber);
      }
      if (ei->postzauber) {
        xfree(ei->postzauber->name);
        xfree(ei->postzauber);
      }
      if (ei->typprefix)
        xfree(ei->typprefix);
      destroy_talent_liste(ei->first_talent);
      destroy_gegenstands_liste(ei->first_gegenstand);
      destroy_meldungs_liste(ei->commands);
      destroy_meldungs_liste(ei->sprueche);
      destroy_meldungs_liste(ei->botschaften);
      destroy_meldungs_liste(ei->first_effect);

    /* Liste mit den Zeiger auf die Meldungen wieder Loeschen */
    destroy_lpmeldung(&ei->meldungen);

	xfree(ei);
	ei = next;
    }
}

void destroy_burgen_liste(burg_t *b)
{
    burg_t *next;

    while (b) {
	next = b->next;
	if (b->name) xfree(b->name);
	if (b->typ) xfree(b->typ);
	if (b->beschr) xfree(b->beschr);
    /* Liste mit den Zeiger auf die Meldungen wieder Loeschen */
    destroy_lpmeldung(&b->meldungen);
    destroy_meldungs_liste(b->first_effect);
	xfree(b);
	b = next;
    }
}


void destroy_schiffs_liste(schiff_t *sch)
{
    schiff_t *next;
    
    while (sch) {
	next = sch->next;
	if (sch->name) xfree(sch->name);
	if (sch->typ) xfree(sch->typ);
	if (sch->beschr) xfree(sch->beschr);
        destroy_meldungs_liste(sch->first_effect);
	xfree(sch);
	sch = next;
    }
}

void destroy_allianz_liste(allianz_t *a)
{
    allianz_t *next;
    
    while (a) {
	  next = a->next;
	  xfree(a);
	  a = next;
    }
}

void destroy_gruppe_liste(gruppe_t *g)
{
    gruppe_t *next;
    
    while (g) {
      destroy_allianz_liste(g->alli);
	  next = g->next;
	  if (g->name) xfree(g->name);
	  if (g->typprefix) xfree(g->typprefix);
	  xfree(g);
	  g = next;
    }
}


/*
 * total = 1: Gesamte Liste löschen
 * total = 0: Nur first und last pointer löschen
 */
void destroy_parteien_liste(partei_t *p, int total)
{
    partei_t *next;
    
    while (p) {
      destroy_allianz_liste(p->alli);
      next = p->next;
      p->first_einheit = NULL;
      p->last_einheit  = NULL;
      if (total) {
        if (p->name) xfree(p->name);
        if (p->email) xfree(p->email);
        if (p->banner) xfree(p->banner);
        if (p->typus) xfree(p->typus);
        if (p->typprefix) xfree(p->typprefix);
        if (p->all_msg) xfree(p->all_msg);
        p->all_msg = NULL;
        destroy_gruppe_liste(p->first_gruppe);
        p->first_gruppe = NULL;
        destroy_battle_liste(p->battle);
        p->battle = NULL;
        destroy_meldungs_liste(p->kaempfe);
        p->kaempfe = NULL;
        destroy_meldungs_liste(p->meldungen);
        p->meldungen = NULL;
        destroy_meldungs_liste(p->fehler);
        p->fehler = NULL;
        destroy_meldungs_liste(p->handel);
        p->handel = NULL;
        destroy_meldungs_liste(p->einkommen);
        p->einkommen = NULL;
        destroy_meldungs_liste(p->produktion);
        p->produktion = NULL;
        destroy_meldungs_liste(p->bewegungen);
        p->bewegungen = NULL;
        destroy_meldungs_liste(p->traenke);
        p->traenke = NULL;
        destroy_meldungs_liste(p->lernen);
        p->lernen = NULL;
        destroy_meldungs_liste(p->magie);
        p->magie = NULL;
        xfree(p);
      }
      p = next;
    }
}

/* alle Regionsinformationen loeschen */
void destroy_map_entry(map_entry_t *e)
{
    map_entry_t *runde;

    for ( ; e != NULL; e = runde)
    {
        runde = e->prev_round;
        if (e->name != NULL)
          xfree(e->name);
        if (e->insel != NULL)
          xfree(e->insel);
        if (e->kraut != NULL)
          xfree(e->kraut);
        if (e->beschr != NULL)
          xfree(e->beschr);
        destroy_resource_liste(e->first_resource);
        e->first_resource = NULL;        
	    destroy_preis_liste(e->first_biete);
	    e->first_biete = e->last_biete = NULL;
	    destroy_preis_liste(e->first_kaufe);
	    e->first_kaufe = e->last_kaufe = NULL;
        xfree(e);
    }
}

/* alle variablen Regionsinformationen loeschen, fuer die keine Statistik gefuehrt werden soll */
/* Dazu gehoeren auch alle alten Regionsinformationen, */
void purge_map(map_t *map)
{
  map_entry_t *e;
  int         i;

  for (i = 0; i < map->region_size; i++) {
	for (e = map->tbl[i]; e != NULL; e = e->next) {
	    destroy_einheiten_liste(e->first_einheit);
	    e->first_einheit = e->last_einheit = NULL;
	    destroy_burgen_liste(e->first_burg);
	    e->first_burg = e->last_burg = NULL;
	    destroy_schiffs_liste(e->first_schiff);
	    e->first_schiff = e->last_schiff = NULL;
	    destroy_meldungs_liste(e->first_text);
	    e->first_text = NULL;
	    destroy_meldungs_liste(e->first_botschaft);
	    e->first_botschaft = NULL;
	    destroy_meldungs_liste(e->first_ereignis);
	    e->first_ereignis = NULL;
	    destroy_meldungs_liste(e->first_durchreise);
	    e->first_durchreise = NULL;
	    destroy_meldungs_liste(e->first_durchschiffung);
	    e->first_durchschiffung = NULL;
	    destroy_meldungs_liste(e->first_kommentare);
	    e->first_kommentare = NULL;
	    destroy_meldungs_liste(e->first_effect);
	    e->first_effect = NULL;
        destroy_grenze_liste(e->first_grenze);
        e->first_grenze = NULL;
        destroy_map_entry(e->prev_round);
        e->prev_round = NULL;
	}
  }
  for (i = 0; i < map->einheit_size; i++)
    map->ei_hash[i] = NULL;
  destroy_parteien_liste(map->first_partei, 1);
  map->first_partei = NULL;
  destroy_lpmeldung(&map->regionen_msg);
}

void destroy_map(map_t *map)
{
    map_entry_t *e, *next;
    ebene_t     *eb;
    int         i;

    purge_map(map);
    destroy_parteien_liste(map->first_partei, 1);
    map->first_partei = NULL;
    for (i = 0; i < map->region_size; i++) {
	  for (e = map->tbl[i]; e != NULL; e = next) {
	    next = e->next;
        destroy_map_entry(e);
      }
    }
    eb=map->ebenen;
    while (eb != NULL)
    {
      eb = eb->next;
      xfree(map->ebenen);
      map->ebenen = eb;
    }
    xfree(map->tbl);
    xfree(map->ei_hash);
    destroy_zauber_liste(map->zauber);
    destroy_trank_liste(map->trank);
    xfree(map);
}


map_entry_t *mp(map_t *map, int x, int y, int z)
{
    map_entry_t *e;
    int         hash;

    hash = abs((y + Y_OFF) * map->row_mult + x + X_OFF) % map->region_size;
    for (e = map->tbl[hash]; e != NULL; e = e->next) {
	if (e->x == x && e->y == y && e->z == z)
	    return e;
    }

    return NULL;
}


einheit_t *ep(map_t *map, int einheit)
{
    einheit_t *ei;
    int       hash;

    if (einheit < 0)
        return NULL;
    hash = einheit % map->einheit_size;
    for (ei = map->ei_hash[hash]; ei != NULL; ei = ei->next_hash) {
	if (ei->nummer == einheit)
	    return ei;
    }

    return NULL;
}


partei_t *pp(map_t *map, int partei)
{
    partei_t *p;

    for (p = map->first_partei; p != NULL; p = p->next)
	if (p->nummer == partei)
	    return p;

    return NULL;
}


map_entry_t *add_to_map(map_t *map, int x, int y, int z, int runde)
{
    map_entry_t *e = mp(map, x, y, z) ;
    ebene_t     *eb;
    int         hash;

    /* Region existiert noch nicht */
    if (e == NULL)
    {
      e = xmalloc(sizeof(map_entry_t));
      memset(e, 0, sizeof(map_entry_t));
      e->x = x;
      e->y = y;
      e->z = z;
      hash = abs((y + Y_OFF) * map->row_mult + x + X_OFF) % map->region_size;
      e->next = map->tbl[hash];
      map->tbl[hash] = e;

      /* ueberpruefen, ob diese Ebene schon existiert */
      for (eb = map->ebenen; eb != NULL && eb->koord != z; eb = eb->next)
         ;

      if (eb == NULL)
      {
        eb = xmalloc(sizeof(ebene_t));
        eb->koord = z;
        eb->max_x = eb->max_y = INT_MIN;
        eb->min_x = eb->min_y = INT_MAX;
        eb->next = map->ebenen;
        map->ebenen = eb;
      }

      if (x > eb->max_x) eb->max_x = x;
      if (y > eb->max_y) eb->max_y = y;
      if (x < eb->min_x) eb->min_x = x;
      if (y < eb->min_y) eb->min_y = y;

      return e;
    }
    else if (e->runde > 0 && e->runde < runde) /* Es gibt alte Informationen */
    {
      /* Damit die neue Region nicht umstaendlich in die Liste im Hash eingefuegt 
      werden muss, wird sie an die Adresse der alten Regionsinformation gepackt
      und die alten Infos werden stattdessen auf neu->prev_round neu allokiert 
      und umkopiert */
      map_entry_t *neu = e; /* das neue Element erhaelt die Adresse der alten Infos */
      e = xmalloc(sizeof(map_entry_t));
      *e = *neu; /* Die alten Regionsinformationen wieder zurueckkopieren */
      memset(neu, 0, sizeof(map_entry_t)); /* Den Inhalt des urspruenglichen Elementes loeschen */
      neu->x = x;
      neu->y = y;
      neu->z = z;
      neu->prev_round = e;
      neu->next = e->next; /* der next Zeiger gehoert zum Hash und muss deshalb noch uebernommen werden */
      e->next = NULL; /* die alten Infos befinden sich nicht mehr im Hash */
      return neu;
    }
    else
      return e;
}

void move_map(int *argc, char *argv[], map_t *map)
{
    map_entry_t *e, **new_tbl;
    ebene_t *eb;
    int i = *argc;
    int x, y;
    int r, hash;
    int tx, ty;

    i++;			/* skip "move" */
    if (argv[i] == NULL || argv[i+1] == NULL) {
	fprintf(stderr, "Fehler bei move: Parameter fehlen.\n");
	exit(1);
    }
    if (map == NULL) {
        *argc = i+2;    /* Die Koordinaten ueberspringen */
        return;			/* Leere Karte -> keine Arbeit */
    }
    
    tx = atoi(argv[i]);
    ty = atoi(argv[i+1]);
    i += 2;

    for (eb = map->ebenen; eb != NULL; eb = eb->next)
    {
      eb->min_x += tx;
      eb->min_y += ty;
      eb->max_x += tx;
      eb->max_y += ty;
    }

    new_tbl = xmalloc(sizeof(map_entry_t*) * map->region_size);
    for (r = 0; r < map->region_size; r++)
	new_tbl[r] = NULL;

    for (r = 0; r < map->region_size; r++) {
      while (map->tbl[r] != NULL) {
	    e = map->tbl[r];
	    map->tbl[r] = e->next;

	    x = e->x + tx;
	    y = e->y + ty;

	    e->x = x;
	    e->y = y;
	    hash = abs((y+Y_OFF) * map->row_mult + x + X_OFF) % map->region_size;
	    e->next = new_tbl[hash];
	    new_tbl[hash] = e;
      }
    }
    xfree(map->tbl);
    map->tbl = new_tbl;

    *argc = i;
}


void write_cr_map(map_t *map, const char *file)
{
    map_entry_t *e = NULL;
    partei_t    *p;
    ebene_t     *eb;
    FILE *fp;
    int x, y;

    fp = xfopen(file, "w");
    fprintf(fp, "VERSION 58\n"); /* Version des Computer Reports */
    fprintf(fp, "\"%s\";locale\n", sprache[map->locale]);
    fprintf(fp, "\"Eressea\";Spiel\n");
    fprintf(fp, "\"Mercator\";Konfiguration\n");
    fprintf(fp, "\"Hex\";Koordinaten\n");
    fprintf(fp, "36;Basis\n");
    fprintf(fp, "1;Umlaute\n");
    fprintf(fp, "%d;Runde\n", map->runde);
    fprintf(fp, "2;Zeitalter\n");
    
    /* eigene Partei zuerst schreiben */
    p = pp(map, map->partei);
	fprintf(fp, "PARTEI %d\n", p->nummer);
	fprintf(fp, "\"%s\";Parteiname\n",
		p->name != NULL ? p->name : "");
	fprintf(fp, "\"%s\";email\n",
		p->email != NULL ? p->email : "");
	fprintf(fp, "\"%s\";banner\n",
		p->banner != NULL ? p->banner : "");
    
    /* PARTEI Block schreiben */
    for (p = map->first_partei; p != NULL; p = p->next) {
      /* Monster, parteigetarnte und eigene Partei auslassen */
      if ((p->nummer <= 0) || (p->nummer == map->partei))
	      continue;

	  fprintf(fp, "PARTEI %d\n", p->nummer);
	  fprintf(fp, "\"%s\";Parteiname\n",
		  p->name != NULL ? p->name : "");
	  fprintf(fp, "\"%s\";email\n",
		  p->email != NULL ? p->email : "");
	  fprintf(fp, "\"%s\";banner\n",
		  p->banner != NULL ? p->banner : "");
    }

    /* Alle Regionen schreiben */
    for (eb = map->ebenen; eb != NULL; eb = eb->next)
    {
      for (y = eb->min_y; y <= eb->max_y; y++)
      {
        for (x = eb->min_x; x <= eb->max_x; x++)
        {
          e = mp(map, x, y, eb->koord);
          if (e == NULL) /* Es gibt keine Region mit diesen Koordinaten */
            continue;
          /* die Ebene 0 ist die "normale" Ebene ohne Z-Koordinate */
          if (eb->koord == 0)
            fprintf(fp, "REGION %d %d\n", x, y);
          else
            fprintf(fp, "REGION %d %d %d\n", x, y, eb->koord);          

	      if (e->name != NULL)
	        fprintf(fp, "\"%s\";Name\n", e->name);
          fprintf(fp, "\"%s\";Terrain\n", region_typ_name[e->typ]);
          if (e->beschr != NULL)
            fprintf(fp,"\"%s\";Beschr\n", e->beschr);
	      if (e->insel != NULL)
            fprintf(fp, "\"%s\";Insel\n", e->insel);

          if (e->bauern > 0)
            fprintf(fp, "%d;Bauern\n", e->bauern);
          if (e->pferde > 0)
            fprintf(fp, "%d;Pferde\n", e->pferde);
          if (e->baeume > 0)
            fprintf(fp, "%d;Baeume\n", e->baeume);
          if (e->mallorn > 0)
            fprintf(fp, "%d;Mallorn\n", e->mallorn);
          if (e->silber > 0)
            fprintf(fp, "%d;Silber\n", e->silber);
          if (e->unterhaltung > 0)
            fprintf(fp, "%d;Unterh\n", e->unterhaltung);
          if (e->rekrutierung > 0)
            fprintf(fp, "%d;Rekruten\n", e->rekrutierung);
          if (e->eisen > 0)
            fprintf(fp, "%d;Eisen\n", e->eisen);
          if (e->laen > 0)
            fprintf(fp, "%d;Laen\n", e->laen);
          if (e->lohn > 0)
            fprintf(fp, "%d;Lohn\n", e->lohn); 
          /* Runde ist kein Standard flag, sollte aber von allen Clients
             unterstuetzt bzw. klaglos ignoriert werden */
          if (e->runde > 0)
            fprintf(fp, "%d;Runde\n", e->runde);
          /* PREISE Block schreiben */
          if ((e->first_biete != NULL) || (e->first_kaufe != NULL)) {
            preis_t *ware;
            fprintf(fp, "PREISE\n");
            for (ware = e->first_biete; ware != NULL; ware = ware->next)
              fprintf(fp, "%d;%s\n", -1*ware->preis, ware->produkt);
            for (ware = e->first_kaufe; ware != NULL; ware = ware->next)
              fprintf(fp, "%d;%s\n", ware->preis, ware->produkt);
          }
          /* RESOURCE Block schreiben */
          if (e->first_resource != NULL) {
            resource_t *res;
            for (res = e->first_resource; res != NULL; res = res->next) {
              fprintf(fp, "RESOURCE %d\n", res->id);
              fprintf(fp, "\"%s\";type\n", res->type);
              fprintf(fp, "%d;number\n", res->number);
              if (res->skill != 0)
                fprintf(fp, "%d;skill\n", res->skill);
            }
          }
          if (e->first_grenze != NULL) {
            grenze_t *grenze;
            int i=0; /* Blockzaehler */
            for (grenze = e->first_grenze; grenze != NULL; grenze = grenze->next, i++) {
              fprintf(fp, "GRENZE %d\n", i);
              fprintf(fp, "\"%s\";typ\n", grenze->typ);
              fprintf(fp, "%d;richtung\n", grenze->richtung);
              if (grenze->prozent != 0)
                fprintf(fp, "%d;prozent\n", grenze->richtung);
            }
          }
          /* BURG Block schreiben (Gebaeude) */
          if (e->first_burg != NULL) {
            burg_t *burg;
            for (burg = e->first_burg; burg != NULL; burg = burg->next) {
              fprintf(fp, "BURG %d\n", burg->nummer);
              fprintf(fp, "\"%s\";Typ\n", burg->typ);
              if (burg->name != NULL)
                fprintf(fp, "\"%s\";Name\n", burg->name);
              if (burg->beschr != NULL)
                fprintf(fp, "\"%s\";Beschr\n", burg->beschr);
              fprintf(fp, "%d;Groesse\n", burg->groesse);
              if (burg->partei != 0)
                fprintf(fp, "%d;Partei\n", burg->partei);
            }
          }
        }/* Ende Ebene */
      }  /* Ende X Koordinate */
    }    /* Ende Y Koordinate */
    fclose(fp);
}
