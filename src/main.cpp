/*
 * Endless Descent Into the Dark and Dangerous Dungeons
 * of the Most Reprehensible Alphabetical Overlord:
 * Quest for Gore and Vengenace
 *
 * main.cpp
 * (c) red_kangaroo, 2019
 * Released under the GNU GPL
 *
 * Simple coffebreak roguelike written for the Seven Day Roguelike Challenge 2019.
 *
 * time_started = 5th March 2019, 20:11
 * time_finished = 9th March 2019, XXX
 */

#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <algorithm>
#include <cmath>

//#define DEBUG

/*
 * Compiles with:
 *  cc -o main main.cpp -lncurses -lm
 * Or with the makefile, just use "make".
 */

// Internal Values
const int WIDTH = 80;
const int HEIGHT = 22;
const int MAX_MOBS = 256;
const int DRUNK_STEPS = 5000;
const int LIGHT = 8;

// Version number
static const char VERSION[] = "v0.4";

// Player Stats and such
int x, y;
int maxhp = 10, hp = maxhp, mp = 0, maxmp = 10;
int kills = 0, totalKills = 0, level = 1;
// Status effects
int timeStop = 0, blind = 0, invincible = 0;
bool freeWait = false;

// MAP
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
// the basic rand() function. Maybe.
int random(int min, int max)
{
	return (min + rand() / (RAND_MAX / (max - min) + 1));
}

// 50/50
bool rand_2()
{
	return (rand() % 2 == 0);
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
				if(isupper(mobs[i].mob_pic))
				{
					// Rewards for killing bosses:
					// Always give small increase to stats.
					rand_2() ? (maxhp += 1) : (maxmp += 1);

					// More specific rewards.
					switch (mobs[i].mob_RGB)
					{
						case 1: maxhp += 1; break; // COLOR_WHITE
						case 2: hp = maxhp; break; // COLOR_RED
						case 3: // COLOR_GREEN
						{
							if(timeStop > 0)
							{
								// Green is immune to cheap tricks.
								return false;
							}
							else
							  timeStop = 10;
							break;
						}
						case 4: // COLOR_YELLOW
						{
							invincible = 10;
							break;
						}
						case 5: // COLOR_BLUE
						{
							maxhp += 1;
							maxmp += 1;
							break;
						}
						case 6: freeWait = true; break; // COLOR_MAGENTA
						case 7: // COLOR_CYAN
						{
							mobs[i].mob_pic = tolower(mobs[i].mob_pic); // Make the letter lowercase.
							timeStop -= 1; // Loose one turn, otherwise it would return as not
							               // taking any time.
              maxmp += 1;    // Still gain a reward as you technically "killed" it.
							return false;  // This should work instead of break, right?
						}
						case 8: // COLOR_BLACK
						{
							// Cannot be harmed by bump attack.
							// Reward is implemented in whirlwind attack code.
							return false;
						}
						default: freeWait = true; break;
					}
				}

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

	return false;
}

bool ghostMove(int m, int n)
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

		  return false;
		}
	}

	return true;
}

void mobAttack(int i)
{
	if(invincible <= 0)
	{
		if(mobs[i].mob_RGB == 6) // Magenta mobs
	  {
	    hp -= 2;
	  }
	  else
	  {
	    hp -= 1;
	  }
	}

  // Letters with tittles drain MP.
	if((mobs[i].mob_pic == 'i' || mobs[i].mob_pic == 'j') && mp > 0)
	{
		mp -= 1;
	}

	if(isupper(mobs[i].mob_pic))
	{
		switch (mobs[i].mob_RGB)
		{
			case 1: // COLOR_WHITE
			{
				maxhp -= 1;
				break;
			}
			case 2: // COLOR_RED
			{
				do // Teleport player
				{
					x = random(1, (WIDTH - 1));
					y = random(1, (HEIGHT - 1));
				} while(!tryMove(x, y, false));

				break;
			}
			case 3: // COLOR_GREEN
			{
				maxmp -= 1;
				break;
			}
			case 4: // COLOR_YELLOW
			{
				// Switch hp and mp, which can be lethal!
				hp = hp + mp;
        mp = hp - mp;
        hp = hp - mp;
				break;
			}
			default: break; // Blue, cyan, magenta and black have effects elsewhere.
		}
	}

	switch (mobs[i].mob_pic)
	{
		// Voiceless consonants will blind you.
		case 'f': case 'k': case 'p':
		case 's': case 't': case 'F':
    case 'K': case 'P': case 'S':
		case 'T':
		{
			if(blind <= 0)
				blind = random(1, 4);
			break;
		}
		default: break;
	}

  return;
}

void mobMove()
{
	for(int i = 0; i < MAX_MOBS; i++)
	{
		// These are dead mobs.
		if(mobs[i].mob_pic == ' ')
		{
			continue;
		}

		// Mob special actions:
		if(isupper(mobs[i].mob_pic))
		{
			if(mobs[i].mob_RGB == 5) // Capital blue
			{
				// Try summoning a random mob.
				int j = random(1, MAX_MOBS - 1);

				if(mobs[j].mob_pic == ' ')
				{
					int p, q;

					do
					{
						p = random(1, (WIDTH - 1));
						q = random(1, (HEIGHT - 1));
					} while(!tryMove(p, q, false));

					mobs[j].mob_x = p;
					mobs[j].mob_y = q;
					mobs[j].mob_pic = (char)random(97, 122);
					mobs[j].mob_RGB = random(2, 7);

					continue; // Pass the turn.
				}
			}

			if(mobs[i].mob_RGB == 2 && random(1, 10) == 1) // Capital red
			{
				int p, q;

				do // Teleport
				{
					p = random(1, (WIDTH - 1));
					q = random(1, (HEIGHT - 1));
				} while(!tryMove(p, q, false));

				mobs[i].mob_x = p;
				mobs[i].mob_y = q;

				continue; // Pass the turn.
			}
		}

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
			case 'A':	case 'E': case 'I':
			case 'O': case 'U':	case 'Y':
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
			case 'x': case 'X': // X can phase.
			{
				if(rand_2())
				{
					if(ghostMove(m1, mobs[i].mob_y))
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
					else if(ghostMove(mobs[i].mob_x, n1))
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
					if(ghostMove(mobs[i].mob_x, n1))
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
					else if(ghostMove(m1, mobs[i].mob_y))
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
				}

				break;
			}
			default:
			{
				if(rand_2())
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
				}

				break;
			}
		}
	}
	return;
}

int reqKills()
{
	int i = 8 + (2 * level);

	if(i >= MAX_MOBS)
	  return MAX_MOBS;
	else
	  return i;
}

void makeMap()
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

	int mob_no = reqKills();
	if(level % 2 == 0) // Even floors have a bonus boss.
	  mob_no += 1;

	for(int i = 0; i < mob_no; i++)
	{
		if(i >= MAX_MOBS)
		  break;

		do
		{
			m = random(1, (WIDTH - 1));
			n = random(1, (HEIGHT - 1));
		} while(!tryMove(m, n, false));

		mobs[i].mob_x = m;
		mobs[i].mob_y = n;

    if((level % 2 == 0) && i == 0) // Add one capital boss per even floor.
		{
			mobs[i].mob_pic = (char)random(65, 90);
			mobs[i].mob_RGB = random(1, 8); // Hmm, I can't seem to randomly get black boss...
		}
		else
		{
			mobs[i].mob_pic = (char)random(97, 122);
			mobs[i].mob_RGB = random(2, 7); // Don't use white for mob_RGB, they are hard to see.
		}
	}

  timeStop = 1; // Free turn at beginning to prevent dying to starting swarmed.
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
  move(0, (WIDTH / 2) - 8);
	addstr("ENDLESS DESCENT");
  move(1, (WIDTH / 2) - 18);
  addstr("INTO THE DARK AND DANGEROUS DUNGEONS");
  move(2, (WIDTH / 2) - 24);
  addstr("OF THE MOST REPREHENSIBLE ALPHABETICAL OVERLORD:");
	move(3, (WIDTH / 2) - 14);
  addstr("QUEST FOR GORE AND VENGEANCE\n\n");
	attroff(COLOR_PAIR(2));

	addstr("Those dastardly minions of the ");
	attron(COLOR_PAIR(6)); addstr("Dark Alphabetical Overlord\n"); attroff(COLOR_PAIR(6));
	addstr("have killed your family, ate your pet, raped your potted\n");
  addstr("plant and TRAMPLED YOUR LAWN! Delve into the perilous and\n");
	addstr("highly unpleasant ");
	attron(COLOR_PAIR(7)); addstr("Vault of Writing"); attroff(COLOR_PAIR(7));
	addstr(" and slay as many of them\n");
	addstr("as you can before you find your inevitable, ");
	attron(COLOR_PAIR(2)); addstr("bloody end"); attroff(COLOR_PAIR(2));
	addstr(".\n\n");
	// There's definitely easier and more elegant way of doing this. I'm not going
	// to sweat it now, though.

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

void eraseLastLine()
{
	move(23, 0);
	printw("                                                                                ");
	return;
}

bool checkFov(int m, int n)
{
	// Is map[m][n] visible?
	float vx,vy,ox,oy,l;

	vx = (float)(m - x);
	vy = (float)(n - y);
	ox = (float)x;
	oy = (float)y;
	l = std::sqrt((vx * vx) + (vy * vy));

	if(l > LIGHT)
	  return false;

	vx /= (l * 8);
	vy /= (l * 8);

	for(int i = 0; i < (int)l * 8; i++)
	{
		if(map[(int)ox][(int)oy] == '#')
			return false;

		ox += vx;
		oy += vy;
	}

	return true;
}

void getTip()
{
	attron(COLOR_PAIR(5));
	switch (random(1, 25))
	{
		case 1: addstr("You cannot have more mana than maximum, so use it."); break;
		case 2: addstr("The dungeon is not actually endless. The Dark Lord awaits."); break;
		case 3: addstr("There is a capital boss on every even level."); break;
		case 4: addstr("Killing bosses will grant you various rewards."); break;
		case 5: addstr("Magenta monsters deal double damage."); break;
		case 6: addstr("Cyan bosses have two lives."); break;
		case 7: addstr("Black bosses cannot be harmed by bump attacks."); break;
		case 8: addstr("Yellow bosses swap your health and mana scores. Beware of sudden death!"); break;
		case 9: addstr("White bosses will drain your maximum health!"); break;
		case 10: addstr("Blue bosses can summon monsters."); break;
		case 11: addstr("Red bosses possess the power of teleportation."); break;
		case 12: addstr("Green bosses are antimagic."); break;
		case 13: addstr("Letters with a tittle drain mana."); break;
		case 14: addstr("Vowels can move diagonally."); break;
		case 15: addstr("Xs can ghost through walls."); break;
		case 16: addstr("Level 100 is very special."); break;
		case 17: addstr("Use whirlwind attack to prevent getting swarmed."); break;
		case 18: addstr("Don't hesitate to teleport in a pickle."); break;
		case 19: addstr("Frozen time may help against lines of enemies. But it will run out."); break;
		case 20: addstr("Waiting is not free, unless it is."); break;
		case 21: addstr("Sometimes, you are lucky."); break;
		case 22: addstr("Your whirlwind attack will eventually improve."); break;
		case 23: addstr("Voiceless consonants may blind you."); break;
		case 24: addstr("Rage makes you invincible."); break;
		case 25: addstr("Loosing is fun!"); break;
		default: addstr("You will die."); break;
	}
	attroff(COLOR_PAIR(5));
	return;
}

int main(void)
{
	printf("Descending into the dungeon...\n");
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

	// Initialize player position
	x = random(1, (WIDTH - 1));
	y = random(1, (HEIGHT - 1));

  // Initialize mobs
	for(int i = 0; i < MAX_MOBS; i++)
	{
		mobs[i].mob_x = 0;
		mobs[i].mob_y = 0;
		mobs[i].mob_pic == ' ';
		mobs[i].mob_RGB == 0;
	}

  // Create dungeon
	makeMap();

	/* MAIN LOOP */
	while(gameOn)
	{
		erase();

		if(timeStop <= 0)
			mobMove();
		else
			timeStop -= 1;

		for(int m = 0; m < WIDTH; m++)
			for(int n = 0; n < HEIGHT; n++)
			{
				if(map[m][n] == '.' && (blind > 0 || !checkFov(m, n)))
				  continue;
				// Walls are always shown.

				move(n, m);
				addch(map[m][n]);
			}

		if(blind <= 0)
		{
			for(int i = 0; i < MAX_MOBS; i++)
			{
				if(mobs[i].mob_pic == ' ' || !checkFov(mobs[i].mob_x, mobs[i].mob_y))
					continue;

				move(mobs[i].mob_y, mobs[i].mob_x);
				attron(COLOR_PAIR(mobs[i].mob_RGB));
				addch(mobs[i].mob_pic);
				attroff(COLOR_PAIR(mobs[i].mob_RGB));
			}
		}
		else
		  blind -= 1;

		if(hp > maxhp)
			hp = maxhp;
		if(mp > maxmp)
			mp = maxmp;

    if(invincible > 0)
		  invincible -= 1;

		move(22, 0);
		printw("Health: %i/%i  Mana: %i/%i  Kills: %i/%i  Level: %i  ", hp, maxhp, mp, maxmp, kills, reqKills(), level);

		// TODO: Color Health and Mana?
		move(23, 0);
		printw("   (.)wait   (t)eleport   (w)hirlwind attack   (f)reeze time   (h)eal   (q)uit");

    // Status effects
		if(invincible > 1)
		{
			move(22, WIDTH - 24);
			attron(COLOR_PAIR(2));
			addstr("Rage");
			attroff(COLOR_PAIR(2));
		}
		if(timeStop > 1)
		{
			move(22, WIDTH - 19);
			attron(COLOR_PAIR(7));
			addstr("Time stop");
			attroff(COLOR_PAIR(7));
		}
		if(freeWait)
		{
			move(22, WIDTH - 9);
			attron(COLOR_PAIR(3));
			addstr("Free wait");
			attroff(COLOR_PAIR(3));
		}

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
			if(timeStop < 0) // Negative time stop means we're paralysed.
			{
				timeStop += 1;
				break;
			}

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
					if(mp < 1 && !freeWait) break;

          if(!freeWait)
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

          // Increase reach every 30 floors.
					int reach = 1;
					reach += level / 30;

					for(int m = x - reach; m <= x + reach; m++)
						for(int n = y - reach; n <= y + reach; n++)
						{
							for(int i = 0; i < MAX_MOBS; i++)
							{
								if(mobs[i].mob_x == m && mobs[i].mob_y == n)
								{
										if(isupper(mobs[i].mob_pic))
										{
											if(mobs[i].mob_RGB == 3) // Green is immune to cheap tricks.
											  continue;
											else if(mobs[i].mob_RGB == 8) // Reward for killing black boss - instant level up.
											{
												kills = reqKills();
											}
										}

										kills += 1;
										totalKills += 1;

										// Note that you won't get any mp for whirlwind kills,
										// which is intended.

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
#ifdef DEBUG
				case 'd': // down one level
				{
					kills = reqKills();
					input = true;
					break;
				}
#endif
				case '?': // help screen
				{
					erase();
					move(0, 0);

					attron(COLOR_PAIR(5));
					addstr("Objective:\n");
					attroff(COLOR_PAIR(5));
					addstr("Kill everything, whether it moves or not.\n\n");

					attron(COLOR_PAIR(5));
					addstr("Controls:\n");
					attroff(COLOR_PAIR(5));
					addstr("Arrow keys to move, or (q)uit.\n");
					addstr("Use mana to\n  (.)wait [1 mana]\n  (t)eleport [3 mana]\n");
					addstr("  (w)hirlwind attack [3 mana]\n  (f)reeze time [5 mana]\n  (h)eal [10 mana]\n");
					addstr("Gain mana by killing enemies.\n\n[press any key to continue]");
					getch();

					input = true;
					timeStop = 1; // So that we don't loose a turn by looking at help.
					break;
				}
				default: break;
			}
		}

		if(hp <= 0)
		{
			eraseLastLine();
			move(23, 0);
			attron(COLOR_PAIR(2));
			printw("You die...");
			attroff(COLOR_PAIR(2));
			getch();

			erase();
			move(0, 0);
			printw("You are dead.\n\n");
			printw("Congratulations, you have killed %i minions of the\n", totalKills);
			printw("despicable Alphabetical Overlord and reached level %i!\n\n", level);
			attron(COLOR_PAIR(6));
			printw("Better luck next life.");
			attroff(COLOR_PAIR(6));
			printw("\n\n[press any key to exit]");
			getch();
			gameOn = false;
		}
		else if(kills >= reqKills())
		{
			eraseLastLine();
			move(23, 0);
			attron(COLOR_PAIR(3));
			printw("Level up!");
			attroff(COLOR_PAIR(3));
			getch();

			kills = 0;
			level += 1;
			freeWait = false;

			if(level >= 100) // VICTORY
			{
				erase();
				move(0, 0);
				addstr("Inconceivable!\n\nAstounding!\n\nI believe it not!\n\n");
				printw("You have killed %i alphabetical minions and reached\n", totalKills);
				printw("level %i. The fearsome and fearless Dark Alphabetical\n", level);
				addstr("Overlord ran like a chicken. You thought about chasing\n");
				addstr("after him and finishing him in a satisfactory show of\n");
				addstr("over-the-top violence and splashes or red ink, but\n");
				addstr("then you decided for a fate worse than death: You will\n");
				addstr("draw obscene images on his walls and eat his lunch!\n\n");
				addstr("MWHAHAHAHAHA!!! That'll teach him!\n\n[press any key to continue]");
				getch();

				erase();
				move(0, 0);
				attron(COLOR_PAIR(5));
				addstr("You are victorious!\n\n");
				attroff(COLOR_PAIR(5));
				addstr("[press any key to exit]");
				getch();

				gameOn = false;
			}
			else
			{
				erase();
				move(0, 0);
				addstr("Covered from head to toe in inky gore and absolutely\n");
				addstr("confident in your strength, you descend deeper into\n");
				addstr("the Vault of Writing. You will have your revenge!\n\n");

				addstr("You also learn a very, very useful piece of lore:\n  ");
				getTip();

				// You feelin' lucky, punk?
				if(random(1, 10) == 1)
				{
					attron(COLOR_PAIR(3));
					addstr("\n\nYou feel lucky! ");
					attroff(COLOR_PAIR(3));

					switch (random(1, 3))
					{
						case 1:
						{
							addstr("Waiting will be free next level.");
							freeWait = true;
							break;
						}
						case 2:
						{
							addstr("Your wounds heal a bit.");
							hp += random(1, 2);
							break;
						}
						default:
						{
							addstr("You regain some magical energy.");
							mp += random(1, 3);
							break;
						}
					}
				}
				if(random(1, 200) < level)
				{
					attron(COLOR_PAIR(2));
					addstr("\n\nWAAAAARRRGH! ");
					attroff(COLOR_PAIR(2));

					addstr("You feel invincible!");
					invincible = 10;
				}

        // Whirlwind attack powers up:
				if(level % 30 == 0)
				{
					addstr("\n\nYou find a sword more ancient than you're already\n");
					addstr("carrying just lying on the floor. Your whirlwind\n");
					addstr("attack is now more deadly than ever. ");

					attron(COLOR_PAIR(2));
					addstr("Bloody awesome!");
					attroff(COLOR_PAIR(2));
				}

				addstr("\n\n[press any key to continue]");
				getch();

        // Build next level.
				do
				{
					x = random(1, (WIDTH - 1));
					y = random(1, (HEIGHT - 1));
				} while(!tryMove(x, y, false));

				makeMap();
			}
		}
	}

	endwin();
	printf("Thank you for playing!\n");
	return 0;
}
