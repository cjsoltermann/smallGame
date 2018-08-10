#include <stdlib.h>
#include <time.h>

#include "game.h"

void dogThink(unsigned int ent) {
  srand(clock());
  shiftEnt(ent, rand() % 3 - 1, rand() % 3 - 1);
}

int main() {
  setup();
  createEnt('@', 7, 7, 0);
  unsigned int dog = createEnt('d', 8, 8, 0);
  setEntThink(dog, dogThink);
  unsigned int wolf = createEnt('w', 9, 9, 0);
  setEntThink(wolf, dogThink);
  struct creature *newDog = createCreature('D', 10, 10, 0, "Mr. Dog", 10, 10, 10);
  loadMap("map1.map");
  setStatus("test2");
  setStatus("test3");
  mainLoop();
  endwin();
}

