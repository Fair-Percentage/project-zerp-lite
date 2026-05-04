#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* game settings */
#define WIDTH 20
#define HEIGHT 10
#define BOMBS 3
#define MOBS 3
#define TBS "\t   "
#define TOPPAD ""
#define RIDEABLE_MOBS 1

#ifdef __linux__
	/* Linux specific code */
	#include <termios.h>
	#include <unistd.h>
	#define CURS_HIDE printf("\033[?25l");
	#define CURS_SHOW printf("\033[?25h");
	#define MENU_CLEAR "\033[H"
	#define CLEAR_TO_END "\033[0J"
	#define LONG_TERM 0
	#define ESCP 27
	#define ENTR 10
	#define ARR_UP 'A'
	#define ARR_LT 'D'
	#define ARR_DN 'B'
	#define ARR_RT 'C'
	#define NOSND
	#define SND(f)
	#define SND_TTIP
	#define ESC_CHECK if(getch()!='['){printf("\033c\033[2J\033[H");CAN_ON return 0;}
	#define ESC_CHECK2 if(getch()!='['){printf("\033c\033[2J\033[H");goto title;}
	#define WAITASEC if(p_i!='\n')nanosleep(&(struct timespec){.tv_sec=0,.tv_nsec=20000000L},0);
	#define CAN_ON can_on();
	#define CAN_OFF can_off();
	typedef unsigned my32;
	
	void can_off(void){
		struct termios a;
		tcgetattr(0, &a);
		a.c_lflag &= ~ICANON;
		tcsetattr(0, TCSANOW, &a);
	}
	void can_on(void){
		struct termios a;
		tcgetattr(0, &a);
		a.c_lflag |= ICANON;
		tcsetattr(0, TCSANOW, &a);
	}
	char getch(void)
		{ return getchar(); }
#else
	/* Code for MS-DOS and Windows */
	#include <conio.h>
	#define ENTR 13
	#define ESC_CHECK
	#define ESC_CHECK2
	#define ARR_UP 'H'
	#define ARR_LT 'K'
	#define ARR_DN 'P'
	#define ARR_RT 'M'
	#define CAN_ON
	#define CAN_OFF

    #ifdef _WIN32
		/* Windows specific code */
		/* Tested with CGA palette in Windows Terminal */
		#define NOSND
		#define SND(f)
		#define SND_TTIP
		#define WAITASEC
		#define CURS_HIDE printf("\033[?25l");
		#define CURS_SHOW printf("\033[?25h");
		#define CLEAR_TO_END "\033[0J"
		#define LONG_TERM 1
		#define MENU_CLEAR "\033c\033[H\033[2J"
		#define ESCP 224 || p_i == 0
		typedef unsigned my32;
	#else 
		/* MS-DOS specific code */
		#include <dos.h>
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
		#define SND_TTIP "v - sound; "
		#define MENU_CLEAR "\033[H"
		#define CLEAR_TO_END "\033[0K"
		#define CURS_HIDE hide_cursor();
		#define CURS_SHOW show_cursor();
		#define LONG_TERM 0
		#define WAITASEC
		#define ESCP 0
		typedef unsigned long my32;
	#endif
#endif

/* ANSI codes */
#define CLEAR_TERM "\033c\033[H\033[2J"
#define SETCUR "\033[H"
#define RESET "\033[0m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[1;33m"
#define CYAN "\033[0;36m"
#define MAGENTA "\033[1;35m"
#define RED "\033[1;31m"
#define BR "\033[1m"
#define DK "\033[0;37m"
#define SEP "\033[0m|\033[1m"
#define CYAN_BR "\033[1;36m"

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
	printf(TBS);
	if (!help || LONG_TERM){
		for (j = 1; j < WIDTH; j++) printf("\315\304\315");
		printf("\n"TBS);
	}
	for (i = 0; i < HEIGHT; i++){
		for (j = 1; j < WIDTH; j++){
			F = field[i][j];
			if (F == '@'){
				if (super)
					if (air)
						if (facing) printf(CYAN_BR"\311@\274" RESET);
						else printf(CYAN_BR"\310@\273" RESET);
					else printf(CYAN_BR"\311@\273" RESET);
				else 
					if (air)
						if (facing) printf(YELLOW"\332@\331" RESET);
						else printf(YELLOW"\300@\277" RESET);
					else printf(YELLOW"\332@\277" RESET);
			}
			else if (F == 'q') printf(MAGENTA "o\313o" RESET);
			else if (F) printf("%c%c%c", 175 + F, 175 + F, 175 + F);
			else printf("   ");
		}
		printf("\n"TBS);
		for (j = 1; j < WIDTH; j++){
			F = field[i][j];
			if (F == '@'){
				if (super)
					if (air)
						if (facing) printf(CYAN_BR"\311\312\315" RESET);
						else printf(CYAN_BR"\315\312\273" RESET);
					else printf(CYAN_BR"\311\312\273" RESET);
				else
					if (air)
						if (facing) printf(YELLOW"\311\312\315" RESET);
						else printf(YELLOW"\315\312\273" RESET);
					else printf(YELLOW"\311\312\273" RESET);
			}
			else if (F == 'q') printf(MAGENTA "\311\316\273" RESET);
			else if (F) printf("%c%c%c", 175 + F, 175 + F, 175 + F);
			else printf("   ");
		}
		printf("\n"TBS);
	}
	if (!help || LONG_TERM){
		for (j = 1; j < WIDTH; j++) printf("\315\304\315");
		printf("\n"TBS);
	}
	CURS_SHOW
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
	int i, j;
	my_srand(time(NULL));
	title:
	CAN_OFF
	CURS_HIDE
	printf(CLEAR_TERM);
	while (1){
		printf(MENU_CLEAR"\n\033[1m"
			"    _____           _           _     ______\n"           
			"   |  __ \\         (_)         | |   |___  /\n"
			"   | |__) | __ ___  _  ___  ___| |_     / / ___ _ __ _ __\n"
			"   |  ___/ '__/ _ \\| |/ _ \\/ __| __|   / / / _ \\ '__| '_ \\\n"
			"   | |   | | | (_) | |  __/ (__| |_   / /_|  __/ |  | |_) |\n"
			"   |_|   |_|  \\___/| |\\___|\\___|\\__| /_____\\___|_|  | .__/\n"
			"                  _/ |                              | |\n"
			"                 |__/                               |_|\n\n\033[0m"
			"      Goal: break all weak blocks and don't get stuck!\n\n");
		if (!legacy_mode) printf(
			"Actions:\n   wasd or arrows - move; b - use charge; l - retry; q or ESC - quit;\n"
			"   h or F1 - tips; "SND_TTIP"ENTER - key repeat; SPACE - wait.\n\n\n");
		else printf(
			"Actions:\n   wasd - move; b - use charge; l - retry; q - quit;\033[0K\n"
			"   h - tips; "SND_TTIP"ENTER - key repeat; SPACE - wait.\033[0K\n\n\n");
		switch (mptr){
		case 0: 
			printf("\033[0K                   \033[1;33m> PLAY TUTORIAL <\033[0m\n"
				   "\033[0K                      PLAY RANDOM\n"
				   "\033[0K                      LOAD SECTOR\n\n");
			break;
		case 1:
			printf("\033[0K                     PLAY TUTORIAL\n"
				   "\033[0K                   \033[1;33m>  PLAY RANDOM  <\033[0m\n"
				   "\033[0K                      LOAD SECTOR\n\n");
			break;
		case 2:
			printf("\033[0K                     PLAY TUTORIAL\n"
				   "\033[0K                      PLAY RANDOM\n"
				   "\033[0K                   \033[1;33m>  LOAD SECTOR  <\033[0m\n\n");
		}
		printf("\n\nKosmoKrab 2026 \033[0K");
		if (legacy_mode) printf("(legacy mode) ");
		p_i = getch();
		if (p_i == ' ' || p_i == ENTR) break;
		if (p_i >= 'A' && p_i <= 'Z') p_i += ('a' - 'A');
		if (p_i == ESCP) { ESC_CHECK p_i = getch(); }
		if (p_i == 's' || p_i == ARR_DN || p_i == 'd' || p_i == ARR_RT)
			mptr = (mptr + 2) % 3;
		if (p_i == 'e' || p_i == 'q' || p_i == 27) 
			{ CAN_ON CURS_SHOW printf(CLEAR_TERM); return 0; }
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
		printf(CLEAR_TERM"\n\nEnter sector id: ");
		CAN_ON
		if(!scanf("%u", &static_seed)){
			static_seed = 0;
			scanf("%s", keyword);
			for (kwp = keyword; *kwp; kwp++)
				static_seed += *kwp * (kwp - keyword);
		}
	break;
	}
	p_i = '?';
	if (legacy_mode) {CAN_ON}
	else {CAN_OFF}
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
	printf(CLEAR_TERM);
	
	while (1){
		int retry = 0;
		int skip_ai = 0;
		printf(TOPPAD TBS);
		printf(BR"Level: %d ", level);
		if (tutorial) printf(SEP" Tutorial ");
		else printf(SEP" Sector: %u ", seed);
		printf(SEP" Last key: %c "SEP" ", p_i);
		if (super) printf("Charged up!");
		else if (bombs) printf("Charges: %d", bombs);
		else printf("No charges!");
		printf(DK"\033[0K\n");
		draw_field();
		printf(BR"Turn: %d "SEP" Score: %d "SEP" Session best: %d \033[0K"DK, turns, score, maxscore);
		if (help && LONG_TERM) printf("\n"TBS"Actions:");
		if (help) printf("\033[0K\n"TBS"  wasd - move; b - charge; l - retry; q - quit;\033[0K\n"
									TBS"  "SND_TTIP"h - hide; space - wait. ");
		printf("> "CLEAR_TO_END);
		NOSND
		prpr_i = p_i;
		if (!legacy_mode) p_i = getch();
		else do {
			p_i = getchar();
			if (p_i != '\n') {
				isrep = 0;
				prev_i = p_i;
			} else {
				if (isrep) p_i = prev_i;
				isrep = 1;
			}
		} while (p_i == '\n');
		turns++;
		/* Alternative keys for actions */
		if (p_i == ESCP){
			ESC_CHECK2
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
			case 'z':  p_i = 'a';  break;
			case 'x':  p_i = 'd';  break;
			case '\'': p_i = 'w'; break;
			case '/':  p_i = 's';  break;
		}
		if (p_i == 'w' && p_y > 0){
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
				field[p_y][p_x] = '@';
			
				climb = alt_climb(p_x, p_y, super);
				if (climb && air) p_i = climb;
			}
		}
		/* Applying the action */
		switch (p_i){
			case 27: goto title;
			case ' ': SND(456) printf("\033c\033[2J"); break;
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
				printf("\033c\033[2J");
				turns--;
				skip_ai++;
				break;
			}
			case 'v': {
				do_sound = !do_sound;
				turns--;
				skip_ai++;
				break;
			}
			case 'b': {
				if (bombs && !super){
					SND(999)
					super = 1;
					bombs--;
				}
				break;
			}
			case 'a': {
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
					if (field[p_y-1][p_x-1] < 3 && 
						field[p_y-1][p_x] < 3){
							if (field[p_y - 1][p_x]) {score++; SND(560)}
							if (field[p_y - 1][p_x - 1]) {score++; SND(560)}
							field[p_y][p_x] = 0;
							field[p_y - 1][p_x] = 0;
							p_y--;
							p_x--;
							field[p_y][p_x] = '@';
						}
				}
				break;
			}
			case 'd': {
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
					if (field[p_y-1][p_x+1] < 3 && 
						field[p_y-1][p_x] < 3){
							if (field[p_y - 1][p_x]) {score++; SND(560)}
							if (field[p_y - 1][p_x + 1]) {score++; SND(560)}
							field[p_y][p_x] = 0;
							field[p_y - 1][p_x] = 0;
							p_y--;
							p_x++;
							field[p_y][p_x] = '@';
						}
				}
				break;
			}
			case 's': {
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
		}
		printf(SETCUR);
		if (win_condition()){
			if (bombs == bms_lv && bms_lv)
				score += 30;

			CURS_HIDE
			printf("\033c\033[2J\n\n\tLevel %d clear!\n\n",level);
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
			printf(SETCUR);
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
			printf("\033c\033[2J");
		}
		if (air && (p_y + 1 < HEIGHT) && !skip_ai 
			&& !field[p_y + 1][p_x]) {
			printf(TOPPAD TBS);
			printf(BR"Level: %d ", level);
			if (tutorial) printf(SEP" Tutorial ");
			else printf(SEP" Sector: %u ", seed);
			printf(SEP" Last key: %c "SEP" ", p_i);
			if (super) printf("Charged up!");
			else if (bombs) printf("Charges: %d", bombs);
			else printf("No charges!");
			printf(DK"\033[0K\n");
			draw_field();
			printf(BR"Turn: %d "SEP" Score: %d "SEP" Session best: %d "DK">\033[0K", 
					turns, score, maxscore);
			printf(SETCUR);
			WAITASEC
		}
		while (air && (p_y + 1 < HEIGHT) && !skip_ai){
			if (field[p_y + 1][p_x] == 'q') retry++; 
			if (field[p_y + 1][p_x]) break;
			field[p_y++][p_x] = 0;
			field[p_y][p_x] = '@';
		}
		if (retry){
			printf("\033c\033[2J");
			bombs = bms_lv;
			retries++;
			super = 0;
			turns = 0;
			load_field();
			load_m();
			p_x = stored_pos[0];
			p_y = stored_pos[1];
			score = score / 2;
			retry = 0;
			SND(300)
		}
		if (!field[p_y + 1][p_x] && p_y + 1 < HEIGHT) air++;
		else air = 0;
		if (field[p_y + 1][p_x] == 'q' && p_y + 1 < HEIGHT && !RIDEABLE_MOBS) air++;
	}
}
