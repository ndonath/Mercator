/* $Id: parteien.c,v 1.9 2002/02/14 20:23:12 marko Exp $ */
#include "parser.h"
#include <assert.h>

/* Informationen ueber Parteien einlesen (Allianz, Einheiten usw.) */



/*
 * Sortiert einfügen oder neu anlegen.
 */
partei_t *finde_partei(map_t *map, int nummer)
{
    partei_t *p, **pp;

    if (nummer < -1)
    return NULL;

    pp = &map->first_partei;
    for (p = map->first_partei; p != NULL && p->nummer < nummer; p = p->next) {
    pp = &p->next;
    }
    if (p == NULL || p->nummer != nummer) {
      p = xmalloc(sizeof(partei_t));
      p->nummer = nummer;
      if (nummer == 0) {
            p->name = xstrdup("Monster");
      } else if (nummer == -1) {
            p->name = xstrdup("Parteigetarnte Einheiten");
      }
      p->next = *pp;
      *pp = p;
    }

    return p;
}


/* Namen einer Gruppe anhand von Partei und Gruppennummer bestimmen */
char *finde_gruppe(map_t *map, int pnr, int gnr)
{
  partei_t *partei = finde_partei(map, pnr);
  gruppe_t *g;
  if (partei != NULL);
    g=partei->first_gruppe;

  while ((g != NULL) && (g->nummer != gnr))
    g = g->next;

  if (g == NULL) {
#ifdef WARNTAGS
    printf("Gruppe %d nicht in Partei \"%s\" gefunden.\n", gnr, partei->name);
#endif
    return "unbekannt";
  } else {
    return g->name;
  }
  
}

/*
 * Alte Syntax:
 *   ALLIIERTE
 *   331;Partei
 *   "Gilde der Navigatoren";Parteiname
 *   27;Status
 *   ...
 */
static void parse_alliierte(report_t *r, map_t *map)
{
    partei_t *p = NULL;

    get_report_line(r);
    while (!r->eof) {
    if (r->argc == 2 && !strcmp(r->argv[1], "Partei")) {
        p = finde_partei(map, atoi(r->argv[0]));
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Parteiname")) {
        if (p != NULL) {
        if (p->name != NULL)
            xfree(p->name);
        p->name = xstrdup(r->argv[0]);
        }
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Status")) {
        if (p != NULL)
        p->status = atoi(r->argv[0]);
        get_report_line(r);
    } else {
        break;        /* end of "ALLIERTE" */
    }
    }
}

/*
 * Neue Syntax:
 *   ALLIANZ 4711
 *   "Gilde der Navigatoren";Parteiname
 *   27;Status
 *   ALLIANZ ...
 */
 /* der letzte Parameter "int set_status" gibt an, ob der Helfe Status bei der
    alliierten Partei gesetzt werden soll (ist bei ALLIANZ als Unterblock
    von GRUPPE z.B. nicht erwuenscht) */
static void parse_allianz(report_t *r, map_t *map, allianz_t **alliierte, int set_status)
{
  partei_t *p = NULL;
  allianz_t *a;

  p = finde_partei(map, atoi(r->argv[0] + 8));
  if (p == NULL) /* fuer Parteien mit Nummern < -1 */
  {
    parse_unbekannt(r);
    return;
  }

  a = xmalloc(sizeof(allianz_t));
  if (*alliierte != NULL)
    a->next = *alliierte;
  *alliierte = a;
  a->partei = p;

  get_report_line(r);

  while (!r->eof) {
    if (r->argc == 2 && !strcmp(r->argv[1], "Parteiname")) {
      if (p->name != NULL)
        xfree(p->name);
      p->name = xstrdup(r->argv[0]);
      get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Status")) {
      a->status = atoi(r->argv[0]);
      if (set_status == 1)
        p->status = a->status;
      get_report_line(r);
    } else {
      break;    /* end of "ALLIANZ %d" oder unbekannt */
    }
  }
}

/* liest den GRUPPE Block ein und weist ihn der Partei mit der angegebenen
   Parteinummer zu */
static void parse_gruppe(report_t *r, map_t *map, int pnr)
{
  partei_t *p = NULL;
  gruppe_t *g = xmalloc(sizeof(gruppe_t));
  sscanf(r->argv[0], "GRUPPE %d", &(g->nummer));
  p = finde_partei(map, pnr);
  g->next = p->first_gruppe;
  p->first_gruppe = g;
  
  get_report_line(r);

  while (!r->eof) {
    if (!strncmp(r->argv[0], "ALLIANZ ", 8)) {
      parse_allianz(r, map, &(g->alli), 0);
    } else if (r->is_subblock == 1) {
      parse_unbekannt(r);    /* unbekannter Subblock von GRUPPE */
    } else if (r->is_block == 1) {
      break;    /* Ende von "GRUPPE %d" oder unbekannt */
    } else if (!strcmp(r->argv[1], "name")) {
      g->name = xstrdup(r->argv[0]);
      get_report_line(r);
    } else if (!strcmp(r->argv[1], "typprefix")) {
      g->typprefix = xstrdup(r->argv[0]);
      get_report_line(r);
    } else {
#if WARNTAGS
      printf("[%d] Unbekannt in GRUPPE: %s;%s\n", r->lnr, r->argv[0], r->argv[1]);
#endif
      get_report_line(r);
    }
  }
}


void add_einheit_to_partei(map_t *map, einheit_t *ei)
{
    partei_t *p;

    assert(ei->partei >= -1);

    p = finde_partei(map, ei->partei);
    if (p != NULL) {
    if (ei->typ != NULL && p->typus == NULL && p->nummer > 0)
        p->typus = xstrdup(ei->typ);

    ei->next_partei = NULL;
    if (p->first_einheit == NULL) {
        p->first_einheit = ei;
        p->last_einheit  = ei;
        ei->prev_partei = NULL;
    } else {
        p->last_einheit->next_partei = ei;
        ei->prev_partei = p->last_einheit;
        p->last_einheit = ei;
    }
    }
}


/*
 * Eine Partei (eigene oder andere)
 */
void parse_partei(report_t *r, map_t *map, int passwort)
{
    int      i;
    unsigned int messagecount=0; /* Zaehler fuer die Meldungen */
    partei_t *p = NULL;        /* gerade gefundene Partei */

    p = finde_partei(map, atoi(r->argv[0] + 6));
    get_report_line(r);         /* überspringe PARTEI xxx */

    p->heroes = -1;
    p->max_heroes = -1;
    p->age = -1;

    while (!r->eof) {
#if 0
    if (r->argv[0] != NULL)
        printf("prolog, %d: argv[0] = \"%s\"", r->lnr, r->argv[0]);
    if (r->argc == 2 && r->argv[1] != NULL)
        printf(", argv[1] = \"%s\"\n", r->argv[1]);
    else
        printf("\n");
#endif
    if (r->argc == 2 && !strcmp(r->argv[1], "Passwort")) { /* Ab CR v54 ueberfluessig */
        get_report_line(r);
            passwort = 1;
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Optionen")) {
        p->optionen = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Punkte")) {
        p->punkte = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Punktedurchschnitt")) {
        p->punktedurchschnitt = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && (!strcmp(r->argv[1], "Typ") ||
                    !strcmp(r->argv[1], "Typus") ||
                    !strcmp(r->argv[1], "race"))) {
        if (p != NULL) {
        if (p->typus != NULL)
            xfree(p->typus);
        p->typus = xstrdup(r->argv[0]);
        }
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Magiegebiet")) {
        get_report_line(r);
    } else if (r->argc == 2 &&
           !strcmp(r->argv[1], "Rekrutierungskosten")) {
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Anzahl Personen")) {
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Anzahl Immigranten")) {
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Max. Immigranten")) {
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "locale")) {
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "age")) {
        p->age = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "typprefix")) {
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "heroes")) {
        p->heroes = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "max_heroes")) {
        p->max_heroes = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Parteiname")) {
        if (p != NULL) {
          if (p->name != NULL) {
            if (strcmp(p->name, r->argv[0])) {
              printf("Neuer Parteiname: %s\n", r->argv[0]);
              printf("Alter Parteiname: %s\n", p->name);
              xfree(p->name);
              p->name = xstrdup(r->argv[0]);
            }
          } else {
             p->name = xstrdup(r->argv[0]);
          }
        }
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "email")) {
        if (p != NULL) {
        if (p->email != NULL)
            xfree(p->email);
        p->email = xstrdup(r->argv[0]);
        }
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "banner")) {
        if (p != NULL) {
        if (p->banner != NULL)
            xfree(p->banner);
        p->banner = xstrdup(r->argv[0]);
        }
        get_report_line(r);
    } else if (!strcmp(r->argv[0], "OPTIONEN")) {
        parse_optionen(r, p);
    } else if (!strcmp(r->argv[0], "MELDUNGEN")) {
        parse_meldungen(r, &p->meldungen); /* fuer Kompatibilitaet mit alten CRs */
    } else if (!strcmp(r->argv[0], "EREIGNISSE")) {
        parse_meldungen(r, &p->fehler);    /* fuer Kompatibilitaet mit alten CRs */
    } else if (!strcmp(r->argv[0], "HANDEL")) {
        parse_meldungen(r, &p->handel);    /* fuer Kompatibilitaet mit alten CRs */
    } else if (!strcmp(r->argv[0], "ALLIIERTE")) {
        parse_alliierte(r, map);                /* fuer Kompatibilitaet mit alten CRs */
    } else if (!strncmp(r->argv[0], "ALLIANZ ", 8)) {
        parse_allianz(r, map, &(p->alli), 1);
    } else if (!strncmp(r->argv[0], "GRUPPE ", 7)) {
        parse_gruppe(r, map, p->nummer);
    } else if (!strcmp(r->argv[0], "EINKOMMEN")) {
        parse_meldungen(r, &p->einkommen); /* fuer Kompatibilitaet mit alten CRs */
    } else if (!strcmp(r->argv[0], "PRODUKTION")) {
        parse_meldungen(r, &p->produktion);/* fuer Kompatibilitaet mit alten CRs */
    } else if (!strcmp(r->argv[0], "BEWEGUNGEN")) {
        parse_meldungen(r, &p->bewegungen);/* fuer Kompatibilitaet mit alten CRs */
    } else if (!strcmp(r->argv[0], "TRAENKE")) {
        parse_meldungen(r, &p->traenke);   /* fuer Kompatibilitaet mit alten CRs */
    } else if (!strcmp(r->argv[0], "KAEMPFE")) {
        parse_meldungen(r, &p->kaempfe);
    } else if (!strcmp(r->argv[0], "ZAUBER")) {
        parse_zauber_alt(r, p);/* fuer Kompatibilitaet mit alten CRs */
    } else if (!strncmp(r->argv[0], "ZAUBER ", 7)) {
        partei_t *pp = finde_partei(map, map->partei);
        parse_zauber_neu(r, pp);
    } else if (!strncmp(r->argv[0], "TRANK", 5)) {
        partei_t *pp = finde_partei(map, map->partei);
        parse_trank(r, pp);
    } else if (!strncmp(r->argv[0], "MESSAGE ", 8)) {
        parse_message(r, &p->meldungen);
        messagecount++;
    } else if (!strncmp(r->argv[0], "BATTLE ", 7)) {
        parse_battle(r, map, &p->battle);
    } else  if (r->is_block) {
        break;
    } else {
        printf("report, Zeile %d, unbekannt in Prolog:\n", r->lnr);
        for (i = 0; i < r->argc; i++) {
          printf("[%d] = \"%s\"\n", i, r->argv[i]);
        }
        get_report_line(r);
    }

    }   

    if (passwort == 1) {
        if (map->partei <= 0)
        {
            partei_t *pp; /* bisherige eigene Partei finden */
            if (map->partei != -1)
            {
              pp = finde_partei(map, map->partei);
              map->partei = p->nummer;
              p->meldungen = pp->meldungen;
              pp->meldungen = NULL;
	        }
	        else
	        {
                  map->partei = p->nummer;
                  printf("Partei: %s\n", p->name);
	        }
        }
    }

    if (messagecount > 0) /* ein Feld mit allen Meldungen erstellen */
    {
      meldung_t **liste;
      meldung_t *help;
      unsigned int i;
      unsigned int counter = 0;
      
      if (p != NULL)
      {
        help =  p->meldungen;
        if (p->all_msg != NULL) /* Hier gibt's schon Meldungen */
        {
           for ( counter = 0; p->all_msg[counter] != NULL; counter++)
              ;

        }
        
        messagecount += counter;
        liste = xmalloc((messagecount+1)*sizeof(meldung_t*));

        for (i=0; i<counter; i++)
          liste[i] = p->all_msg[i];
        if (p->all_msg != NULL)
          xfree(p->all_msg);

        for (i= counter; i<messagecount; i++)
        {
          assert(help != NULL);
          liste[i] = help;
          help = help->next;
        }
        assert(help == NULL);
        liste[i] = NULL;
        p->all_msg = liste;
      }
    }


}
