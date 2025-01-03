/* $Id: language.c,v 1.19 2006/02/08 01:45:36 schwarze Exp $ */
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

#include "mercator.h"

/* Deutsch per Default */

/* moegliche Zauberarten */
const char *zauberart[] = {
  "Pr&auml;kampfzauber",
  "Kampfzauber",
  "Postkampfzauber",
  "normaler Zauber",
};

/* Zaubermodifikationen */
const char *zaubermod[] = {
    "Schiffszauber",
    "Fernzauber"
};

/* Kampfstati */
const char *kampfstatus[] = {
  "aggressiv",
  "vorne",
  "hinten",
  "defensiv",
  "kämpft nicht",
  "flieht"
};

const char *kuestenname[] = {
    "undefinierter Küste",
    "Nordwestküste",
    "Nordostküste",
    "Ostküste",
    "Südostküste",
    "Südwestküste",
    "Westküste"
};

const char *monatsnamen[] = {
    "Feldsegen",
    "Nebeltage",
    "Sturmmond",
    "Herdfeuer",
    "Eiswind",
    "Schneebann",
    "Blütenregen",
    "Mond der milden Winde",
    "Sonnenfeuer"
};


/* Woerterbuch mit den Uebersetzungen der Schluesselworte */
static const char * const woerterbuch[][2] = {
{"Adressen", "addresses"},
{"Allianzen", "alliances"},
{"Alter", "age"},
{"Anzahl", "count"},
{"Arbeitspl&auml;tze", "workers"},
{"Auswertung", "report"},
{"Bauern", "peasants"},
{"Bauernlohn", "peasant's wage"},
{"B&auml;ume", "trees"},
{"Bäume", "trees"},
{"Befestigung", "fortification"},
{"bekommt keine Hilfe", "unaided"},
{"belagert", "besieges"},
{"Bewache", "guard"},
{"bewacht", "guards"},
{"Bewegungen", "movements"},
{"Botschaften", "messages"},
{"Burg", "stronghold"},
{"Burgherr", "lord of the castle"},
{"bz2", "bz2"},
{"bzip2", "bzip2"},
{"Computer", "computer"},
{"Debug", "debug"},
{"Durchreise", "passed by"},
{"Durchschiffung", "passed ships"},
{"Einheiten", "units"},
{"Einkommen", "income"},
{"Eisen", "iron"},
{"Ereignisse", "events"},
{"Fertigstellung", "completion"},
{"Festung", "fortress"},
{"fremde Parteien", "foreign factions"},
{"Gib", "give"},
{"Gr&ouml;&szlig;e", "size"},
{"Grundmauern", "foundation"},
{"Gruppe", "group"},
{"Gruppen", "groups"},
{"Handel", "trade"},
{"Handelsposten", "trade centre"},
{"Held", "hero"},
{"Helden", "heroes"},
{"hungert", "starves"},
{"Kampf in", "battle in"},
{"K&auml;mpfe", "battles"},
{"k&auml;mpfe", "combat"},
{"Kampfzauber", "combat spell"},
{"Kapit&auml;n", "captain"},
{"keiner", "none"},
{"Kommentar", "comment"},
{"Kommentare", "comments"},
{"Komponenten", "components"},
{"Kraut", "herb"},
{"Lehren und Lernen", "teaching und learning"},
{"Lohn", "wage"},
{"Luxusg&uuml;ter", "luxuries"},
{"Magie und Artefakte", "magic and artefacts"},
{"Markt", "market"},
{"Materialpool", "item pool"},
{"Meldungen und Ereignisse", "messages and events"},
{"nada", "nothing"},
{"Name", "name"},
{"normaler Zauber","normal spell"},
{"Optionen", "options"},
{"Ozean", "ocean"},
{"Partei", "faction"},
{"Parteien", "factions"},
{"parteigetarnt", "disguised"},
{"Parteitarnung", "faction-stealth"},
{"Personen", "people"},
{"Pers.", "people"},
{"Pferde", "horses"},
{"Postkampfzauber","post-combat spell"},
{"Pr&auml;kampfzauber","pre-combat spell"},
{"Produktion", "production"},
{"Punkte", "score"},
{"Punktedurchschnitt", "average"},
{"rar", "rar"},
{"Region", "region"},
{"Regionsmeldungen", "region messages"},
{"Regionszauber", "region spells"},
{"Reichsschatz", "treasury"},
{"Reichweite", "range"},
{"Reisen und Bewegung", "travel and movements"},
{"Rekrutierung", "recruitments"},
{"Rohstoffe und Produktion", "resources and production"},
{"Runde", "turn"},
{"Schaden", "damage"},
{"Silber", "silver"},
{"Silberpool", "silver pool"},
{"Sonstiges", "other messages"},
{"Spion", "spy"},
{"Statistik", "statistics"},
{"Status", "status"},
{"Steine", "stones"},
{"Steuereintreiben", "taxation"},
{"Straße", "road"},
{"Stufe", "level"},
{"Tage", "days"},
{"Tage insgesamt", "days total"},
{"Talentverschiebungen", "skill changes"},
{"tgz", "tgz"},
{"Tr&auml;nke", "potions"},
{"Turm", "tower"},
{"txt", "txt"},
{"&Uuml;bersicht", "overview"},
{"&Uuml;berschu&szlig;", "surplus"},
{"Umgebung", "surroundings"},
{"unbekannt", "unknown"},
{"Unterhaltung", "entertainment"},
{"Verr&auml;ter", "traitor"},
{"Verschiedenes", "miscellaneous messages"},
{"von max.", "of max."},
{"Wahrnehmung (deaktiviert)", "perception (deactivated)"},
{"Warnungen und Fehler", "warnings and errors"},
{"Weltkarte", "world map"},
{"Wirtschaft und Handel", "economy and trade"},
{"Wochen", "weeks"},
{"Zauber", "spells"},
{"Zeitung", "journal"},
{"zip", "zip"},
{"Zipped", "zipped"},
{"Zitadelle", "citadel"},
{"Zugvorlage", "template"}
};

/* Stuktueren mit Zeichenketten; definiert in html.c */
extern burgi_t burgen[];
extern helfe_info_t helfe_info[];
const char *option_info[18];


/* Gib die Uebersetzung von einem bestimmten Wort zurueck */
const char *translate(const char *deutsch, locale_e sprache)
{
  extern config_t config;
  int i;
  if (sprache == de)
    return deutsch;
  else
    for (i=0; i<sizeof(woerterbuch)/sizeof(woerterbuch[0]); i++)
    {
      if (strcmp(deutsch, woerterbuch[i][0]) == 0)
        return woerterbuch[i][sprache];
    }

  if (config.verbose > 0) fprintf(stderr,
    "Warnung in translate: Keine Uebersetzung fuer '%s' gefunden.\n",
    deutsch);
  return deutsch;
}



/* Die Schluesselwoerter uebersetzen, die in der
   HTML Ausgabe verwendet werden */
void set_global_names (locale_e locale, game_e game)
{
  extern config_t config;

  burgen[0].typ = translate("nada", locale);
  burgen[1].typ = translate("Grundmauern", locale);
  burgen[2].typ = translate("Handelsposten", locale);
  burgen[3].typ = translate("Befestigung", locale);
  burgen[4].typ = translate("Turm", locale);
  burgen[5].typ = translate("Burg", locale);
  burgen[6].typ = translate("Festung", locale);
  burgen[7].typ = translate("Zitadelle", locale);

  helfe_info[0].text = translate("Silber", locale);
  helfe_info[1].text = translate("k&auml;mpfe", locale);
  helfe_info[2].text = translate("Wahrnehmung (deaktiviert)", locale);
  helfe_info[3].text = translate("Gib", locale);
  helfe_info[4].text = translate("Bewache", locale);
  helfe_info[5].text = translate("Parteitarnung", locale);
  helfe_info[6].text = NULL;

  if (game == game_eressea) {
    option_info[0] = translate("Auswertung", locale);
    option_info[1] = translate("Computer", locale);
    option_info[2] = translate("Zugvorlage", locale);
    option_info[3] = translate("Silberpool", locale);
    option_info[4] = translate("Statistik", locale);
    option_info[5] = translate("Debug", locale);
    option_info[6] = translate("Zipped", locale);
    option_info[7] = translate("Zeitung", locale);
    option_info[8] = translate("Materialpool", locale);
    option_info[9] = translate("Adressen", locale);
    option_info[10] = translate("bzip2", locale);
    option_info[11] = translate("Punkte", locale);
    option_info[12] = translate("Talentverschiebungen", locale);
    option_info[13] = NULL;
  } else if (game == game_gav62mod) {
    option_info[0] = translate("Auswertung", locale);
    option_info[1] = translate("Computer", locale);
    option_info[2] = translate("Zeitung", locale);
    option_info[3] = translate("Kommentar", locale);
    option_info[4] = translate("Statistik", locale);
    option_info[5] = translate("Debug", locale);
    option_info[6] = translate("zip", locale);
    option_info[7] = translate("rar", locale);
    option_info[8] = translate("bz2", locale);
    option_info[9] = translate("tgz", locale);
    option_info[10] = translate("txt", locale);
    option_info[11] = translate("Verschiedenes", locale);
    option_info[12] = translate("Einkommen", locale);
    option_info[13] = translate("Handel", locale);
    option_info[14] = translate("Produktion", locale);
    option_info[15] = translate("Bewegungen", locale);
    option_info[16] = translate("Silberpool", locale);
    option_info[17] = NULL;
  } else {
    option_info[0] = NULL;
  }

  zauberart[0] = translate("Pr&auml;kampfzauber",locale);
  zauberart[1] = translate("Kampfzauber",locale);
  zauberart[2] = translate("Postkampfzauber",locale);
  zauberart[3] = translate("normaler Zauber",locale);
  
  if (locale == en) /* ins Englische uebersetzen */
  {
    
    kampfstatus[0] = "aggressive";
    kampfstatus[1] = "front";
    kampfstatus[2] = "back";
    kampfstatus[3] = "defensive";
    kampfstatus[4] = "doesn't fight";
    kampfstatus[5] = "escapes";

    kuestenname[0] = "undefined coast";
    kuestenname[1] = "north west cast";
    kuestenname[2] = "north coast";
    kuestenname[3] = "east coast";
    kuestenname[4] = "south east coast";
    kuestenname[5] = "south west coast";
    kuestenname[6] = "west coast";
    
    zaubermod[0] = "ship spell";
    zaubermod[1] = "far spell";
  }
  else if (locale != de)
  {
    /* Welche Sprache!? Versuchen wir's mal mit Deutsch 
       und geben vorsichtshalber eine Warnung aus */
    if (config.verbose > 0) fprintf(stderr,
      "Warnung in set_global_names: unbekannte Sprache.\n");
  }
}
