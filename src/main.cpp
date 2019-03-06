#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>

/*
Compiles with:
 cc -o main main.cpp -lncurses
Or with the makefile, just write "make".
 */

// Internal Values
const int WIDTH = 80;
const int HEIGHT = 22;
const int MAX_MOBS = 256;
const int DRUNK_STEPS = 5000;

// Version number
static const char VERSION[] = "v0.1";

// Player Stats and such
int maxhp = 10, hp = maxhp, mp = 0, maxmp = 10, kills = 0, totalKills = 0, level = 1;
char map[WIDTH][HEIGHT];

// Mobs
typedef struct
{
	int mob_x;
	int mob_y;
	char mob_pic;
	int mob_RGB;
} MOB;
MOB mobs[MAX_MOBS];

// This should work as a more uniformly distributed random number generator than
// the basic rand() function.
int random(int min, int max)
{
	return (min + rand() / (RAND_MAX / (max - min) + 1));
}

bool tryMove(int m, int n, bool kill)
{
	if(m < 0 || m >= WIDTH || n < 0 || n >= HEIGHT)
	  return false;

	for(int i = 0; i < MAX_MOBS; i++)
	{
		if(mobs[i].mob_x == m && mobs[i].mob_y == n)
		{
			if(mobs[i].mob_pic == ' ')
			{
				continue;
			}

			if(kill)
			{
				if(mp < maxmp) mp += 1;
				kills += 1;
        totalKills += 1;

				mobs[i].mob_pic = ' ';
				mobs[i].mob_x = 0;
				mobs[i].mob_y = 0;
			}
			else
			  return false;
		}
	}

  // This is deliberate fallthrough so that the player moves forward when attacking
	// and cannot just kill monsters standing in a line.
	if(map[m][n] == '.')
	  return true;

	/*
	// Jewels of power
	if(map[m][n] == '*')
	  // TODO
	  return true;
	*/

	return false;
}

void mobAttack(int i)
{
  if(mobs[i].mob_RGB == 6)
  {
    hp -= 2;
  }
  else
  {
    hp -= 1;
  }

  return;
}

void mobMove(int x, int y)
{
	for(int i = 0; i < MAX_MOBS; i++)
	{
		// These are dead mobs.
		if(mobs[i].mob_pic == ' ')
		{
			continue;
		}
		// TODO: Mob special actions.
		/*
		// 'X' teleports around.
		if(mobs[i].mob_pic == 'x' && random(1, 10) == 1)
		{
			do
			{
				mobs[i].mob_x = random(1, (WIDTH - 1));
				mobs[i].mob_y = random(1, (HEIGHT - 1));
			} while(!tryMove(mobs[i].mob_x, mobs[i].mob_y, false));
		}
		*/

		// Get direction to move closer to player.
		int dx = std::max(-1, std::min(1, x - mobs[i].mob_x));
    int dy = std::max(-1, std::min(1, y - mobs[i].mob_y));
    int m1,n1,m2,n2;

		m1 = mobs[i].mob_x + dx;
		n1 = mobs[i].mob_y + dy;
		m2 = mobs[i].mob_x + random(0, 3) - 1;
		n2 = mobs[i].mob_y + random(0, 3) - 1;

		switch (mobs[i].mob_pic)
		{
			// Vowels can move diagonally.
			case 'a':	case 'e': case 'i':
			case 'o': case 'u':	case 'y':
			{
				if(tryMove(m1, n1, false))
				{
					if(m1 == x && n1 == y) // Attack the player.
					{
						mobAttack(i);
					}
					else // Move.
					{
						mobs[i].mob_x = m1;
						mobs[i].mob_y = n1;
					}
				}
				// Random movement if we cannot get closer to player.
				else if(tryMove(m2, n2, false))
				{
					// Player should not be here, but better safe than sorry.
					if(m2 == x && n2 == y)
					{
						mobAttack(i);
					}
					else
					{
						mobs[i].mob_x = m2;
						mobs[i].mob_y = n2;
					}
				}
				break;
			}
			default:
			{
				if(random(1,2) == 1)
				{
					if(tryMove(m1, mobs[i].mob_y, false))
					{
						if(m1 == x && mobs[i].mob_y == y)
						{
							mobAttack(i);
						}
						else
						{
							mobs[i].mob_x = m1;
						}
					}
					else if(tryMove(mobs[i].mob_x, n1, false))
					{
						if(mobs[i].mob_x == x && n1 == y)
						{
							mobAttack(i);
						}
						else
						{
							mobs[i].mob_y = n1;
						}
					}
				}
				else
				{
					if(tryMove(mobs[i].mob_x, n1, false))
					{
						if(mobs[i].mob_x == x && n1 == y)
						{
							mobAttack(i);
						}
						else
						{
							mobs[i].mob_y = n1;
						}
					}
					else if(tryMove(m1, mobs[i].mob_y, false))
					{
						if(m1 == x && mobs[i].mob_y == y)
						{
							mobAttack(i);
						}
						else
						{
							mobs[i].mob_x = m1;
						}
					}
					break;
				}
			}
		}
	}
	return;
}

int reqKills()
{
	int i = 8 + (2 * level);

	if(i > MAX_MOBS)
	  return MAX_MOBS;
	else
	  return i;
}

void makeMap(int x, int y)
{
	int m,n,rng;

	for(m = 0; m < WIDTH; m++)
	  for(n = 0; n < HEIGHT; n++)
		{
			map[m][n] = '#';
		}

	m = x;
	n = y;

	for(int i = 0; i < DRUNK_STEPS; i++)
	{
		map[m][n] = '.';
		rng = random(0, 4);

		switch(rng)
		{
			case 0: if(m < WIDTH - 2) m++; break;
			case 1: if(m > 1) m--; break;
			case 2: if(n < HEIGHT - 2) n++; break;
			case 3: if(n > 1) n--; break;
			default: m = x; n = y; break;
		}
	}

	for(int i = 0; i < reqKills(); i++)
	{
		if(i > MAX_MOBS)
		  break;

		do
		{
			m = random(1, (WIDTH - 1));
			n = random(1, (HEIGHT - 1));
		} while(!tryMove(m, n, false));

		mobs[i].mob_x = m;
		mobs[i].mob_y = n;
		mobs[i].mob_pic = (char)random(97, 122);
		mobs[i].mob_RGB = random(2, 7); // Don't use white for mob_RGB, they are hard to see.
	}

	return;
}

void initColors()
{
  if(has_colors() == false) // Checks whether the terminal supports displaying color.
	{
    endwin();
    printf("Your terminal does not support color.\n");
    exit(1);
  }

	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_RED, COLOR_BLACK);
	init_pair(3, COLOR_GREEN, COLOR_BLACK);
	init_pair(4, COLOR_YELLOW, COLOR_BLACK);
	init_pair(5, COLOR_BLUE, COLOR_BLACK);
	init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(7, COLOR_CYAN, COLOR_BLACK);
	init_pair(8, COLOR_BLACK, COLOR_WHITE);
}

void startScreen()
{
  //move(0, 0);
	attron(COLOR_PAIR(2));
  move(0, (WIDTH / 2) - 7);
	addstr("ENDLESS DESCENT");
  move(1, (WIDTH / 2) - 15);
  addstr("INTO THE DARK AND DANK DUNGEONS");
  move(2, (WIDTH / 2) - 24);
  addstr("OF THE MOST REPREHENSIBLE ALPHABETICAL OVERLORD\n\n");
	attroff(COLOR_PAIR(2));

	addstr("Those dastardly minions of the Dark Alphabetical Overlord\n");
	addstr("have killed your family, ate your pet, raped your potted\n");
  addstr("plant and TRAMPLED YOUR LAWN! Delve into the perilous and\n");
	addstr("highly unpleasant Vault of Writing and slay as many of them\n");
	addstr("as you can before you find your inevitable, bloody end.\n\n");

  addstr("Made as a part of the Seven Day Roguelike Challenge 2019, ");
  addstr(VERSION);

  attron(COLOR_PAIR(5));
  addstr("\n\nControls:\n");
  attroff(COLOR_PAIR(5));
	addstr("Arrow keys to move, or (q)uit.\n");
	addstr("Use mana to\n  (.)wait [1 mana]\n  (t)eleport [3 mana]\n");
	addstr("  (w)hirlwind attack [3 mana]\n  (f)reeze time [5 mana]\n  (h)eal [10 mana]\n");
	addstr("Gain mana by killing enemies.\n\n[press any key to start]");
	getch();
}

/* MAIN LOOP */
int main(void)
{
	srand(time(0)); // Set seed for random generation.

	initscr(); /* Start curses mode. */
	cbreak(); // One keystroke at a time.
	noecho(); // Do not print keystrokes.
	keypad(stdscr, TRUE); // recognize special keys.
	start_color(); // Colored text.
	initColors(); // Initialize color pairs.

  // Let's rumble!
  startScreen();

	bool gameOn = true;
	int timeStop = 0;
	int x, y;

	makeMap(x, y);

	while(gameOn)
  {
    // Initialize dungeon
    x = random(1, (WIDTH - 1));
  	y = random(1, (HEIGHT - 1));

  	makeMap(x, y);

    while(gameOn)
  	{
  		for(int m = 0; m < WIDTH; m++)
  		  for(int n = 0; n < HEIGHT; n++)
  			{
  				move(n, m);
  				addch(map[m][n]);
  			}

      if(timeStop <= 0)
  		  mobMove(x, y);
  		else
  		  timeStop -= 1;

  		for(int i = 0; i < MAX_MOBS; i++)
  		{
  			if(mobs[i].mob_pic == ' ')
  			  continue;

  			move(mobs[i].mob_y, mobs[i].mob_x);
  			attron(COLOR_PAIR(mobs[i].mob_RGB));
  			addch(mobs[i].mob_pic);
  			attroff(COLOR_PAIR(mobs[i].mob_RGB));
  		}

  		if(hp > maxhp)
  		  hp = maxhp;
  		if(mp > maxmp)
  		  mp = maxmp;

  		move(22, 0);
  		printw("Health: %i/%i  Mana: %i/%i  Kills: %i/%i  Level: %i", hp, maxhp, mp, maxmp, kills, reqKills(), level);
  		// TODO: Color Health and Mana?
  		move(23, 0);
  		printw("   (.)wait   (t)eleport   (w)hirlwind attack   (f)reeze time   (h)eal   (q)uit");
  		// BUG workaround:
  		move(0, 0);
  		printw("##");

  		move(y, x);
  		//addch('@');
  		echochar('@'); // Works as addch() and refresh() together.
  		move(y, x); // To get the cursor blinking over our '@'.
  		//refresh(); /* Print it on to the real screen. */

      /* USER INPUT */
  		int cmd;
  		bool input = false;
  		while(!input && (hp > 0)) // I had to add hp check because of high damage mobs.
  		{
  			cmd = getch();
  			switch (cmd)
  			{
  				case KEY_UP:
  				{
  					if(tryMove(x, y-1, true))
  					{
  						input = true;
  						y--;
  					}
  					break;
  				}
  				case KEY_DOWN:
  				{
  					if(tryMove(x, y+1, true))
  					{
  						input = true;
  						y++;
  					}
  					break;
  				}
  				case KEY_RIGHT:
  				{
  					if(tryMove(x+1, y, true))
  					{
  						input = true;
  						x++;
  					}
  					break;
  				}
  				case KEY_LEFT:
  				{
  					if(tryMove(x-1, y, true))
  					{
  						input = true;
  						x--;
  					}
  					break;
  				}
  				case '.': // wait
  				{
  					if(mp < 1) break;

  					mp -= 1;
  					input = true;
  					break;
  				}
  				case 'f': // freeze time
  				{
  					if(mp < 5) break;

  					timeStop = random(3,6);

  					mp -= 5;
  					input = true;
  					break;
  				}
  				case 'h': // heal
  				{
  					if(mp >= 10 && hp < maxhp)
  					{
  						hp += 1;
  						mp -= 10;
  						input = true;
  					}
  					break;
  				}
  				case 'q':
  				{
  					input = true;
  					//gameOn = false;
            endwin();
            exit(0);
  					break;
  				}
  				case 't': // teleportation
  				{
  					if(mp < 3) break;

  					do
  					{
  						x = random(1, (WIDTH - 1));
  						y = random(1, (HEIGHT - 1));
  					} while(!tryMove(x, y, false));

  					mp -= 3;
  					input = true;
  					break;
  				}
  				case 'w': // whirlwind attack
  				{
  					if(mp < 3) break;

  					for(int m = x - 1; m <= x + 1; m++)
  						for(int n = y - 1; n <= y + 1; n++)
  						{
  							for(int i = 0; i < MAX_MOBS; i++)
  							{
  								if(mobs[i].mob_x == m && mobs[i].mob_y == n)
  								{
  										kills += 1;
                      totalKills += 1;

  										mobs[i].mob_pic = ' ';
  										mobs[i].mob_x = 0;
  										mobs[i].mob_y = 0;
  								}
  							}
  						}

  					mp -= 3;
  					input = true;
  					break;
  				}
          //case '?': // help screen
  				default: break;
  			}
  		}

  		erase();

  		if(hp <= 0)
  		{
  			move(0, 0);
  			printw("You die...\n\n");
        printw("Congratulations, you have killed %i minions of the\n", totalKills);
        printw("despicable Alphabetical Overlord and reached level %i!\n\n", level);
        printw("Better luck next life.\n\n");
  			getch();
  			gameOn = false;
  		}
  		else if(kills >= reqKills())
  		{
  			move(0, 0);
  			addstr("Level up!\n\n[press any key to continue]");
  			getch();

  			kills = 0;
  			level += 1;

  			if(level >= MAX_MOBS) // VICTORY
        {
          erase();
          move(0, 0);
    			addstr("Inconceivable!\n\nAstounding!\n\nI believe it not!\n\n");
          printw("You have killed %i alphabetical minions and reached\n", totalKills);
          printw("level %i. The fearsome and fearless Dark Alphabetical\n", level);
          addstr("Overlord ran like a chicken. You thought about chasing\n");
          addstr("after him and finishing him in a satisfactory show of\n");
          addstr("over-the-top violence and splashes or red ink, but\n");
          addstr("then you decide for a fate worse than death: You will\n");
          addstr("eat his lunch and draw obscene images on his walls!\n\n");
          addstr("MWHAHAHAHAHA!!! That'll teach him!\n");
          getch();
          erase();
          move(0, 0);
          addstr("You are victorious!\n");
    			getch();
          gameOn = false;
          break;
        }
        else
        {
          makeMap(x, y);

    			do
    			{
    				x = random(1, (WIDTH - 1));
    				y = random(1, (HEIGHT - 1));
    			} while(!tryMove(x, y, false));
        }
  		}
  	}

    addstr("Try again? [y/n]\n");
    char q;

    while(true)
    {
      q = getch();

      if(q == 'y')
      {
        gameOn = true;
        break;
      }
      else if(q == 'n')
        break;
    }
  }

	endwin();
	return 0;
}
