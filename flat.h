const int FLAT_SIZE = 20;
const long FLAT_PAGE_SIZE = 10000;
const int FLAT_IDS_OFFSET = 4;
const int FLAT_OFFSET = 8;

struct Flat {
  int id;
  int building_id;
  int floor;
  int rooms;
  int square;
  long offset;
  Building *building;
};

struct FlatResult {
  int count;
  int limit;
  Flat **rows;
};

struct FlatFocus {
  int row;
  int max;
  bool enabled;
  Flat *flat;
};

int flatPage;

void flat_load_or_init() {
  FILE *f = fopen("flat.dat", "rb+");

  if (!f) {
    f = fopen("flat.dat", "wb");

    {
      int count = 0;
      fwrite(&count, sizeof(int), 1, f);

      int last_id = 0;
      fwrite(&last_id, sizeof(int), 1, f);

      char *pageMock = (char*)calloc(FLAT_SIZE * 10, sizeof(char));
      for(int i = 0; i < FLAT_PAGE_SIZE; i++) {
        fwrite(pageMock, sizeof(char), FLAT_SIZE * 10, f);
      }
    }
  }

  fclose(f);
}

FlatResult* flat_select() {
  FlatResult *result = new FlatResult;
  FILE *f = fopen("flat.dat", "rb+");
  fread(&result->count, sizeof(int), 1, f);

  long select_offset = (flatPage - 1) * FLAT_SIZE * 10 + FLAT_OFFSET;
  result->limit = (flatPage * 10 > result->count) ?
    result->count - (flatPage - 1) * 10 :
    10;

  if (result->count > 0) {
    result->rows = (Flat**)malloc(sizeof(Flat*) * result->limit);

    fseek(f, select_offset, SEEK_SET);
    for(int i = 0; i < result->limit; i++) {
      result->rows[i] = new Flat;
      result->rows[i]->offset = ftell(f);
      fread(&result->rows[i]->id, sizeof(int), 1, f);
      fread(&result->rows[i]->building_id, sizeof(int), 1, f);
      fread(&result->rows[i]->floor, sizeof(int), 1, f);
      fread(&result->rows[i]->rooms, sizeof(int), 1, f);
      fread(&result->rows[i]->square, sizeof(int), 1, f);
      result->rows[i]->building = building_find_by_id(result->rows[i]->building_id);
    }
  }

  fclose(f);
  return result;
}

void flat_index_window(WINDOW *w, FlatResult *flats, FlatFocus *focus) {
  werase(w);
  mvwaddstr(w, 1, 3, "#\tЭтаж\tКомнаты\tПлощадь\tДом");

  for(int i = 0; i < flats->limit; i++) {
    char building_string[255] = "";
    if (flats->rows[i]->building) {
      sprintf(building_string, "%s %d", flats->rows[i]->building->street,
        flats->rows[i]->building->number);
    } else {
      sprintf(building_string, "N/A");
    }

    mvwprintw(w, 3 + i * 2, 3, "%d\t%d\t%d\t%d\t%s",
      flats->rows[i]->id,
      flats->rows[i]->floor,
      flats->rows[i]->rooms,
      flats->rows[i]->square,
      building_string
    );
  }

  if (flats->count > 0) {
    focus->enabled = true;
    focus->row = 1;
    focus->max = flats->limit;
    focus->flat = flats->rows[focus->row - 1];
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
  slk_refresh();
}

void flat_add_record(FIELD* fields[]) {
  FILE *f = fopen("flat.dat", "rb+");

  int count = 0;
  fread(&count, sizeof(int), 1, f);

  int last_id = 0;
  fread(&last_id, sizeof(int), 1, f);

  Flat *flat = new Flat;
  flat->id = last_id + 1;
  flat->building_id = atoi(field_buffer(fields[0], 0));
  flat->floor = atoi(field_buffer(fields[1], 0));
  flat->rooms = atoi(field_buffer(fields[2], 0));
  flat->square = atoi(field_buffer(fields[3], 0));

  fseek(f, FLAT_OFFSET + FLAT_SIZE * count, SEEK_SET);
  fwrite(&flat->id, 1, sizeof(int), f);
  fwrite(&flat->building_id, 1, sizeof(int), f);
  fwrite(&flat->floor, 1, sizeof(int), f);
  fwrite(&flat->rooms, 1, sizeof(int), f);
  fwrite(&flat->square, 1, sizeof(int), f);
  fseek(f, 0, SEEK_SET);
  count++;
  fwrite(&count, 1, sizeof(int), f);
  last_id++;
  fwrite(&last_id, 1, sizeof(int), f);
  fclose(f);
  free(flat);
}

void add_flat_window() {
  WINDOW *w = newwin(0, 0, 0, 0);
  keypad(w, true);
  FORM *form;
  FIELD *fields[5];

  fields[0] = new_field(1, 4, 1, 10, 0, 0);
  fields[1] = new_field(1, 4, 3, 10, 0, 0);
  fields[2] = new_field(1, 4, 5, 10, 0, 0);
  fields[3] = new_field(1, 4, 7, 10, 0, 0);
  fields[4] = NULL;

  set_field_back(fields[0], A_UNDERLINE);
  field_opts_off(fields[0], O_AUTOSKIP);
  set_field_type(fields[0], TYPE_INTEGER, 0, 1, 999);

  set_field_back(fields[1], A_UNDERLINE);
  field_opts_off(fields[1], O_AUTOSKIP);
  set_field_type(fields[1], TYPE_INTEGER, 0, 1, 999);

  set_field_back(fields[2], A_UNDERLINE);
  field_opts_off(fields[2], O_AUTOSKIP);
  set_field_type(fields[2], TYPE_INTEGER, 0, 1, 999);

  set_field_back(fields[3], A_UNDERLINE);
  field_opts_off(fields[3], O_AUTOSKIP);
  set_field_type(fields[3], TYPE_INTEGER, 0, 1, 999);

  form = new_form(fields);

  form_opts_off(form, O_BS_OVERLOAD);
  form_opts_off(form, O_NL_OVERLOAD);
  set_form_win(form, w);
  // set_form_sub(form, derwin(w, 18, 76, 1, 1));

  post_form(form);

  mvwaddstr(w, 1, 1, "Дом:");
  mvwaddstr(w, 3, 1, "Этаж:");
  mvwaddstr(w, 5, 1, "Комнаты:");
  mvwaddstr(w, 7, 1, "Площадь:");

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
          flat_add_record(fields);
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

void flatController() {
  WINDOW *w = newwin(0, 0, 0, 0);
  FlatFocus *focus = new FlatFocus;
  keypad(w, true);

  flatPage = 1;
  FlatResult *flats;
  flats = flat_select();
  flat_index_window(w, flats, focus);

  int ctrlInput;

  do {
    ctrlInput = wgetch(w);

    switch(ctrlInput) {
      case KEY_F(1):
        add_flat_window();
        free(flats);
        flats = flat_select();
        flat_index_window(w, flats, focus);
        break;

      case KEY_F(2):
        // edit_building_window(focus->building);
        free(flats);
        flats = flat_select();
        flat_index_window(w, flats, focus);
        break;

      case KEY_F(3):
        // building_delete_record(focus->building);
        free(flats);
        flats = flat_select();
        flat_index_window(w, flats, focus);
        break;

      case KEY_DOWN:
        if (focus->enabled) {
          mvwaddstr(w, 1 + focus->row * 2, 1, "  ");
          focus->row++;
          if (focus->row > focus->max) {
            focus->row = 1;
          }
          focus->flat = flats->rows[focus->row - 1];
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
          focus->flat = flats->rows[focus->row - 1];
          mvwaddstr(w, 1 + focus->row * 2, 1, "->");
        }
        break;

      case KEY_LEFT:
        flatPage--;
        if (flatPage < 1) {
          flatPage = 1;
        } else {
          free(flats);
          flats = flat_select();
          flat_index_window(w, flats, focus);
        }
        break;

      case KEY_RIGHT:
        flatPage++;
        if (flatPage * 10 - flats->count > 9) {
          flatPage--;
        } else {
          free(flats);
          flats = flat_select();
          flat_index_window(w, flats, focus);
        }
        break;

      defaut:
        break;
    }
  } while(ctrlInput != ESC_KEY);

  delwin(w);
}
