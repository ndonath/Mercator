/* $Id: merian.c,v 1.12 2000/03/27 15:52:58 enno Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <limits.h>
#include "mercator.h"

/*
 * Vorläufig ist unsere Welt begrenzt...
 */

#define MAX_X 800
#define MAX_Y 800


#define M(m, x, y) (*((m)->mm + MAX_X * (y) + (x)))

typedef struct merian_struct {
    char *mm;			/* MAX_X * MAX_Y characters */
    int  max_x, max_y;

} merian_t;


int get_number(merian_t *m, int x, int y, int v, int *x_off)
{
    char buf[6];
    int res = 0;
    int i;
    int sign;

    switch (M(m, x, y)) {
    case '+':
    case ' ':
	sign = 1;
	if (v) y++; else x++;
	break;
    case '-':
	sign = -1;
	if (v) y++; else x++;
	break;
    default:
	sign = 1;
	break;
    }
    while (M(m, x, y) == ' ') {
	if (v) y++; else x++;
    }
    i = 0;
    while (i < 5 && strchr("0123456789", M(m, x, y)) != NULL) {
	buf[i] = M(m, x, y);
	i++;
	if (v) y++; else x++;
    }
    buf[i] = 0;
    if (i >= 5 || buf[0] == 0) {
	res = 1;
    } else {
	*x_off = sign * atoi(buf);
	res = 0;
    }

    return res;
}


void parse_merian(map_t *map, merian_t *m)
{
    int x, y;			/* Koordinaten in Datei */
    int start_x, start_y;	/* Startecke der Karte (Dateikoordinaten) */
    int end_x, end_y;		/* Endeecke (Dateikoordinaten) */
    int x_off, y_off;
    int kx, ky;			/* Kartenkoordinaten */
    int mpc;
    int dummy;

    /*
     * Zeile Suchen, die nur "-", "+" und Leerzeichen enthält, aber mindestens
     * ein "+" oder '-'.
     */
    mpc = 0;
    for (y = 0; y < MAX_Y; y++) {
	for (x = 0; x < MAX_X && M(m, x, y) != 0; x++) {
	    if (strchr("-+ \n\r", M(m, x, y)) == NULL) {
		break;
	    }
	    if (M(m, x, y) == '+' || M(m, x, y) == '-')
		mpc++;
	}
	if (mpc > 0 && M(m, x, y) == 0)
	    break;
    }
    for (x = 0; x < MAX_X; x++)
	if (M(m, x, y) == '+' || M(m, x, y) == '-')
	    break;
    start_x = x;
    /*
     * Dekodiere die X-Koordinate in der oberen linken Kartenecke.
     */
    if (get_number(m, x, y, 1, &x_off) != 0) {
	fprintf(stderr, "Fehler beim Lesen der Koordinaten\n");
	exit(1);
    }
    end_x = x;
    while (get_number(m, end_x + 2, y, 1, &dummy) == 0) {
	end_x += 2;
    }
    get_number(m, end_x, y, 1, &dummy);
    printf("end_x = %d, Wert = %d\n", end_x, dummy);
    /*
     * Dekodiere die Y-Koordinate am linken oberen Kartenrand.
     */
    y_off = 0;
    start_y = 0;
    end_y   = 0;
    while (y < MAX_Y) {
	for (x = 0; x < start_x; x++) {
	    if (M(m, x, y) != 0 && 
		strchr("-+0123456789", M(m, x, y)) != NULL) {
		if (get_number(m, x, y, 0, &y_off) != 0) {
		    fprintf(stderr, "Fehler beim Lesen der Koordinaten\n");
		    exit(1);
		}
		start_y = y;
		end_y = y;
		while (get_number(m, x, end_y + 1, 0, &dummy) == 0) {
		    end_y++;
		}
		get_number(m, x, end_y, 0, &dummy);
		printf("end_y = %d, Wert = %d\n", end_y, dummy);
		goto loop_exit;
	    }
	}
	y++;
    }
 loop_exit:
    assert(y < MAX_Y);
    printf("start_x = %d, start_y = %d\n", start_x, start_y);
    printf("x_off = %d, y_off = %d\n", x_off, y_off);

    for (y = start_y, ky = y_off; y <= end_y; y++, ky++) {
	for (x = start_x, kx = x_off; x <= end_x; x += 2, kx++) {
	    switch (M(m, x, y)) {
	    case '+':
	    case 'E':
	    case 'e':
		set_region_typ(map, kx, ky, "Ebene");
		break;
	    case 'W':
	    case 'w':
		set_region_typ(map, kx, ky, "Wald");
		break;
	    case 'S':
	    case 's':
		set_region_typ(map, kx, ky, "Sumpf");
		break;
	    case 'G':
	    case 'g':
		set_region_typ(map, kx, ky, "Gletscher");
		break;
	    case 'H':
	    case 'h':
		set_region_typ(map, kx, ky, "Hochland");
		break;
	    case 'B':
	    case 'b':
		set_region_typ(map, kx, ky, "Berge");
		break;
	    case '.':
	    case 'O':
	    case 'o':
		set_region_typ(map, kx, ky, "Ozean");
		break;
	    case 'D':
	    case 'd':
		set_region_typ(map, kx, ky, "Wueste");
		break;
	    default:
		;
		break;
	    }
	}
    }
}


void merge_mer_strips(int *argc, char *argv[], map_t *map)
{
    char line[1000];
    int  x, y, c, i;

    (*argc)++;
    printf("x und y eingeben\n");
    if (fgets(line, 1000, stdin) == NULL)
	return;
    while (line[0] != ' ' && line[0] != '\n') {
	sscanf(line, "%d %d", &x, &y);
	printf("x = %d, y = %d\n", x, y);
	printf("Merian-Zeile eingeben\n");
	if (fgets(line, 1000, stdin) == NULL)
	    break;
	for (i = 0; i < 1000 && line[i] != '\n' && line[i] != 0; i++) {
	    c = line[i];
	    if (i % 2 == 1 && c != ' ')
		printf("Fehler, Leerzeichen erwartet!\n");
	    else {
		switch (c) {
		case '+':
		case 'E':
		case 'e':
		    set_region_typ(map, x + i / 2, y, "Ebene");
		    break;
		case 'W':
		case 'w':
		    set_region_typ(map, x + i / 2, y, "Wald");
		    break;
		case 'S':
		case 's':
		    set_region_typ(map, x + i / 2, y, "Sumpf");
		    break;
		case 'G':
		case 'g':
		    set_region_typ(map, x + i / 2, y, "Gletscher");
		    break;
		case 'H':
		case 'h':
		    set_region_typ(map, x + i / 2, y, "Hochland");
		    break;
		case 'B':
		case 'b':
		    set_region_typ(map, x + i / 2, y, "Berge");
		    break;
		case '.':
		case '~':
		case 'O':
		case 'o':
		    set_region_typ(map, x + i / 2, y, "Ozean");
		    break;
		case 'D':
		case 'd':
		    set_region_typ(map, x + i / 2, y, "Wueste");
		    break;
		case ' ':
		case '/':
		    ;			/* ignore */
		    break;
		default:
		    printf("unbekanntes Zeichen: '%c'\n", c);
		    break;
		}
	    }
	}
	printf("x und y eingeben\n");
	if (fgets(line, 1000, stdin) == NULL)
	    break;
    }
}


void read_mer(int *argc, char *argv[], map_t **pmap)
{
    map_t    *map = *pmap;
    int      i = *argc;
    char     *s;
    FILE     *fp;
    int      l, x;
    merian_t *m;

    m = xmalloc(sizeof(merian_t));

    i++;			/* skip "read-mer" */
    if (map != NULL) {
	printf("Lösche alte Karte!\n");
	destroy_map(map);
    }
    map = make_map(REGION_HASH_SIZE, ROW_MULT, EINHEIT_HASH_SIZE);
    if (argv[i] == 0) {
	fprintf(stderr, "read-mer benötigt Argument!\n");
	exit(1);
    }
    fp = xfopen(argv[i], "r");
    i++;			/* skip filename */
    m->mm = xmalloc(MAX_X * MAX_Y);
    m->max_y = 0;
    do {
	s = fgets(m->mm + MAX_X * m->max_y, MAX_X, fp);
	m->max_y++;
    } while (s != NULL);
    fclose(fp);

    m->max_x = 0;
    for (l = 0; l < m->max_y; l++) {
	for (x = 0; x < MAX_X; x++) {
	    if (M(m, x, l) == 0 || M(m, x, l) == '\n') {
		M(m, x, l) = 0;
		if (x > m->max_x)
		    m->max_x = x;
		break;
	    }
	}
    }

    parse_merian(map, m);
    xfree(m->mm);
    xfree(m);

    printf("x = [%d .. %d], y = [%d .. %d]\n",
	   map->min_x, map->max_x, map->min_y, map->max_y);
    *pmap = map;
    *argc = i;
}


void write_cr_map(map_t *map, const char *file)
{
    map_entry_t *e;
    partei_t    *p;
    FILE *fp;
    int x, y;

    fp = xfopen(file, "w");
    fprintf(fp, "VERSION 29;Version des Computer Reports\n");
    fprintf(fp, "ADRESSEN\n");
    for (p = map->first_partei; p != NULL; p = p->next) {
	if (p->nummer <= 0)	/* Monster und Parteigetarnt auslassen */
	    continue;

	fprintf(fp, "%d;Partei\n", p->nummer);
	fprintf(fp, "\"%s\";Parteiname\n",
		p->name != NULL ? p->name : "");
	fprintf(fp, "\"%s\";email\n",
		p->email != NULL ? p->email : "");
	fprintf(fp, "\"%s\";banner\n",
		p->banner != NULL ? p->banner : "");
    }
    for (y = map->min_y; y <= map->max_y; y++)
	for (x = map->min_x; x <= map->max_x; x++) {
	    e = mp(map, x, y);
	    if (e == NULL)
		continue;
	    fprintf(fp, "REGION %d %d\n", x, y);
	    if (e->name != NULL)
		fprintf(fp, "\"%s\";Name\n", e->name);
	    fprintf(fp, "\"%s\";Terrain\n", region_typ(e));
	    if (e->insel != NULL)
		fprintf(fp, "\"%s\";Insel\n", e->insel);
	    if (e->strasse > 0)
		fprintf(fp, "%d;Strasse\n", e->strasse);
	}

    fclose(fp);
}

char *format[] = {
    "",
    "%c%01d",
    "%c%02d",
    "%c%03d",
    "%c%04d"
};


#ifdef ENNOS_MERIAN_WRITER

#define xp(r, f) (((r)->x - (f)->ursprung[0])*2 + ((r)->y - (f)->ursprung[1]))
#define yp(r, f) ((r)->y-(f)->ursprung[1])

#if 0
void
merian(FILE * out, faction * f)
#else
void
merian(FILE * out, vset* regs, faction * f)
#endif
{
        int x1 = INT_MAX, x2=INT_MIN, y1=INT_MAX, y2=INT_MIN;
        region *left=0, *right=0, *top=0, *bottom=0;
        char ** line;
        char * c;
        int y, xw;

#ifdef FAST_REGION
        seen_region * sd;
        if (!seen) return;
        for (sd = seen; sd!=NULL; sd = sd->next) {
                region * r = sd->r;
#else
        void ** it;
        if (!regs->size) return;
        for (it = regs->data; it!=regs->data+regs->size;++it) {
                region * r = (region*)*it;
#endif
                if (!left || xp(r, f)<xp(left, f)) left=r;
                if (!right || xp(r, f)>xp(right, f)) right=r;
                if (!top || yp(r, f)>yp(top, f)) top=r;
                if (!bottom || yp(r, f)<yp(bottom, f)) bottom=r;
        }
        x1 = xp(left, f);
        x2 = xp(right, f)+1;
        y1 = yp(bottom, f);
        y2 = yp(top, f)+1;

        xw = max(abs(x1), x2);
        if (xw>99) xw = 3;
        else if (xw>9) xw = 2;
        else xw = 1;

        fputs("CR-Version: 42\n"
	      "MERIAN Version 1.01\n"
	      "Kartenart: Hex(Standard)\n\n"
	      "LEGENDE\n"
	      "Hochland = H\n"
	      "Gletscher = G\n"
	      "Berge = B\n"
	      "Wald = W\n"
	      "Sumpf = S\n"
	      "Ozean = .\n"
	      "Wueste = D\n"
	      "Ebene = E\n\n", out);

        line = (char**)malloc((y2-y1)*sizeof(char*));
        c = (char*)malloc((y2-y1)*((x2-x1)*sizeof(char)+13));
        memset(c, ' ', (y2-y1)*((x2-x1)*sizeof(char)+13));
        {
                int width = 1+x2-x1;
                int lx = top->x - f->ursprung[0] - (xp(top, f)-x1) / 2;
                int i;
                fputs("         ", out);
                for (i=0;i!=width;++i) {
                        int x = lx+(i/2);
                        int o = abs(x1-y2);
                        if (i%2 == o % 2)
                                fprintf(out, "%c", (x<0)?'-':(x>0)?'+':' ');
                        else fputc(' ', out);
                }
                fputc('\n', out);
                switch (xw) {
                case 3:
                        fputs("         ", out);
                        for (i=0;i!=width;++i) {
                                int x = lx+(i/2);
                                int o = abs(x1-y2);
                                if (i%2 == o % 2)
                                        fprintf(out, "%c", (abs(x)/100)?((abs(x)/
100)+'0'):' ');
                                else fputc(' ', out);
                        }
                        fputc('\n', out);
                case 2:
                        fputs("         ", out);
                        for (i=0;i!=width;++i) {
                                int x = lx+(i/2);
                                int o = abs(x1-y2);
                                if (i%2 == o % 2)
                                        fprintf(out, "%c", (abs(x)%100)/10?((abs(
x)%100)/10+'0'):(abs(x)<100?' ':'0'));
                                else fputc(' ', out);
                        }
                        fputc('\n', out);         
                default:
                        fputs("         ", out);
                        for (i=0;i!=width;++i) {
                                int x = lx+(i/2);
                                int o = abs(x1-y2);
                                if (i%2 == o % 2)
                                        fprintf(out, "%c", abs(x)%10+'0');
                                else fputc(' ', out);
                        }
                        fputc('\n', out);
                }
                fputs("        ", out);
                for (i=0;i!=width;++i) {
                        int o = abs(x1-y2);
                        if (i%2 == o % 2)
                                fputc('/', out);
                        else fputc(' ', out);
                }
        }
        fputc('\n', out);
        for (y=y1;y!=y2;++y) {
                line[y-y1] = c + (y-y1)*((x2-x1)*sizeof(char)+13);
                sprintf(line[y-y1], "%4d", y);
                line[y-y1][4] = ' ';
                sprintf(line[y-y1]+7+(x2-x1), "%4d", y);
        }

#ifdef FAST_REGION
        for (sd = seen; sd!=NULL; sd = sd->next) {
                region * r = sd->r;
#else
        for (it = regs->data; it!=regs->data+regs->size;++it) {
                region * r = (region*)*it;
#endif
                line[yp(r, f)-y1][5+(xp(r, f)-x1)] = ' ';
                line[yp(r, f)-y1][6+(xp(r, f)-x1)] = terrainsymbols[mainterrain(r
)];
        }

        /* print header */
        for (y=y1;y!=y2;++y) {               
                fputs(line[y2-y-1], out);
                fputc('\n', out);
        }

        {
                int width = 1+x2-x1;
                int lx = bottom->x - f->ursprung[0] - (xp(bottom, f)-x1) / 2;
                int i;
                fputs("     ", out);
                for (i=0;i!=width;++i) {
                        int o = abs(x1-y1);
                        if (i%2 == o % 2)
                                fputc('/', out);
                        else fputc(' ', out);
                }
                fputc('\n', out);
                fputs("    ", out);
                for (i=0;i!=width;++i) {
                        int x = lx+(i/2);
                        int o = abs(x1-y1);
                        if (i%2 == o % 2)
                                fprintf(out, "%c", (x<0)?'-':(x>0)?'+':' ');
                        else fputc(' ', out);
                }
                fputc('\n', out);
                switch (xw) {
                case 3:
                        fputs("    ", out);
                        for (i=0;i!=width;++i) {
                                int x = lx+(i/2);
                                int o = abs(x1-y1);
                                if (i%2 == o % 2)
                                        fprintf(out, "%c", (abs(x)/100)?((abs(x)/
100)+'0'):' ');
                                else fputc(' ', out);
                        }
                        fputc('\n', out);
                case 2:
                        fputs("    ", out);
                        for (i=0;i!=width;++i) {
                                int x = lx+(i/2);  
                                int o = abs(x1-y1);
                                if (i%2 == o % 2)
                                        fprintf(out, "%c", (abs(x)%100)/10?((abs(
x)%100)/10+'0'):(abs(x)<100?' ':'0'));
                                else fputc(' ', out);
                        }
                        fputc('\n', out);
                default:
                        fputs("    ", out);
                        for (i=0;i!=width;++i) {
                                int x = lx+(i/2);
                                int o = abs(x1-y1);
                                if (i%2 == o % 2)
                                        fprintf(out, "%c", abs(x)%10+'0');
                                else fputc(' ', out);
                        }
                        fputc('\n', out);
                }
        }

        free(c);
        free(line);
}  
#endif
