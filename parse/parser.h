/* $Id: parser.h,v 1.9 2002/02/14 20:24:40 marko Exp $ */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "../map.h"

#define WARNTAGS 1 /* Bei 0 werden Warnungen unterdrueckt */

#define REPORT_LINE_LEN 4000
#define REPORT_ARGS        5	/* 2 sollte laut Enno auch reichen... */

typedef struct report_struct {
    FILE *fp;
    char line[REPORT_LINE_LEN];
    char *argv[REPORT_ARGS];
    int  is_string[REPORT_ARGS];
    int  argc;
    int  lnr;
    int  eof;
    short is_block; /* 1 wenn aktuelle Zeile Blockanfang ist */
    short is_subblock; /* 1 wenn aktuelle Zeile Unterblockanfang ist */
} report_t;


/* parser.c */
void get_report_line(report_t *r);
void parsefehler(report_t *r, char *block);
void parse_optionen(report_t *r, partei_t *p);
void parse_unbekannt(report_t *r);

/* zauber.c */
void parse_kampfzauber(report_t *r, einheit_t *ei);
void parse_zauber_alt(report_t *r, partei_t *p);
void parse_zauber_neu(report_t *r, partei_t *p);
void parse_trank(report_t *r, partei_t *p);

/* meldungen.c */
char *get_value(meldung_t *m, char *tag);
void parse_meldungen(report_t *r, meldung_t **p);
void parse_message(report_t *r, meldung_t **p_m); /* neues Format */
void sortiere_meldungen(map_t *map);
void parse_battle(report_t *r, map_t *map, battle_t **p_m);
void parse_messagetypes(report_t *r);

/* einheiten.c */
void parse_einheit(report_t *r, map_t *map, int x, int y, int z);

/* partei.c */
void add_einheit_to_partei(map_t *map, einheit_t *ei);
partei_t *finde_partei(map_t *map, int nummer);
void parse_partei(report_t *r, map_t *map, int passwort);

/* region.c */
void parse_effects(report_t *r, meldung_t **mp);
void parse_region(report_t *r, map_t *map, int offset);
void flood_fill(map_t *map, map_entry_t *e);
