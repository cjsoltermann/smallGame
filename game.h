#ifndef GAME_H
#define GAME_H

#include <ncurses.h>

#define MAPWIDTH 20
#define MAPHEIGHT 20
#define MAPAREA MAPWIDTH * MAPHEIGHT
#define MAXENTS 400
#define WALLCHAR '\5'

#define LENGTH(X) (sizeof X / sizeof X[0])
#define XYTOINDEX(X, Y) ((Y) * MAPWIDTH + (X))
#define INDEXTOX(I) (I % MAPWIDTH)
#define INDEXTOY(I) (I / MAPWIDTH)
#define BITSET(B, I) ((B >> 7 - I) & 1)
#define SETBIT(B, I) (B |= (1 << 7 - I))
#define ISVALID(X, Y) (X < MAPWIDTH && X >= 0 && Y < MAPHEIGHT && Y >= 0)
#define ISSOLID(X, Y) (tiles[map[XYTOINDEX(X, Y)]].at & SOLID)
#define ENABLESTATE(S) state |= (S)
#define DISABLESTATE(S) state |= ~(S)
#define TOGGLESTATE(S) state ^= (S)
#define STATEENABLED(S) state & (S)
#define RANDRANGE(MIN, MAX) (rand() % (MAX) - (MIN)) + (MIN)

struct point {
  int x;
  int y;
};

struct tile {
  chtype c;
  uint8_t at;
};

struct ent {
  chtype c;
  struct point loc;
  void (*think)(unsigned int ent);
  uint8_t at;
  void *data;
};

struct creature {
  char name[30];
  int health;
  int attack;
  int speed;
  struct ent *ent;
};

union arg {
  int i;
  char *s;
  void *v;
  struct point p;
};

struct key {
  short code;
  uint8_t mask;
  void (*func)(const union arg*);
  const union arg arg;
  int cost;
};

enum tiles { FLOOR, WALL, DOOR, FOUNTAIN };
enum directions { UP, DOWN, LEFT, RIGHT };
enum states {
  GAME =    1 << 7,
  CURSOR =  1 << 6,
  EDIT =    1 << 5,
};
enum tileAts {
  SOLID =   1 << 7,
  CONNECT = 1 << 6,
};
enum entAts {
  GHOST = 1 << 7,
};

void getSurround(int i, unsigned char *map, unsigned char *surround);
chtype calculateWall(unsigned char *map, int i);
void drawMap(unsigned char* map);
void loadMap(char* file);
unsigned int createEnt(chtype c, int x, int y, uint8_t at);
void deleteEnt(unsigned int i);
void drawEnts();
void moveEnt(unsigned int i, unsigned int x, unsigned int y);
#define shiftEnt(I, X, Y) moveEnt(I, getLoc(I).x + (X), getLoc(I).y + (Y))
unsigned int entAt(unsigned int x, unsigned int y);
struct point getLoc(unsigned int ent);
void setEntThink(unsigned int ent, void (*fn)(unsigned int ent));
struct creature *createCreature(chtype c, int x, int y, uint8_t at, char *name, int health, int attack, int speed);
void processKeys(short code);
void drawStatus();
void clearStatus();
void addToLog(char *s, ...);
void setStatus(char *s, ...);
void setup();
void mainLoop();
void quit(const union arg *arg);
void shiftPlayer(const union arg *arg);
void shiftCamera(const union arg *arg);
void count(const union arg *arg);
void placeWall(const union arg *arg);
void saveMap(const union arg *arg);
void toggleEdit(const union arg *arg);
void showLog(const union arg *arg);
void error(const union arg *arg);

#endif
