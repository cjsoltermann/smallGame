#include <ncurses.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#define VERBOSELOG 0
#define SHOWTURN 1

#define MAPWIDTH 20
#define MAPHEIGHT 20
#define MAPAREA MAPWIDTH * MAPHEIGHT
#define MAXENTS 400
#define WALLCHAR '\5'
#define LOGLENGTH 1000
#define MESSAGELENGTH 150
#define NAMELENGTH 50

#define UP {0, -1}
#define DOWN {0, 1}
#define LEFT {-1, 0}
#define RIGHT {1, 0}

#define LENGTH(X) (sizeof X / sizeof X[0])
#define XYTOINDEX(X, Y) ((Y) * MAPWIDTH + (X))
#define INDEXTOX(I) ((I) % MAPWIDTH)
#define INDEXTOY(I) ((I) / MAPWIDTH)
#define BITSET(B, I) ((B >> 7 - I) & 1)
#define SETBIT(B, I) (B |= (1 << 7 - I))
#define ISVALID(X, Y) (X < MAPWIDTH && X >= 0 && Y < MAPHEIGHT && Y >= 0)
#define ISSOLID(X, Y) (tiles[map[XYTOINDEX(X, Y)]].at & SOLID)
#define ENABLESTATE(S) (state |= (S))
#define DISABLESTATE(S) (state |= ~(S))
#define TOGGLESTATE(S) (state ^= (S))
#define STATEENABLED(S) (state & (S))
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
  char sig;
  char name[NAMELENGTH];
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

enum states {
  GAME =    1 << 7,
  CURSOR =  1 << 6,
  EDIT =    1 << 5,
  DEAD =    1 << 0,
  ALL =     255,
};
enum tileAts {
  SOLID =   1 << 7,
  CONNECT = 1 << 6,
};
enum entAts {
  GHOST =  1 << 7,
  HIDDEN = 1 << 6,
  PLAYER = 1,
};

void getMapSurround(int i, unsigned char *surround);
void getEntSurround(int i, unsigned char *surround);
chtype calculateWall(unsigned char *map, int i);
void drawMap(unsigned char* map);
void loadMap(char* file);
unsigned int createEnt(chtype c, int x, int y, uint8_t at);
void deleteEnt(unsigned int i);
void drawEnts();
void moveEnt(unsigned int i, unsigned int x, unsigned int y);
#define shiftEnt(I, X, Y) moveEnt(I, getLoc(I).x + (X), getLoc(I).y + (Y))
unsigned int entAt(unsigned int x, unsigned int y);
int entDead(unsigned int ent);
unsigned int getPlayer();
struct point getLoc(unsigned int ent);
void setThink(unsigned int ent, void (*fn)(unsigned int ent));
unsigned int createCreature(chtype c, int x, int y, uint8_t at, char *name, int health, int attack, int speed);
int isCreature(unsigned int ent);
struct creature *creatureData(unsigned int ent);
void attack(unsigned int from, unsigned int to);
void processKeys(short code);
void drawStatus();
void drawMessages();
void addToLog(char *s, ...);
void setStatus(char *s);
void showMessage(char *s, ...);
void setup();
void mainLoop();
void updateEnt(unsigned int ent);
void updateEnts();
void quit(const union arg *arg);
void shiftPlayer(const union arg *arg);
void shiftCamera(const union arg *arg);
void shiftCursor(const union arg *arg);
void playerAttack(const union arg *arg);
void count(const union arg *arg);
void placeWall(const union arg *arg);
void saveMap(const union arg *arg);
void toggleEdit(const union arg *arg);
void toggleCursor(const union arg *arg);
void showLog(const union arg *arg);
void saveLog(char *file);
void error(const union arg *arg);
void testMessage(const union arg *arg);
void printCreature(const union arg *arg);
inline int wrap(int i, int n);

struct tile tiles[] = {
  {' ', 0 },
  {WALLCHAR, SOLID | CONNECT},
  {'+', CONNECT},
  {'#', SOLID},
};

struct ent *ents[MAXENTS];

uint8_t state = GAME;
struct point camera = {0, 0};
int turn;

struct key keys[] = {
  //key     mode              function          arg              cost
  { 'q',    ALL,              quit,             { 0           }    },
  { 'w',    GAME,             shiftPlayer,      { .p = UP     }, 1 },
  { 's',    GAME,             shiftPlayer,      { .p = DOWN   }, 1 },
  { 'a',    GAME,             shiftPlayer,      { .p = LEFT   }, 1 },
  { 'd',    GAME,             shiftPlayer,      { .p = RIGHT  }, 1 },
  { 'w',    EDIT | CURSOR,    shiftCursor,      { .p = UP     },   },
  { 's',    EDIT | CURSOR,    shiftCursor,      { .p = DOWN   },   },
  { 'a',    EDIT | CURSOR,    shiftCursor,      { .p = LEFT   },   },
  { 'd',    EDIT | CURSOR,    shiftCursor,      { .p = RIGHT  },   },
  { 'i',    GAME,             shiftCamera,      { .p = UP     },   },
  { 'k',    GAME,             shiftCamera,      { .p = DOWN   },   },
  { 'j',    GAME,             shiftCamera,      { .p = LEFT   },   },
  { 'l',    GAME,             shiftCamera,      { .p = RIGHT  },   },
  { 'n',    GAME,             count,            { 0           },   },
  { 'p',    GAME | EDIT,      toggleEdit,       { 0           },   },
  { 'e',    EDIT,             placeWall,        { 0           },   },
  { 'r',    EDIT,             saveMap,          { 0           },   },
  { 'b',    GAME,             showLog,          { 0           },   },
  { 'u',    ALL,              error,            { .s = "Test" }, 1 },
  { 'c',    GAME | CURSOR,    toggleCursor,     { 0           },   },
  { 'x',    CURSOR,           printCreature,    { 0           },   },
  { 'x',    GAME,             testMessage,      { 0           },   },
};

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

char status[MESSAGELENGTH] = "Health: %h";
char message[MESSAGELENGTH];
char *debugLog[LOGLENGTH];
char *gameLog[LOGLENGTH];

int nearAttack(unsigned int ent) {
  int i;
  unsigned char surround[9];
  getEntSurround(XYTOINDEX(ents[ent]->loc.x, ents[ent]->loc.y), surround);
#if VERBOSELOG
  addToLog("Ent #%d generating surround", ent);
  addToLog("%d%d%d", surround[0], surround[1], surround[2]);
  addToLog("%d%d%d", surround[3], surround[4], surround[5]);
  addToLog("%d%d%d", surround[6], surround[7], surround[8]);
#endif
  for(i = 0; i < 9; i++) {
    if(ents[surround[i]]->at & PLAYER) {
      attack(ent, surround[i]);
      return 1;
    }
  }
  return 0;
}

void dogThink(unsigned int ent) {
  int direction;
  if (nearAttack(ent))
    return;
  direction = RANDRANGE(1,5);
  switch (direction) {
    case 1:
      shiftEnt(ent, 0, -1);
      break;
    case 2:
      shiftEnt(ent, 0, 1);
      break;
    case 3:
      shiftEnt(ent, -1, 0);
      break;
    case 4:
      shiftEnt(ent, 1, 0);
      break;
  }
}

int main() {
  setup();
  createCreature('@', 7, 7, PLAYER, "Christian", 50, 10, 10);
  unsigned int newDog = createCreature('D', 10, 10, 0, "Mr. Dog", 10, 10, 10);
  setThink(newDog, dogThink);
  unsigned int anotherDog = createCreature('d', 11, 11, 0, "Mrs. Dog", 10, 10, 10);
  setThink(anotherDog, dogThink);
  loadMap("map1.map");
  srand(time(NULL));
  mainLoop();
  endwin();
}

void setup() {
  addToLog("Beginning setup...");
  setlocale(LC_ALL, "en_US.UTF-8");
  if(!initscr()) exit(1);
  cbreak();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  addToLog("Setup successful!");
  createEnt('@', 0, 0, HIDDEN | GHOST);
}

void quit(const union arg *arg) {
  int i;
  addToLog("Ending window...");
  endwin();
  addToLog("Freeing memory...");
  for(i = 0; i < MAXENTS; i++) {
    if(ents[i] == NULL) continue;
    if(ents[i]->data) free(ents[i]->data);
    free(ents[i]);
  }
  addToLog("Saving log...");
  saveLog("log");
  for(i = 0; i < LOGLENGTH; i++)
    free(debugLog[i]);
  exit(0);
}

void shiftPlayer(const union arg *arg) {
  playerAttack(arg);
  struct point p = arg->p;
  unsigned int player = getPlayer();
  shiftEnt(player, p.x, p.y);
}

void shiftCursor(const union arg *arg) {
  struct point p = arg->p;
  shiftEnt(0, p.x, p.y);
}

void shiftCamera(const union arg *arg) {
  struct point p = arg->p;
  camera.x += p.x;
  camera.y += p.y;
}

void playerAttack(const union arg *arg) {
  struct point p, loc;
  unsigned int player, ent;
  int x, y;
  p = arg->p;
  player = getPlayer();
  loc = getLoc(player);
  x = loc.x + p.x;
  y = loc.y + p.y;
  if ((ent = entAt(x,y)))
    attack(player,ent);
}

void count(const union arg *arg) {
  static int counter;
  showMessage("You've pressed that button %d times!", counter++);
}

void placeWall(const union arg *arg) {
  map[XYTOINDEX(ents[0]->loc.x, ents[0]->loc.y)] = WALL;
}

void toggleEdit(const union arg *arg) {
  toggleCursor(arg);
  TOGGLESTATE(EDIT);
}

void toggleCursor(const union arg *arg) {
  TOGGLESTATE(CURSOR);
  TOGGLESTATE(GAME);
  if(STATEENABLED(CURSOR))
    ents[0]->at &= ~HIDDEN;
  else
    ents[0]->at |= HIDDEN;
}

void updateEnt(unsigned int ent) {
  if(ents[ent] && ents[ent]->think) ents[ent]->think(ent);
}

void updateEnts() {
  int i;
  for(i = 0; i < MAXENTS; i++) {
    updateEnt(i);
  }
}

void mainLoop() {
  addToLog("Beginning main loop.");
  int lastTurn = 0;
  while(1) {
    for(;lastTurn < turn; lastTurn++)
      updateEnts();
    while(turn == lastTurn) {
      drawMap(map);
      drawEnts();
#if SHOWTURN
      showMessage("Turn: %d", turn);
#endif
      drawStatus();
      drawMessages();
      free(*gameLog);
      *gameLog = NULL;
      processKeys(getch());
    }
  }
}

void drawMap(unsigned char* map) {
  int i;
  chtype c;
  for(i = 0; i < MAPAREA; i++) {
    c = tiles[map[i]].c;
    if(c == WALLCHAR)
      c = calculateWall(map, i);
    mvaddch(INDEXTOY(i) - camera.y, INDEXTOX(i) - camera.x, c);
  }
}

void saveMap(const union arg *arg) {
  int i;
  FILE *f = fopen("custom.map", "w");
  for(i = 0; i < MAPAREA; i++)
    fputc(map[i], f);
}

void loadMap(char *file) {
  addToLog("Loading map %s", file);
  int i, c;
  FILE *f = fopen(file, "r");
  if(!f) {
    addToLog("Map does not exist, creating...");
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
    else {
      addToLog("Map file ended early!");
      break;
    }
  }
}

chtype calculateWall(unsigned char *map, int i) {
  unsigned char surround[9];
  getMapSurround(i, surround);
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

void getMapSurround(int i, unsigned char *surround) {
  int n, j;
  for(n = 0, j = -MAPWIDTH - 1; n < 9; n++, j = ((n / 3 - 1) * MAPWIDTH) + (n % 3 - 1)) {
    surround[n] = map[i + j];
  }
}

void getEntSurround(int i, unsigned char *surround) {
  int n, j;
  unsigned int ent;
  for(n = 0, j = -MAPWIDTH - 1; n < 9; n++, j = ((n / 3 - 1) * MAPWIDTH) + (n % 3 - 1)) {
    ent = entAt(INDEXTOX(i + j), INDEXTOY(i + j));
    surround[n] = ent;
  }
}

void drawEnts() {
  int i;
  for(i = 0; i < MAXENTS; i++)
    if(ents[i] && !(ents[i]->at & HIDDEN)) mvaddch(ents[i]->loc.y - camera.y, ents[i]->loc.x - camera.x, ents[i]->c);
}

unsigned int createEnt(chtype c, int x, int y, uint8_t at) {
  struct ent *ent = malloc(sizeof(struct ent));
  ent->c = c;
  ent->loc.x = x;
  ent->loc.y = y;
  ent->think = NULL;
  ent->at = at;
  ent->data = NULL;
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
  struct creature *data;
  if ((data = creatureData(i)))
    free(data);
  free(ents[i]);
  ents[i] = NULL;
}

void moveEnt(unsigned int i, unsigned int x, unsigned int y) {
  if((ISVALID(x, y) && !entAt(x, y) && (!ISSOLID(x, y))) || ents[i]->at & GHOST) {
    ents[i]->loc.x = x;
    ents[i]->loc.y = y;
  }
}

unsigned int entAt(unsigned int x, unsigned int y) {
  int i;
  for(i = 1; i < MAXENTS; i++) {
    if(ents[i] && ents[i]->loc.x == x && ents[i]->loc.y == y) return i;
  }
  return 0;
}

int entDead(unsigned int ent) {
  struct creature *data;
  if (ents[ent]) {
    if ((data = creatureData(ent)) && !(data->health > 0))
      return 2;
    return 0;
  }
  return 1;
}

unsigned int getPlayer() {
  int i;
  for(i = 0; i < MAXENTS; i++) {
    if(ents[i] && ents[i]->at & PLAYER) return i;
  }
  return 0;
}

struct point getLoc(unsigned int ent) {
  return ents[ent]->loc;
}

void setThink(unsigned int ent, void (*fn)(unsigned int ent)) {
  if(ents[ent]) ents[ent]->think = fn;
}

unsigned int createCreature(chtype c, int x, int y, uint8_t at, char *name, int health, int attack, int speed) {
  int i;
  unsigned int e = createEnt(c, x, y, at);
  struct creature *cr = malloc(sizeof(struct creature));
  for(i = 0; i < NAMELENGTH && name[i] != '\0'; cr->name[i] = name[i], i++);
  cr->sig = 'c';
  cr->health = health;
  cr->attack = attack;
  cr->speed = speed;
  cr->ent = ents[e];
  ents[e]->data = cr;
  return e;
}

int isCreature(unsigned int ent) {
  if(ents[ent] && ents[ent]->data) {
    if(*(char *)ents[ent]->data == 'c') return 1;
  }
  return 0;
}
struct creature *creatureData(unsigned int ent) {
  if(ents[ent] && ents[ent]->data) {
    return (struct creature *)ents[ent]->data;
  }
  return NULL;
}

void attack(unsigned int from, unsigned int to) {
  int attack;
  if (!isCreature(from) || !isCreature(to)) return;
  struct creature *fromd = creatureData(from);
  struct creature *tod = creatureData(to);
  attack = RANDRANGE(0, fromd->attack);
  tod->health -= attack;
  addToLog("Attack: %s(%d) -> %s(%d): %d damage", fromd->name, from, tod->name, to, attack);
  showMessage("%s attacked %s dealing %d points of damage", fromd->name, tod->name, attack); 
  if(entDead(to))
    deleteEnt(to);
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

void parseStatus(char *dest, const char *src) {
  unsigned int i, j, player;
  struct creature *data;

  player = getPlayer();
  data = creatureData(player);

  for(i = 0, j = 0; i < MESSAGELENGTH && j < MESSAGELENGTH && src[i] != '\0'; i++, j++) {
    if(src[i] == '%') {
      if(src[++i] == '\0') return;
      if(src[i] == 's') {
        snprintf(dest + j, MESSAGELENGTH - j, "%d", turn);
        break;
      }
      if(src[i] == 'h') {
        snprintf(dest + j, MESSAGELENGTH - j, "%d", data->health);
        break;
      }
      dest[j] = '%';
      break;
    }
    dest[j] = src[i];
  }
}

void drawStatus() {
  int i, maxy;
  char buf[MESSAGELENGTH];

  parseStatus(buf, status);

  maxy = getmaxy(stdscr);

  for(i = 0; i < MESSAGELENGTH && buf[i] != '\0'; i++)
    mvaddch(maxy - 1, i, buf[i]);

  for(; i < MESSAGELENGTH; i++)
    mvaddch(maxy - 1, i, ' ');
}

void drawMessages() {
  int i, j, y, maxy, maxx;

  getmaxyx(stdscr, maxy, maxx);

  for(j = 0; j < LOGLENGTH && gameLog[j + 1]; j++);

  for(y = maxy - 2; y > maxy - 6 && gameLog[wrap(j, LOGLENGTH)]; y--, j--) {
    for(i = 0; i < maxx; i++)
      mvaddch(y, i, ' ');
    mvaddstr(y, 0, gameLog[wrap(j, LOGLENGTH)]);
  }
  for(; y > maxy - 6; y--) {
    for(i = 0; i < MESSAGELENGTH; i++)
      mvaddch(y, i, ' ');
  }
}

void addToLog(char *s, ...) {
  int i;
  for(i = 0; i < LOGLENGTH && debugLog[i] != NULL; i++);
  free(debugLog[(i + 1) % LOGLENGTH]);
  debugLog[(i + 1) % LOGLENGTH] = NULL;
  debugLog[i] = malloc(MESSAGELENGTH);

  va_list args;
  va_start(args, s);
  vsnprintf(debugLog[i], MESSAGELENGTH, s, args);
  va_end(args);
}

void setStatus(char *s) {
  strncpy(status, s, MESSAGELENGTH);
}

void showMessage(char *s, ...) {
  int i;
  for(i = 0; i < LOGLENGTH && gameLog[i] != NULL; i++);
  free(gameLog[(i + 1) % LOGLENGTH]);
  gameLog[(i + 1) % LOGLENGTH] = NULL;
  gameLog[i] = malloc(MESSAGELENGTH);

  va_list args;
  va_start(args, s);
  vsnprintf(gameLog[i], MESSAGELENGTH, s, args);
  va_end(args);
}

void showLog(const union arg *arg) {
  int i, j;
  int y = getmaxy(stdscr) - 1;
  erase();

  for(i = 0; i < LOGLENGTH && debugLog[i + 1] != NULL; i++);
  for(j = y; j > 0 && debugLog[wrap(i, LOGLENGTH)] != NULL; i--, j--) {
    mvaddstr(j, 0, debugLog[wrap(i, LOGLENGTH)]);
  }

  getch();
  erase();
}

void saveLog(char *file) {
  int i;
  FILE *f = fopen(file, "w");
  for(i = 0; i < LOGLENGTH; i++) {
    if(debugLog[i]) {
      fputs(debugLog[i], f);
      fputc('\n', f);
    }
  }
}

void error(const union arg *arg) {
  addToLog("Error: %s on turn %d", arg->s, turn);
}

void testMessage(const union arg *arg) {
  showMessage("poopy butt");
}

void printCreature(const union arg *arg) {
  unsigned int ent;
  struct creature *creature;
  if ((ent = entAt(ents[0]->loc.x, ents[0]->loc.y)) && (creature = creatureData(ent)))
    showMessage("%s has %d health", creature->name, creature->health);
}

int wrap(int i, int n) {
  return (i % n + n) % n;
}
