/* $Id: meldungen.c,v 1.15 2002/02/14 20:59:48 marko Exp $ */
#include "parser.h"
#include "assert.h"


/* Den Wert eines bestimmten tags ermitteln 
   In dieser Funktion wird kein Speicher allockiert, der zurueckgegebene
   Zeiger zeigt auf ein Element der Liste */
char *get_value(meldung_t *m, char *tag)
{
  tags_t *current=m->werte;
  
  while (current != NULL)
  {
    if ((current->argc == 2) && (strcmp(current->argv[1], tag) == 0))
      return current->argv[0];
    current = current->next;
  }
  return NULL;
}



void parse_meldungen(report_t *r, meldung_t **p)
{
    meldung_t *m;
    assert(*p == NULL);
    get_report_line(r);		/* skip "KAEMPFE" (oder ähnlich) */
    while (!r->eof && r->is_string[0]) {
	  m = xmalloc(sizeof(meldung_t));
	  m->next = NULL;
	  *p = m;
	  p = &m->next;
          m->zeile = xstrdup(r->argv[0]);
	  get_report_line(r);
    }
}


/* eine Meldung aus der meldung-Liste loeschen und in "liste" einfuegen */
static void meldung_anhaengen(meldung_t **anfang, meldung_t **merker, meldung_t **meldung, meldung_t **liste)
{
  meldung_t *liste_anfang;

/* anfang ist das erste Element, meldung das aktuelle und merker das vorherige */

  liste_anfang = *meldung;
  *meldung = (*meldung)->next;
  liste_anfang->next = *liste;
  *liste = liste_anfang;

  if (liste_anfang == *anfang) /* Das erste Element wurde veschoben */
    *anfang = *merker = *meldung;
  else
  {
    (*merker)->next = *meldung;
    *meldung = *merker;
  }
}


/* Die Meldungen in Kaempfe, Handel usw. unterteilen
   Dies ist nur ein Hack, bis die Meldungen richtig eingelesen werden
   statt nur den rendered-String zu betrachten */
void sortiere_meldungen(map_t *map)
{
    meldung_t *anfang;
    meldung_t *merker;
    partei_t *ep;
    
    if ((map->partei == 0) || (map->partei == -1))
      return;

    ep = finde_partei(map, map->partei);

    if (ep == NULL) /* falls es die Partei nicht gibt */
      return;

    /* noch stehen alle Meldungen in ep->meldungen */
    merker = anfang = ep->meldungen;
    
    while (ep->meldungen != NULL)
    {

       if (ep->meldungen->type == 1511758069)  /* Kraeutervorkommen in Region schreiben*/
       {
         map_entry_t *e;
         const char *const kraut = get_value(ep->meldungen, "herb");
         const char *const region = get_value(ep->meldungen, "region");
         
         if ((kraut != NULL) && (region != NULL))
         {
           int x, y, z=0; /* Regionskoordinaten */
           /* die Regionen haben in  alten Reporten ein anderes Format */
           if (map->version >= 57)
             sscanf(region, "%d %d %d", &x, &y, &z);
           else
             sscanf(region, "%d, %d", &x, &y);

           e = mp(map, x, y, z); /* richtige Region suchen */
           if (e == NULL)
             e = add_to_map(map, x, y, z, map->runde);
           if (e->kraut != NULL)
             xfree(e->kraut);
           e->kraut = xstrdup(kraut);
         }
       }
    
       switch( ep->meldungen->type )
       {
         case 577243974:
         case 1549031288:
         case 5281483:
         case 170076:
         case 1478912224:
         case 107552268: /* Unterhalt fuer Gebaeude - passt hier nicht so ganz */
           meldung_anhaengen(&anfang, &merker, &ep->meldungen, &ep->handel);
           break;
         case 2020970388:
         case 771334452:
           meldung_anhaengen(&anfang, &merker, &ep->meldungen, &ep->einkommen);
           break;
         case 704703128:
         case 1750373495:
         case 1511758069:
         case 2087428775:
         case 1670080073:
         case 1349776898:
           meldung_anhaengen(&anfang, &merker, &ep->meldungen, &ep->produktion);
           break;
         case 1423091461:
         case 891175669:
         case 1242100855:
           meldung_anhaengen(&anfang, &merker, &ep->meldungen, &ep->bewegungen);
           break;
         case 2038283703:
         case 458029150:
         case 1880103454:
         case 340958798:
         case 848660175:
         case 1513395888:
         case 766242104:
         case 251105481:
         case 361073458:
         case 976452995:
         case 2090314345:
         case 1128215023:
         case 571284347:
         case 2004700229:
         case 1060448783:
         case 709972261:
         case 1616323319:
         case 1045796952:
         case 2094553546:
         case 1115140909:
         case 1407769869: /* Die Einheit hat keine Kraeuter */
         case 324167062:  /* NUMMER EINHEIT ist schon belegt */
         case 405635298:  /* Einheit xxxx wurde nicht gefunden */
         case 794962043:  /* Fehler bei Zauber */
         case 625375172:  /* 'NACH NW NW' - Die Einheit fährt nicht mit uns. */
         case 94592724:   /* NACH NW NW' - Kann die zu transportierende Einheit nicht finden. */
         case 1291981293: /* LEHREN xxxx' - Schnuffel (xxxx) ist mindestens gleich gut wie wir. */
         case 2042128202: /* 'VERKAUFE alles Myrrhe' - Dieses Gut hat die Einheit nicht. */
         case 1755362615: /* 'BEWACHEN' - Die Einheit ist nicht bewaffnet und kampffähig. */
         case 44612214:   /* 'OPTION ZIPPED NICHT' - Die Optionen ZIP und BZIP2 können nur um, nicht ausgeschaltet werden. */
         case 1948430386: /* 'KAMPFZAUBER xxxx' - Selbst in der Bibliothek von Xontormia konnte dieser Spruch nicht gefunden werden. */
           meldung_anhaengen(&anfang, &merker, &ep->meldungen, &ep->fehler);
           break;
         case 200064037:
         case 443066738:
           meldung_anhaengen(&anfang, &merker, &ep->meldungen, &ep->lernen);
           break;
         case 442874678:
         case 1279580895:
         case 1539288494:
           meldung_anhaengen(&anfang, &merker, &ep->meldungen, &ep->magie);
           break;
         default :
           merker = ep->meldungen;
           ep->meldungen = ep->meldungen->next;
           break;
       }
    }
    ep->meldungen = anfang; /* die Liste wieder auf das erste Element zurueckseten */

}

/*
 * Eine Message
 */
void parse_message(report_t *r, meldung_t **p_m)
{
    meldung_t *m;

    assert(p_m != NULL);

    get_report_line(r);         /* überspringe MESSAGE xxx */

    while (*p_m != NULL)
      p_m = &(*p_m)->next;

    
    m = xmalloc(sizeof(meldung_t));
    m->next = NULL;

    while ((r->eof == 0) && (r->is_block == 0)) {
        if (r->argc == 2 && !strcmp(r->argv[1], "rendered")) {
            m->zeile = xstrdup(r->argv[0]);
        } else if (!strcmp(r->argv[1], "type")) {
            /* Meldungstyp eintragen */
            m->type = strtoul(r->argv[0], (char**)NULL, 10);
        } else { /* sonstige Werte auch abspeichern */
            tags_t *z = xmalloc(sizeof(tags_t));
            int i;
            z->argc = r->argc;
            z->argv = xmalloc(z->argc * sizeof(char*));
            for (i=0; i<z->argc; i++)
              z->argv[i] = xstrdup(r->argv[i]);
            z->next = m->werte;
            m->werte = z;
        }
        get_report_line(r);
    }
    *p_m = m;

}

/*
 * BATTLE x y ist Oberblock von MESSAGE
 */
void parse_battle(report_t *r, map_t *map, battle_t **p_b)
{
    int x, y, z; /* Regionskoordinaten */
    battle_t    *b = xmalloc(sizeof(battle_t));

    if (sscanf(r->argv[0], "BATTLE %d %d %d", &x, &y, &z) == 2)
      z = 0; /* nur 2 Koordinaten gegeben */
   
    b->region = mp(map, x, y, z);
    if (b->region == NULL)
      b->region = add_to_map(map, x, y, z, map->runde);

    get_report_line(r);         /* überspringe BATTLE x y */
    
    while (!strncmp(r->argv[0], "MESSAGE ", 8)) {
        parse_message(r, &(b->details));
    }

    b->next = *p_b;
    *p_b = b;
}


/*
 * Eine Message
 */
void parse_messagetypes(report_t *r)
{
    get_report_line(r);         /* überspringe MESSAGETYPES */

    while (!r->eof && (r->is_string[0] || !isupper(r->argv[0][0]))) {
        get_report_line(r);
    }
}
