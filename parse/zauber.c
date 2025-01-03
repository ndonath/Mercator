/* $Id: zauber.c,v 1.3 2001/07/22 17:27:16 marko Exp $ */
#include <assert.h>
#include "parser.h"

/* Parsen von Traenken (Alchemie), Zaubern und Kampfzaubern */

void parse_kampfzauber(report_t *r, einheit_t *ei)
{
 /* art kann 0, 1 oder 2 annehmen fuer Prae-, normale oder Postkampfzauber*/
    int art;
    kampfzauber_t *kampf;

    sscanf(r->argv[0], "KAMPFZAUBER %d", &art);

    get_report_line(r); /* skip KAMPFZAUBER */
    kampf = xmalloc(sizeof(kampfzauber_t));

    while (!r->eof) {
      if (r->argc == 2 && !strcmp(r->argv[1], "name")) {
        kampf->name = xstrdup(r->argv[0]);
      } else if (r->argc == 2 && !strcmp(r->argv[1], "level")) {
        kampf->level = (short) atoi(r->argv[0]);
      } else if (isupper(r->argv[0][0]) != 0) { /* neuer Block */
        break;
      } 
      #if WARNTAGS
        else
          printf("Unbekanntes Argument %s im Kampfzauber!\n", r->argv[1]);
      #endif
      get_report_line(r);
    }

    if (art == 0)
    {
      assert(ei->praezauber == NULL);
      ei->praezauber = kampf;
    }
    else if (art == 1)
    {
      assert(ei->normalzauber == NULL);
      ei->normalzauber = kampf;
    }
    else if (art == 2)
    {
      assert(ei->postzauber == NULL);
      ei->postzauber = kampf;
    }
    else {
    #if WARNTAGS
        printf("Unbekannter Kampfzauber!\n");
    #endif
        xfree(kampf);
    }
}

void parse_zauber_alt(report_t *r, partei_t *p)
{
    zauber_t *z;

    get_report_line(r);		/* skip ZAUBER */
    while (!r->eof && r->argc == 2 && !strcmp(r->argv[1], "Spruch")) {
	if (r->argc != 2 || strcmp(r->argv[1], "Spruch")) {
	    fprintf(stderr, "ZAUBER-Spruch erwartet\n");
	    break;
	}
	z = xmalloc(sizeof(zauber_t));
	z->spruch = xstrdup(r->argv[0]);
	get_report_line(r);	/* skip "Spruch" */
	while (!r->eof) {
	    if (r->argc == 2 && !strcmp(r->argv[1], "Stufe")) {
		z->stufe = atoi(r->argv[0]);
		get_report_line(r);
	    } else if (r->argc == 2 && !strcmp(r->argv[1], "Beschr")) {
		z->beschr = xstrdup(r->argv[0]);
		get_report_line(r);
	    } else {
		break;		/* end of "ZAUBER (Spruch)" */
	    }
	}
        z->art = -1;
        z->rang = -1;
	z->next = p->zauber;
	p->zauber = z;
    }
}



/* Parsen der zu dem Zauber gehoerigen Komponenten */
static void parse_komponenten(report_t *r, zauber_t *z)
{
  komponenten_t *komp = NULL;
  short mult; /* Gegenstand mit Stufe multiplizieren? */

  get_report_line(r); /* skip KOMPONENTEN */

  while (!r->eof && (isupper(r->argv[0][0]) == 0)) {

    komp = xmalloc(sizeof(komponenten_t));

    /* mult wird noch ignoriert... */
	sscanf(r->argv[0], "%d %hd", &komp->anzahl, &mult);
	komp->name = xstrdup(r->argv[1]);
    komp->mult = mult;
    komp->next = z->komp;
    z->komp = komp;  
    get_report_line(r);
  }
}


/*
 * Parse die Zauber des neuen Magiesystems.
 * Komponenten:
 * <Anzahl> <multiplizieren-mit-stufe (boolean 0 oder 1)>;<Resource>
 * <Resource> können u.a. silber, kräuter, items und aura sein.
 */
void parse_zauber_neu(report_t *r, partei_t *p)
{
    zauber_t *z;

    get_report_line(r);		/* skip ZAUBER */

    z = xmalloc(sizeof(zauber_t));
    z->rang = -1; /* Default */
    z->mod = 0;
    z->art = -1;

    while (!r->eof) {
	if (r->argc == 2 && !strcmp(r->argv[1], "name")) {
            z->spruch = xstrdup(r->argv[0]);
            get_report_line(r);
        } else if (r->argc == 2 && !strcmp(r->argv[1], "type")) {
            get_report_line(r);
        } else if (r->argc == 2 && !strcmp(r->argv[1], "level")) {
            z->stufe = atoi(r->argv[0]);
            get_report_line(r);
        } else if (r->argc == 2 && !strcmp(r->argv[1], "rank")) {
            z->rang = (short) atoi(r->argv[0]);
            get_report_line(r);
        } else if (r->argc == 2 && !strcmp(r->argv[1], "info")) {
            z->beschr = xstrdup(r->argv[0]);
            get_report_line(r);
        } else if (r->argc == 2 && !strcmp(r->argv[1], "class")) {
            if (strcmp(r->argv[0], "precombat") == 0)
              z->art = 0;
            else if (strcmp(r->argv[0], "combat") == 0)
              z->art = 1;
            else if (strcmp(r->argv[0], "postcombat") == 0)
              z->art = 2;
            else if (strcmp(r->argv[0], "normal") == 0)
              z->art = 3;
            get_report_line(r);
        } else if (r->argc == 2 && !strcmp(r->argv[1], "familiar")) {
            get_report_line(r);
        } else if (r->argc == 2 && !strcmp(r->argv[1], "ship")) {
            z->mod |= 1;
            get_report_line(r);
        } else if (r->argc == 2 && !strcmp(r->argv[1], "far")) {
            z->mod |= 2;
            get_report_line(r);
        } else if (!strcmp(r->argv[0], "KOMPONENTEN")) {
             parse_komponenten(r, z);
        } else {
            break;
        }
    }
    z->next = p->zauber;
    p->zauber = z;
}


/* Parsen der Trankzutaten */
static void parse_zutaten(report_t *r, trank_t *t)
{
  gegenstand_t *komp = NULL;

  get_report_line(r); /* skip ZUTATEN */

  while (!r->eof && (r->is_string[0] == 1)) {

    komp = xmalloc(sizeof(gegenstand_t));
	komp->name = xstrdup(r->argv[0]);

    komp->next = t->komp;
    t->komp = komp;
    get_report_line(r);
  }
}



/*
 * Einen Trank
 */
void parse_trank(report_t *r, partei_t *p)
{
  trank_t *t;

  t = xmalloc(sizeof(trank_t));

  get_report_line(r);         /* überspringe TRANK xxx */

  while (!r->eof) {
    if (r->argc == 2 && !strcmp(r->argv[1], "Name")) {
      t->name = xstrdup(r->argv[0]);
      get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Beschr")) {
      t->beschr = xstrdup(r->argv[0]);
      get_report_line(r);
    } else if (r->argc == 2 && !strcmp(r->argv[1], "Stufe")) {
      t->stufe = atoi(r->argv[0]);
      get_report_line(r);
    } else if (!strcmp(r->argv[0], "ZUTATEN")) {
      parse_zutaten(r, t);
    } else {
      break;
    }
  }
  
    t->next = p->trank;
    p->trank = t;
}
