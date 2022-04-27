#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <locale.h>

#define MS_ROWS 10
#define BOMB_COUNT 10
#define MS_COLS 10

typedef struct {
  bool isBomb;
  bool isFlagged;
  bool isRevealed;
  WINDOW *window;
} Field;

int rowCount, colCount;
int rowMiddle, colMiddle;
int x = 0, y = 0;
bool isGameLost = false;
WINDOW *gameLostWindow;


Field game[MS_ROWS][MS_COLS];

const int X_UP = 0;
const int X_DOWN = 1;
const int Y_UP = 2;
const int Y_DOWN = 3;

Field* getField() { return &game[x][y]; }

int fieldValue() {
  int value = 0;
  for (int i = -1; i < 2; i++) {
    for (int j = -1; j < 2; j++) {
      if (i == 0 && j == 0) continue;
      int dX = x + i;
      int dY = y + j;
      if (dX > 0 && dY > 0 && dX < MS_ROWS && dY < MS_COLS && game[dX][dY].isBomb)
        value++;
    }
  }

  return value;
}

void printFieldAt(int x, int y) {
  Field *field = &game[x][y];
  if (field->isRevealed) {
    if (field->isBomb) {
      mvwprintw(field->window, 1, 1, "🚩");
    } else {
      mvwprintw(field->window, 1, 2, "%d", fieldValue());
    }
  } else if (field->isFlagged) {
      mvwprintw(field->window, 1, 2, "⚑");
  }

  wrefresh(field->window);
}

void printField() {
  printFieldAt(x, y);
}

void highlight();

void refreshAll() {
  for (int x = 0; x < MS_ROWS; x++) {
    for (int y = 0; y < MS_ROWS; y++) {
      box(game[x][y].window, 0, 0);
      printFieldAt(x, y);
    }
  }
  highlight();
}

void highlight() {
  WINDOW *win = getField()->window;
  wattron(win, A_BOLD | A_BLINK);
  box(win, 1, 1);
  wattroff(win, A_BOLD | A_BLINK);
  printField();
}

void moveFocus(const int direction) {
  box(getField()->window, 0, 0);
  printField();

  if (direction == X_DOWN && x < MS_ROWS - 1) {
    x++;
  } else if (direction == X_UP && x > 0) {
    x--;
  } else if (direction == Y_UP && y < MS_COLS - 1) {
    y++;
  } else if (direction == Y_DOWN && y > 0) {
    y--;
  }

  highlight();
}

void placeBombs() {
  srand(time(NULL));

  for (int i = 0; i < BOMB_COUNT; ) {
    int x = rand() % MS_ROWS + 1;
    int y = rand() % MS_COLS + 1;
    Field *field = &game[x][y];
    if (!field->isBomb) {
      field->isBomb = true;
      i++;
    }
  }
}

void resetBoard() {
  isGameLost = false;
  for (int i = 0; i < MS_ROWS; i++) {
    for (int j = 0; j < MS_COLS; j++) {
      Field *field = &game[i][j];
      field->isBomb = false;
      field->isRevealed = false;
      field->isFlagged = false;
      wclear(field->window);
      box(field->window, 0, 0);
      wrefresh(field->window);
    }
  }
  placeBombs();
  highlight();
}

void createBoard() {
  int rowOffset = (rowCount - MS_ROWS * 3) / 2;
  int colOffset = (colCount - MS_COLS * 5) / 2;

  for (int i = 0; i < MS_ROWS; i++) {
    for (int j = 0; j < MS_COLS; j++) {
      Field field;
      field.isBomb = false;
      field.isFlagged = false;
      field.isRevealed = false;
      field.window = newwin(3, 5, rowOffset + i * 3, colOffset + j * 5);
      game[i][j] = field;

      box(field.window, 0, 0);
      wrefresh(field.window);
    }
  }

  placeBombs();
}

void handleLoss() {
  gameLostWindow = newwin(0, 0, 0, 0);
  const char* text = "You lost. Hit Delete to retry or Enter to close this window.";
  mvwprintw(gameLostWindow, rowCount / 2, (colCount - strlen(text)) / 2, "%s", text);
  wrefresh(gameLostWindow);
}

void handleReveal() {
  if (isGameLost)
    return;

  Field *field = getField();
  if (field->isRevealed)
    return;

  field->isRevealed = true;

  if (field->isBomb) {
    isGameLost = true;
    handleLoss();
  }
  
  printField();
}

void handleFlag() {
  if (isGameLost)
    return;

  Field *field = getField();
  if (field->isRevealed)
    return;

  field->isFlagged = !field->isFlagged;
  printField();
}


void cleanup() {
  for (int i = 0; i < MS_ROWS; i++) {
    for (int j = 0; j < MS_COLS; j++) {
      delwin(game[i][j].window);
    }
  }
  if (gameLostWindow)
    delwin(gameLostWindow);
  endwin();
  exit(0);
}

#ifdef DEBUG
#include <execinfo.h>
void segfaultHandler(int signal) {
  void *stack[10];

  size_t size = backtrace(stack, 10);

  fprintf(stderr, "Error: signal %d:\n", signal);
  backtrace_symbols_fd(stack, size, STDERR_FILENO);
  exit(1);
}
#endif

int main() {
  // Catch le funny signal
  signal(SIGINT, cleanup);

#ifdef DEBUG
  signal(SIGSEGV, segfaultHandler);
#endif

  // Necessary to render emojis
  setlocale(LC_ALL, "");

  initscr();
  getmaxyx(stdscr, rowCount, colCount);
  rowMiddle = rowCount / 2;
  colMiddle = colCount / 2;

  // Bye cursor
  curs_set(0);

  noecho();
  cbreak();

  const char *messages[] = {
    "Welcome to Minesweeper",
    "",
    "You can move the cursor with arrow keys or WASD",
    "R/LeftClick to reveal",
    "F/RightClick to flag",
    "Delete to restart",
    "",
    "Hit Enter to start!"
  };

  for (int i = 0, offset = -3; i < 8; i++, offset++) {
    const char* message = messages[i];
    mvprintw(rowCount / 2 + offset, (colCount - strlen(message)) / 2, "%s",
             message);
    refresh();
  }
  while (getch() != '\n') {};

  erase();
  refresh();

  createBoard();
  highlight();

  // Make keypad and Function keys work
  keypad(stdscr, true);
  mousemask(ALL_MOUSE_EVENTS, NULL);
  // Make mouse not take 2 years
  mouseinterval(0);

  while (true) {
    MEVENT mouse;
    int c = getch();

    if (gameLostWindow) {
      if (c == '\n') {
        werase(gameLostWindow);
        wrefresh(gameLostWindow);
        delwin(gameLostWindow);
        gameLostWindow = NULL;
        refresh();
        refreshAll();
      } else if (c == KEY_BACKSPACE) {
        werase(gameLostWindow);
        wrefresh(gameLostWindow);
        delwin(gameLostWindow);
        refresh();
        gameLostWindow = NULL;
        resetBoard();
      }
      continue;
    }

    switch (c) {
    case KEY_UP:
    case 'w':
      moveFocus(X_UP);
      break;
    case KEY_DOWN:
    case 's':
      moveFocus(X_DOWN);
      break;
    case KEY_RIGHT:
    case 'd':
      moveFocus(Y_UP);
      break;
    case KEY_LEFT:
    case 'a':
      moveFocus(Y_DOWN);
      break;
    case 'f':
      handleFlag();
      break;
    case 'r':
      handleReveal();
      break;
    case KEY_BACKSPACE:
      resetBoard();
      break;
    case KEY_MOUSE:
      if (getmouse(&mouse) == OK) {
        if (mouse.bstate & BUTTON1_PRESSED) {
          handleReveal();
        } else if (mouse.bstate & BUTTON3_PRESSED) {
          handleFlag();
        }
      }
      break;
    case 'q':
      cleanup();
      exit(0);
    }
  }


  cleanup();
}
