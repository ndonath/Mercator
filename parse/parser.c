/* $Id: parser.c,v 1.14 2002/06/23 18:39:32 marko Exp $ */
#include <assert.h>
#include <time.h>
#include "parser.h"

extern int CRVERSION;

report_t *make_report(char *filename)
{
    report_t *r;

    r = xmalloc(sizeof(report_t));
    r->fp = xfopen(filename, "r");
    r->lnr = 0;
    r->eof = 0;

    return r;
}

static void destroy_report(report_t *r)
{
    fclose(r->fp);
    xfree(r);
}

void get_report_line(report_t *r)
{
    char *s;
    int i;
    char tmp[REPORT_LINE_LEN];

    if (fgets(r->line, REPORT_LINE_LEN, r->fp) == NULL) {
      r->eof = 1;
      return;
    }
    if (r->line[0] == 0x3f) {
      /* remove UTF-8 endianess indicator */
      strcpy(tmp, r->line + 1);
      strcpy(r->line, tmp);
    }
    s = r->line;
    r->is_block = 0;
    r->is_subblock = 0;
    r->argc = 0;
    while (*s != '\r' && *s != '\n' && *s != 0) {
      if (*s == '"') {
          s++;        /* skip start " */
          r->argv[r->argc] = s;
          while (*s != '"' && *s != 0) {
          /*
           * Eigentlich sollte es \n in Strings nicht geben, aber
           * manche Mailer zerhacken gerne Zeilen...
           */
            if (*s != '\r' && *s != '\n')
                s++;    /* search end " */
            else {
              if (fgets(s + 1,
                    REPORT_LINE_LEN - (1 + s - r->argv[r->argc]),
                    r->fp) == NULL) {
              fprintf(stderr, "unerwartetes Dateiende in Zeile %d\n", r->lnr);
              r->eof = 1;
              return;
              }
              r->lnr++;
              s++;
            }
          }
          assert(*s == '"');
          *s++ = 0;        /* cut string */
          if (*s == ';')
            s++;        /* skip ; */
          r->is_string[r->argc] = 1;
          r->argc++;
      } else {
          r->argv[r->argc] = s;
          if ((r->argc == 0) && (*s >= 'A') && (*s <= 'Z')) {
            r->is_block = 1;
          }
          while (*s != ';' && *s != '\r' && *s != '\n' && *s != 0)
            s++;
          if (*s != 0)
            *s++ = 0;
          r->is_string[r->argc] = 0;
          r->argc++;
      }
    }
    r->lnr++;

    if ((r->is_block == 1) && (strstr(r->line, " ") == NULL))
      r->is_subblock = 1;
    for (i=r->argc; i<REPORT_ARGS; i++)
      r->argv[i] = NULL;
    return;
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
        break;        /* end of "ADRESSEN" */
    }
    }
}

/* Fehlermeldungen des Parsers ausgeben */
void parsefehler(report_t *r, char *block)
{
#    ifdef WARNTAGS
   if (r->argc == 2)
     printf("[%d] skip %s;%s in %s\n", r->lnr, r->argv[0], r->argv[1], block);
   else
     printf("[%d] skip %s in %s\n", r->lnr, r->argv[0], block);
#    endif
}


/*
 * Optionen überspringen...
 */
void parse_optionen(report_t *r, partei_t *p)
{

    assert(p != NULL);
    get_report_line(r);        /* skip "OPTIONEN" (oder ähnlich) */
    while (!r->eof && !isupper(r->argv[0][0])) {
    get_report_line(r);
    }
}

/* unbekannte Bloecke parsen */
void parse_unbekannt(report_t *r)
{
#if WARNTAGS
    printf("[%d] Unbekannter Block: %s\n", r->lnr, r->line);
#endif
    do
      get_report_line(r);
    while (r->is_block == 0);
}

void parse_fehler(report_t *r)
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


/* Eine Liste mit Meldungen zu einer lpmeldungs Liste hinzufuegen */
static void  add_messages_to_list(map_t **map, meldung_t *msg,
             const int x, const int y, const int z)
{
  lpmeldung_t *list_start, *list_current;
  
  list_start = list_current = xmalloc(sizeof(lpmeldung_t));
  list_current->pmeldung = msg;
  list_current->next = NULL;
  list_current->x = x;
  list_current->y = y;
  list_current->z = z;

  while (msg->next != NULL)
  {
    msg = msg->next;
    list_current->next = xmalloc(sizeof(lpmeldung_t));
    list_current = list_current->next;
    list_current->pmeldung = msg;
    list_current->x = x;
    list_current->y = y;
    list_current->z = z;
    list_current->next = NULL;
  }
  list_current->next = (*map)->regionen_msg;
  (*map)->regionen_msg = list_start;
}

/* Nachbearbeitung des vollstaendig eingelesenen Reportes. Dazu werden 
   alle Regionen noch einmal durchlaufen */
static void post_parse(map_t *map)
{
    map_entry_t *e;
    register int x;
    int y;
    ebene_t *eb;

    for (eb = map->ebenen; eb != NULL; eb=eb->next) {
      for (y = eb->min_y; y <= eb->max_y; y++) {
        for (x = eb->min_x; x <= eb->max_x; x++) {
          e = mp(map, x, y, eb->koord);
          if (e == NULL)
            continue;
          /* Inselnamen setzen */
          if (e->insel != NULL)
            flood_fill(map, e);
          /* Alle Regionsereignisse in eine Liste schreiben */
          if (e->first_ereignis != NULL)
            add_messages_to_list(&map, e->first_ereignis, e->x, e->y, e->z);
       }
     }
    }
}


void parse_report(char *filename, map_t *map)
{
    report_t *r;
    int parteien = 0; /* Zaehler fuer die Menge der Parteien */

    map->partei = -1;
    r = make_report(filename);
    get_report_line(r);        /* erste Zeile holen */

    while (!r->eof)
    {
      if (!strncmp(r->argv[0], "VERSION ", 8)) {
        map->version = atoi(r->argv[0] + 8);
#if WARNTAGS
        printf("Version des Reports: %d\n", map->version);
        if (map->version > CRVERSION)
          printf("Warnung: Reportversion höher als bekannt!\n");
#endif
        get_report_line(r);
      } else if (r->argc == 2 && !strcmp(r->argv[1], "locale")) {
         if (strcmp(r->argv[0], "en") == 0)
           map->locale = en;
         else if (strcmp(r->argv[0], "de") == 0)
           map->locale = de;
         else
           map->locale = de;
        get_report_line(r);        
      } else if (r->argc == 2 && !strcmp(r->argv[1], "noskillpoints")) {
        map->noskillpoints = atoi(r->argv[0]);
        get_report_line(r);        
      } else if (r->argc == 2 && !strcmp(r->argv[1], "date")) {
#if WARNTAGS
        time_t u_time = atoi(r->argv[0]);
        struct tm *t = gmtime(&u_time);
        printf("Report vom %d.%d.%d  %2d:%2d:%2d Uhr\n", t->tm_mday, t->tm_mon+1,
                        t->tm_year+1900, t->tm_hour, t->tm_min, t->tm_sec);
#endif
          get_report_line(r);
      } else if (r->argc == 2 && !strcmp(r->argv[1], "Spiel")) {
#if WARNTAGS
	if ((strcmp(r->argv[0], "Eressea") != 0) && (strcmp(r->argv[0], "E3") != 0))
            printf("Warnung: Nur Eressea Reporte werden unterstuetzt!\n");
#endif
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
      } else if (r->argc == 2 && !strcmp(r->argv[1], "mailto")) {
          get_report_line(r);
      } else if (r->argc == 2 && !strcmp(r->argv[1], "mailcmd")) {
          get_report_line(r);
      } else if (!strncmp(r->argv[0], "PARTEI ", 7)) {
          parteien++;
          parse_partei(r, map, parteien);
      } else if (!strcmp(r->argv[0], "UEBERSETZUNG")) {
          get_report_line(r);
      } else if (!strcmp(r->argv[0], "FEHLER")) {
          parse_fehler(r);
      } else if (!strcmp(r->argv[0], "ADRESSEN")) {
          parse_adressen(r, map);
      } else if (!strncmp(r->argv[0], "REGION", 6)) {
          parse_region(r, map, 7);
      } else if (!strncmp(r->argv[0], "DURCHREISEREGION", 16)) {
          parse_region(r, map, 17);
      } else if (!strcmp(r->argv[0], "MESSAGETYPES")) {
          parse_messagetypes(r);
      } else if (!strncmp(r->argv[0], "MESSAGETYPE ", 12)) {
          parse_messagetypes(r);
      } else if (!strcmp(r->argv[0], "TRANSLATION")) { /* vorlaeufig (quick-hack) */
          parse_messagetypes(r);
      } else if (r->argc == 2 && !strcmp(r->argv[1], "charset")) {
          get_report_line(r);
      } else if (r->argc == 2 && !strcmp(r->argv[1], "Build")) {
          get_report_line(r);
      } else if (r->argc == 2 && !strcmp(r->argv[1], "max_units")) {
          get_report_line(r);
      } else {
#if WARNTAGS
        int      i;
        printf("report, line %d, unknown content:\n", r->lnr);
        for (i = 0; i < r->argc; i++) {
        printf("[%d] = \"%s\"\n", i, r->argv[i]);
      }
#endif
        get_report_line(r);
    }
  }
  destroy_report(r);
  post_parse(map);

 /* Meldungen in die richtigen Kategorien unterteilen */
  sortiere_meldungen(map);
}
