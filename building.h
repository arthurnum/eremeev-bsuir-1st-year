const int BUILDING_SIZE = 136;
const long PAGE_SIZE = 10000;
const int STREET_SIZE = 128;
const int IDS_OFFSET = 4;
const int BUILDING_OFFSET = 8;

struct Building {
  int id;
  int number;
  char* street;
  long offset;
};

struct BuildingResult {
  int count;
  int limit;
  Building **rows;
};

struct Focus {
  int row;
  int max;
  bool enabled;
  Building *building;
};

int buildingPage;

void building_load_or_init() {
  FILE *f = fopen("building.dat", "rb+");

  if (!f) {
    f = fopen("building.dat", "wb");

    {
      int count = 0;
      fwrite(&count, sizeof(int), 1, f);

      int last_id = 0;
      fwrite(&last_id, sizeof(int), 1, f);

      char *pageMock = (char*)calloc(BUILDING_SIZE, sizeof(char));
      for(int i = 0; i < PAGE_SIZE; i++) {
        fwrite(pageMock, sizeof(char), BUILDING_SIZE, f);
      }
    }
  }

  fclose(f);
}

BuildingResult* building_select() {
  BuildingResult *result = new BuildingResult;
  FILE *f = fopen("building.dat", "rb+");
  fread(&result->count, sizeof(int), 1, f);

  long select_offset = (buildingPage - 1) * BUILDING_SIZE * 10 + BUILDING_OFFSET;
  result->limit = (buildingPage * 10 > result->count) ?
    result->count - (buildingPage - 1) * 10 :
    10;

  if (result->count > 0) {
    result->rows = (Building**)malloc(sizeof(Building*) * result->limit);

    fseek(f, select_offset, SEEK_SET);
    for(int i = 0; i < result->limit; i++) {
      result->rows[i] = new Building;
      result->rows[i]->street = (char*)calloc(STREET_SIZE, sizeof(char));
      result->rows[i]->offset = ftell(f);
      fread(&result->rows[i]->id, sizeof(int), 1, f);
      fread(&result->rows[i]->number, sizeof(int), 1, f);
      fread(result->rows[i]->street, STREET_SIZE*sizeof(char), 1, f);
    }
  }

  fclose(f);
  return result;
}

Building* building_find_by_id(int id) {
  FILE *f = fopen("building.dat", "rb+");
  int count;
  fread(&count, sizeof(int), 1, f);

  if (count < 1) { return NULL; }

  bool searching = true;
  int left = 1;
  int right = count;
  Building *result = new Building;
  result->street = (char*)calloc(STREET_SIZE, sizeof(char));

  while(searching) {
    if (right - left < 4) {
      for(int i = left; i <= right; i++) {
        fseek(f, BUILDING_OFFSET + (i - 1) * BUILDING_SIZE, SEEK_SET);
        fread(&result->id, sizeof(int), 1, f);
        if (result->id == id) {
          fread(&result->number, sizeof(int), 1, f);
          fread(result->street, STREET_SIZE*sizeof(char), 1, f);
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
      fseek(f, BUILDING_OFFSET + (t - 1) * BUILDING_SIZE, SEEK_SET);
      fread(&result->id, sizeof(int), 1, f);
      if (result->id == id) {
        fread(&result->number, sizeof(int), 1, f);
        fread(result->street, STREET_SIZE*sizeof(char), 1, f);
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

void building_add_record(FIELD* fields[]) {
  FILE *f = fopen("building.dat", "rb+");

  int count = 0;
  fread(&count, sizeof(int), 1, f);

  int last_id = 0;
  fread(&last_id, sizeof(int), 1, f);

  Building *house = new Building;
  house->id = last_id + 1;
  house->number = atoi(field_buffer(fields[1], 0));

  fseek(f, BUILDING_OFFSET + BUILDING_SIZE * count, SEEK_SET);
  fwrite(&house->id, 1, sizeof(int), f);
  fwrite(&house->number, 1, sizeof(int), f);
  fwrite(trim_whitespaces(field_buffer(fields[0], 0), STREET_SIZE),
    STREET_SIZE, sizeof(char), f);
  fseek(f, 0, SEEK_SET);
  count++;
  fwrite(&count, 1, sizeof(int), f);
  last_id++;
  fwrite(&last_id, 1, sizeof(int), f);
  fclose(f);
  free(house);
}

void building_update_record(Building* building, FIELD* fields[]) {
  FILE *f = fopen("building.dat", "rb+");

  building->number = atoi(field_buffer(fields[1], 0));

  fseek(f, building->offset, SEEK_SET);
  fwrite(&building->id, 1, sizeof(int), f);
  fwrite(&building->number, 1, sizeof(int), f);
  fwrite(trim_whitespaces(field_buffer(fields[0], 0), STREET_SIZE),
    STREET_SIZE, sizeof(char), f);
  fclose(f);
}

void building_delete_record(Building* building) {
  FILE *f = fopen("building.dat", "rb+");

  int count;
  fread(&count, sizeof(int), 1, f);

  if (count < 1) { return; }

  if (count > 1) {
    long buf_size = (count * BUILDING_SIZE + BUILDING_OFFSET) -
    (building->offset + BUILDING_SIZE);
    char *buf = (char*)malloc(buf_size);

    fseek(f, building->offset + BUILDING_SIZE, SEEK_SET);
    fread(buf, 1, buf_size, f);
    fseek(f, building->offset, SEEK_SET);
    fwrite(buf, 1, buf_size, f);
  }

  char *pageMock = (char*)calloc(BUILDING_SIZE, sizeof(char));
  count--;
  fseek(f, count * BUILDING_SIZE + BUILDING_OFFSET, SEEK_SET);
  fwrite(pageMock, sizeof(char), BUILDING_SIZE, f);
  fseek(f, 0, SEEK_SET);
  fwrite(&count, 1, sizeof(int), f);

  fclose(f);
}

void add_building_window() {
  WINDOW *w = newwin(0, 0, 0, 0);
  keypad(w, true);
  FORM *form;
  FIELD *fields[3];

  fields[0] = new_field(1, 55, 1, 10, 0, 0);
  fields[1] = new_field(1, 4, 3, 10, 0, 0);
  fields[2] = NULL;

  set_field_back(fields[0], A_UNDERLINE);
  field_opts_off(fields[0], O_AUTOSKIP);

  set_field_back(fields[1], A_UNDERLINE);
  field_opts_off(fields[1], O_AUTOSKIP);
  set_field_type(fields[1], TYPE_INTEGER, 0, 1, 999);

  form = new_form(fields);

  form_opts_off(form, O_BS_OVERLOAD);
  form_opts_off(form, O_NL_OVERLOAD);
  set_form_win(form, w);
  // set_form_sub(form, derwin(w, 18, 76, 1, 1));

  post_form(form);

  mvwaddstr(w, 1, 1, "Улица:");
  mvwaddstr(w, 3, 1, "Дом №:");

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
          building_add_record(fields);
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

void edit_building_window(Building* building) {
  if (!building) { return; }

  WINDOW *w = newwin(0, 0, 0, 0);
  keypad(w, true);
  FORM *form;
  FIELD *fields[3];

  fields[0] = new_field(1, 55, 1, 10, 0, 0);
  fields[1] = new_field(1, 4, 3, 10, 0, 0);
  fields[2] = NULL;

  set_field_back(fields[0], A_UNDERLINE);
  field_opts_off(fields[0], O_AUTOSKIP);
  set_field_buffer(fields[0], 0, building->street);

  set_field_back(fields[1], A_UNDERLINE);
  field_opts_off(fields[1], O_AUTOSKIP);
  set_field_type(fields[1], TYPE_INTEGER, 0, 1, 999);
  char numstr[4] = "";
  sprintf(numstr, "%d", building->number);
  set_field_buffer(fields[1], 0, numstr);

  form = new_form(fields);

  form_opts_off(form, O_BS_OVERLOAD);
  form_opts_off(form, O_NL_OVERLOAD);
  set_form_win(form, w);
  // set_form_sub(form, derwin(w, 18, 76, 1, 1));

  post_form(form);

  mvwaddstr(w, 1, 1, "Улица:");
  mvwaddstr(w, 3, 1, "Дом №:");

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
          building_update_record(building, fields);
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

void building_index_window(WINDOW *w, BuildingResult *buildings, Focus *focus) {
  werase(w);
  mvwaddstr(w, 1, 3, "#\tАдрес");

  for(int i = 0; i < buildings->limit; i++) {
    mvwprintw(w, 3 + i * 2, 3, "%d\t%s %d",
      buildings->rows[i]->id, buildings->rows[i]->street, buildings->rows[i]->number);
  }

  if (buildings->count > 0) {
    focus->enabled = true;
    focus->row = 1;
    focus->max = buildings->limit;
    focus->building = buildings->rows[focus->row - 1];
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

void buildingController() {
  WINDOW *w = newwin(0, 0, 0, 0);
  Focus *focus = new Focus;
  keypad(w, true);

  buildingPage = 1;
  BuildingResult *buildings;
  buildings = building_select();
  building_index_window(w, buildings, focus);

  int ctrlInput;

  do {
    ctrlInput = wgetch(w);

    switch(ctrlInput) {
      case KEY_F(1):
        add_building_window();
        free(buildings);
        buildings = building_select();
        building_index_window(w, buildings, focus);
        break;

      case KEY_F(2):
        edit_building_window(focus->building);
        free(buildings);
        buildings = building_select();
        building_index_window(w, buildings, focus);
        break;

      case KEY_F(3):
        building_delete_record(focus->building);
        free(buildings);
        buildings = building_select();
        building_index_window(w, buildings, focus);
        break;

      case KEY_DOWN:
        if (focus->enabled) {
          mvwaddstr(w, 1 + focus->row * 2, 1, "  ");
          focus->row++;
          if (focus->row > focus->max) {
            focus->row = 1;
          }
          focus->building = buildings->rows[focus->row - 1];
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
          focus->building = buildings->rows[focus->row - 1];
          mvwaddstr(w, 1 + focus->row * 2, 1, "->");
        }
        break;

      case KEY_LEFT:
        buildingPage--;
        if (buildingPage < 1) {
          buildingPage = 1;
        } else {
          free(buildings);
          buildings = building_select();
          building_index_window(w, buildings, focus);
        }
        break;

      case KEY_RIGHT:
        buildingPage++;
        if (buildingPage * 10 - buildings->count > 9) {
          buildingPage--;
        } else {
          free(buildings);
          buildings = building_select();
          building_index_window(w, buildings, focus);
        }
        break;

      defaut:
        break;
    }
  } while(ctrlInput != ESC_KEY);

  delwin(w);
}
