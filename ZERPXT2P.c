#include <stdio.h>
#include <time.h>
#include <conio.h>
#include <dos.h>

/* game settings */
#define WIDTH 26
#define HEIGHT 10
#define BOMBS 4
#define MOBS 4
#define RIDEABLE_MOBS 1

#define ESCP 0
#define ENTR 13

#define ARR_UP 'H'
#define ARR_LT 'K'
#define ARR_DN 'P'
#define ARR_RT 'M'

void hide_cursor(void) {
	union REGS regs;
	regs.h.ah = 0x01;
	regs.h.ch = 0x20;
	regs.h.cl = 0x00;
	int86(0x10, &regs, &regs);
}
void show_cursor(void) {
	union REGS regs;
	regs.h.ah = 0x01;
	regs.h.ch = 0x06;
	regs.h.cl = 0x07;
	int86(0x10, &regs, &regs);
}
#define NOSND nosound();
#define SND(f) if (do_sound) sound(f);
#define CURS_HIDE hide_cursor();
#define CURS_SHOW show_cursor();
#define TBS "  "

typedef unsigned long my32;

unsigned char field[HEIGHT][WIDTH];
unsigned char stored_field[HEIGHT][WIDTH];
unsigned int seed;
char facing, air, help = 0, super;
char facing2, air2, super2;
int stored_pos[2], stored_pos2[2];

static my32 my_seed;
void my_srand(my32 s){ my_seed = s; }
my32 my_rand(){
	my_seed = my_seed * 1664525 + 1013904223;
	return my_seed >> 16;
}

char alt_climb(const int p_x, const int p_y){
	if (facing && p_x < WIDTH - 1 && (field[p_y][p_x + 1] < 3 || super) && field[p_y + 1][p_x + 1]){
		return 'd';
	}
	if (p_x > 1 && (field[p_y][p_x - 1] < 3 || super) && field[p_y + 1][p_x - 1]){
		return 'a';
	}
	if (p_x < WIDTH - 1 && (field[p_y][p_x + 1] < 3 || super) && field[p_y + 1][p_x + 1]){
		return 'd';
	}
	return 0;
}
char alt_climb2(const int p_x, const int p_y){
	if (facing2 && p_x < WIDTH - 1 && (field[p_y][p_x + 1] < 3 || super2) && field[p_y + 1][p_x + 1]){
		return '>';
	}
	if (p_x > 1 && (field[p_y][p_x - 1] < 3 || super2) && field[p_y + 1][p_x - 1]){
		return '<';
	}
	if (p_x < WIDTH - 1 && (field[p_y][p_x + 1] < 3 || super2) && field[p_y + 1][p_x + 1]){
		return '>';
	}
	return 0;
}

/* x, y, dirrection */
int monsters[MOBS + 27][3];

void delmons(int y, int x){
	int i;
	for (i = 0; i < MOBS + 27; i++)
		if (monsters[i][0] == x && monsters[i][1] == y){
			monsters[i][0] = 0;
			monsters[i][1] = 0;
			break;
		}
}

void load_m(void){
	int k = 0, i, j;
	for (i = 0; i < MOBS + 27; i++){
		monsters[i][0] = 0;
		monsters[i][1] = 0;
		monsters[i][2] = 0;
	}
	for (i = 0; i < HEIGHT; i++)
		for (j = 1; j < WIDTH; j++)
			if (field[i][j] == 'q'){
				monsters[k][0] = j;
				monsters[k][1] = i;
				monsters[k][2] = my_rand() % 4;
				if (++k >= MOBS + 27) return;
			}
}

int win_condition(void){
	int i, j;
	for (i = 0; i < HEIGHT; i++)
		for (j = 1; j < WIDTH; j++)
			if (field[i][j] == 1) return 0;
	return 1;
}

void store_field(void){
	int i, j;
	for (i = 0; i < HEIGHT; i++)
		for (j = 0; j < WIDTH; j++)
			stored_field[i][j] = field[i][j];
}

void load_field(void){
	int i, j;
	for (i = 0; i < HEIGHT; i++)
		for (j = 0; j < WIDTH; j++)
			field[i][j] = stored_field[i][j];
}

char *campaign[] = {
"0","0\n\
                 #\n\
    @  ?         #\n\
#############    #\n\
            #    !\n\
            #########!###\n\
                     !\n\
  !  !  #\n\
#########\n\
        #        !\n\
        #################",
"0\n\
             ######\n\
             #    #\n\
  ######     # #  #\n\
  #    #######!#  #\n\
  # @?       % #!!#\n\
  #####!!####!!####\n\
      #### #!  #\n\
           #!  #\n\
           #####\n",
"2\n\n\
        #########\n\
        #   #   #\n\
        #   #   #\n\
        # @ # ? #\n\
        #########\n\
            #\n\
    #       #       #\n\
    #!!!!!!!#!!!!!!!#\n\
    #!!!!!!!#!!!!!!!#",
"0\n\
\n\
                   !\n\
\n\
          #\n\
                   !\n\
   #   ##\n\
     #      #     !\n\
 #     #    #       !\n\
!#          #\n\
!#  @ #  ?  #      !",
""
};

int open_level(int n){
	char *ptr = campaign[n];
	char start_bombs;
	int i, j;
	if (n > 4) return -1;
	start_bombs = *ptr - '0';
	ptr += 2;
	for (i = 0; i < HEIGHT; i++)
		for (j = 0; j < WIDTH; j++)
			field[i][j] = 0;
	for (i = 0; *ptr && i < HEIGHT; i++, ptr++)
		for(j = 1; *ptr && *ptr != '\n' && j < WIDTH; j++, ptr++)
			switch (*ptr){
				case ' ': field[i][j] =  0 ; break;
				case '!': field[i][j] =  1 ; break;
				case '[': field[i][j] =  2 ; break;
				case '#': field[i][j] =  3 ; break;
				case '%': field[i][j] = 'q'; break;
				case '@': 
					field[i][j] = '@';
					stored_pos[0] = j;
					stored_pos[1] = i;
					break;
				case '?': 
					field[i][j] = '?';
					stored_pos2[0] = j;
					stored_pos2[1] = i;
					break;
				default:
					field[i][j] = *ptr - 175;
			}
	load_m();
	return start_bombs;
}

void draw_field(void){
	int i, j;
	char t, F;
	CURS_HIDE
	textcolor(LIGHTGRAY);
	printf(TBS);
	if (!help){
		for (j = 1; j < WIDTH; j++) cputs("\315\304\315");
		printf("\n"TBS);
	}
	for (i = 0; i < HEIGHT; i++){
		for (j = 1; j < WIDTH; j++){
			F = field[i][j];
			if (F == '@') cputs("   ");
			else if (F == '?') cputs("   ");
			else if (F == 'q') {
				textcolor(LIGHTMAGENTA); 
				cputs("o\313o"); 
				textcolor(LIGHTGRAY);
			}
			else if (F) cprintf("%c%c%c", 175 + F, 175 + F, 175 + F);
			else cputs("   ");
		}
		printf("\n"TBS);
		for (j = 1; j < WIDTH; j++){
			F = field[i][j];
			if (F == '@') cputs("   ");
			else if (F == '?') cputs("   ");
			else if (F == 'q') { 
				textcolor(LIGHTMAGENTA); 
				cputs("\311\316\273"); 
				textcolor(LIGHTGRAY);
			}
			else if (F) cprintf("%c%c%c", 175 + F, 175 + F, 175 + F);
			else cputs("   ");
		}
		printf("\n"TBS);
	}
	if (!help){
		for (j = 1; j < WIDTH; j++) cputs("\315\304\315");
		printf("\n"TBS);
	}
	CURS_SHOW
}

void drawat(int x, int y){
	if (super) textcolor(LIGHTCYAN);
	else textcolor(YELLOW);
	gotoxy(x * 3, 3 + y * 2  - help);
	if (air)
		if (facing) cputs(super ? "\311@\274": "\332@\331");
		else cputs(super ? "\310@\273" : "\300@\277");
	else cputs(super ? "\311@\273" : "\332@\277");
	gotoxy(x * 3, 4 + y * 2  - help);
	if (air)
		if (facing) cputs("\311\312\315");
		else cputs("\315\312\273");
	else cputs("\311\312\273");
	textcolor(LIGHTGRAY);
}
void drawat2(int x, int y){
	if (super2) textcolor(LIGHTCYAN);
	else textcolor(LIGHTGREEN);
	gotoxy(x * 3, 3 + y * 2  - help);
	if (air2)
		if (facing2) cputs(super2 ? "\311?\274": "\332?\331");
		else cputs(super2 ? "\310?\273" : "\300?\277");
	else cputs(super2 ? "\311?\273" : "\332?\277");
	gotoxy(x * 3, 4 + y * 2  - help);
	if (air2)
		if (facing2) cputs("\311\312\315");
		else cputs("\315\312\273");
	else cputs("\311\312\273");
	textcolor(LIGHTGRAY);
}
void undrawat(int x, int y){
	if (field[y][x]) return;
	gotoxy(x * 3, 3 + y * 2  - help);
	cputs("   ");
	gotoxy(x * 3, 4 + y * 2  - help);
	cputs("   ");
}
void drawmob(int x, int y){
	gotoxy(x * 3, 3 + y * 2  - help);
	textcolor(LIGHTMAGENTA); 
	cputs("o\313o");
	gotoxy(x * 3, 4 + y * 2  - help);
	cputs("\311\316\273");
	textcolor(LIGHTGRAY);
}

void shuffle(int points[][2], int n){
	int i, j, temp0, temp1;
	for (i = n - 1; i > 0; i--){
		j = my_rand() % (i + 1);
		temp0 = points[i][0];
		temp1 = points[i][1];
		points[i][0] = points[j][0];
		points[i][1] = points[j][1];
		points[j][0] = temp0;
		points[j][1] = temp1;
	}
}

void make_field(void){
	int x, y, m = 0, i, flag, c, *p, *l;
	int points[WIDTH * HEIGHT][2];

	my_srand(seed);
	for (x = 0; x < WIDTH; x++){
		for (y = 0; y < HEIGHT; y++){
			points[x + y * WIDTH][0] = x;
			points[x + y * WIDTH][1] = y;
		}
	}
	for(p = (int*)field, l = p + sizeof(field)/sizeof(int); p < l; *p++ = 0);

	shuffle(points, WIDTH * HEIGHT);
	for (i = 0; i < WIDTH * HEIGHT; i++){
		int check[8][2];
		x = points[i][0] + 1;
		y = points[i][1];
		check[0][0] = x + 1;  check[0][1] = y + 1;
		check[1][0] = x - 1;  check[1][1] = y - 1;
		check[2][0] = x - 1;  check[2][1] = y + 1;
		check[3][0] = x + 1;  check[3][1] = y - 1;
		check[4][0] = x;      check[4][1] = y + 1;
		check[5][0] = x;      check[5][1] = y - 1;
		check[6][0] = x + 1;  check[6][1] = y;
		check[7][0] = x - 1;  check[7][1] = y;
		flag = 0;
		
		for (c = 0; c < 8; c++)
			if (check[c][0] >  0 && check[c][0] < WIDTH && 
				check[c][1] >= 0 && check[c][1] < HEIGHT)
				if (field[check[c][1]][check[c][0]])
					if (field[check[c][1]][check[c][0]] != 'q') flag++;
			
		if (flag == 0) field[y][x] = 3;
		if (flag == 1) field[y][x] = 3;
		if (flag == 2) field[y][x] = 1;
		if (flag > 2 && m < MOBS && (x + 1 < WIDTH / 2 || x - 1 > WIDTH / 2)){
			field[y][x] = 'q';
			m++;
		}
	}
	x = WIDTH / 2 - 1;
	y = HEIGHT - 1;
	while (field[y][x] && y > 0) y--;
	field[y][x] = '@';
	stored_pos[0] = x;
	stored_pos[1] = y;
	x = WIDTH / 2 + 1;
	y = HEIGHT - 1;
	while (field[y][x] && y > 0) y--;
	field[y][x] = '?';
	stored_pos2[0] = x;
	stored_pos2[1] = y;
	load_m();
}

int main(void){
	char tutorial = 0;
	int p_x, p_y, p_x2, p_y2;
	unsigned char p_i = '?';
	unsigned char prpr_i = '?';
	unsigned static_seed = 0;
	char sector_order = 0;
	int score, maxscore;
	char leader;
	int score2, scr_lv2;
	int level, turns;
	int bombs, bms_lv;
	int scr_lv, retries;
	int do_sound = 1, mptr = 1;
	char keyword[20] = "", *kwp;
	int i, j, dodraw = 1;
	int p_yt = 0, p_xt = 0;
	int p_yt2 = 0, p_xt2 = 0;
	char bothfall = 0;
	my_srand(time(NULL));
	title:
	mptr = 1;
	CURS_HIDE
	clrscr();
	textcolor(WHITE);
	gotoxy(5, 2);cputs("_____           _           _     ______");
	gotoxy(4, 3);cputs("|  __ \\         (_)         | |   |___  /");
	gotoxy(4, 4);cputs("| |__) | __ ___  _  ___  ___| |_     / / ___ _ __ _ __");
	gotoxy(4, 5);cputs("|  ___/ '__/ _ \\| |/ _ \\/ __| __|   / / / _ \\ '__| '_ \\");
	gotoxy(4, 6);cputs("| |   | | | (_) | |  __/ (__| |_   / /_|  __/ |  | |_) |");
	gotoxy(4, 7);cputs("|_|   |_|  \\___/| |\\___|\\___|\\__| /_____\\___|_|  | .__/");
	gotoxy(19, 8);cputs("_/ |");gotoxy(53, 8);cputs("| |");
	gotoxy(18, 9);cputs("|__/");gotoxy(53, 9);cputs("|_|");
	textcolor(LIGHTGRAY);
	gotoxy(7, 11); cputs("Goal: break all weak blocks and don't get stuck!");
	gotoxy(4,24); cputs("KosmoKrab 2026");
	while (1){
		gotoxy(1, 13); cputs("");
		printf(
			" WASD, numpad or arrows - move; B and . - use charge; L - retry;\n"
			" H or F1 - tips; V - sound; SPACE - wait; Q or ESC - quit.");
		if (mptr == 0) textcolor(YELLOW);
		gotoxy(19,18);cputs(mptr == 0 ? "> PLAY TUTORIAL <" : "  PLAY TUTORIAL  ");
		textcolor(LIGHTGRAY);
		if (mptr == 1) textcolor(YELLOW);
		gotoxy(19,19);cputs(mptr == 1 ? ">  RANDOM MODE  <" : "   RANDOM MODE   ");
		textcolor(LIGHTGRAY);
		if (mptr == 2) textcolor(YELLOW);
		gotoxy(19,20);cputs(mptr == 2 ? ">  FIXED ORDER  <" : "   FIXED ORDER   ");
		textcolor(LIGHTGRAY);
		gotoxy(17,24);
		p_i = getch();
		if (p_i == ' ' || p_i == ENTR) break;
		if (p_i >= 'A' && p_i <= 'Z') p_i += ('a' - 'A');
		if (p_i == ESCP) p_i = getch();
		if (p_i == 's' || p_i == ARR_DN || p_i == 'd' || p_i == ARR_RT)
			mptr = (mptr + 2) % 3;
		if (p_i == 'e' || p_i == 'q' || p_i == 27) 
			{ CURS_SHOW clrscr(); return 0; }
		else mptr = (mptr + 2) % 3;
	}
	CURS_SHOW
	switch (mptr){
	case 0:
		help = 1;
		tutorial = 1;
		sector_order = 0;
	break;
	case 1:
		help = 0;
		tutorial = 0;
		sector_order = 0;
	break;
	case 2:
		help = 0;
		tutorial = 0;
		sector_order = 1;
		clrscr();
		printf("\n\n Enter starting sector ID: ");
		if(!scanf("%u", &static_seed)){
			static_seed = 0;
			scanf("%s", keyword);
			for (kwp = keyword; *kwp; kwp++)
				static_seed += *kwp * (kwp - keyword);
		}
	break;
	}
	p_i = '?';
	here:
	my_srand(my_rand() + clock());
	if (sector_order) seed = static_seed;
	else seed = my_rand();
	facing = 1;		air = 0;
	level = 1;		turns = 0;		super = 0;
	score = 0;		maxscore = 0;	bombs = BOMBS;
	facing2 = 1;	air2 = 0;		super2 = 0;
	score2 = 0;		leader = ' ';
	
	if (tutorial) bombs = open_level(1);
	else make_field();
	if (bombs < 0) { make_field(); tutorial = 0; bombs = BOMBS; }
	bms_lv = bombs;
	scr_lv = 0;
	scr_lv2 = 0;
	retries = 0;
	store_field();
	p_x = stored_pos[0];
	p_y = stored_pos[1];
	p_x2 = stored_pos2[0];
	p_y2 = stored_pos2[1];
	clrscr();
	
	while (1){
		int retry = 0;
		int skip_ai = 0;
		gotoxy(7,1);
		textcolor(WHITE);
		cprintf("Level: %d ", level);
		putchar('|');
		if (tutorial) cprintf(" Tutorial ");
		else cprintf(" Sector: %u ", seed);
		putchar('|');
		cprintf(" Last key: %c ", p_i);
		putchar('|');
		cprintf(" Leader: %c ", leader);
		putchar('|');
		if (super) cprintf(" Charged up!");
		else if (bombs) cprintf(" Charges: %d  ", bombs);
		else cprintf(" No charges!");
		printf("\n");
		if (dodraw) draw_field();
		else { 
			undrawat(p_xt, p_yt);
			undrawat(p_xt2, p_yt2);
		}
		drawat(p_x, p_y);
		drawat2(p_x2, p_y2);
		if (help) gotoxy(7,22);
		else gotoxy(7,24);
		textcolor(WHITE);
		cprintf("Turn: %d ", turns);
		textcolor(LIGHTGRAY);
		putch('|');
		textcolor(WHITE);
		cprintf(" @ score: %d ", score);
		textcolor(LIGHTGRAY);
		putch('|');
		textcolor(WHITE);
		cprintf(" ? score: %d ", score2);
		textcolor(LIGHTGRAY);
		putch('|');
		textcolor(WHITE);
		cprintf(" Session best: %d ", maxscore);
		textcolor(LIGHTGRAY);
		if (help) {
			gotoxy(3,23);
			cputs("  WASD, arrows - move; B and . - use charge; L - retry;");
			gotoxy(3,24);
			cputs("  H - tips; V - sound; SPACE - wait; Q or ESC - quit. ");
		}
		cputs("> ");
		clreol();
		dodraw = 1;
		NOSND
		prpr_i = p_i;
		p_i = getch();
		undrawat(p_x, p_y);
		undrawat(p_x2, p_y2);
		p_xt = p_x;
		p_yt = p_y;
		p_xt2 = p_x2;
		p_yt2 = p_y2;
		turns++;
		/* Alternative keys for actions */
		if (p_i >= 'A' && p_i <= 'Z') 
			p_i += ('a' - 'A');
		if (p_i == ESCP){
			p_i = getch();
			switch (p_i){
				/* Arrow key handling. */
				case ARR_UP: p_i = '^'; break;
				case ARR_LT: p_i = '<'; break;
				case ARR_DN: p_i = 'V'; break;
				case ARR_RT: p_i = '>'; break;
				case   ';' : p_i = 'h'; break;
			}
		}
		switch (p_i){
			/* Numpad controls. */
			case '7': p_i = '<';  break;
			case '4': p_i = '<';  break;
			case '9': p_i = '>';  break;
			case '6': p_i = '>';  break;
			case '8': p_i = '^';  break;
			case '5': p_i = 'v';  break;
			case '2': p_i = 'v';  break;
		}
		if (p_i == 'w' && p_y > 0){
			dodraw = 0;
			SND(456)
			if (field[p_y - 1][p_x] == 'q'){
				if (!super) retry++;
				else {
					delmons(p_y - 1, p_x);
					score += 9;
				}
			}
			if (field[p_y - 1][p_x] < 3 || super){
				char climb;
				if (field[p_y - 1][p_x] >= 3) super = 0;
				field[p_y][p_x] = 0;
				if (field[p_y - 1][p_x]) {score++; SND(560)}
				p_y--;
				field[p_y][p_x] = 0;
				undrawat(p_x, p_y);
				field[p_y][p_x] = '@';
			
				climb = alt_climb(p_x, p_y);
				if (climb && air) p_i = climb;
			}
		}
		if (p_i == '^' && p_y2 > 0){
			dodraw = 0;
			SND(456)
			if (field[p_y2 - 1][p_x2] == 'q'){
				if (!super2) retry++;
				else {
					delmons(p_y2 - 1, p_x2);
					score2 += 9;
				}
			}
			if (field[p_y2 - 1][p_x2] < 3 || super2){
				char climb;
				if (field[p_y2 - 1][p_x2] >= 3) super2 = 0;
				field[p_y2][p_x2] = 0;
				if (field[p_y2 - 1][p_x2]) {score2++; SND(560)}
				p_y2--;
				field[p_y2][p_x2] = 0;
				undrawat(p_x2, p_y2);
				field[p_y2][p_x2] = '?';
			
				climb = alt_climb2(p_x2, p_y2);
				if (climb && air2) p_i = climb;
			}
		}
		/* Applying the action */
		switch (p_i){
			case 27: goto title;
			case ' ': SND(456) dodraw = 0; break;
			case 'w': break;
			case '^': break;
			case 'q': goto title;
			case 'l': {
				if (prpr_i == 'l') goto here;
				retry++;
				break;
			}
			case 'h': {
				help = !help;
				clrscr();
				turns--;
				skip_ai++;
				break;
			}
			case 'v': {
				dodraw = 0;
				do_sound = !do_sound;
				turns--;
				skip_ai++;
				break;
			}
			case 'b': {
				dodraw = 0;
				if (bombs && !super){
					SND(999)
					super = 1;
					bombs--;
				}
				break;
			}
			case '.': {
				dodraw = 0;
				if (bombs && !super2){
					SND(999)
					super2 = 1;
					bombs--;
				}
				break;
			}
			case 'a': {
				dodraw = 0;
				SND(456)
				facing = 0;
				if (p_x <= 1) break;
				if (field[p_y][p_x - 1] == 'q'){
					if (!super) retry++;
					else {
						delmons(p_y, p_x - 1);
						score += 9;
					}
				}
				if (field[p_y][p_x - 1] < 3  || super){
					if (field[p_y][p_x - 1] >= 3) super = 0;
					if (field[p_y][p_x - 1]) {score++; SND(560)}
					field[p_y][p_x] = 0;
					p_x--;
					field[p_y][p_x] = '@';
				} else {
					if (p_y <= 0) break;
					if (field[p_y - 1][p_x - 1] < 3 && 
						field[p_y - 1][p_x] < 3){
							if (field[p_y - 1][p_x]) {score++; SND(560)}
							if (field[p_y - 1][p_x - 1]) {score++; SND(560)}
							field[p_y][p_x] = 0;
							field[p_y - 1][p_x] = 0;
							undrawat(p_x, p_y - 1);
							p_y--;
							p_x--;
							field[p_y][p_x] = '@';
						}
				}
				break;
			}
			case '<': {
				SND(456)
				dodraw = 0;
				facing2 = 0;
				if (p_x2 <= 1) break;
				if (field[p_y2][p_x2 - 1] == 'q'){
					if (!super2) retry++;
					else {
						delmons(p_y2, p_x2 - 1);
						score2 += 9;
					}
				}
				if (field[p_y2][p_x2 - 1] < 3  || super2){
					if (field[p_y2][p_x2 - 1] >= 3) super2 = 0;
					if (field[p_y2][p_x2 - 1]) {score2++; SND(560)}
					field[p_y2][p_x2] = 0;
					p_x2--;
					field[p_y2][p_x2] = '?';
				} else {
					if (p_y2 <= 0) break;
					if (field[p_y2-1][p_x2-1] < 3 && 
						field[p_y2-1][p_x2] < 3){
							if (field[p_y2 - 1][p_x2]) {score2++; SND(560)}
							if (field[p_y2 - 1][p_x2 - 1]) {score2++; SND(560)}
							field[p_y2][p_x2] = 0;
							field[p_y2 - 1][p_x2] = 0;
							undrawat(p_x2, p_y2 - 1);
							p_y2--;
							p_x2--;
							field[p_y2][p_x2] = '?';
						}
				}
				break;
			}
			case 'd': {
				dodraw = 0;
				SND(456)
				facing = 1;
				if (p_x + 1 >= WIDTH) break;
				if (field[p_y][p_x + 1] == 'q'){
					if (!super) retry++;
					else {
						delmons(p_y, p_x + 1);
						score += 9;
					}
				}
				if (field[p_y][p_x + 1] < 3  || super){
					if (field[p_y][p_x + 1] >= 3) super = 0;
					if (field[p_y][p_x + 1]) {score++; SND(560)}
					field[p_y][p_x] = 0;
					p_x++;
					field[p_y][p_x] = '@';
				} else {
					if (p_y <= 0) break;
					if (field[p_y - 1][p_x + 1] < 3 && 
						field[p_y - 1][p_x] < 3){
							if (field[p_y - 1][p_x]) {score++; SND(560)}
							if (field[p_y - 1][p_x + 1]) {score++; SND(560)}
							field[p_y][p_x] = 0;
							field[p_y - 1][p_x] = 0;
							undrawat(p_x, p_y - 1);
							p_y--;
							p_x++;
							field[p_y][p_x] = '@';
						}
				}
				break;
			}
			case '>': {
				SND(456)
				dodraw = 0;
				facing2 = 1;
				if (p_x2 + 1 >= WIDTH) break;
				if (field[p_y2][p_x2 + 1] == 'q'){
					if (!super2) retry++;
					else {
						delmons(p_y2, p_x2 + 1);
						score2 += 9;
					}
				}
				if (field[p_y2][p_x2 + 1] < 3  || super2){
					if (field[p_y2][p_x2 + 1] >= 3) super2 = 0;
					if (field[p_y2][p_x2 + 1]) {score2++; SND(560)}
					field[p_y2][p_x2] = 0;
					p_x2++;
					field[p_y2][p_x2] = '?';
				} else {
					if (p_y2 <= 0) break;
					if (field[p_y2-1][p_x2+1] < 3 && 
						field[p_y2-1][p_x2] < 3){
							if (field[p_y2 - 1][p_x2]) {score2++; SND(560)}
							if (field[p_y2 - 1][p_x2 + 1]) {score2++; SND(560)}
							field[p_y2][p_x2] = 0;
							field[p_y2 - 1][p_x2] = 0;
							undrawat(p_x2, p_y2 - 1);
							p_y2--;
							p_x2++;
							field[p_y2][p_x2] = '?';
						}
				}
				break;
			}
			case 's': {
				dodraw = 0;
				SND(456)
				if (p_y + 1 >= HEIGHT) break;
				if (field[p_y + 1][p_x] == 'q'){
					if (!super) retry++;
					else {
						delmons(p_y + 1, p_x);
						score += 9;
					}
				}
				if (field[p_y + 1][p_x] < 3 || super){
					if (field[p_y + 1][p_x] >= 3) super = 0;
					field[p_y][p_x] = 0;
					if (field[p_y + 1][p_x]) {score++; SND(560)}
					p_y++;
					field[p_y][p_x] = '@';
				}
				break;
			}
			case 'V': {
				SND(456)
				dodraw = 0;
				if (p_y2 + 1 >= HEIGHT) break;
				if (field[p_y2 + 1][p_x2] == 'q'){
					if (!super2) retry++;
					else {
						delmons(p_y2 + 1, p_x2);
						score2 += 9;
					}
				}
				if (field[p_y2 + 1][p_x2] < 3 || super2){
					if (field[p_y2 + 1][p_x2] >= 3) super2 = 0;
					field[p_y2][p_x2] = 0;
					if (field[p_y2 + 1][p_x2]) {score2++; SND(560)}
					p_y2++;
					field[p_y2][p_x2] = '?';
				}
				break;
			}
			default: {
				dodraw = 0;
				p_i = '?';
				turns--;
				skip_ai++;
				break;
			}
		}
		if (score > maxscore){
			maxscore = score;
			leader = '@';
		}
		if (score2 > maxscore){
			maxscore = score2;
			leader = '?';
		}
		
		for (i = 0; i < MOBS + 27 && !skip_ai && turns % 2; i++){
			int k[4], *monst = monsters[i];
			int dir_x, dir_y;
			if (!monst[0]) continue;
			k[0] = monst[2];
			k[3] = (monst[2] + 2) % 4;
			if (my_rand() % 2){
				k[1] = (monst[2] + 1) % 4;
				k[2] = (monst[2] + 3) % 4;
			} else {
				k[1] = (monst[2] + 3) % 4;
				k[2] = (monst[2] + 1) % 4;
			}
			for (j = 0; j < 4; j++){
				monst[2] = k[j];
				dir_y = monst[1];
				dir_x = monst[0];
				if (!dir_x) break;
				switch (k[j]){
					case 0: dir_x++; break;
					case 1: dir_y++; break;
					case 2: dir_x--; break;
					case 3: dir_y--; break;
				}
				if (dir_x > 0 && dir_x < WIDTH && dir_y >= 0 && dir_y < HEIGHT){
					char cell = field[dir_y][dir_x];
					if (!cell || (j == 3 && cell != 'q' && cell != '@' && cell != '?')){
						field[monst[1]][monst[0]] = 0;
						undrawat(monst[0], monst[1]);
						monst[1] = dir_y;
						monst[0] = dir_x;
						field[dir_y][dir_x] = 'q';
					}
					else if (cell == '@' || cell == '?') retry++;
					else continue;
				}
				else continue;
				break;
			}
			drawmob(monst[0], monst[1]);
		}
		gotoxy(1,1); cputs("");
		if (win_condition()){
			if (bombs == bms_lv && bms_lv)
				score += 30;
			dodraw = 1;
			super = 0;
			CURS_HIDE
			clrscr();
			printf("\n\n\tLevel %d clear!\n\n",level);
			if (tutorial) printf("\t Pack:         Tutorial\n");
			else printf("\t Sector:       %u\n", seed);
			printf( "\t Turns:        %d\n"
					"\t @ score gain: %d\n"
					"\t ? score gain: %d\n"
					"\t Charges used: %d\n"
					"\t Retries:      %d\n"
					"\n"
					"\tHit any key... ", 
				turns, score - scr_lv, score2 - scr_lv2, bms_lv - bombs, retries);
			NOSND
			getch();
			gotoxy(1,1); cputs("");
			CURS_SHOW

			level++;
			turns = 0;
			bombs = BOMBS;
			my_srand(my_rand() + clock());
			if (sector_order) seed++;
			else seed = my_rand();
			if (tutorial) bombs = open_level(level);
			else {make_field(); tutorial = 0;}
			if (bombs < 0) {
				tutorial = 0; 
				bombs = BOMBS;
				goto title;
			}
			bms_lv = bombs;
			scr_lv = score;
			scr_lv2 = score2;
			retries = 0;
			store_field();
			p_x = stored_pos[0];
			p_y = stored_pos[1];
			p_x2 = stored_pos2[0];
			p_y2 = stored_pos2[1];
			clrscr();
		}
		if ((air && (p_y + 1 < HEIGHT) && !skip_ai && !field[p_y + 1][p_x]) || 
			(air2 && (p_y2 + 1 < HEIGHT) && !skip_ai && !field[p_y2 + 1][p_x2])) {
			undrawat(p_xt, p_yt);
			drawat(p_x, p_y);
			undrawat(p_xt2, p_yt2);
			drawat2(p_x2, p_y2);
			p_yt = p_y;
			p_xt = p_x;
			p_yt2 = p_y2;
			p_xt2 = p_x2;
			gotoxy(1,1); cputs("");
		}
		for (i = 0; i < 2; i++){
			if (field[p_y - 1][p_x] == '?' || field[p_y2 - 1][p_x2] == '@') bothfall = 1;
			while (air && (p_y + 1 < HEIGHT) && !skip_ai){
				if (p_i == '^' || p_i == '<' || p_i == 'V' || p_i == '>' || p_i == '.')
					if (!bothfall) break;
				if (field[p_y + 1][p_x] == 'q') retry++; 
				if (field[p_y + 1][p_x]) break;
				field[p_y++][p_x] = 0;
				field[p_y][p_x] = '@';
			}
			if (field[p_y - 1][p_x] == '?' || field[p_y2 - 1][p_x2] == '@') bothfall = 1;
			while (air2 && (p_y2 + 1 < HEIGHT) && !skip_ai){
				if (p_i == 'w' || p_i == 'a' || p_i == 's' || p_i == 'd' || p_i == 'b') 
					if (!bothfall) break;
				if (field[p_y2 + 1][p_x2] == 'q') retry++; 
				if (field[p_y2 + 1][p_x2]) break;
				field[p_y2++][p_x2] = 0;
				field[p_y2][p_x2] = '?';
			}
		}
		bothfall = 0;
		if (retry){
			clrscr();
			bombs = bms_lv;
			retries++;
			super = 0;
			super2 = 0;
			turns = 0;
			load_field();
			load_m();
			p_x = stored_pos[0];
			p_y = stored_pos[1];
			p_x2 = stored_pos2[0];
			p_y2 = stored_pos2[1];
			dodraw = 1;
			score = score / 2;
			score2 = score2 / 2;
			retry = 0;
			SND(300)
		}
		if (!field[p_y + 1][p_x] && p_y + 1 < HEIGHT) air++;
		else air = 0;
		if (field[p_y + 1][p_x] == 'q' && p_y + 1 < HEIGHT && !RIDEABLE_MOBS) air++;
		if (!field[p_y2 + 1][p_x2] && p_y2 + 1 < HEIGHT) air2++;
		else air2 = 0;
		if (field[p_y2 + 1][p_x2] == 'q' && p_y2 + 1 < HEIGHT && !RIDEABLE_MOBS) air2++;
		if (field[p_y2 + 1][p_x2] == '@' && air && p_y2 + 1 < HEIGHT) air2++;
		if (field[p_y + 1][p_x] == '?' && air2 && p_y + 1 < HEIGHT) air++;
	}
}
