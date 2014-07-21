#ifndef COMMON_H
#define COMMON_H
#include <map>
#include <string>
#include <vector>
typedef unsigned short key_t;
#define LAYOUT_SEPARATE 1LU
#define LAYOUT_SHOW_METADATA 2LU // Such as title and composer.
#define LAYOUT_SHOW_PART_ID 4LU
#define LAYOUT_DIRTY 0x80000000LU
#define LAYOUT_DEFAULT 0x7fffffffLU
typedef struct{
 unsigned long layout;
 int max_char_screen, max_char_file; // Maximum characters per line.
} BrlSysArg;
#define ORAL_CLEF 0x00000001
#define ORAL_KEY 0x00000002
#define ORAL_TIME 0x00000004
#define ORAL_LEFT_HAND 0x00000008
#define ORAL_RIGHT_HAND 0x00000010
#define ORAL_STAFFWISE 0x00000020
#define ORAL_TIMEWISE 0x00000040
#define ORAL_PITCH 0x00000080
#define ORAL_OCTAVE_EVERYTIME 0x00000100
#define ORAL_OCTAVE_NUM 0x00000200
#define ORAL_OCTAVE_BRL 0x00000400
#define ORAL_OCTAVE_ABS 0x00000600
#define ORAL_OCTAVE_REPRESENTATION_BITS 0x00000600
#define ORAL_DURATION 0x00000800
#define ORAL_CHORD_DIRECTION_FOLLOWING_BRL 0x00001000
#define ORAL_CHORD_DOWNWARD_ONLY 0x00002000
#define ORAL_CHORD_UPWARD_ONLY 0x00003000
#define ORAL_CHORD_DIRECTION_BITS 0x00003000
#define ORAL_SLUR_AND_TIED_ABSOLUTE_POSITION 0x00004000
#define ORAL_MALE 0x40000000
#define ORAL_DIRTY 0x80000000
typedef struct{
 int part, staff, begin, end;
} OralOutArg;
typedef struct{
 unsigned long rate, volume, flags;
} OralSysArg;
typedef struct{
 std::vector<std::vector<bool> > staff_open;
 int begin, end;
 char key_shift;
} PlayOutArg;
#define MAX_PLAYER_VOLUME 10
typedef struct{
 int customer_tempo, original_tempo; // Quarter notes per minute.
 unsigned long volume;
} PlaySysArg;
class page_map{ // Interface for paged plaintext.
 protected:
  std::map<size_t, std::string> page_table;
  void clear() throw(){page_table.clear();}
 public:
  typedef std::map<size_t, std::string>::const_iterator page_table_const_iterator;
  bool is_new_page(size_t m) const throw(){return page_table.count(m)>0;}
  virtual size_t page_begin(page_table_const_iterator) const throw()=0;
  virtual std::string page_number(size_t m) const throw(){return is_new_page(m) ? page_table.find(m)->second : "";}
  page_table_const_iterator page_table_begin() const throw(){return page_table.begin();}
  virtual size_t page_end(page_table_const_iterator) const throw()=0;
  page_table_const_iterator page_table_end() const throw(){return page_table.end();}
  void set_page(size_t m, const std::string& p) throw(){page_table[m]=p;}
};
long gcd(long, long) throw();
#endif
