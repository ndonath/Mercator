/* $Id: einheiten.c,v 1.11 2002/03/06 21:32:11 marko Exp $ */
#include "parser.h"
#include <assert.h>


/* Diese Funktion ordnet einer Einheit alle Meldungen zu, die sich auf eben
   jene beziehen. Es werden nur Zeiger auf die Meldungen angelegt und nichts
   allockiert */
static void einheit_meldung_zuordnen(einheit_t *e, partei_t *p)
{
  int i;
  char *value; /* Wert zum zugehoerigen Schluesselwort (tag) */
  int unit, target, mage;

  if (p->all_msg == NULL)
    return;

  for (i=0; p->all_msg[i] != NULL; i++)
  {
    unit = mage = target = -1;
    /* Wert des Schluessels "unit" bestimmen */
    value = get_value(p->all_msg[i], "unit");
    if (value != NULL)
      unit = atoi(value);
    value = get_value(p->all_msg[i], "target");
    if (value != NULL)
      target = atoi(value);
    value = get_value(p->all_msg[i], "mage");
    if (value != NULL)
      mage = atoi(value);

    if ((e->nummer == unit) || (e->nummer == target) || (e->nummer == mage))
    {
       lpmeldung_t *lpm = xmalloc(sizeof(lpmeldung_t));
       lpm->pmeldung = p->all_msg[i];
       lpm->next = e->meldungen;
       e->meldungen = lpm;
    }
  }
}

static void parse_talente(report_t *r, einheit_t *ei)
{
    talent_t *t;

    get_report_line(r);        /* skip "TALENTE" */
    while (!r->eof && atoi(r->argv[0]) != 0) {
    t = xmalloc(sizeof(talent_t));
    sscanf(r->argv[0], "%d %hd", &t->tage, &t->stufe);
    t->name = xstrdup(r->argv[1]);
    if (ei->first_talent == NULL) {
        ei->first_talent = t;
        ei->last_talent  = t;
    } else {
        ei->last_talent->next = t;
        ei->last_talent = t;
    }
    t->next = NULL;

    get_report_line(r);
    }
}


static void parse_gegenstaende(report_t *r, einheit_t *ei)
{
    gegenstand_t *g;

    get_report_line(r);        /* skip "GEGENSTANDE" */
    while (!r->eof && atoi(r->argv[0]) != 0) {

    /* Regionssilber zaehlen */
    if (strcmp(r->argv[1], "Silber") == 0) {
      ei->silber = atoi(r->argv[0]);
      get_report_line(r);
      continue;
    }
        
    g = xmalloc(sizeof(gegenstand_t));
    sscanf(r->argv[0], "%d", &g->anzahl);
    g->name = xstrdup(r->argv[1]);
    if (ei->first_gegenstand == NULL) {
        ei->first_gegenstand = g;
        ei->last_gegenstand  = g;
    } else {
        ei->last_gegenstand->next = g;
        ei->last_gegenstand = g;
    }
    g->next = NULL;

    get_report_line(r);
  }
}


static void parse_einheitsbotschaften(report_t *r, einheit_t *ei)
{
    meldung_t *m, *last = NULL;

    assert(ei->botschaften == NULL);

    get_report_line(r);        /* skip "EINHEITSBOTSCHAFTEN" */
    while (!r->eof && r->is_string[0]) {
    m = xmalloc(sizeof(meldung_t));
    m->next = NULL;
    if (ei->botschaften == NULL) {
        ei->botschaften = m;
    } else {
        last->next = m;
    }
    last = m;
    m->zeile = xstrdup(r->argv[0]);
    get_report_line(r);
    }
}


void parse_einheit(report_t *r, map_t *map, int x, int y, int z)
{
    map_entry_t *e;
    einheit_t   *ei;
    int         i;
    int         hash;
    partei_t    *p;

    e = mp(map, x, y, z);
    assert(e != NULL);
    ei = xmalloc(sizeof(einheit_t));
    ei->nummer = atoi(r->argv[0] + 8);
    ei->anzahl = 1;        /* default */
    ei->region = e;        /* Position der Einheit */
    ei->partei = -1;        /* -1 = unbekannt, 0 = Monster */
    ei->tarnung = -1;        /* -1 = unbekannt, sonst Level */
    ei->aura = -1;
    ei->auramax = -1;
    ei->hunger = -1;
    ei->temp = -1;              /* war keine Temp-Einheit */
    ei->hero = 0;
    /*
     * Füge Einheit in die Hashtabelle aller Einheiten in der Karte ein,
     * Schlüssel ist die Einheitennummer.
     */
    hash = ei->nummer % map->einheit_size;
    ei->next_hash = map->ei_hash[hash];
    map->ei_hash[hash] = ei;

    get_report_line(r);        /* skip "EINHEIT %d" */
    while (!r->eof) {
    if (!strncmp(r->argv[0], "EINHEIT ", 8) ||
        !strncmp(r->argv[0], "REGION ", 7) ||
        !strncmp(r->argv[0], "DURCHREISEREGION ", 16) ||
        !strncmp(r->argv[0], "GRENZE ", 7)) 
        /* normalerweise steht GRENZE aber nicht nach EINHEIT... */
        break;
    if (r->argc == 2 && !strcmp(r->argv[1], "Name")) {
        ei->name = xstrdup(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Beschr")) {
        ei->beschr = xstrdup(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Typ")) {
        ei->typ = xstrdup(r->argv[0]);
        if (ei->partei != 0) {
        p = pp(map, ei->partei);
        if (p != NULL && p->typus == NULL && p->nummer > 0)
            p->typus = xstrdup(r->argv[0]);
        }
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "wahrerTyp")) {
        if (r->argv[0][0] != 0)
            ei->wahrer_typ = xstrdup(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "typprefix")) {
        ei->typprefix = xstrdup(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "privat")) {
        ei->privat = xstrdup(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "temp")) {
            ei->temp = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Anzahl")) {
        ei->anzahl = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Partei")) {
        ei->partei = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Parteiname")) {
        /*@@@ könnte man auswerten... */
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Parteitarnung")) {
        ei->parteitarnung = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Anderepartei")) {
        ei->andere_partei = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "bewacht")) {
        ei->bewacht = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "belagert")) {
        ei->belagert = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "folgt")) {
        ei->folgt = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "alias")) {
        ei->alias = atoi(r->argv[0]);
        get_report_line(r);
    /* veraltet - Das Silber steht jetzt in der Gegenstandsliste */
    /* aber der Kompatibilitaet zuliebe bleibt es vorerst */
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Silber")) {
        ei->silber = atoi(r->argv[0]);
        get_report_line(r); 
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Burg")) {
        ei->ist_in = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Kampfstatus")) {
        ei->ks = atoi(r->argv[0]);
        if (map->version < 57) /* vor V57 gab's nur 4 Stati */
        {
          switch( ei->ks )
          {
            case 1:
              (ei->ks)++;
              break;
            case 2:
            case 3:
              ei->ks += 2;
              break;
            default:
              break;
          }
        }
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "weight")) {
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "hero")) {
        ei->hero = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Tarnung")) {
        ei->tarnung = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Schiff")) {
        ei->ist_in = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Default")) {
        printf("default ignoriert\n");
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "hp")) {
        ei->hp = xstrdup(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "hunger")) {
        ei->hunger = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Aura")) {
        ei->aura = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Auramax")) {
        ei->auramax = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Verraeter")) {
        ei->verraeter = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "gruppe")) {
        ei->gruppe = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "unaided")) {
        /* hier muesste auch ein "ei->unaided = 1;" reichen */
        ei->unaided = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "familiarmage")) {
        get_report_line(r);
    } else if (!strcmp(r->argv[0], "COMMANDS")) {
        parse_meldungen(r, &ei->commands);
    } else if (!strcmp(r->argv[0], "TALENTE")) {
        parse_talente(r, ei);
    } else if (!strcmp(r->argv[0], "SPRUECHE")) {
        parse_meldungen(r, &ei->sprueche);
    } else if (!strcmp(r->argv[0], "GEGENSTAENDE")) {
        parse_gegenstaende(r, ei);
	} else if (!strcmp(r->argv[0], "EFFECTS")) {
	    parse_effects(r, &ei->first_effect);
    } else if (!strcmp(r->argv[0], "EINHEITSBOTSCHAFTEN")) {
        parse_einheitsbotschaften(r, ei);
    } else if (!strncmp(r->argv[0], "KAMPFZAUBER ", 12)) {
        parse_kampfzauber(r, ei);
    } else if (r->is_block == 1) {
        break;
    } else if (r->is_subblock == 1) {
        parse_unbekannt(r);
    } else {
#if WARNTAGS    
        printf("Zeile %d, unbekannter Inhalt:\n", r->lnr);
        for (i = 0; i < r->argc; i++) {
          printf("[%d] = \"%s\"\n", i, r->argv[i]);
        }
#endif
        get_report_line(r);
    }
    }

    if (e->first_einheit == NULL) {
      e->first_einheit = ei;
      e->last_einheit  = ei;
      ei->next = NULL;
    } else {
/* Reportsortierung */
/*     e->last_einheit->next = ei;
      e->last_einheit = ei;
*/
      einheit_t *temp, *merker=e->first_einheit;
    /* nach Parteien sortiert einfuegen */
      for (temp=e->first_einheit; 
           ((temp != NULL) && (ei->partei != temp->partei));
            temp = temp->next)
      {
        merker = temp;
      }
      
      /* temp zeigt jetzt auf die erste Einheit,
         die zur gleichen Partei gehoert */
      
      for ( ;
           ((temp != NULL) && (ei->partei == temp->partei));
            temp = temp->next)
      {
        merker = temp;
      }
      
      /* nun zeigt merker auf die letzte Einheit in der gleichen Partei */

      if (temp == e->first_einheit)
      {
        ei->next = temp->next;
        temp->next = ei;
      }
      else
      {
        merker->next = ei;
        ei->next = temp;
        if (temp == NULL)
          e->last_einheit = ei;
       }
    }

    einheit_meldung_zuordnen(ei, finde_partei(map, map->partei));

    add_einheit_to_partei(map, ei);
}
