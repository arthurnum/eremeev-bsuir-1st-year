const int PERSON_SIZE = 116;
const long PERSON_PAGE_SIZE = 10000;
const int NAME_SIZE = 48;
const int SURNAME_SIZE = 64;
const int PERSON_IDS_OFFSET = 4;
const int PERSON_OFFSET = 8;

struct Person {
  int id;
  char* surname;
  char* name;
  long offset;
};

struct PersonResult {
  int count;
  int limit;
  Person **rows;
};

struct PersonFocus {
  int row;
  int max;
  bool enabled;
  Person *person;
};

int personPage;

void person_load_or_init() {
  FILE *f = fopen("person.dat", "rb+");

  if (!f) {
    f = fopen("person.dat", "wb");

    {
      int count = 0;
      fwrite(&count, sizeof(int), 1, f);

      int last_id = 0;
      fwrite(&last_id, sizeof(int), 1, f);

      char *pageMock = (char*)calloc(PERSON_SIZE, sizeof(char));
      for(int i = 0; i < PERSON_PAGE_SIZE; i++) {
        fwrite(pageMock, sizeof(char), PERSON_SIZE, f);
      }
    }
  }

  fclose(f);
}

PersonResult* person_select() {
  PersonResult *result = new PersonResult;
  FILE *f = fopen("person.dat", "rb+");
  fread(&result->count, sizeof(int), 1, f);

  long select_offset = (personPage - 1) * PERSON_SIZE * 10 + PERSON_OFFSET;
  result->limit = (personPage * 10 > result->count) ?
    result->count - (personPage - 1) * 10 :
    10;

  if (result->count > 0) {
    result->rows = (Person**)malloc(sizeof(Person*) * result->limit);

    fseek(f, select_offset, SEEK_SET);
    for(int i = 0; i < result->limit; i++) {
      result->rows[i] = new Person;
      result->rows[i]->surname = (char*)calloc(SURNAME_SIZE, sizeof(char));
      result->rows[i]->name = (char*)calloc(NAME_SIZE, sizeof(char));
      result->rows[i]->offset = ftell(f);
      fread(&result->rows[i]->id, sizeof(int), 1, f);
      fread(result->rows[i]->surname, SURNAME_SIZE*sizeof(char), 1, f);
      fread(result->rows[i]->name, NAME_SIZE*sizeof(char), 1, f);
    }
  }

  fclose(f);
  return result;
}

Person* person_find_by_id(int id) {
  FILE *f = fopen("person.dat", "rb+");
  int count;
  fread(&count, sizeof(int), 1, f);

  if (count < 1) { return NULL; }

  bool searching = true;
  int left = 1;
  int right = count;
  Person *result = new Person;
  result->surname = (char*)calloc(SURNAME_SIZE, sizeof(char));
  result->name = (char*)calloc(NAME_SIZE, sizeof(char));

  while(searching) {
    if (right - left < 4) {
      for(int i = left; i <= right; i++) {
        fseek(f, PERSON_OFFSET + (i - 1) * PERSON_SIZE, SEEK_SET);
        fread(&result->id, sizeof(int), 1, f);
        if (result->id == id) {
          fread(result->surname, SURNAME_SIZE*sizeof(char), 1, f);
          fread(result->name, NAME_SIZE*sizeof(char), 1, f);
          searching = false;
          break;
        }
      }
      if (searching) {
        free(result);
        result = NULL;
        searching = false;
      }
    } else {
      int t = (left + right) / 2;
      fseek(f, PERSON_OFFSET + (t - 1) * PERSON_SIZE, SEEK_SET);
      fread(&result->id, sizeof(int), 1, f);
      if (result->id == id) {
        fread(result->surname, SURNAME_SIZE*sizeof(char), 1, f);
        fread(result->name, NAME_SIZE*sizeof(char), 1, f);
        searching = false;
      } else {
        if (result->id > id) {
          right = t;
        } else {
          left = t;
        }
      }
    }
  }

  fclose(f);
  return result;
}

void person_index_window(WINDOW *w, PersonResult *persons, PersonFocus *focus) {
  werase(w);
  mvwaddstr(w, 1, 3, "#\tИмя");

  for(int i = 0; i < persons->limit; i++) {
    mvwprintw(w, 3 + i * 2, 3, "%d\t%s %s",
      persons->rows[i]->id, persons->rows[i]->surname, persons->rows[i]->name);
  }

  if (persons->count > 0) {
    focus->enabled = true;
    focus->row = 1;
    focus->max = persons->limit;
    focus->person = persons->rows[focus->row - 1];
    mvwaddstr(w, 1 + focus->row * 2, 1, "->");
  } else {
    focus->enabled = false;
  }

  curs_set(0);
  wrefresh(w);
  touchwin(w);

  slk_set(1, "Новый", 0);
  slk_set(2, "Измен", 0);
  slk_set(3, "Удал", 0);
  slk_set(4, "", 0);
  slk_refresh();
}

Person* person_search_window() {
  WINDOW *w = newwin(0, 0, 0, 0);
  PersonFocus *focus = new PersonFocus;
  keypad(w, true);

  personPage = 1;
  PersonResult *persons;
  persons = person_select();
  person_index_window(w, persons, focus);

  Person *result = NULL;
  bool exit = false;
  int ctrlInput;

  do {
    ctrlInput = wgetch(w);

    switch(ctrlInput) {
      case KEY_DOWN:
        if (focus->enabled) {
          mvwaddstr(w, 1 + focus->row * 2, 1, "  ");
          focus->row++;
          if (focus->row > focus->max) {
            focus->row = 1;
          }
          focus->person = persons->rows[focus->row - 1];
          mvwaddstr(w, 1 + focus->row * 2, 1, "->");
        }
        break;

      case KEY_UP:
        if (focus->enabled) {
          mvwaddstr(w, 1 + focus->row * 2, 1, "  ");
          focus->row--;
          if (focus->row < 1) {
            focus->row = focus->max;
          }
          focus->person = persons->rows[focus->row - 1];
          mvwaddstr(w, 1 + focus->row * 2, 1, "->");
        }
        break;

      case KEY_LEFT:
        personPage--;
        if (personPage < 1) {
          personPage = 1;
        } else {
          free(persons);
          persons = person_select();
          person_index_window(w, persons, focus);
        }
        break;

      case KEY_RIGHT:
        personPage++;
        if (personPage * 10 - persons->count > 9) {
          personPage--;
        } else {
          free(persons);
          persons = person_select();
          person_index_window(w, persons, focus);
        }
        break;

      case ESC_KEY:
        exit = true;
        break;

      case KEY_ENTER:
      case 10:
        result = focus->person;
        exit = true;
        break;

      default:
        break;
    }
  } while(!exit);

  delwin(w);
  return result;
}

void person_add_record(FIELD* fields[]) {
  FILE *f = fopen("person.dat", "rb+");

  int count = 0;
  fread(&count, sizeof(int), 1, f);

  int last_id = 0;
  fread(&last_id, sizeof(int), 1, f);

  Person *person = new Person;
  person->id = last_id + 1;

  fseek(f, PERSON_OFFSET + PERSON_SIZE * count, SEEK_SET);
  fwrite(&person->id, 1, sizeof(int), f);
  fwrite(trim_whitespaces(field_buffer(fields[0], 0), SURNAME_SIZE),
    SURNAME_SIZE, sizeof(char), f);
  fwrite(trim_whitespaces(field_buffer(fields[1], 0), NAME_SIZE),
    NAME_SIZE, sizeof(char), f);
  fseek(f, 0, SEEK_SET);
  count++;
  fwrite(&count, 1, sizeof(int), f);
  last_id++;
  fwrite(&last_id, 1, sizeof(int), f);
  fclose(f);
  free(person);
}

void add_person_window() {
  WINDOW *w = newwin(0, 0, 0, 0);
  keypad(w, true);
  FORM *form;
  FIELD *fields[3];

  fields[0] = new_field(1, 55, 1, 10, 0, 0);
  fields[1] = new_field(1, 40, 3, 10, 0, 0);
  fields[2] = NULL;

  set_field_back(fields[0], A_UNDERLINE);
  field_opts_off(fields[0], O_AUTOSKIP);

  set_field_back(fields[1], A_UNDERLINE);
  field_opts_off(fields[1], O_AUTOSKIP);

  form = new_form(fields);

  form_opts_off(form, O_BS_OVERLOAD);
  form_opts_off(form, O_NL_OVERLOAD);
  set_form_win(form, w);
  // set_form_sub(form, derwin(w, 18, 76, 1, 1));

  post_form(form);

  mvwaddstr(w, 1, 1, "Фамилия:");
  mvwaddstr(w, 3, 1, "Имя:");

  curs_set(1);
  form_driver(form, REQ_FIRST_FIELD);
  wrefresh(w);

  bool exit =false;
  int ch;
  while(!exit) {
    ch = wgetch(w);

    switch(ch) {
      case KEY_DOWN:
        form_driver(form, REQ_NEXT_FIELD);
        form_driver(form, REQ_END_LINE);
        break;

      case KEY_UP:
        form_driver(form, REQ_PREV_FIELD);
        form_driver(form, REQ_END_LINE);
        break;

      case KEY_BACKSPACE:
      case 127:
        form_driver(form, REQ_DEL_PREV);
        break;

      case ESC_KEY:
        exit = true;
        break;

      case KEY_ENTER:
      case 10:
        if (form_driver(form, REQ_VALIDATION) == E_OK) {
          person_add_record(fields);
          exit = true;
        }
        break;

      default:
        form_driver(form, ch);
        break;
    }
    wrefresh(w);
  }

  unpost_form(form);
  free_form(form);
  free_field(fields[0]);
  free_field(fields[1]);
  delwin(w);
}

void personController() {
  WINDOW *w = newwin(0, 0, 0, 0);
  PersonFocus *focus = new PersonFocus;
  keypad(w, true);

  personPage = 1;
  PersonResult *persons;
  persons = person_select();
  person_index_window(w, persons, focus);

  int ctrlInput;

  do {
    ctrlInput = wgetch(w);

    switch(ctrlInput) {
      case KEY_F(1):
        add_person_window();
        free(persons);
        persons = person_select();
        person_index_window(w, persons, focus);
        break;

      case KEY_F(2):
        // edit_building_window(focus->building);
        free(persons);
        persons = person_select();
        person_index_window(w, persons, focus);
        break;

      case KEY_F(3):
        // building_delete_record(focus->building);
        free(persons);
        persons = person_select();
        person_index_window(w, persons, focus);
        break;

      case KEY_DOWN:
        if (focus->enabled) {
          mvwaddstr(w, 1 + focus->row * 2, 1, "  ");
          focus->row++;
          if (focus->row > focus->max) {
            focus->row = 1;
          }
          focus->person = persons->rows[focus->row - 1];
          mvwaddstr(w, 1 + focus->row * 2, 1, "->");
        }
        break;

      case KEY_UP:
        if (focus->enabled) {
          mvwaddstr(w, 1 + focus->row * 2, 1, "  ");
          focus->row--;
          if (focus->row < 1) {
            focus->row = focus->max;
          }
          focus->person = persons->rows[focus->row - 1];
          mvwaddstr(w, 1 + focus->row * 2, 1, "->");
        }
        break;

      case KEY_LEFT:
        personPage--;
        if (personPage < 1) {
          personPage = 1;
        } else {
          free(persons);
          persons = person_select();
          person_index_window(w, persons, focus);
        }
        break;

      case KEY_RIGHT:
        personPage++;
        if (personPage * 10 - persons->count > 9) {
          personPage--;
        } else {
          free(persons);
          persons = person_select();
          person_index_window(w, persons, focus);
        }
        break;

      default:
        break;
    }
  } while(ctrlInput != ESC_KEY);

  delwin(w);
}
