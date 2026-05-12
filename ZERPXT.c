#include <stdio.h>
#include <time.h>
#include <conio.h>
#include <dos.h>

/* game settings */
#define WIDTH 20
#define HEIGHT 10
#define BOMBS 3
#define MOBS 3
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
#define TBS "\t   "

typedef unsigned long my32;

unsigned char field[HEIGHT][WIDTH];
unsigned char stored_field[HEIGHT][WIDTH];
unsigned int seed;
char facing, air, help = 0, super;
int stored_pos[2];

static my32 my_seed;
void my_srand(my32 s){ my_seed = s; }
my32 my_rand(){
	my_seed = my_seed * 1664525 + 1013904223;
	return my_seed >> 16;
}

char alt_climb(const int p_x, const int p_y, const int super){
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
"0","0\n\n\
    @     #\n\
#######   #\n\
      #   !\n\
      #######!#####\n\
             !\n\
 !  !\n\
#######\n\
      #          !\n\
      #############",
"0\n\n\n\
 #####      ######\n\
 #   ########  ! #\n\
 # @        % ####\n\
 #####!####!###\n\
     ###  ###\n\n\n",
"1\n\n\n\
       #####\n\
       #   #\n\
       #   #\n\
       # @ #\n\
       #####\n\
   #           #\n\
   #!!!!!!!!!!!#\n\
   #!!!!!!!!!!!#",
"0\n\n\
              !\n\
\n\
       #\n\
     #        !\n\
\n\
    #    #   !\n\
      #  #     !\n\
         #\n\
     # @ #    !",
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
			if (F == '@'){
				if (super) textcolor(LIGHTCYAN);
				else textcolor(YELLOW);
				if (air)
					if (facing) cputs(super ? "\311@\274": "\332@\331");
					else cputs(super ? "\310@\273" : "\300@\277");
				else cputs(super ? "\311@\273" : "\332@\277");
				textcolor(LIGHTGRAY);
			}
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
			if (F == '@'){
				if (super) textcolor(LIGHTCYAN);
				else textcolor(YELLOW);
				if (air)
					if (facing) cputs(super ? "\311\312\315": "\311\312\315");
					else cputs(super ? "\315\312\273" : "\315\312\273");
				else cputs(super ? "\311\312\273" : "\311\312\273");
				textcolor(LIGHTGRAY);
			}
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
	gotoxy(9 + x * 3, 3 + y * 2  - help);
	if (air)
		if (facing) cputs(super ? "\311@\274": "\332@\331");
		else cputs(super ? "\310@\273" : "\300@\277");
	else cputs(super ? "\311@\273" : "\332@\277");
	gotoxy(9 + x * 3, 4 + y * 2  - help);
	if (air)
		if (facing) cputs(super ? "\311\312\315": "\311\312\315");
		else cputs(super ? "\315\312\273" : "\315\312\273");
	else cputs(super ? "\311\312\273" : "\311\312\273");
	textcolor(LIGHTGRAY);
}
void undrawat(int x, int y){
	if (field[y][x]) return;
	gotoxy(9 + x * 3, 3 + y * 2  - help);
	cputs("   ");
	gotoxy(9 + x * 3, 4 + y * 2  - help);
	cputs("   ");
}
void drawmob(int x, int y){
	gotoxy(9 + x * 3, 3 + y * 2  - help);
	textcolor(LIGHTMAGENTA); 
	cputs("o\313o");
	gotoxy(9 + x * 3, 4 + y * 2  - help);
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
	x = WIDTH / 2;
	y = HEIGHT - 1;
	while (field[y][x] && y > 0) y--;
	field[y][x] = '@';
	stored_pos[0] = x;
	stored_pos[1] = y;
	load_m();
}

int main(void){
	char tutorial = 0;
	int p_x, p_y;
	unsigned char prev_i = '?';
	unsigned char p_i = '?';
	unsigned char prpr_i = '?';
	unsigned static_seed = 0;
	int score, maxscore;
	int level, turns;
	int bombs, bms_lv;
	int scr_lv, retries;
	int do_sound = 1, mptr = 1;
	char keyword[20] = "", *kwp;
	char isrep = 0, legacy_mode = 0;
	int i, j, dodraw = 1;
	int p_yt = 0, p_xt = 0;
	my_srand(time(NULL));
	title:
	CURS_HIDE
	clrscr();
	textcolor(WHITE);
	putchar('\n');
	cputs("    _____           _           _     ______");					 putchar('\n');
	cputs("   |  __ \\         (_)         | |   |___  /");					 putchar('\n');
	cputs("   | |__) | __ ___  _  ___  ___| |_     / / ___ _ __ _ __");		 putchar('\n');
	cputs("   |  ___/ '__/ _ \\| |/ _ \\/ __| __|   / / / _ \\ '__| '_ \\"); putchar('\n');
	cputs("   | |   | | | (_) | |  __/ (__| |_   / /_|  __/ |  | |_) |");	 putchar('\n');
	cputs("   |_|   |_|  \\___/| |\\___|\\___|\\__| /_____\\___|_|  | .__/");putchar('\n');
	cputs("                  _/ |                              | |");		 putchar('\n');
	cputs("                 |__/                               |_|");		 putchar('\n');
	putchar('\n');
	textcolor(LIGHTGRAY);
	cputs(  "      Goal: break all weak blocks and don't get stuck!");
	while (1){
		gotoxy(1, 13); cputs("");
		if (!legacy_mode) printf(
			"Actions:\n   wasd or arrows - move; b - use charge; l - retry; q or ESC - quit;\n"
			"   h or F1 - tips; v - sound; ENTER - key repeat; SPACE - wait.\n\n\n");
		else printf(
			"Actions:\n   wasd - move; b - use charge; l - retry; q - quit;                 \n"
			"   h - tips; v - sound; ENTER - key repeat; SPACE - wait.      \n\n\n");
		switch (mptr){
		case 0: 
			textcolor(YELLOW);
			cputs("                   > PLAY TUTORIAL <");putchar('\n');
			textcolor(LIGHTGRAY);
			cputs("                      PLAY RANDOM   ");putchar('\n');
			cputs("                      LOAD SECTOR   ");putchar('\n');
			break;
		case 1:
			cputs("                     PLAY TUTORIAL  ");putchar('\n');
			textcolor(YELLOW);
			cputs("                   >  PLAY RANDOM  <");putchar('\n');
			textcolor(LIGHTGRAY);
			cputs("                      LOAD SECTOR   ");putchar('\n');
			break;
		case 2:
			cputs("                     PLAY TUTORIAL  ");putchar('\n');
			cputs("                      PLAY RANDOM   ");putchar('\n');
			textcolor(YELLOW);
			cputs("                   >  LOAD SECTOR  <");putchar('\n');
			textcolor(LIGHTGRAY);
		}
		printf("\n\n\nAltern. controls: adok, numpad.");
		printf("\nKosmoKrab 2026 ");
		if (legacy_mode) printf("(cooked mode, hit L to change) ");
		else printf("(raw mode, hit L to change)    ");
		p_i = getch();
		if (p_i == ' ' || p_i == ENTR) break;
		if (p_i >= 'A' && p_i <= 'Z') p_i += ('a' - 'A');
		if (p_i == ESCP) p_i = getch();
		if (p_i == 's' || p_i == ARR_DN || p_i == 'd' || p_i == ARR_RT)
			mptr = (mptr + 2) % 3;
		if (p_i == 'e' || p_i == 'q' || p_i == 27) 
			{ CURS_SHOW clrscr(); return 0; }
		if (p_i == 'l') legacy_mode = !legacy_mode;
		else mptr = (mptr + 2) % 3;
	}
	CURS_SHOW
	switch (mptr){
	case 0:
		help = 1;
		tutorial = 1;
		static_seed = 0;
	break;
	case 1:
		help = 0;
		tutorial = 0;
		static_seed = 0;
	break;
	case 2:
		help = 0;
		tutorial = 0;
		clrscr();
		printf("\n\nEnter sector id: ");
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
	if (static_seed) 
		{ seed = static_seed; static_seed = 0; }
	else seed = my_rand();
	facing = 1;		air = 0;
	level = 1;		turns = 0;		super = 0;
	score = 0;		maxscore = 0;	bombs = BOMBS;
	
	if (tutorial) bombs = open_level(1);
	else make_field();
	if (bombs < 0) { make_field(); tutorial = 0; bombs = BOMBS; }
	bms_lv = bombs;
	scr_lv = 0;
	retries = 0;
	store_field();
	p_x = stored_pos[0];
	p_y = stored_pos[1];
	clrscr();
	
	while (1){
		int retry = 0;
		int skip_ai = 0;
		gotoxy(12,1);
		textcolor(WHITE);
		cprintf("Level: %d ", level);
		putchar('|');
		if (tutorial) cprintf(" Tutorial ");
		else cprintf(" Sector: %u ", seed);
		putchar('|');
		cprintf(" Last key: %c ", p_i);
		putchar('|');
		if (super) cprintf(" Charged up!");
		else if (bombs) cprintf(" Charges: %d  ", bombs);
		else cprintf(" No charges!");
		printf("\n");
		if (dodraw) draw_field();
		else { 
			undrawat(p_xt, p_yt);
			drawat(p_x, p_y);
		}
		if (help) gotoxy(12,22);
		else gotoxy(12,24);
		textcolor(WHITE);
		cprintf("Turn: %d ", turns);
		textcolor(LIGHTGRAY);
		putch('|');
		textcolor(WHITE);
		cprintf(" Score: %d ", score);
		textcolor(LIGHTGRAY);
		putch('|');
		textcolor(WHITE);
		cprintf(" Session best: %d ", maxscore);
		textcolor(LIGHTGRAY);
		if (help) {
			gotoxy(12,23);
			cputs("  wasd - move; b - charge; l - retry; q - quit;");
			gotoxy(12,24);
			cputs("  v - sound; h - hide; space - wait. ");
		}
		cputs("> ");
		clreol();
		dodraw = 1;
		NOSND
		prpr_i = p_i;
		if (!legacy_mode) p_i = getch();
		else do {
			p_i = getchar();
			if (p_i != '\n') {
				isrep = 0;
				prev_i = p_i;
			} else {
				if (!turns) break;
				if (isrep) p_i = prev_i;
				isrep = 1;
			}
		} while (p_i == '\n');
		undrawat(p_x, p_y);
		p_xt = p_x;
		p_yt = p_y;
		turns++;
		/* Alternative keys for actions */
		if (p_i == ESCP){
			p_i = getch();
			switch (p_i){
				/* Arrow key handling. */
				case ARR_UP: p_i = 'w'; break;
				case ARR_LT: p_i = 'a'; break;
				case ARR_DN: p_i = 's'; break;
				case ARR_RT: p_i = 'd'; break;
			}
		}
		if (p_i == ';') p_i = 'h';
		if (p_i == ENTR) p_i = prpr_i;
		if (p_i >= 'A' && p_i <= 'Z') p_i += ('a' - 'A');
		switch (p_i){
			/* Alternative controls. */
			case 'o': p_i = 'w';  break;
			case 'k': p_i = 's';  break;
			/* Numpad controls. */
			case '7': p_i = 'a';  break;
			case '4': p_i = 'a';  break;
			case '9': p_i = 'd';  break;
			case '6': p_i = 'd';  break;
			case '8': p_i = 'w';  break;
			case '5': p_i = 's';  break;
			case '2': p_i = 's';  break;
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
			
				climb = alt_climb(p_x, p_y, super);
				if (climb && air) p_i = climb;
			}
		}
		/* Applying the action */
		switch (p_i){
			case 27: goto title;
			case ' ': SND(456) dodraw = 0; break;
			case 'w': break;
			case 'q': 
				if (legacy_mode) while (getchar() != '\n');
				goto title;
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
			default: {
				dodraw = 0;
				p_i = '?';
				turns--;
				skip_ai++;
				break;
			}
		}
		if (score > maxscore) maxscore = score;
		
		for (i = 0; i < MOBS + 27 && !skip_ai; i++){
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
					if (!cell || (j == 3 && cell != 'q' && cell != '@')){
						field[monst[1]][monst[0]] = 0;
						undrawat(monst[0], monst[1]);
						monst[1] = dir_y;
						monst[0] = dir_x;
						field[dir_y][dir_x] = 'q';
					}
					else if (cell == '@') retry++;
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
			CURS_HIDE
			clrscr();
			printf("\n\n\tLevel %d clear!\n\n",level);
			if (tutorial) printf("\t Pack:         Tutorial\n");
			else printf("\t Sector:       %u\n", seed);
			printf( "\t Turns:        %d\n"
					"\t Score gain:   %d\n"
					"\t Charges used: %d\n"
					"\t Retries:      %d\n"
					"\n"
					"\tHit any key... ", 
				turns, score - scr_lv, bms_lv - bombs, retries);
			NOSND
			getch();
			gotoxy(1,1); cputs("");
			CURS_SHOW

			level++;
			turns = 0;
			bombs = BOMBS;
			my_srand(my_rand() + clock());
			seed = my_rand();
			if (tutorial) bombs = open_level(level);
			else {make_field(); tutorial = 0;}
			if (bombs < 0) {
				tutorial = 0; 
				bombs = BOMBS;
				goto title;
			}
			bms_lv = bombs;
			scr_lv = score;
			retries = 0;
			store_field();
			p_x = stored_pos[0];
			p_y = stored_pos[1];
			clrscr();
		}
		if (air && (p_y + 1 < HEIGHT) && !skip_ai 
			&& !field[p_y + 1][p_x]) {
			undrawat(p_xt, p_yt);
			drawat(p_x, p_y);
			p_yt = p_y;
			p_xt = p_x;
			gotoxy(1,1); cputs("");
		}
		while (air && (p_y + 1 < HEIGHT) && !skip_ai){
			if (field[p_y + 1][p_x] == 'q') retry++;
			if (field[p_y + 1][p_x]) break;
			field[p_y++][p_x] = 0;
			field[p_y][p_x] = '@';
		}
		if (retry){
			clrscr();
			bombs = bms_lv;
			retries++;
			super = 0;
			turns = 0;
			load_field();
			load_m();
			p_x = stored_pos[0];
			p_y = stored_pos[1];
			dodraw = 1;
			score = score / 2;
			retry = 0;
			SND(300)
		}
		if (!field[p_y + 1][p_x] && p_y + 1 < HEIGHT) air++;
		else air = 0;
		if (field[p_y + 1][p_x] == 'q' && p_y + 1 < HEIGHT && !RIDEABLE_MOBS) air++;
	}
}
