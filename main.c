#include <ncurses.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

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
  void *v;
  struct ent e;
  struct tile t;
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
#define shiftEnt(I, X, Y) moveEnt(I, ents[I]->loc.x + (X), ents[I]->loc.y + (Y))
unsigned int entAt(unsigned int x, unsigned int y);
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

struct tile tiles[] = {
  {' ', 0 },
  {WALLCHAR, SOLID | CONNECT},
  {'+', CONNECT},
  {'#', SOLID},
};

struct ent *ents[MAXENTS];

struct key keys[] = {
  //key        mode        function        arg        cost
  { 'q',       GAME,         quit,        { 0 }          },
  { 'w',       GAME,      shiftPlayer, { .p = {0,-1} }, 1},
  { 's',       GAME,      shiftPlayer, { .p = {0, 1} }, 1},
  { 'a',       GAME,      shiftPlayer, { .p = {-1,0} }, 1},
  { 'd',       GAME,      shiftPlayer, { .p = {1, 0} }, 1},
  { 'i',       GAME,      shiftCamera, { .p = {0, 1} },  },
  { 'k',       GAME,      shiftCamera, { .p = {0,-1} },  },
  { 'j',       GAME,      shiftCamera, { .p = {1, 0} },  },
  { 'l',       GAME,      shiftCamera, { .p = {-1,0} },  },
  { 'n',       GAME,         count,       { 0 },         },
  { 'p',       GAME,       toggleEdit,    { 0 },         },
  { 'e',       EDIT,       placeWall,     { 0 },         },
  { 'r',       EDIT,        saveMap,      { 0 },         },
  { 'r',       GAME,        showLog,      { 0 },         },
};

uint8_t state = GAME;
struct point camera = {0, 0};
int turn;

unsigned char map[MAPAREA] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
  0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
  0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
  0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
  0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
  0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
  0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
  0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
  0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
  0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
  0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
  0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
  0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
  0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
  0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
  0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
  0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
  0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
char status[50] = "test";
char statusLog[1000];

void setup() {
  setlocale(LC_ALL, "en_US.UTF-8");
  if(!initscr()) exit(1);
  cbreak();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
}

void quit(const union arg *arg) {
  endwin();
  exit(0);
}

void shiftPlayer(const union arg *arg) {
  struct point p = arg->p;
  shiftEnt(0, p.x, p.y);
}

void shiftCamera(const union arg *arg) {
  struct point p = arg->p;
  camera.x += p.x;
  camera.y += p.y;
}

void count(const union arg *arg) {
  static int counter;
  setStatus("You've pressed that button %d times!", counter++);
}

void placeWall(const union arg *arg) {
  map[XYTOINDEX(ents[0]->loc.x, ents[0]->loc.y)] = WALL;
}

void toggleEdit(const union arg *arg) {
  TOGGLESTATE(EDIT);
  if(STATEENABLED(EDIT)) {
    loadMap("custom.map");
    ents[0]->at |= GHOST;
  }
  else {
    loadMap("map1.map");
    ents[0]->at &= ~GHOST;
  }
}

void dogThink(unsigned int ent) {
  srand(clock());
  shiftEnt(ent, rand() % 3 - 1, rand() % 3 - 1);
}

void updateEnts() {
  int i;
  for(i = 0; i < MAXENTS; i++) {
    if(ents[i] && ents[i]->think) ents[i]->think(i);
  }
}

void mainLoop() {
  int lastTurn = 0;
  while(1) {
    for(;lastTurn < turn; lastTurn++)
      updateEnts();
    while(turn == lastTurn) {
      drawMap(map);
      drawEnts();
      drawStatus();
      processKeys(getch());
    }
  }
}

int main() {
  setup();
  createEnt('@', 7, 7, 0);
  unsigned int dog = createEnt('d', 8, 8, 0);
  ents[dog]->think = dogThink;
  unsigned int wolf = createEnt('w', 9, 9, 0);
  ents[wolf]->think = dogThink;
  struct creature *newDog = createCreature('D', 10, 10, 0, "Mr. Dog", 10, 10, 10);
  loadMap("map1.map");
  setStatus("test2");
  setStatus("test3");
  mainLoop();
  endwin();
}

void drawMap(unsigned char* map) {
  int i;
  chtype c;
  for(i = 0; i < MAPAREA; i++) {
    c = tiles[map[i]].c;
    if(c == WALLCHAR)
      c = calculateWall(map, i);
    mvaddch(INDEXTOY(i) + camera.y, INDEXTOX(i) + camera.x, c);
  }
}

void saveMap(const union arg *arg) {
  int i;
  FILE *f = fopen("custom.map", "w");
  for(i = 0; i < MAPAREA; i++)
    fputc(map[i], f);
}

void loadMap(char *file) {
  int i, c;
  FILE *f = fopen(file, "r");
  if(!f) {
    setStatus("Map %s does not exist, creating...", file);
    f = fopen(file, "w+");
    for(i = 0; i < MAPAREA; i++)
      fputc(map[i], f);
    rewind(f);
  }
  for(i = 0; i < MAPAREA; i++) {
    c = fgetc(f);
    if(c != EOF) {
      map[i] = (unsigned char)c;
    }
    else break;
  }
}

chtype calculateWall(unsigned char *map, int i) {
  unsigned char surround[9];
  getSurround(i, map, surround);
  int t = tiles[surround[1]].at & CONNECT;
  int l = tiles[surround[3]].at & CONNECT;
  int r = tiles[surround[5]].at & CONNECT;
  int b = tiles[surround[7]].at & CONNECT;

  if(t) {
    if(b) {
      if(l) {
        if(r)
          return ACS_PLUS;
        return ACS_RTEE;
      }
      if(r)
        return ACS_LTEE;
      return ACS_VLINE;
    }
    if(l) {
      if(r)
        return ACS_BTEE;
      return ACS_LRCORNER;
    }
    if(r)
      return ACS_LLCORNER;
    return ACS_VLINE;
  }
  if(b) {
    if(l) {
      if(r)
        return ACS_TTEE;
      return ACS_URCORNER;
    }
    if(r)
      return ACS_ULCORNER;
    return ACS_VLINE;
  }
  if(l)
    return ACS_HLINE;
  if(r)
    return ACS_HLINE;

  return ACS_PLUS;
}

void getSurround(int i, unsigned char *map, unsigned char *surround) {
  int n, j;
  for(n = 0, j = -MAPWIDTH - 1; n < 9; n++, j = ((n / 3 - 1) * MAPWIDTH) + (n % 3 - 1)) {
    surround[n] = map[i + j];
  }
}

void drawEnts() {
  int i;
  for(i = 0; i < MAXENTS; i++)
    if(ents[i]) mvaddch(ents[i]->loc.y + camera.y, ents[i]->loc.x + camera.x, ents[i]->c);
}

unsigned int createEnt(chtype c, int x, int y, uint8_t at) {
  struct ent *ent = malloc(sizeof(struct ent));
  ent->c = c;
  ent->loc.x = x;
  ent->loc.y = y;
  ent->think = NULL;
  ent->at = at;
  int i;
  for(i = 0; i < MAXENTS; i++) {
    if(!ents[i]) {
      ents[i] = ent;
      break;
    }
  }
  return i;
}

void deleteEnt(unsigned int i) {
  free(ents[i]);
}

void moveEnt(unsigned int i, unsigned int x, unsigned int y) {
  if(ISVALID(x, y) && entAt(x, y) == -1 && (!ISSOLID(x, y) || ents[i]->at & GHOST)) {
    ents[i]->loc.x = x;
    ents[i]->loc.y = y;
  }
}

unsigned int entAt(unsigned int x, unsigned int y) {
  int i;
  for(i = 0; i < MAXENTS; i++) {
    if(ents[i] && ents[i]->loc.x == x && ents[i]->loc.y == y) return i;
  }
  return -1;
}

struct creature *createCreature(chtype c, int x, int y, uint8_t at, char *name, int health, int attack, int speed) {
  int i;
  unsigned int e = createEnt(c, x, y, at);
  struct creature *cr = malloc(sizeof(struct creature));
  for(i = 0; i < 50 && name[i] != '\0'; cr->name[i] = name[i], i++);
  cr->health = health;
  cr->attack = attack;
  cr->speed = speed;
  cr->ent = ents[e];
  ents[e]->data = cr;
  return cr;
}

void processKeys(short code){
  int i;
  for(i = 0; i < LENGTH(keys); i++) {
    if(keys[i].code == code && STATEENABLED(keys[i].mask)) {
      keys[i].func(&keys[i].arg);
      turn += keys[i].cost;
      return;
    }
  }
}

void drawStatus() {
  int i;
  for(i = 0; i < 50; i++) {
    if(status[i] != '\0')
      mvaddch(getmaxy(stdscr) - 1, i, status[i]);
    else return;
  }
  //mvaddstr(getmaxy(stdscr) - 1, 0, status);
}

void clearStatus() {
  int i;
  for(i = 0; i < 50; i++) {
    status[i] = ' ';
  }
}

void addToLog(char *s, ...) {
  int i;
  for(i = 0; i < 1000 && statusLog[i] != '\0';i++);
  statusLog[++i] = '\n';
  va_list args;
  va_start(args, s);
  
  vsnprintf(statusLog + i, 50, s, args);
}

void setStatus(char *s, ...) {
  addToLog(status);
  clearStatus();
  va_list args;
  va_start(args, s);

  vsnprintf(status, 50, s, args);

  va_end(args);
}

void showLog(const union arg *arg) {
  int i;
  erase();
  move(0,0);
  for(i = 0; i < 1000; i++) {
    if(statusLog[i] == '\0') break;
    addch(statusLog[i]);
  }
  getch();
  erase();
}
