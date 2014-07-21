#ifndef PSEUDO_GUI_MODULE_H
#define PSEUDO_GUI_MODULE_H
#include <list>
#include "common.h"
class GUI_element{
 private:
  std::list<key_t> escape_key;
  std::string label;
  short base_row, base_column;
 protected:
  short focus_row, focus_column;
  GUI_element(short, short, const std::string&) throw();
  virtual void remove_display() const throw()=0;
  virtual void update_display() const throw()=0;
 public:
  typedef union{
   bool as_boolean;
   int as_integer;
   unsigned int as_unsigned_integer;
   unsigned long as_unsigned_long;
  } value_t;
  void add_escape_key(size_t, ...) throw();
  virtual key_t focus() throw()=0;
  virtual value_t get_value() const throw()=0;
  bool is_escape_key(key_t) const throw();
  void paint() const throw();
  void vanish() const throw();
};
class check_box: public GUI_element{
 private:
  bool state;
  void remove_display() const throw();
  void update_display() const throw();
 public:
  check_box(short, short, const std::string&, bool) throw();
  key_t focus() throw();
  value_t get_value() const throw();
  void set_value(bool b) throw(){state=b;}
};
class element_list: public GUI_element{
 private:
  std::vector<GUI_element*> entry;
  size_t index;
  void remove_display() const throw();
  void update_display() const throw();
 public:
  element_list() throw(): GUI_element(0, 0, ""), index(0){}
  void add_element(GUI_element&) throw();
  void clear() throw();
  key_t focus() throw();
  value_t get_value() const throw();
};
class magnitude_box: public GUI_element{
 private:
  unsigned long length, max;
  void remove_display() const throw();
  void update_display() const throw();
 public:
  magnitude_box(short, short, const std::string&, unsigned long, unsigned long) throw();
  key_t focus() throw();
  value_t get_value() const throw();
};
class number_box: public GUI_element{
 private:
  int max, min, number;
  void remove_display() const throw();
  void update_display() const throw();
 public:
  number_box(short, short, const std::string&, int, int, int) throw();
  key_t focus() throw();
  value_t get_value() const throw();
  void reset_lower_bound(int) throw();
  void reset_upper_bound(int) throw();
};
class select_box: public GUI_element{
 private:
  std::vector<std::string> selection;
  size_t index;
  void remove_display() const throw();
  void update_display() const throw();
 public:
  select_box(short, short, const std::string&, size_t, size_t, const char**) throw();
  key_t focus() throw();
  value_t get_value() const throw();
};
key_t get_input() throw();
void gotoxy(short, short) throw();
#endif
