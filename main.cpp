#include "stdlib.h"
#include "string.h"
#include "ctype.h"
#include <ncurses.h>
#include <form.h>
#include <locale.h>

#define ESC_KEY 27

static char* trim_whitespaces(char *str, int maxlen)
{
	char *end;

	// trim leading space
	while(isspace(*str))
		str++;

	if(*str == 0) // all spaces?
		return str;

	// trim trailing space
	end = str + strnlen(str, maxlen) - 1;

	while(end > str && isspace(*end))
		end--;

	// write new null terminator
	*(end+1) = '\0';

	return str;
}

#include "building.h"
#include "person.h"

void load_or_init_db_files() {
  building_load_or_init();
  person_load_or_init();
}

void set_main_screen() {
  erase();
  slk_set(1, "Дом", 0);
  slk_set(2, "Лицо", 0);
  slk_set(3, "", 0);
  slk_refresh();
}

int main() {
  setlocale(LC_ALL, "ru_RU.UTF-8");
  load_or_init_db_files();
  slk_init(3);
  initscr();
  keypad(stdscr, true);
  start_color();
  cbreak();
  noecho();

  set_main_screen();

  int ctrlInput;

  do {
    ctrlInput = getch();

    switch(ctrlInput) {
      case KEY_F(1):
        buildingController();
        set_main_screen();
        break;

      case KEY_F(2):
        personController();
        set_main_screen();
        break;

      defaut:
        break;
    }
  } while(ctrlInput != ESC_KEY);

  endwin();
  return 0;
}
