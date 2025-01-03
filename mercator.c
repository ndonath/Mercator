/* $Id: mercator.c,v 1.16 2006/02/12 15:54:04 schwarze Exp $ */
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
#include "mercator.h"

const int CRVERSION = 69;   /* hoechste bekannte CR-Version */
const char *const MVERSION = "0";
const int MYEAR = 2006;     /* Jahr des Copyright */
config_t config;   /* globale Konfigurationsdaten */

void get_arg_string(int *argc, char *argv[], char **dest)
{
    if (argv[(*argc)+1] == NULL) {
        fprintf(stderr,
          "Fehler in get_arg_string: %s benötigt ein Argument\n",
          argv[*argc]);
        exit(1);
    }
    *dest = argv[(*argc)+1];
    *argc += 2;
}

void get_arg_int(int *argc, char *argv[], unsigned *dest)
{
    if (argv[(*argc)+1] == NULL) {
        fprintf(stderr,
          "Fehler in get_arg_string: %s benötigt ein Argument\n",
          argv[*argc]);
        exit(1);
    }
    sscanf(argv[(*argc)+1], "%u", dest);
    *argc += 2;
}


void read_cr(int *argc, char *argv[], map_t **pmap)
{
    map_t *map = *pmap;
    int   i = *argc;

    if (argv[i+1] == NULL) {
	fprintf(stderr, "Fehler bei %s: Dateiname fehlt.\n", argv[i]);
	exit(1);
    }
    if (!strcmp("read-cr", argv[i]) || !strcmp("-rc", argv[i])) {
	if (map != NULL) {
	  if (config.verbose > 1) fprintf(stdout,
            "Info zu read-cr: Lösche die alte Karte.\n");
	  destroy_map(map);
	}
	map = make_map(REGION_HASH_SIZE, ROW_MULT, EINHEIT_HASH_SIZE);
    } else { /* merge-cr */
	if (map == NULL) {
	  fprintf(stderr,
            "Fehler bei merge-cr: Keine alte Karte vorhanden.\n");
	  exit(1);
	}
	purge_map(map);
    }
    if (config.verbose > 1) fprintf(stdout,
      "Info zu %s: Lese %s...\n", argv[i], argv[i+1]);
    i++;
    parse_report(argv[i], map);
    i++;

    *pmap = map;

    *argc = i;
}


void write_map(int *argc, char *argv[], map_t *map)
{
    int i = *argc;
    
    i++;			/* skip "write-map" */

    if (argv[i] == NULL) {
	fprintf(stderr, "Fehler bei write-map: Dateiname fehlt.\n");
	exit(1);
    }

    write_cr_map(map, argv[i]);
    i++;

    *argc = i;
}

void write_html(int *argc, char *argv[], map_t *map)
{
    int  i = *argc;

    i++;			/* skip "write-html" */
    if (argv[i] == NULL) {
	fprintf(stderr, "Fehler bei write_html: Verzeichnisname fehlt.\n");
	exit(1);
    }
    if (config.verbose > 1) {
      fprintf(stdout,
        "Info zu write-html: Schreibe %s/...\n", argv[i]);
      fflush(stdout);
    }
    write_html_map(map, argv[i], 18, 20);
    i++;

    *argc = i;
}



int main(int argc, char *argv[])
{
    map_t *map = NULL;
    int   i;

    fprintf(stdout,
      "Mercator, Version %d.%s   (C) %d Ingo Schwarze (GPL)\n",
      CRVERSION, MVERSION, MYEAR);
    if (argc == 1)
    { 
      fprintf(stderr, "README lesen hilft!\n");
    
      fprintf(stderr, "Optionen:\n");
      fprintf(stderr, " -p   lib-path    Resourcenpfad einstellen\n");
      fprintf(stderr, " -st  style       Bilderstil einstellen\n");
      fprintf(stderr, " -sc  scale       Verkleinerungsfaktor einstellen\n");
      fprintf(stderr, " -se  select      Regionenfilter einstellen\n");
      fprintf(stderr, " -rc  read-cr     CR einlesen\n");
      fprintf(stderr, " -mc  merge-cr    CR hinzufuegen\n");
      fprintf(stderr, " -m   move        Karte verschieben\n");
      fprintf(stderr, " -v   verbose     Geschwaetzigkeit einstellen\n");
      fprintf(stderr, " -wh  write-html  HTML Ausgabe im angegebenen Verzeichnis erzeugen\n");
      fprintf(stderr, " -o   write-cr    CR schreiben\n");
      fprintf(stderr, " -wp  write-png   Uebersichtskarte als png Datei erzeugen\n");
    }

#ifdef WIN32
    config.path = NULL
#else
    config.path = CONFIGPATH;
#endif
    config.style = "minimal";
    config.scale = 1;
    config.region_flags = 119;
    config.verbose = 2;

    i = 1;
    while (i < argc) {
        if (!strcmp("lib-path", argv[i]) ||
            !strcmp("-p", argv[i])) {
            get_arg_string(&i, argv, &config.path);
        } else if (!strcmp("select", argv[i]) ||
                   !strcmp("-se", argv[i])) {
            get_arg_int(&i, argv, &config.region_flags);
        } else if (!strcmp("style", argv[i]) ||
                   !strcmp("-st", argv[i])) {
            get_arg_string(&i, argv, &config.style);
        } else if (!strcmp("verbose", argv[i]) ||
                   !strcmp("-v", argv[i])) {
            get_arg_int(&i, argv, &config.verbose);
        } else if (!strcmp("read-cr", argv[i]) ||
                   !strcmp("merge-cr", argv[i])||
                   !strcmp("-rc", argv[i]) ||
                   !strcmp("-mc", argv[i])) {
	    read_cr(&i, argv, &map);
	} else if (!strcmp("move", argv[i]) ||
               !strcmp("-m", argv[i])) {
	    move_map(&i, argv, map);
        } else if (!strcmp("scale", argv[i]) ||
                   !strcmp("-sc", argv[i])) {
            get_arg_int(&i, argv, &config.scale);
	} else if (!strcmp("write-html", argv[i]) ||
                   !strcmp("-wh",        argv[i])    ) {
	    if (map == NULL) {
		fprintf(stderr,
                  "Fehler bei write-html: Erst lesen, dann schreiben.\n");
		exit(1);
	    }
	    write_html(&i, argv, map);
	} else if (!strcmp("write-map", argv[i]) ||
              (!strcmp("write-cr", argv[i])) ||
              (!strcmp("-o", argv[i]))) {
	    if (map == NULL) {
		fprintf(stderr,
                  "Fehler bei write-map: Erst lesen, dann schreiben.\n");
		exit(1);
	    }
	    write_map(&i, argv, map);
	} else if (!strcmp("write-png", argv[i]) ||
                   !strcmp("-wp", argv[i])          ) {
	    if (map == NULL) {
		fprintf(stderr,
                  "Fehler bei write-png: Erst lesen, dann schreiben.\n");
		exit(1);
	    }
	    write_png(&i, argv, map);
	} else {
	    fprintf(stderr,
              "Fehler: unbekanntes Kommando '%s'.\n", argv[i]);
	    exit(1);
	}	    
    }

    if (map != NULL)
      destroy_map(map);

    check_mem();
    return 0;
}
