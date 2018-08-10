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
  { 'u',       GAME,         error,    { .s = "Test" }, },
};
