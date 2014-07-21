#ifndef FRONT_H
#define FRONT_H
#include <numeric>
#include <limits>
#include <time.h>
#include "braille_output.h"
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#define B_KEY 0x4200
#define BACK_KEY 0x0800
#define BLANK_KEY 0x2000
#define CTRL_A_KEY 0x0100
#define CTRL_C_KEY 0x0300
#define CTRL_END_KEY 0xe075 // Ctrl+End combination keys.
#define CTRL_HOME_KEY 0xe077 // Ctrl+Home combination keys.
#define CTRL_LEFT_KEY 0xe073 // Ctrl+left combination keys.
#define CTRL_RIGHT_KEY 0xe074 // Ctrl+right combination keys.
#define CTRL_P_KEY 0x1000
#define CTRL_X_KEY 0x1800
#define DELETE_KEY 0xe053
#define DOWN_KEY 0xe050
#define END_KEY 0xe04f
#define ENTER_KEY 0x0d00
#define ESC_KEY 0x1b00
#define F2_KEY 0x003c
#define F3_KEY 0x003d
#define H_KEY 0x4800
#define HOME_KEY 0xe047
#define I_KEY 0x4900
#define LEFT_KEY 0xe04b
#define M_KEY 0x4d00
#define N_KEY 0x4e00
#define O_KEY 0x4f00
#define P_KEY 0x5000
#define PERIOD_KEY 0x2e00 // '.' character.
#define RIGHT_KEY 0xe04d
#define S_KEY 0x5300
#define TAB_KEY 0x0900
#define UP_KEY 0xe048
#define b_KEY 0x6200
#define h_KEY 0x6800
#define i_KEY 0x6900
#define m_KEY 0x6d00
#define n_KEY 0x6e00
#define o_KEY 0x6f00
#define p_KEY 0x7000
#define s_KEY 0x7300
typedef unsigned char choice_t;
class Front{
 private:
  class cursor_control_block{
   private:
    class line_content: public std::vector<short>{
     public:
      std::string content;
      line_content() throw(){}
      line_content(size_type size) throw(): std::vector<short>(size){}
    };
    std::map<short, line_content> step;
    short line, index;
   public:
    cursor_control_block() throw(): line(0), index(0){}
    void append_step(short, short) throw();
    short column() const throw();
    void go_down(short) throw();
    void go_end() throw();
    void go_first_home() throw();
    void go_home() throw();
    void go_last_end(short) throw();
    void go_left() throw();
    void go_right() throw();
    void go_up() throw();
    void put(short row, const std::string& s) throw(){step[row].content+=s;}
    void reset_position() throw(){go_first_home();}
    void reset_step_map() throw(){step.clear();}
    short row() const throw(){return line;}
  } cursor;
  std::vector<cursor_control_block> page_frame;
  char path[MAX_PATH], file_title[FILENAME_MAX];
  bms_score basic_data;
  brl_output output_data;
  short bottom;
  choice_t main_choice;
  static const choice_t main_menu_range, option_menu_entries;
  Front() throw();
  virtual ~Front() throw(){}
  void assign_step(short, size_t) throw();
  void assign_text(short, std::string, std::string::size_type=0) throw();
 public:
  static Front console;
  static void clear_key_queue() throw();
  static void midi_debug(bool (*)(unsigned long) throw(), bool) throw();
  static void no_score_message() throw();
  BrlSysArg state;
  key_t browse(clock_t=std::numeric_limits<clock_t>::max()) throw();
  bool choose_input(const char*) throw();
  void export_text(FILE*, int) const throw();
  const char* get_file_title() const throw(){return file_title;}
  choice_t get_main_choice(){return this->main_choice;}
  const char* get_path() const throw(){return path;}
  choice_t main_menu() throw();
  void option_menu(BrlSysArg&, OralSysArg&, PlaySysArg&) throw();
  void print_score(int, bool=true) throw();
  void print_total_score(int, bool=true) throw();
  void save(int) const throw();
  bms_score& score() throw(){return basic_data;}
  const bms_score& score() const throw(){return basic_data;}
  void score_identifier(char*, size_t, bool=true) const throw();
  bool set_oral_range(OralOutArg&) const throw();
  bool set_play_range(PlayOutArg&) const throw();
  const brl_output& sheet() const throw(){return output_data;}
  brl_output& sheet() throw(){return output_data;}
  key_t start_message(bool) const throw();
  bool want_retry(const char*) throw();
};
#endif // FRONT_H_INCLUDED
