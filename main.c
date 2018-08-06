#include <ncurses.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#define LENGTH(X) (sizeof X / sizeof X[0])
#define XYTOINDEX(X, Y) ((Y) * 20 + (X))
#define INDEXTOX(I) (I % 20)
#define INDEXTOY(I) (I / 20)
#define BITSET(B, I) ((B >> 7 - I) & 1)
#define SETBIT(B, I) (B |= (1 << 7 - I))

struct tile {
  chtype c;
  uint8_t at;
};

struct ent {
  chtype c;
  int x;
  int y;
  uint8_t at;
};

union arg {
  int i;
  void *v;
};

struct key {
  short code;
  uint8_t mask;
  void (*func)(const union arg*);
  const union arg arg;
};

enum tiles { FLOOR, WALL };
enum states {
  GAME = 1 << 7,
};

void getSurround(int i, char *map, char *surround);
chtype calculateWall(char *map, int i);
void drawMap(char* map);
unsigned int createEnt(chtype c, int x, int y, uint8_t at);
void deleteEnt(unsigned int i);
void drawEnts();
void processKeys(short code);
void setup();
int main();
void quit(const union arg *arg);

struct tile tiles[] = {
  {' ', 0 },
  {'#', 0 },
};

struct ent *ents[400];

struct key keys[] = {
  { 'q', GAME, quit, { 0 } },
};

uint8_t state = GAME;

char map[400] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,1,1,1,0,0,0,1,0,0,0,0,0,0,0,
  0,0,0,0,0,0,1,1,1,0,0,1,1,0,0,0,0,0,0,0,
  0,0,0,0,0,0,1,1,1,0,0,0,1,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, };

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

int main() {
  setup();
  unsigned int player = createEnt('d', 7, 7, 0);
  while(1) {
    drawMap(map);
    drawEnts();
    processKeys(getch());
  }
  endwin();
}

void drawMap(char* map) {
  int i;
  chtype c;
  for(i = 0; i < 400; i++) {
    c = tiles[map[i]].c;
    if(c == '#')
      c = calculateWall(map, i);
    mvaddch(INDEXTOY(i), INDEXTOX(i), c);
  }
}

chtype calculateWall(char *map, int i) {
  char surround[9];
  getSurround(i, map, surround);
  int t = tiles[surround[1]].c == '#';
  int l = tiles[surround[3]].c == '#';
  int r = tiles[surround[5]].c == '#';
  int b = tiles[surround[7]].c == '#';
/*
  if(t && b && l && r) return ACS_PLUS;
  if(t && r && b) return ACS_LTEE;
  if(t && l && b) return ACS_RTEE;
  if(t && l && r) return ACS_BTEE;
  if(b && l && r) return ACS_TTEE;
  if(t && r) return ACS_LLCORNER;
  if(t && l) return ACS_LRCORNER;
  if(b && r) return ACS_ULCORNER;
  if(b && l) return ACS_URCORNER;
  if(l) return ACS_HLINE;
  if(r) return ACS_HLINE;
  if(t) return ACS_VLINE;
  if(b) return ACS_VLINE;
*/
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

  return '1';
}

void getSurround(int i, char *map, char *surround) {
  int n, j;
  for(n = 0, j = -21; n < 9; n++, j = ((n / 3 - 1) * 20) + (n % 3 - 1)) {
    surround[n] = map[i + j];
  }
}

void drawEnts() {
  int i;
  for(i = 0; i < 400; i++)
    if(ents[i]) mvaddch(ents[i]->y, ents[i]->x, ents[i]->c);
}

unsigned int createEnt(chtype c, int x, int y, uint8_t at) {
  struct ent *ent = malloc(sizeof(struct ent));
  ent->c = c;
  ent->x = x;
  ent->y = y;
  ent->at = at;
  int i;
  for(i = 0; i < 400; i++)
    ents[i] = !ents[i] ? ent : NULL;
  return i;
}

void deleteEnt(unsigned int i) {
  free(ents[i]);
}

void processKeys(short code){
  int i;
  for(i = 0; i < LENGTH(keys); i++) {
    if(keys[i].code == code) keys[i].func(&keys[i].arg);
  }
}
