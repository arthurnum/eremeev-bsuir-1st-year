const int CONTRACT_SIZE = 12;
const long CONTRACT_PAGE_SIZE = 10000;
const int CONTRACT_IDS_OFFSET = 4;
const int CONTRACT_OFFSET = 8;

struct Contract {
  int id;
  int flat_id;
  int person_id;
  long offset;
  Flat *flat;
  Person *person;
};

struct ContractResult {
  int count;
  int limit;
  Contract **rows;
};

struct ContractFocus {
  int row;
  int max;
  bool enabled;
  Contract *contract;
};

int contractPage;

void contract_load_or_init() {
  FILE *f = fopen("contract.dat", "rb+");

  if (!f) {
    f = fopen("contract.dat", "wb");

    {
      int count = 0;
      fwrite(&count, sizeof(int), 1, f);

      int last_id = 0;
      fwrite(&last_id, sizeof(int), 1, f);

      char *pageMock = (char*)calloc(CONTRACT_SIZE * 10, sizeof(char));
      for(int i = 0; i < CONTRACT_PAGE_SIZE; i++) {
        fwrite(pageMock, sizeof(char), CONTRACT_SIZE * 10, f);
      }
    }
  }

  fclose(f);
}

ContractResult* contract_select() {
  ContractResult *result = new ContractResult;
  FILE *f = fopen("contract.dat", "rb+");
  fread(&result->count, sizeof(int), 1, f);

  long select_offset = (contractPage - 1) * CONTRACT_SIZE * 10 + CONTRACT_OFFSET;
  result->limit = (contractPage * 10 > result->count) ?
    result->count - (contractPage - 1) * 10 :
    10;

  if (result->count > 0) {
    result->rows = (Contract**)malloc(sizeof(Contract*) * result->limit);

    fseek(f, select_offset, SEEK_SET);
    for(int i = 0; i < result->limit; i++) {
      result->rows[i] = new Contract;
      result->rows[i]->offset = ftell(f);
      fread(&result->rows[i]->id, sizeof(int), 1, f);
      fread(&result->rows[i]->flat_id, sizeof(int), 1, f);
      fread(&result->rows[i]->person_id, sizeof(int), 1, f);
      result->rows[i]->flat = flat_find_by_id(result->rows[i]->flat_id);
      result->rows[i]->person = person_find_by_id(result->rows[i]->person_id);
    }
  }

  fclose(f);
  return result;
}

void contract_index_window(WINDOW *w, ContractResult *contracts, ContractFocus *focus) {
  werase(w);
  mvwaddstr(w, 1, 3, "#");
  mvwaddstr(w, 1, 9, "Адрес");
  mvwaddstr(w, 1, 37, "Лицо");

  for(int i = 0; i < contracts->limit; i++) {
    mvwprintw(w, 3 + i * 2, 3, "%d",
      contracts->rows[i]->id
    );

    if (contracts->rows[i]->flat) {
      char flat_str[8] = "";
      sprintf(flat_str, "%d m2", contracts->rows[i]->flat->square);

      char building_str[128] = "";
      if (contracts->rows[i]->flat->building) {
        sprintf(building_str, "%s %d", contracts->rows[i]->flat->building->street,
          contracts->rows[i]->flat->building->number);
      } else {
        sprintf(building_str, "N/A");
      }
      mvwprintw(w, 3 + i * 2, 9, "%s | %s", building_str, flat_str);
    }

    if (contracts->rows[i]->person) {
      mvwprintw(w, 3 + i * 2, 37, "%s %s",
        contracts->rows[i]->person->surname,
        contracts->rows[i]->person->name);
    }
  }

  if (contracts->count > 0) {
    focus->enabled = true;
    focus->row = 1;
    focus->max = contracts->limit;
    focus->contract = contracts->rows[focus->row - 1];
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

void contract_add_record(FIELD* fields[]) {
  FILE *f = fopen("contract.dat", "rb+");

  int count = 0;
  fread(&count, sizeof(int), 1, f);

  int last_id = 0;
  fread(&last_id, sizeof(int), 1, f);

  Contract *contract = new Contract;
  contract->id = last_id + 1;
  contract->flat_id = atoi(field_buffer(fields[0], 0));
  contract->person_id = atoi(field_buffer(fields[1], 0));

  fseek(f, CONTRACT_OFFSET + CONTRACT_SIZE * count, SEEK_SET);
  fwrite(&contract->id, 1, sizeof(int), f);
  fwrite(&contract->flat_id, 1, sizeof(int), f);
  fwrite(&contract->person_id, 1, sizeof(int), f);
  fseek(f, 0, SEEK_SET);
  count++;
  fwrite(&count, 1, sizeof(int), f);
  last_id++;
  fwrite(&last_id, 1, sizeof(int), f);
  fclose(f);
  free(contract);
}

void add_contract_window() {
  WINDOW *w = newwin(0, 0, 0, 0);
  keypad(w, true);
  FORM *form;
  FIELD *fields[5];

  fields[0] = new_field(1, 4, 1, 11, 0, 0); //flat_id
  fields[1] = new_field(1, 4, 3, 11, 0, 0); //person_id
  fields[2] = new_field(1, 64, 1, 20, 0, 0); //Flat
  fields[3] = new_field(1, 64, 3, 20, 0, 0); //Person
  fields[4] = NULL;

  set_field_back(fields[0], A_UNDERLINE);
  field_opts_off(fields[0], O_AUTOSKIP | O_EDIT);
  set_field_type(fields[0], TYPE_INTEGER, 0, 1, 999);

  set_field_back(fields[1], A_UNDERLINE);
  field_opts_off(fields[1], O_AUTOSKIP | O_EDIT);
  set_field_type(fields[1], TYPE_INTEGER, 0, 1, 999);

  field_opts_off(fields[2], O_AUTOSKIP | O_EDIT);
  field_opts_off(fields[3], O_AUTOSKIP | O_EDIT);

  form = new_form(fields);

  form_opts_off(form, O_BS_OVERLOAD);
  form_opts_off(form, O_NL_OVERLOAD);
  set_form_win(form, w);
  // set_form_sub(form, derwin(w, 18, 76, 1, 1));

  post_form(form);

  mvwaddstr(w, 1, 1, "Квартира:");
  mvwaddstr(w, 3, 1, "Лицо:");

  curs_set(0);
  form_driver(form, REQ_FIRST_FIELD);
  wrefresh(w);

  bool exit =false;
  int ch;
  while(!exit) {
    ch = wgetch(w);

    switch(ch) {
      case KEY_F(1):
        {
          Flat *flat = flat_search_window();
          if (flat) {
            char buf[4] = "";
            sprintf(buf, "%d", flat->id);
            set_field_buffer(fields[0], 0, buf);
            char flat_str[128] = "";
            if (flat->building) {
              sprintf(flat_str, "%s %d | %d m2",
                flat->building->street,
                flat->building->number,
                flat->square
              );
            } else {
              sprintf(flat_str, "N/A | %d m2", flat->square);
            }
            set_field_buffer(fields[2], 0, flat_str);
            free(flat);
          }
        }
        touchwin(w);
        break;

      case KEY_F(2):
        {
          Person *person = person_search_window();
          if (person) {
              char buf[4] = "";
              sprintf(buf, "%d", person->id);
              set_field_buffer(fields[1], 0, buf);
              char person_str[128] = "";
              sprintf(person_str, "%s %s", person->surname, person->name);
              set_field_buffer(fields[3], 0, person_str);
              free(person);
          }
        }
        touchwin(w);
        break;

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
          contract_add_record(fields);
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

void contractController() {
  WINDOW *w = newwin(0, 0, 0, 0);
  ContractFocus *focus = new ContractFocus;
  keypad(w, true);

  contractPage = 1;
  ContractResult *contracts;
  contracts = contract_select();
  contract_index_window(w, contracts, focus);

  int ctrlInput;

  do {
    ctrlInput = wgetch(w);

    switch(ctrlInput) {
      case KEY_F(1):
        add_contract_window();
        free(contracts);
        contracts = contract_select();
        contract_index_window(w, contracts, focus);
        break;

      case KEY_F(2):
        // edit_building_window(focus->building);
        free(contracts);
        contracts = contract_select();
        contract_index_window(w, contracts, focus);
        break;

      case KEY_F(3):
        // building_delete_record(focus->building);
        free(contracts);
        contracts = contract_select();
        contract_index_window(w, contracts, focus);
        break;

      case KEY_DOWN:
        if (focus->enabled) {
          mvwaddstr(w, 1 + focus->row * 2, 1, "  ");
          focus->row++;
          if (focus->row > focus->max) {
            focus->row = 1;
          }
          focus->contract = contracts->rows[focus->row - 1];
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
          focus->contract = contracts->rows[focus->row - 1];
          mvwaddstr(w, 1 + focus->row * 2, 1, "->");
        }
        break;

      case KEY_LEFT:
        contractPage--;
        if (contractPage < 1) {
          contractPage = 1;
        } else {
          free(contracts);
          contracts = contract_select();
          contract_index_window(w, contracts, focus);
        }
        break;

      case KEY_RIGHT:
        contractPage++;
        if (contractPage * 10 - contracts->count > 9) {
          contractPage--;
        } else {
          free(contracts);
          contracts = contract_select();
          contract_index_window(w, contracts, focus);
        }
        break;

      default:
        break;
    }
  } while(ctrlInput != ESC_KEY);

  delwin(w);
}
