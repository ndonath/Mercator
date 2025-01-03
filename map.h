/* $Id: map.h,v 1.29 2006/02/08 01:44:23 schwarze Exp $ */
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

#ifndef _MAP_H
#define _MAP_H

#define EINHEIT_HASH_SIZE 10001
#define REGION_HASH_SIZE  10001
#define ROW_MULT          1001

#include <stdio.h>

typedef struct config_struct {
    char     *path;    /* Pfad zum Mercator-Resourcen-Verzeichnis */
    char     *style;   /* Stil der Bilder: Verzeichnis unter *path */
    unsigned scale;    /* Verkleinerungsfaktor fuer die Bilder */
    unsigned helfe_alles;   /* helfe_e Code fuer HELFE ALLES */
    unsigned region_flags;  /* which regions should be included? */
    unsigned verbose;  /* 0=quiet, 1=errors, 2=info, 3=debug, 4=dump */
} config_t;

typedef enum {
  game_unknown = 0,
  game_eressea = 1,
  game_gav62mod = 2
} game_e;

typedef enum {
    helfe_silber = 1,		/* Mitversorgen von Einheiten */
    helfe_kaempfe = 2,		/* Bei Verteidigung mithelfen */
    helfe_wahrnehmung = 4,	/* Bei Wahrnehmung mithelfen (deaktiviert) */
    helfe_gib = 8,		/* Dinge annehmen ohne KONTAKTIERE */
    helfe_bewache = 16,	/* Laesst Steuern eintreiben etc. */
    helfe_partei = 32   /* laesst andere die eigene Parteitarnung durchschauen */
} helfe_e;

typedef enum {
  de = 0,  /* wenn keine locale gegeben ist, ist 0 der Standard */
  en = 1
} locale_e;

extern const char *sprache[]; /* definiert in map.c */

/* Regionstypen */
typedef enum {
    T_UNBEKANNT, T_NICHTS, T_OZEAN, T_LAND,
    T_EBENE, T_WALD, T_SUMPF, T_WUESTE, T_HOCHLAND,
    T_BERGE, T_VULKAN, T_AKTIV, T_GLETSCHER,
    T_FEUER, T_RAND, T_NEBEL, T_DICHT
} region_t;

#define NR_REGIONS 17

extern const char *region_typ_name[];

/* Struktur, um unbekannte/variable Eintraege im cr abspeichern zu koennen */
/* (keine Bloecke) */
typedef struct tag_struct {
  struct tag_struct *next;
  char   **argv;
  char   argc;
} tags_t;


typedef struct meldung_struct {
    struct meldung_struct *next;
    unsigned long type; /* Meldungstyp */
    char *zeile;
    tags_t *werte; /* sonstige Werte */
} meldung_t;

typedef struct talent_struct {
    struct talent_struct *next;
    int  tage;
    short  stufe;
    char *name;
} talent_t;

typedef struct gegenstand_struct {
    struct gegenstand_struct *next;
    int  anzahl;
    char *name;
} gegenstand_t;

typedef struct aktiver_kampfzauber {
  char* name; /* Name des Zaubers */
  short level;  /* Stufe des Zaubers */
} kampfzauber_t;

typedef struct komponenten_struct {
  struct  komponenten_struct *next;
  int anzahl;  /* benoetigte Menge */
  char *name;  /* Name der Komponente */
  short mult;  /* anzahl mit Stufe multiplizieren? (0/1) */
} komponenten_t; /* Komponenten fuer Zauber */

typedef struct zauber_struct {
    struct zauber_struct *next;
    char *spruch; /* Name des Spruches */
    int  stufe;   /* Stufe */
    char *beschr; /* Beschreibung */
    short art;      /* Art, 0=Praekampf 1=Kampf 2=Postkampf 3=normal */
    short rang;     /* Rang des Zaubers */
    short mod;    /* Bitarray fuer Modifikationen (0=keine, 1=Schiff, 2=Fern) */
    komponenten_t *komp; /* Komponenten (Aura & Co.) */
} zauber_t;

typedef struct trank_struct {
    struct trank_struct *next;
    char *name;   /* Name des Tranks */
    int  stufe;   /* Stufe */
    char *beschr; /* Beschreibung */
    gegenstand_t *komp; /* benoetigte Kraeuter */
} trank_t;


/* eine Liste mit Zeigern auf Meldungen, um eine Untermenge an Meldungen bilden
    zu koennen, ohne alle Tags kopieren zu muessen */
typedef struct meldungszeiger_struct {
   struct meldungszeiger_struct *next;
   meldung_t *pmeldung; /* Zeiger auf eine Meldung */
   int x, y, z; /* Koordinaten fuer die die Meldung betreffende Region */
} lpmeldung_t; /* _l_ist of _p_ointer of meldung_t */

typedef struct einheit_struct {
    struct einheit_struct *next; /* nächster in Region */
    struct einheit_struct *next_hash; /* nächster in Hash-Tabelle */
    struct einheit_struct *next_partei;	/* nächster in Partei */
    struct einheit_struct *prev_partei;	/* voriger in Partei */
    int  nummer;
    struct map_entry_struct *region;
    char         *name;
    char         *beschr;
    char         *privat;
    char         *typ;
    char         *wahrer_typ;
    char         *typprefix;
    int          temp;
    int          aura;
    int          auramax;
    short        hunger;
    short        ks;		/* Kampfstatus */
    char         *hp;		/* verwundet, etc. */
    short        tarnung;	/* aktuelle Tarnstufe */
    int          anzahl;
    int          partei;
    short        parteitarnung;
    int          andere_partei;   /* zu TARNE PARTEI nummer */
    short        bewacht;
    int          silber;
    int          ist_in;	/* Gebäude- oder Burg-Nummer */
    int          belagert;	/* Belagert Burg (oder 0) */
    int          folgt;		/* Folgt Einheit (oder 0) */
    int          alias;     /* alias-nummer (oder 0) */
    short        verraeter; /* diese Einheit ist ein Verraeter (0 oder 1) */
    int          gruppe;    /* Nummer der Gruppe - oder 0 */
    short        unaided;   /* bekommt keine Hilfe */
    short        hero;
    int          weight;    /* in Einheiten von Silberstuecken */
    talent_t     *first_talent, *last_talent;
    gegenstand_t *first_gegenstand, *last_gegenstand;
    meldung_t    *commands;
    meldung_t    *sprueche;
    meldung_t    *botschaften;
    lpmeldung_t  *meldungen;    /* liste mit Zeigern auf (globale) Meldungen, die diese Einheit betreffen */
    meldung_t    *first_effect; /* Effekte (Zauber), die auf diese Einheit wirken */
    kampfzauber_t *praezauber; /* Praekampfzauber */
    kampfzauber_t *normalzauber; /* normale Kampfzauber */
    kampfzauber_t *postzauber; /* Postkampfzauber */
    int          ausgegeben;   /* Flag für html-Ausgabe, ob schon abgearbeitet */
} einheit_t;

typedef struct allianz_struct {
  struct allianz_struct *next;
  struct partei_struct *partei; /* Zeiger auf Alliierte Partei */
  int status; /* helfe-status */
} allianz_t;

typedef struct gruppe_struct {
  struct gruppe_struct *next;
  char *name; /* Name der Gruppe */
  int   nummer; /* Nummer aus dem CR, z.B. GRUPPE 42 */
  char *typprefix;
  allianz_t *alli; /* Liste aller Alliierter */
} gruppe_t;

typedef struct preis_struct {
    struct preis_struct *next;
    char *produkt;
    int  preis;
} preis_t;

typedef struct schiff_struct {
    struct schiff_struct *next;		/* in Region */
    int       nummer;
    char      *name;
    char      *typ;
    char      *beschr;
    int       kapitaen;
    int       partei;
    int       prozent;   /* Fertigstellung in % = groesse/max_groesse */
    int       schaden;
    int       ladung;
    int       max_ladung;
    int       cargo;      /* in Einheiten von Silberstuecken */
    int       capacity;
    int       groesse;    /* aktuelle Schiffsgroesse (im Bau) */
    int       max_groesse;/* Groesse eines fertiggestellten Schiffes */ 
    int       kueste;		/* Küste, an der es liegt (n,s,o,w) */
    int       ausgegeben;
    meldung_t    *first_effect;
} schiff_t;

typedef struct burg_struct {
    struct burg_struct *next;	/* in Einheit */
    char      *name;
    char      *typ;
    char      *beschr;
    int       nummer;
    int       besitzer;		/* Einheit */
    int       partei;
    int       groesse;
    int       ausgegeben;
    meldung_t    *first_effect;
    lpmeldung_t  *meldungen;    /* liste mit Zeigern auf (globale) Meldungen, die diese Einheit betreffen */
} burg_t;

typedef struct grenze_struct {
    struct grenze_struct *next;
    char      *typ;         /* Typ - bisher nur "Straße" bekannt */
    char      richtung;     /* Richtung (NW=0 NO O SO S SW W=5) */
    char      prozent;      /* Fertigstellung in % */
} grenze_t;


typedef struct resource_struct {
  struct resource_struct *next;  
  char *type;  /* Name der Resource, z.B. Eisen */
  int number;  /* Menge der Resource */
  unsigned int id; /* ID fuer diesen Block - aendert sich ueber die Runden nicht */
  unsigned int skill; /* benoetigtes Talent */
} resource_t;


typedef struct map_entry_struct {
    struct map_entry_struct *next;
    struct map_entry_struct *prev_round; /* Diese Region in der vorherige Runde (fuer History) */
    int       x, y, z;      /* Koordinaten */
    char      *name;        /* Name (malloc) */
    char      *insel;       /* Inselname (malloc) */
    char      *kraut;       /* Kraut in dieser Region (malloc) */
    char      *beschr;      /* Regionsbeschreibung */
    region_t  typ;
    int       baeume;
    int       schoesslinge;
    int       bauern;
    int       silber;
    int       eisen;
    int       laen;
    int       pferde;
    int       unterhaltung;
    int       rekrutierung;
    int       luxusgueter;
    int       mallorn;
    int       lohn;    /* Eressea: Spielerlohn; Sitanleta: Bauernlohn */
    int       strasse; /* veraltet - muesste bei Gelegenheit mal rausgeschmissen werden */
    short     verorkt; /* 1=Region ist verorkt */
    int       runde;   /* Runde, aus der die Regionsinfos stammen */
    resource_t *first_resource;  /* vorhandene Rohstoffe */
    preis_t   *first_biete, *last_biete;
    preis_t   *first_kaufe, *last_kaufe;
    einheit_t *first_einheit, *last_einheit;
    burg_t    *first_burg, *last_burg;
    schiff_t  *first_schiff, *last_schiff;
    meldung_t *first_text;
    meldung_t *first_botschaft;
    meldung_t *first_ereignis;
    meldung_t *first_durchreise;
    meldung_t *first_durchschiffung;
    meldung_t *first_kommentare;
    meldung_t *first_effect;
    grenze_t  *first_grenze;
} map_entry_t;


typedef struct battle_struct {
	struct battle_struct *next;
	map_entry_t *region; /* Zeiger auf Region */
	meldung_t   *details; /* der Kampfreport */
} battle_t;

typedef struct partei_struct {
    struct    partei_struct *next;
    int       nummer;
    char      *name;
    char      *email;
    char      *banner;
    char      *typus;
    char      *typprefix;
    int       status;		/* helfe-status */
    int       optionen;         /* zip etc. */
    int       punkte;
    int       punktedurchschnitt;
    int       schatz;
    int       heroes;
    int       max_heroes;
    int       age;
    gruppe_t  *first_gruppe; /* Gruppen innerhalb dieser Partei */
    meldung_t **all_msg;      /* Ein Feld mit Zeigern auf alle Meldungen in Reportsortierung */
    battle_t  *battle;
    meldung_t *kaempfe;    /* veraltet, Kaempfe haben jetzt einen eigenen Block (BATTLE) */
    meldung_t *meldungen;
    meldung_t *fehler;     /* Fehlermeldungen */
    meldung_t *lernen;     /* Lernen und Lehren */
    meldung_t *handel;     /* Handel */
    meldung_t *einkommen;  /* Einnahmen aus Unterhaltung/Steuern */
    meldung_t *produktion; /* Rohstoffe und Produktion */
    meldung_t *bewegungen; /* Reise und Bewegungen */
    meldung_t *traenke;
    meldung_t *magie; /* Meldungen zu Magie und Artefakten */
    einheit_t *first_einheit;
    einheit_t *last_einheit;
    allianz_t *alli;
    zauber_t  *zauber;
    trank_t   *trank;
} partei_t;

typedef struct ebenen_struct {
  struct ebenen_struct *next;
  int koord;     /* im Report vorhandene Z-Koordinate */
  int min_x, max_x; /* maximale bzw. minimale Koordinaten der Ebene */
  int min_y, max_y;
} ebene_t;

typedef struct map_struct {
    map_entry_t **tbl;		/* Hashtabelle für Regionen */
    einheit_t   **ei_hash;	/* Hashtabelle für Einheiten */
    ebene_t   *ebenen;      /* alle vorkommenden Z-Koordinaten*/
    int       region_size, row_mult;
    int       einheit_size;
    int       partei;		/* Nummer der Partei */
    int       runde;            /* aktuelle Runde */
    int       version;          /* Version des CR */
    game_e    game;             /* Atlantis-Spielvariante */
    partei_t  *first_partei;	/* Parteien und Adressen */
    zauber_t  *zauber;          /* Liste der Zauber */
    trank_t   *trank;           /* Liste der neuen Traenke */
    locale_e  locale;           /* Sprache des Reports */
    lpmeldung_t *regionen_msg;   /* Liste mit Zeigern auf alle Regionsmeldungen */
    int       noskillpoints;    /* Talentpunkte im CR? */
} map_t;

/* map.c */ 
#if MALLOC_DEBUG
#  define xmalloc(s) xdmalloc(s, __FILE__, __LINE__)
#  define xstrdup(s) xdstrdup(s, __FILE__, __LINE__)
#  define xfree(p)   xdfree(p, __FILE__, __LINE__)
void *xdmalloc(size_t size, char *f, int l);
void *xdstrdup(const char *str, char *f, int l);
void xdfree(void *ptr, char *f, int l);
#else
void *xmalloc(size_t size);
void *xstrdup(const char *str);
void xfree(void *ptr);
#endif

/* map.c */
FILE        *xfopen(const char *path, const char *mode);
FILE        *config_open(const char *name);
map_t       *make_map(int region_size, int row_mult, int einheit_size);
void        destroy_map(map_t *map);
map_entry_t *mp(map_t *map, int x, int y, int z);
einheit_t   *ep(map_t *map, int einheit);
partei_t    *pp(map_t *map, int partei);
map_entry_t *add_to_map(map_t *map, int x, int y, int z, int runde);
void purge_map(map_t *map);
void move_map(int *argc, char *argv[], map_t *map);
void check_mem(); /* alloc-test */
int region_equiv(region_t r1, region_t r2);

/* parser/parser.c */
void parse_report(char *filename, map_t *map);

#endif
