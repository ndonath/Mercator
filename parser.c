/* $Id: parser.c,v 1.14 2000/05/21 13:21:17 butenuth Exp $ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <ctype.h>
#include "map.h"

#define WARNTAGS 1

void parse_region(report_t *r, map_t *map, int offset);

report_t *make_report(char *filename)
{
    report_t *r;

    r = xmalloc(sizeof(report_t));
    r->fp = xfopen(filename, "r");
    r->lnr = 0;
    r->eof = 0;

    return r;
}


void destroy_report(report_t *r)
{
    fclose(r->fp);
    xfree(r);
}


void get_report_line(report_t *r)
{
    char *s;

    if (fgets(r->line, REPORT_LINE_LEN, r->fp) == NULL) {
	r->eof = 1;
	return;
    }
    s = r->line;
    r->argc = 0;
    while (*s != '\r' && *s != '\n' && *s != 0) {
	if (*s == '"') {
	    s++;		/* skip start " */
	    r->argv[r->argc] = s;
	    while (*s != '"' && *s != 0) {
		/*
		 * Eigentlich sollte es \n in Strings nicht geben, aber
		 * manche Mailer zerhacken gerne Zeilen...
		 */
		if (*s != '\r' && *s != '\n')
		    s++;	/* search end " */
		else {
		    if (fgets(s + 1,
			      REPORT_LINE_LEN - (1 + s - r->argv[r->argc]),
			      r->fp) == NULL) {
			fprintf(stderr, "unerwartetes Dateiende in Zeile %d\n",
				r->lnr);
			r->eof = 1;
			return;
		    }
		    r->lnr++;
		    s++;
		}
	    }
	    assert(*s == '"');
	    *s++ = 0;		/* cut string */
	    if (*s == ';')
		s++;		/* skip ; */
	    r->is_string[r->argc] = 1;
	    r->argc++;
	} else {
	    r->argv[r->argc] = s;
	    while (*s != ';' && *s != '\r' && *s != '\n' && *s != 0)
		s++;
	    if (*s != 0)
		*s++ = 0;
	    r->is_string[r->argc] = 0;
	    r->argc++;
	}
    }
    r->lnr++;

    return;
}


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


/*
 * Alte Syntax.
 */
void parse_adressen(report_t *r, map_t *map)
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
	} else {
	    break;		/* end of "ADRESSEN" */
	}
    }
}


void parse_meldungen(report_t *r, map_t *map, meldung_t **p)
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


/*
 * Eine Message
 */
void parse_message(report_t *r, map_t *map, meldung_t **p_m)
{
    int nr = atoi(r->argv[0] + 8);
    meldung_t *m;

    assert(p_m != NULL);

    get_report_line(r);         /* überspringe MESSAGE xxx */
    while (!r->eof && (r->is_string[0] || !isupper(r->argv[0][0]))) {
        if (r->argc == 2 && !strcmp(r->argv[1], "rendered")) {
            m = xmalloc(sizeof(meldung_t));
            m->next = NULL;
            m->zeile = xstrdup(r->argv[0]);
            while (*p_m != NULL)
                p_m = &(*p_m)->next;
            *p_m = m;
            (void)nr;           /* noch nicht genutzt */
            get_report_line(r);
        } else {
            get_report_line(r);
        }
    }

}


/*
 * BATTLE x y ist Oberblock von MESSAGE
 */
void parse_battle(report_t *r, map_t *map, meldung_t **p_m)
{
    get_report_line(r);         /* überspringe BATTLE x y */
    
    while (!strncmp(r->argv[0], "MESSAGE ", 8)) {
        parse_message(r, map, p_m);
    }
}


/*
 * Eine Message
 */
void parse_messagetypes(report_t *r, map_t *map)
{
    get_report_line(r);         /* überspringe MESSAGETYPES */

    while (!r->eof && (r->is_string[0] || !isupper(r->argv[0][0]))) {
        get_report_line(r);
    }
}


/*
 * Optionen überspringen...
 */
void parse_optionen(report_t *r, map_t *map, partei_t *p)
{

    assert(p != NULL);
    get_report_line(r);		/* skip "OPTIONEN" (oder ähnlich) */
    while (!r->eof && !isupper(r->argv[0][0])) {
	get_report_line(r);
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
void parse_alliierte(report_t *r, map_t *map)
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
	    break;		/* end of "ALLIERTE" */
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
void parse_allianz(report_t *r, map_t *map)
{
    partei_t *p = NULL;

    p = finde_partei(map, atoi(r->argv[0] + 8));
    get_report_line(r);

    while (!r->eof) {
	if (r->argc == 2 && !strcmp(r->argv[1], "Parteiname")) {
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
	    break;		/* end of "ALLIANZ %d" oder unbekannt */
	}
    }
}


void parse_fehler(report_t *r, map_t *map)
{
    get_report_line(r);
    while (!r->eof) {
	if (r->argc == 1 &&
	    (!strcmp(r->argv[0], "KAEMPFE") ||
	     !strcmp(r->argv[0], "EREIGNISSE") ||
	     !strcmp(r->argv[0], "EINKOMMEN"))) {
	    get_report_line(r);
	    break;
	}
	get_report_line(r);
    }
}


void set_region_typ(map_t *map, int x, int y, char *typ)
{
    map_entry_t *e;
    int         i, etyp;
    const char  *old = NULL;

    e = mp(map, x, y);
    if (e == NULL)
	e = add_to_map(map, x, y);
    if (!strcmp("Wald", typ)) {
	i = T_EBENE;
    } else if (!strcmp("Wueste", typ)) {
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

    if (e->typ == T_UNKNOWN) {
	/*
	 * Neue Region, nicht vorher bekannt.
	 */
	e->typ = i;
	if (e->typ == T_EBENE) {
	    if (!strcmp("Wald", typ))
		e->etyp = ET_WALD;
	    else
		e->etyp = ET_EBENE;
	}
    } else {
	/*
	 * Alte Region, schon bekannt.
	 */
	if (e->typ != i) {
	    /*
	     * Region war schon bekannt, Typ hat sich geändert:
	     */
	    if (e->typ == T_EBENE) {
		if (e->etyp == ET_WALD)
		    old = "Wald";
		else
		    old = "Ebene";
	    } else {
		old = region_typ_name[e->typ];
	    }
		if (e->name != NULL)
			fprintf(stderr, "%s (%d, %d) inkonsistent: %s -> %s\n",
				e->name, x, y, old, typ);
		else fprintf(stderr, "(%d, %d) inkonsistent: %s -> %s\n", x, y, old, typ);
	    e->typ = i;		/* überschreibe alten Eintrag */
	} else {
	    /*
	     * Region war schon bekannt, Typ ist gleich. Es könnte immer
	     * noch der etyp (Wald oder Ebene) neu sein:
	     */
	    if (e->typ == T_EBENE) {
		if (!strcmp("Wald", typ))
		    etyp = ET_WALD;
		else
		    etyp = ET_EBENE;
		if (etyp != e->etyp) {
		    if (e->name != NULL)
			fprintf(stderr, "%s (%d, %d) ist jetzt ",
				e->name, x, y);
		    fprintf(stderr, "(%d, %d) ist jetzt ", x, y);
		    if (etyp == ET_WALD)
			fprintf(stderr, "Wald.\n");
		    else
			fprintf(stderr, "Ebene.\n");
		    e->etyp = etyp;
		}
	    }
	}
    }
}


void set_region_name(map_t *map, int x, int y, const char *name)
{
    map_entry_t *e;
    char        *n, *p;

    n = xstrdup(name);
    p = strchr(n, '(');
    if (p != NULL && p > n)
	*(p - 1) = 0;

    e = mp(map, x, y);
    if (e == NULL)
	e = add_to_map(map, x, y);

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


/*
 * Preis < 0: Man kann kaufen, Preis > 0: Man kann verkaufen
 */
void parse_preise(report_t *r, map_t *map, map_entry_t *e)
{
    preis_t *p;
    int     preis;

    get_report_line(r);
    while (!r->eof && r->argc == 2 && atoi(r->argv[0]) != 0) {
	p = xmalloc(sizeof(preis_t));
	preis = atoi(r->argv[0]);
	p->produkt = xstrdup(r->argv[1]);
	p->next = NULL;
	if (preis < 0) {	/* Bauern kaufen */
	    p->preis = -preis;
	    if (e->first_biete == NULL) {
		e->first_biete = p;
		e->last_biete  = p;
	    } else {
		e->last_biete->next = p;
		e->last_biete = p;
	    }
	} else {		/* Bauern bieten */
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


void parse_talente(report_t *r, map_t *map, map_entry_t *e, einheit_t *ei)
{
    talent_t *t;

    get_report_line(r);		/* skip "TALENTE" */
    while (!r->eof && atoi(r->argv[0]) != 0) {
	t = xmalloc(sizeof(talent_t));
	sscanf(r->argv[0], "%d %d", &t->tage, &t->stufe);
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


void parse_gegenstaende(report_t *r, map_t *map, map_entry_t *e, einheit_t *ei)
{
    gegenstand_t *g;

    get_report_line(r);		/* skip "GEGENSTANDE" */
    while (!r->eof && atoi(r->argv[0]) != 0) {
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


void parse_einheitsbotschaften(report_t *r, map_t *map, map_entry_t *e,
			       einheit_t *ei)
{
    meldung_t *m, *last = NULL;

    assert(ei->botschaften == NULL);

    get_report_line(r);		/* skip "EINHEITSBOTSCHAFTEN" */
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


void parse_schiff(report_t *r, map_t *map, map_entry_t *e)
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
	} else if (r->argc == 2 && !strcmp(r->argv[1], "MaxLadung")) {
	    sch->max_ladung = atoi(r->argv[0]);
	    get_report_line(r);
	} else if (!strncmp(r->argv[0], "SCHIFF", 6) ||
		   !strncmp(r->argv[0], "BURG", 4) ||
		   !strncmp(r->argv[0], "EINHEIT", 7) ||
		   !strncmp(r->argv[0], "DURCHREISEREGION", 16) ||
		   !strncmp(r->argv[0], "REGION ", 7) ||
		   !strncmp(r->argv[0], "UMGEBUNG", 8)) {
	    break;
	} else {
	    printf("beende Schiff wegen <%s>\n", r->argv[0]);
	    break;
	}
    }
}


void parse_burg(report_t *r, map_t *map, map_entry_t *e)
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
	    b->unterhalt = atoi(r->argv[0]);
	    get_report_line(r);
	} else if (!strncmp(r->argv[0], "BURG ", 5)) {
	    break;
	} else if (!strncmp(r->argv[0], "SCHIFF", 6) ||
		   !strncmp(r->argv[0], "BURG", 4) ||
		   !strncmp(r->argv[0], "EINHEIT", 7) ||
		   !strncmp(r->argv[0], "REGION ", 7) ||
		   !strncmp(r->argv[0], "DURCHREISEREGION", 16) ||
		   !strncmp(r->argv[0], "UMGEBUNG", 8)) {
	    break;
	} else {
	    printf("beende Burg wegen <%s, %s>\n",
		   r->argv[0], r->argc > 1 ? r->argv[1] : "(null)");
	    break;
	}
    }
}


void add_einheit_to_partei(map_t *map, map_entry_t *e, einheit_t *ei)
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


void parse_einheit(report_t *r, map_t *map, int x, int y)
{
    map_entry_t *e;
    einheit_t   *ei;
    int         i;
    int         hash;
    partei_t    *p;

    e = mp(map, x, y);
    assert(e != NULL);
    ei = xmalloc(sizeof(einheit_t));
    ei->nummer = atoi(r->argv[0] + 8);
    ei->anzahl = 1;		/* default */
    ei->region = e;		/* Position der Einheit */
    ei->partei = -1;		/* -1 = unbekannt, 0 = Monster */
    ei->tarnung = -1;		/* -1 = unbekannt, sonst Level */
    ei->aura = -1;
    ei->auramax = -1;
    ei->hunger = -1;
    ei->temp = -1;              /* war keine Temp-Einheit */
    /*
     * Füge Einheit in die Hashtabelle aller Einheiten in der Karte ein,
     * Schlüssel ist die Einheitennummer.
     */
    hash = ei->nummer % map->einheit_size;
    ei->next_hash = map->ei_hash[hash];
    map->ei_hash[hash] = ei;

    get_report_line(r);		/* skip "EINHEIT %d" */
    while (!r->eof) {
	if (!strncmp(r->argv[0], "EINHEIT ", 8) ||
	    !strncmp(r->argv[0], "REGION ", 7) ||
	    !strncmp(r->argv[0], "DURCHREISEREGION ", 16))
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
	} else if (r->argc == 2 && !strcmp(r->argv[1], "privat")) {
	    /* "study bogenschiessen";privat */
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
	} else if (r->argc == 2 && !strcmp(r->argv[1], "Silber")) {
	    ei->silber = atoi(r->argv[0]);
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "Burg")) {
	    ei->ist_in = atoi(r->argv[0]);
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "Kampfstatus")) {
	    ei->ks = atoi(r->argv[0]);
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
	} else if (!strcmp(r->argv[0], "COMMANDS")) {
	    parse_meldungen(r, map, &ei->commands);
	} else if (!strcmp(r->argv[0], "TALENTE")) {
	    parse_talente(r, map, e, ei);
	} else if (!strcmp(r->argv[0], "SPRUECHE")) {
	    parse_meldungen(r, map, &ei->sprueche);
	} else if (!strcmp(r->argv[0], "GEGENSTAENDE")) {
	    parse_gegenstaende(r, map, e, ei);
	} else if (!strcmp(r->argv[0], "EINHEITSBOTSCHAFTEN")) {
	    parse_einheitsbotschaften(r, map, e, ei);
	} else {
	    printf("Zeile %d, unbekannter Inhalt:\n", r->lnr);
	    for (i = 0; i < r->argc; i++) {
		printf("[%d] = \"%s\"\n", i, r->argv[i]);
	    }
	    get_report_line(r);
	}
    }
    if (e->first_einheit == NULL) {
	e->first_einheit = ei;
	e->last_einheit  = ei;
    } else {
	e->last_einheit->next = ei;
	e->last_einheit = ei;
    }
    ei->next = NULL;

    add_einheit_to_partei(map, e, ei);
}


void parse_string_in_umgebung(report_t *r, map_t *map, map_entry_t *e)
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


void parse_umgebung(report_t *r, map_t *map, map_entry_t *e, int x, int y)
{
    get_report_line(r);
    while (!r->eof) {
	if (!strncmp(r->argv[0], "EINHEIT", 7)) {
	    parse_einheit(r, map, x, y);
	} else if (!strcmp(r->argv[0], "REGIONSEREIGNISSE")) {
	    parse_meldungen(r, map, &e->first_ereignis);
	} else if (!strcmp(r->argv[0], "REGIONSBOTSCHAFTEN")) {
	    parse_meldungen(r, map, &e->first_botschaft);
	} else if (!strcmp(r->argv[0], "REGIONSKOMMENTAR")) {
	    parse_meldungen(r, map, &e->first_kommentare);
	} else if (!strncmp(r->argv[0], "DURCHREISEREGION", 16)) {
	    parse_region(r, map, 17);
	} else if (!strncmp(r->argv[0], "BURG ", 5)) {
	    parse_burg(r, map, e);
	} else if (!strncmp(r->argv[0], "SCHIFF ", 7)) {
	    parse_schiff(r, map, e);
	} else if (r->argc == 1 && r->is_string[0]) {
	    parse_string_in_umgebung(r, map, e);
	} else if (!strcmp(r->argv[0], "UMGEBUNG") ||
		   !strncmp(r->argv[0], "REGION ", 7)) {
	    break;
	} else {
	    printf("skip %s in UMGEBUNG\n", r->argv[0]);
	    get_report_line(r);
	}
    }
}


void parse_grenze(report_t *r, map_entry_t *e)
{
    int nr;

    sscanf(r->argv[0], "GRENZE %d", &nr);
    get_report_line(r);    /* skip "GRENZE %d" */

    while (!r->eof && !(!r->is_string[0] && isupper(r->argv[0][0]))) {
        get_report_line(r);
    }
}


void parse_effects(report_t *r, map_entry_t *e)
{
    meldung_t *m, **mp = &e->first_effect;

    get_report_line(r);    /* skip "EFFECTS" */

    while (!r->eof && r->is_string[0]) {
	m = xmalloc(sizeof(meldung_t));
        *mp = m;
        mp = &m->next;
	m->next = NULL;
        m->zeile = xstrdup(r->argv[0]);
        get_report_line(r);
    }
}


void parse_region(report_t *r, map_t *map, int offset)
{
    map_entry_t *e;
    int         x, y;
    int         i;

    sscanf(r->argv[0] + offset, "%d %d", &x, &y);

    e = mp(map, x, y);
    if (e == NULL)
	e = add_to_map(map, x, y);

    get_report_line(r);		/* skip "REGION %d %d" */
				/* or   "DURCHREISEREGION %d %d" */
    while (!r->eof) {
	if (r->argc == 2 && !strcmp(r->argv[1], "Name")) {
	    set_region_name(map, x, y, r->argv[0]);
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "Beschr")) {
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
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "Terrain")) {
	    set_region_typ(map, x, y, r->argv[0]);
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "Baeume")) {
	    e->baeume = atoi(r->argv[0]);
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "Bauern")) {
	    e->bauern = atoi(r->argv[0]);
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "Silber")) {
	    e->silber = atoi(r->argv[0]);
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "Eisen")) {
	    e->eisen = atoi(r->argv[0]);
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
	} else if (r->argc == 2 && !strcmp(r->argv[1], "TNorden")) {
	    set_region_typ(map, x, y - 1, r->argv[0]);
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "TOsten")) {
	    set_region_typ(map, x + 1, y, r->argv[0]);
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "TSueden")) {
	    set_region_typ(map, x, y + 1, r->argv[0]);
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "TWesten")) {
	    set_region_typ(map, x - 1, y, r->argv[0]);
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "NNorden")) {
	    set_region_name(map, x, y - 1, r->argv[0]);
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "NOsten")) {
	    set_region_name(map, x + 1, y, r->argv[0]);
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "NSueden")) {
	    set_region_name(map, x, y + 1, r->argv[0]);
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "NWesten")) {
	    set_region_name(map, x - 1, y, r->argv[0]);
	    get_report_line(r);
	} else if (!strcmp(r->argv[0], "PREISE")) {
	    parse_preise(r, map, e);
	} else if (!strncmp(r->argv[0], "EINHEIT", 7)) {
	    parse_einheit(r, map, x, y);
	} else if (!strncmp(r->argv[0], "SCHIFF ", 7)) {
	    parse_schiff(r, map, e);
	} else if (!strncmp(r->argv[0], "BURG ", 5)) {
	    parse_burg(r, map, e);
	} else if (!strncmp(r->argv[0], "MESSAGE ", 8)) {
	    parse_message(r, map, &e->first_ereignis);
	} else if (!strncmp(r->argv[0], "GRENZE ", 7)) {
	    parse_grenze(r, e);
	} else if (!strcmp(r->argv[0], "EFFECTS")) {
	    parse_effects(r, e);
	} else if (!strcmp(r->argv[0], "REGIONSEREIGNISSE")) {
	    parse_meldungen(r, map, &e->first_ereignis);
	} else if (!strcmp(r->argv[0], "DURCHREISE")) {
	    parse_meldungen(r, map, &e->first_durchreise);
	} else if (!strcmp(r->argv[0], "REGIONSBOTSCHAFTEN")) {
	    parse_meldungen(r, map, &e->first_botschaft);
	} else if (!strcmp(r->argv[0], "REGIONSKOMMENTAR")) {
	    parse_meldungen(r, map, &e->first_kommentare);
	} else if (!strncmp(r->argv[0], "DURCHREISEREGION", 16)) {
	    parse_region(r, map, 17);
	} else if (!strncmp(r->argv[0], "UMGEBUNG", 8)) {
	    parse_umgebung(r, map, e, x, y);
	} else if (!strncmp(r->argv[0], "REGION", 6) ||
		   !strncmp(r->argv[0], "DURCHREISEREGION", 16) ||
		   !strcmp(r->argv[0], "MESSAGETYPES")) {
	    break;
	} else {
#if WARNTAGS
	    printf("line %d, unknown content in region:\n", r->lnr);
	    for (i = 0; i < r->argc; i++) {
		printf("[%d] = \"%s\"\n", i, r->argv[i]);
	    }
#endif
	    break;
	}
    }
    if (e->luxusgueter == 0)
	e->luxusgueter = e->bauern / 100;
    /*@@@ ... unterhaltung */
    /*@@@ ... rekrutierung */
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
	z->next = p->zauber;
	p->zauber = z;
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
    /* int nr = atoi(r->argv[0] + 7); */
    
    get_report_line(r);		/* skip ZAUBER */

    while (!r->eof) {
	if (r->argc == 2 && !strcmp(r->argv[1], "name")) {
            get_report_line(r);
        } else if (r->argc == 2 && !strcmp(r->argv[1], "type")) {
            get_report_line(r);
        } else if (r->argc == 2 && !strcmp(r->argv[1], "level")) {
            get_report_line(r);
        } else if (r->argc == 2 && !strcmp(r->argv[1], "rank")) {
            get_report_line(r);
        } else if (r->argc == 2 && !strcmp(r->argv[1], "info")) {
            get_report_line(r);
        } else if (r->argc == 2 && !strcmp(r->argv[1], "class")) {
            get_report_line(r);
        } else if (r->argc == 2 && !strcmp(r->argv[1], "familiar")) {
            get_report_line(r);
        } else if (r->argc == 2 && !strcmp(r->argv[1], "ship")) {
            get_report_line(r);
        } else if (!strcmp(r->argv[0], "KOMPONENTEN")) {
            get_report_line(r); /* skip KOMPONENTEN */
            while (!r->eof && !(!r->is_string[0] && isupper(r->argv[0][0]))) {
                get_report_line(r);
            }
        } else {
            break;
        }
    }
}

static struct {
    int dx;
    int dy;
} richtung[] = {
    { -1,  1 },			/* nordwest */
    {  0,  1 },			/* nordost */
    {  1,  0 },			/* ost */
    {  1, -1 },			/* südost */
    {  0, -1 },			/* südwest */
    { -1,  0 }			/* west */
};

void flood_fill(map_t *map, map_entry_t *e)
{
    map_entry_t *en;
    int         i;

    for (i = 0; i < 6; i++) {
	en = mp(map, e->x + richtung[i].dx, e->y + richtung[i].dy);
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

void insel_fill(map_t *map)
{
    map_entry_t *e;
    int x, y;

    for (y = map->min_y; y <= map->max_y; y++)
	for (x = map->min_x; x <= map->max_x; x++) {
	    e = mp(map, x, y);
	    if (e == NULL || e->insel == NULL)
		continue;
	    flood_fill(map, e);
	}

}


/*
 * Eine Partei (eigene oder andere)
 */
void parse_partei(report_t *r, map_t *map)
{
    int      i;
    int      passwort = 0;
    partei_t *p = NULL;		/* eigene Partei */

    p = finde_partei(map, atoi(r->argv[0] + 6));
    get_report_line(r);         /* überspringe PARTEI xxx */

    while (!r->eof) {
#if 0
	if (r->argv[0] != NULL)
	    printf("prolog, %d: argv[0] = \"%s\"", r->lnr, r->argv[0]);
	if (r->argc == 2 && r->argv[1] != NULL)
	    printf(", argv[1] = \"%s\"\n", r->argv[1]);
	else
	    printf("\n");
#endif
	if (r->argc == 2 && !strcmp(r->argv[1], "Passwort")) {
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
	    parse_optionen(r, map, p);
	} else if (!strcmp(r->argv[0], "MELDUNGEN")) {
	    parse_meldungen(r, map, &p->meldungen);
	} else if (!strcmp(r->argv[0], "EREIGNISSE")) {
	    parse_meldungen(r, map, &p->ereignisse);
	} else if (!strcmp(r->argv[0], "HANDEL")) {
	    parse_meldungen(r, map, &p->handel);
	} else if (!strcmp(r->argv[0], "ALLIIERTE")) {
	    parse_alliierte(r, map);
	} else if (!strncmp(r->argv[0], "ALLIANZ ", 8)) {
	    parse_allianz(r, map);
	} else if (!strcmp(r->argv[0], "EINKOMMEN")) {
	    parse_meldungen(r, map, &p->einkommen);
	} else if (!strcmp(r->argv[0], "PRODUKTION")) {
	    parse_meldungen(r, map, &p->produktion);
	} else if (!strcmp(r->argv[0], "BEWEGUNGEN")) {
	    parse_meldungen(r, map, &p->bewegungen);
	} else if (!strcmp(r->argv[0], "TRAENKE")) {
	    parse_meldungen(r, map, &p->traenke);
	} else if (!strcmp(r->argv[0], "ALLIIERTE")) {
	    parse_alliierte(r, map);
	} else if (!strcmp(r->argv[0], "KAEMPFE")) {
	    parse_meldungen(r, map, &p->kaempfe);
	} else if (!strcmp(r->argv[0], "ZAUBER")) {
	    parse_zauber_alt(r, p);
	} else if (!strncmp(r->argv[0], "ZAUBER ", 7)) {
	    parse_zauber_neu(r, p);
	} else if (!strncmp(r->argv[0], "MESSAGE ", 8)) {
	    parse_message(r, map, &p->meldungen);
	} else if (!strncmp(r->argv[0], "BATTLE ", 7)) {
	    parse_battle(r, map, &p->kaempfe);
	} else  if (isupper(r->argv[0][0])) {
	    break;
	} else {
	    printf("report, Zeile %d, unbekannt in Prolog:\n", r->lnr);
	    for (i = 0; i < r->argc; i++) {
		printf("[%d] = \"%s\"\n", i, r->argv[i]);
	    }
	    get_report_line(r);
	}
    }

    if (passwort != 0) {
        if (map->partei <= 0)
            map->partei = p->nummer;
        printf("partei %d\n", p->nummer);
    }
}



/*
 * Einen Trank
 */
void parse_trank(report_t *r, map_t *map)
{
    get_report_line(r);         /* überspringe TRANK xxx */

    while (!r->eof) {
	if (r->argc == 2 && !strcmp(r->argv[1], "Name")) {
            get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "Beschr")) {
            get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "Stufe")) {
            get_report_line(r);
	} else if (!strcmp(r->argv[0], "ZUTATEN")) {
            get_report_line(r); /* überspringe ZUTATEN */
            while (!r->eof && r->is_string[0]) {
                get_report_line(r);
            }
        } else {
            break;
        }
    }
}


void parse_report(char *filename, map_t *map)
{
    report_t *r;
    int      i;
    int      version;

    map->partei = -1;
    r = make_report(filename);
    get_report_line(r);		/* erste Zeile holen */

    while (!r->eof) {
	if (!strncmp(r->argv[0], "VERSION ", 8)) {
	    version = atoi(r->argv[0] + 8);
	    printf("Version des Reports: %d\n", version);
	    if (version > 48)
		printf("Warnung: Reportversion höher als bekannt!\n");
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "Spiel")) {
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "Konfiguration")) {
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "Koordinaten")) {
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "Basis")) {
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "Umlaute")) {
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "Zeitalter")) {
	    get_report_line(r);
	} else if (r->argc == 2 && !strcmp(r->argv[1], "Runde")) {
	    map->runde = atoi(r->argv[0]);
	    get_report_line(r);
        } else if (!strncmp(r->argv[0], "PARTEI ", 7)) {
	    parse_partei(r, map);
        } else if (!strcmp(r->argv[0], "UEBERSETZUNG")) {
	    get_report_line(r);
	} else if (!strcmp(r->argv[0], "FEHLER")) {
	    parse_fehler(r, map);
	} else if (!strcmp(r->argv[0], "ADRESSEN")) {
	    parse_adressen(r, map);
        } else if (!strncmp(r->argv[0], "REGION", 6)) {
	    parse_region(r, map, 7);
        } else if (!strncmp(r->argv[0], "TRANK", 5)) {
	    parse_trank(r, map);
	} else if (!strncmp(r->argv[0], "DURCHREISEREGION", 16)) {
	    parse_region(r, map, 17);
	} else if (!strcmp(r->argv[0], "MESSAGETYPES")) {
	    parse_messagetypes(r, map);
	} else {
#if WARNTAGS
	    printf("report, line %d, unknown content:\n", r->lnr);
	    for (i = 0; i < r->argc; i++) {
		printf("[%d] = \"%s\"\n", i, r->argv[i]);
	    }
#endif
	    get_report_line(r);
	}
    }
    destroy_report(r);
    insel_fill(map);
}


