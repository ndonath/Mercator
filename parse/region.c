/* $Id: region.c,v 1.17 2002/06/23 18:30:41 marko Exp $ */
#include "parser.h"

void set_region_typ(map_t *map, int x, int y, int z, char *typ)
{
    map_entry_t *e;
    int         i;

    e = mp(map, x, y, z);
    if (e == NULL)
    e = add_to_map(map, x, y, z, map->runde);
    if (!strcmp("Wueste", typ)) {
        i = T_WUESTE;
    } else {
    for (i = 0; region_typ_name[i] != NULL; i++)
        if (!strcmp(region_typ_name[i], typ))
        break;
    if (region_typ_name[i] == NULL) {
        printf("unbekannter Regionstyp: %s\n", typ);
        return;
    }
    }
    /* Jetzt enthält i den neuen Regionstypindex */

    if (e->typ != i && e->typ != T_UNBEKANNT) {
        if ( region_equiv(e->typ, i) ) {
            /*
             * Zulaessige Aenderung des Regionstyps
             */
            if (e->name != NULL)
                fprintf(stderr, "%s (%d, %d) ist jetzt ", e->name, x, y);
            else
                fprintf(stderr, "(%d, %d) ist jetzt ", x, y);
            fprintf(stderr, "%s.\n", region_typ_name[i]);
        } else {
            /*
             * Unzulaessige Aenderung des Regionstyps
             */
            if (e->name != NULL)
                fprintf(stderr, "%s (%d, %d) inkonsistent: %s -> %s\n",
                    e->name, x, y, region_typ_name[e->typ],
                    region_typ_name[i]);
            else
                fprintf(stderr, "(%d, %d) inkonsistent: %s -> %s\n",
                    x, y, region_typ_name[e->typ],
                    region_typ_name[i]);
        }
    }
    e->typ = i;
}


static void set_region_name(map_t *map, int x, int y, int z, const char *name)
{
    map_entry_t *e;
    char        *n, *p;

    n = xstrdup(name);
    p = strchr(n, '(');
    if (p != NULL && p > n)
    *(p - 1) = 0;

    e = mp(map, x, y, z);
    if (e == NULL)
    e = add_to_map(map, x, y, z, map->runde);

    if (e->name != NULL) {
    if (strcmp(e->name, n)) {
        fprintf(stderr, "Neuer Regionsname (%d, %d):\n", x, y);
        fprintf(stderr, "Alt: %s\n", e->name);
        fprintf(stderr, "Neu: %s\n", n);
        xfree(e->name);
        e->name = xstrdup(n);
    }
    } else {
    if (strlen(name) > 0) {
        e->name = xstrdup(n);
    }
    }
    xfree(n);
}

void parse_effects(report_t *r, meldung_t **mp)
{
    meldung_t *m;

    get_report_line(r);    /* skip "EFFECTS" */

    while (!r->eof && (r->is_block == 0)) {
    m = xmalloc(sizeof(meldung_t));
        *mp = m;
        mp = &m->next;
    m->next = NULL;
        m->zeile = xstrdup(r->argv[0]);
        get_report_line(r);
    }
}

/*
 * Preis < 0: Man kann kaufen, Preis > 0: Man kann verkaufen
 */
static void parse_preise(report_t *r, map_entry_t *e)
{
    preis_t *p;
    int     preis;

    get_report_line(r);
    while (!r->eof && (r->is_block == 0)) {
      p = xmalloc(sizeof(preis_t));
      preis = atoi(r->argv[0]);
      p->produkt = xstrdup(r->argv[1]);
      p->next = NULL;
      if (preis < 0) {    /* Bauern kaufen */
          p->preis = -preis;
          if (e->first_biete == NULL) {
          e->first_biete = p;
          e->last_biete  = p;
          } else {
          e->last_biete->next = p;
          e->last_biete = p;
          }
      } else {        /* Bauern bieten */
          p->preis = preis;
          if (e->first_kaufe == NULL) {
          e->first_kaufe = p;
          e->last_kaufe  = p;
          } else {
          e->last_kaufe->next = p;
          e->last_kaufe = p;
          }
      }
      get_report_line(r);
    }
}



static void parse_schiff(report_t *r, map_entry_t *e)
{
    schiff_t *sch;

    sch = xmalloc(sizeof(schiff_t));

    sch->next = NULL;
    sch->kueste = -1;
    if (e->first_schiff == NULL) {
      e->first_schiff = sch;
      e->last_schiff = sch;
    } else {
      e->last_schiff->next = sch;
      e->last_schiff = sch;
    }
    sscanf(r->argv[0], "SCHIFF %d", &sch->nummer);
    get_report_line(r);    /* skip "SCHIFF %d" */
    while (!r->eof) {
    if (r->argc == 2 && !strcmp(r->argv[1], "Name")) {
        sch->name = xstrdup(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Typ")) {
        sch->typ = xstrdup(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Beschr")) {
        sch->beschr = xstrdup(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Partei")) {
        sch->partei = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Kapitaen")) {
        sch->kapitaen = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Kueste")) {
        sch->kueste = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Prozent")) {
        sch->prozent = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Schaden")) {
        sch->schaden = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Ladung")) {
        sch->ladung = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Groesse")) {
        sch->groesse = atoi(r->argv[0]);
           /* Prozentzahlen ausrechnen */
           if (strcmp(sch->typ, "Boot") == 0)
           {
             sch->max_groesse=5;
             sch->prozent = sch->groesse*20;
           }
           else if (strcmp(sch->typ, "Langboot") == 0)
           {
             sch->max_groesse=50;
             sch->prozent = sch->groesse*2;
           }
           else if (strcmp(sch->typ, "Drachenschiff") == 0)
           {
             sch->max_groesse=100;
             sch->prozent = sch->groesse;
           }
           else if (strcmp(sch->typ, "Karavelle") == 0)
           {
             sch->max_groesse=250;
             sch->prozent = sch->groesse*2/5;
           }
           else if (strcmp(sch->typ, "Trireme") == 0)
           {
             sch->max_groesse=200;
             sch->prozent = sch->groesse/2;
           }
            
           if (sch->prozent == 100)
             sch->prozent = 0; /* Keine Prozentzahlen bei fertigen Schiffen anzeigen */
          
           get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "MaxLadung")) {
        sch->max_ladung = atoi(r->argv[0]);
        get_report_line(r);
    } else if(r->argc == 2 && !strcmp(r->argv[1], "cargo")) {
        /* ignore as long as the old Ladung is available */
        get_report_line(r);
    } else if(r->argc == 2 && !strcmp(r->argv[1], "capacity")) {
        /* ignore as long as the old MaxLadung is available */
        get_report_line(r);
    } else if (r->is_block == 1) {
        break;
    } else {
        parsefehler(r, "SCHIFF");
	get_report_line(r);
    }
    }
}

/* Diese Funktion ordnet einem Gebaeude alle Meldungen zu, die sich darauf 
   beziehen. Es werden nur Zeiger auf die Meldungen angelegt und nichts
   allockiert */
static void burg_meldung_zuordnen(burg_t *b, partei_t *p)
{
  int i;
  char *value; /* Wert zum zugehoerigen Schluesselwort (tag) */
  if (p->all_msg == NULL)
    return;

  for (i=0; p->all_msg[i] != NULL; i++)
  {
    /* Wert des Schluessels "building" bestimmen */
    value = get_value(p->all_msg[i], "building");
    if ((value != NULL) && (atoi(value) == b->nummer))
    {
      lpmeldung_t *lpm = xmalloc(sizeof(lpmeldung_t));
      lpm->pmeldung = p->all_msg[i];
      lpm->next = b->meldungen;
      b->meldungen = lpm;
    }
  }
}


static void parse_burg(report_t *r, map_entry_t *e, partei_t *p)
{
    burg_t *b;

    b = xmalloc(sizeof(burg_t));

    b->next = NULL;
    if (e->first_burg == NULL) {
      e->first_burg = b;
      e->last_burg = b;
    } else {
      e->last_burg->next = b;
      e->last_burg = b;
    }
    sscanf(r->argv[0], "BURG %d", &b->nummer);
    get_report_line(r);    /* skip "BURG %d" */
    while (!r->eof) {
    if (r->argc == 2 && !strcmp(r->argv[1], "Name")) {
        b->name = xstrdup(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Typ")) {
        b->typ = xstrdup(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Beschr")) {
        b->beschr = xstrdup(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Besitzer")) {
        b->besitzer = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Partei")) {
        b->partei = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Groesse")) {
        b->groesse = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Unterhalt")) {
        get_report_line(r);
    } else if (!strncmp(r->argv[0], "BURG ", 5)) {
        break;
    } else if (r->is_block == 1) {
        break;
    } else {
        parsefehler(r, "BURG");
    }
    }
    burg_meldung_zuordnen(b, p);
}


static void parse_string_in_umgebung(report_t *r, map_entry_t *e)
{
    meldung_t *m;
    meldung_t *last_text;

    for (m = e->first_text; m != NULL && m->next != NULL; m = m->next)
    ;
    last_text = m;

    while (!r->eof && r->is_string[0]) {
    m = xmalloc(sizeof(meldung_t));
    m->next = NULL;
    if (e->first_text == NULL) {
        e->first_text = m;
        last_text = m;
    } else {
        last_text->next = m;
        last_text = m;
    }
    m->zeile = xstrdup(r->argv[0] + 12); /* überspringe Anfang */
    get_report_line(r);
    }
}


static void parse_umgebung(report_t *r, map_t *map, map_entry_t *e, int x, int y, int z)
{
    get_report_line(r);
    while (!r->eof) {
    if (!strncmp(r->argv[0], "EINHEIT", 7)) {
        parse_einheit(r, map, x, y, z);
    } else if (!strcmp(r->argv[0], "REGIONSEREIGNISSE")) {
        parse_meldungen(r, &e->first_ereignis);
    } else if (!strcmp(r->argv[0], "REGIONSBOTSCHAFTEN")) {
        parse_meldungen(r, &e->first_botschaft);
    } else if (!strcmp(r->argv[0], "REGIONSKOMMENTAR")) {
        parse_meldungen(r, &e->first_kommentare);
    } else if (!strncmp(r->argv[0], "DURCHREISEREGION", 16)) {
        parse_region(r, map, 17);
    } else if (!strncmp(r->argv[0], "BURG ", 5)) {
        parse_burg(r, e, finde_partei(map, map->partei));
    } else if (!strncmp(r->argv[0], "SCHIFF ", 7)) {
        parse_schiff(r, e);
    } else if (r->argc == 1 && r->is_string[0]) {
        parse_string_in_umgebung(r, e);
    } else if (r->is_block == 1) {
        break;
    } else {
#     ifdef WARNTAGS
        parsefehler(r, "UMGEBUNG");
#     endif
      get_report_line(r);
    }
   }
}


static void parse_grenze(report_t *r, map_entry_t *e)
{
    int nr;
    grenze_t *gr = xmalloc(sizeof(grenze_t));

    sscanf(r->argv[0], "GRENZE %d", &nr);
    get_report_line(r);    /* skip "GRENZE %d" */

    while (!r->eof) {
/*   DURCHREISE steht faelschlicherweise unter GRENZE */
/*   if (r->is_subblock == 1) {
       parse_unbekannt(r);
       continue;
     } else
*/
      if (r->is_block == 1) {
       break;
     } else if (!strcmp(r->argv[1], "typ")) {
       gr->typ = xstrdup(r->argv[0]);
     } else if (!strcmp(r->argv[1], "richtung")) {
       gr->richtung = atoi(r->argv[0]);
     } else if (!strcmp(r->argv[1], "prozent")) {
       gr->prozent = atoi(r->argv[0]);
     }
     else
        parsefehler(r, "GRENZE");
     get_report_line(r);
    }
    
    gr->next = e->first_grenze;
    e->first_grenze = gr;    
}


static void parse_resource(report_t *r, map_entry_t *e)
{
    resource_t *rs = xmalloc(sizeof(resource_t));

    sscanf(r->argv[0], "RESOURCE %d", &(rs->id));

    get_report_line(r);
    while (!r->eof) {
     if ((r->argc > 1) && (!strcmp(r->argv[1], "type"))) {
       rs->type = xstrdup(r->argv[0]);
     } else if ((r->argc > 1) && (!strcmp(r->argv[1], "number"))) {
       rs->number = atoi(r->argv[0]);
     } else if ((r->argc > 1) && (!strcmp(r->argv[1], "skill"))) {
       rs->skill = atoi(r->argv[0]);
     } else if (!strncmp(r->argv[0], "REGION", 6) ||
           !strcmp(r->argv[0], "DURCHREISE") ||
           !strcmp(r->argv[0], "PREISE") || 
           !strncmp(r->argv[0], "EINHEIT ", 8) || 
           !strncmp(r->argv[0], "BURG ", 5) || 
           !strncmp(r->argv[0], "SCHIFF ", 7) ||
           !strncmp(r->argv[0], "GRENZE ", 7) || 
           !strncmp(r->argv[0], "RESOURCE ", 9) || 
           !strcmp(r->argv[0], "DURCHSCHIFFUNG") || 
           !strncmp(r->argv[0], "EFFECTS", 7) ||
           !strncmp(r->argv[0], "MESSAGE ", 8) ||
           !strcmp(r->argv[0], "MESSAGETYPES") || 
           !strncmp(r->argv[0], "MESSAGETYPE ", 12)) {
        break;
     } else if (r->is_block == 1) {
       parse_unbekannt(r);
       continue; /* Aufruf von get_report_line umgehen */
     }
     else
       parsefehler(r, "RESOURCE");
      get_report_line(r);
   }
    
    rs->next = e->first_resource;
    e->first_resource = rs;    
}

void parse_region(report_t *r, map_t *map, int offset)
{
    map_entry_t *e;
    int          x, y, z;
    int          ebenen;

    ebenen = sscanf(r->argv[0] + offset, "%d %d %d", &x, &y, &z);

    if (ebenen == 2)
      z = 0;
      
    e = add_to_map(map, x, y, z, map->runde);

    get_report_line(r);        /* skip "REGION %d %d" */
                /* or   "DURCHREISEREGION %d %d" */
    while (!r->eof) {
    if (r->argc == 2 && !strcmp(r->argv[1], "Name")) {
        set_region_name(map, x, y, z, r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Beschr")) {
        if (e->beschr != NULL) /* koennte noch von einem alten Report stammen */
          xfree(e->beschr);
        e->beschr = xstrdup(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Strasse")) {
        e->strasse = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Insel")) {
        if (e->insel != NULL && strcmp(e->insel, r->argv[0])) {
          printf("neuer Inselname, alt: %s, neu: %s\n",
               e->insel, r->argv[0]);
          xfree(e->insel);
        }
        e->insel = xstrdup(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Lohn")) {
        e->lohn = atoi(r->argv[0]) + 1; /* +1, da der Bauernlohn abgespeichert wird */
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "herb")) {
        if (e->kraut != NULL) /* koennte noch von einem alten Report stammen */
          xfree(e->kraut);
        e->kraut = xstrdup(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Terrain")) {
        set_region_typ(map, x, y, z, r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Baeume")) {
        if (map->version < 59) /* ab v59 gibt's den RESOURCE Block */
          e->baeume = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Schoesslinge")) {
        get_report_line(r); /* Schoesslinge stehen zusaetzlich im RESOURCE Block,
                               also koennen diese hier ignoriert werden */
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Bauern")) {
        /* um nicht Reports mit leeren Regionen (z.B. Nachbarregionen) als 
           Quelle der Infos abzuspeichern, wird die Runde nur fuer Regionen
           mit bekannter Bauernanzahl gemerkt */
        e->runde = map->runde; /* Alter der Infos merken */
        e->bauern = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Silber")) {
        e->silber = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Eisen")) {
        if (map->version < 59) /* ab v59 gibt's den RESOURCE Block */
          e->eisen = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Steine")) {
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Laen")) {
        e->laen = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Pferde")) {
        e->pferde = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Unterh")) {
        e->unterhaltung = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Rekruten")) {
        e->rekrutierung = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "maxLuxus")) {
        e->luxusgueter = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Mallorn")) {
        e->mallorn = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Verorkt")) {
        e->verorkt = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Runde")) {
        e->runde = atoi(r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "visibility")) {
        get_report_line(r); /* erstmal ignorieren */
    } else if (r->argc == 2 && !strcmp(r->argv[1], "TNorden")) {
        set_region_typ(map, x, y, z - 1, r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "TOsten")) {
        set_region_typ(map, x + 1, y, z, r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "TSueden")) {
        set_region_typ(map, x, y + 1, z, r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "TWesten")) {
        set_region_typ(map, x - 1, y, z, r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "NNorden")) {
        set_region_name(map, x, y - 1, z, r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "NOsten")) {
        set_region_name(map, x + 1, y, z, r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "NSueden")) {
        set_region_name(map, x, y + 1, z, r->argv[0]);
        get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "NWesten")) {
        set_region_name(map, x - 1, y, z, r->argv[0]);
        get_report_line(r);
    } else if (!strcmp(r->argv[0], "PREISE")) {
        parse_preise(r, e);
    } else if (!strncmp(r->argv[0], "EINHEIT", 7)) {
        parse_einheit(r, map, x, y, z);
    } else if (!strncmp(r->argv[0], "SCHIFF ", 7)) {
        parse_schiff(r, e);
    } else if (!strncmp(r->argv[0], "BURG ", 5)) {
        parse_burg(r, e, finde_partei(map, map->partei));
    } else if (!strncmp(r->argv[0], "MESSAGE ", 8)) {
        parse_message(r, &e->first_ereignis);
    } else if (!strncmp(r->argv[0], "GRENZE ", 7)) {
        parse_grenze(r, e);
    } else if (!strcmp(r->argv[0], "EFFECTS")) {
        parse_effects(r, &e->first_effect);
    } else if (!strcmp(r->argv[0], "REGIONSEREIGNISSE")) {
        parse_meldungen(r, &e->first_ereignis);
    } else if (!strcmp(r->argv[0], "DURCHREISE")) {
        parse_meldungen(r, &e->first_durchreise);
    } else if (!strcmp(r->argv[0], "REGIONSBOTSCHAFTEN")) {
        parse_meldungen(r, &e->first_botschaft);
    } else if (!strcmp(r->argv[0], "REGIONSKOMMENTAR")) {
        parse_meldungen(r, &e->first_kommentare);
    } else if (!strncmp(r->argv[0], "DURCHREISEREGION", 16)) {
        parse_region(r, map, 17);
    } else if (!strcmp(r->argv[0], "DURCHSCHIFFUNG")) {
        parse_meldungen(r, &e->first_durchschiffung);
    } else if (!strncmp(r->argv[0], "UMGEBUNG", 8)) {
        parse_umgebung(r, map, e, x, y, z);
    } else if (!strncmp(r->argv[0], "RESOURCE ", 9)) {
        parse_resource(r, e);
    } else if (!strncmp(r->argv[0], "REGION", 6) ||
           !strncmp(r->argv[0], "DURCHREISEREGION", 16) ||
           !strcmp(r->argv[0], "MESSAGETYPES") || 
           !strncmp(r->argv[0], "MESSAGETYPE ", 12)) {
        break;
    } else if (r->is_block == 1) { /* sonstiger Block */
        parse_unbekannt(r);
    } else {
#if WARNTAGS
        int   i;
        printf("line %d, unknown content in region:\n", r->lnr);
        for (i = 0; i < r->argc; i++) {
          printf("[%d] = \"%s\"\n", i, r->argv[i]);
        }
#endif
        /* Einlesen fortsetzten, da dies nur eine unbekannte Option war */
        get_report_line(r);
    }
    }
    if (e->luxusgueter == 0)
    e->luxusgueter = e->bauern / 100;
    /*@@@ ... unterhaltung */
    /*@@@ ... rekrutierung */
}

static struct {
    int dx;
    int dy;
} richtung[] = {
    { -1,  1 },            /* nordwest */
    {  0,  1 },            /* nordost */
    {  1,  0 },            /* ost */
    {  1, -1 },            /* südost */
    {  0, -1 },            /* südwest */
    { -1,  0 }            /* west */
};

void flood_fill(map_t *map, map_entry_t *e)
{
    map_entry_t *en;
    int         i;

    for (i = 0; i < 6; i++) {
    en = mp(map, e->x + richtung[i].dx, e->y + richtung[i].dy, e->z);
    if (en != NULL && en->typ != T_OZEAN) {
        if (en->insel == NULL || strcmp(en->insel, e->insel)) {
        if (en->insel != NULL)
            xfree(en->insel);
        en->insel = xstrdup(e->insel);
        flood_fill(map, en);
        }
    }
    }
}
