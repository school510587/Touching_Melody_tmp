#ifndef BRAILLE_OUTPUT_H
#define BRAILLE_OUTPUT_H
#include "score.h"
class brl_token: public std::string{
 private:
  union{
   void* address;
  };
  int type;
  short position[2]; // It represents {row, column}.
 public:
  typedef enum{
   accent, 
   accidental, 
   arpeggiate, 
   arranger, 
   barline, 
   blank_cell, 
   chord, 
   clef, 
   coda, 
   composer, 
   dashes, 
   detached_legato, 
   delayed_turn, 
   dot, 
   down_bow, 
   ending, 
   fermata, 
   fingering, 
   grace, 
   harmonic, 
   harmony, 
   in_accord, 
   inverted_mordent, 
   inverted_turn, 
   key, 
   lyric, 
   lyric_prefix, 
   lyricist, 
   measure_continuity, 
   metronome, 
   mordent, 
   music_prefix, 
   note, 
   notehead, 
   octave, 
   octave_shift, 
   parentheses, 
   pedal, 
   repeat, 
   rest, 
   segno, 
   spiccato, 
   slur, 
   staccato, 
   staccatissimo, 
   strong_accent, 
   tenuto, 
   tied, 
   time, 
   title, 
   trill_mark, 
   tuplet, 
   turn, 
   up_bow, 
   wedge, 
   whole_rests, 
   words
  } type_t;
  brl_token() throw(): std::string(), address(NULL), type(0){}
  brl_token(const std::string& s) throw(): std::string(s), address(NULL), type(0){}
  brl_token(const std::string& s, type_t t) throw(): std::string(s), address(NULL), type(t){}
  virtual ~brl_token() throw(){}
  void add_dot() throw(){*this+='\'';}
  void add_blank() throw(){*this+=' ';}
  short column() const throw(){return position[1];}
  bool end_blank() const throw(){return !empty() && at(length()-1)==' ';}
  bool followed_blank() const throw(){return !empty() &&(type==key || type==time);}
  bool followed_dot(type_t) const throw();
  short row() const throw(){return position[0];}
  void set_position(int, int) throw();
};
class brl_measure: public std::vector<brl_token>{
 private:
  size_t measure_number, page_number;
  short position[2]; // It represents {row, column}.
 public:
  brl_measure(size_t n) throw(): measure_number(n){position[0]=position[1]=0;}
  const char* c_str() const throw();
  short column() const throw(){return position[1];}
  char last_char() const throw(){return back()[back().length()-1];}
  size_t length() const throw();
  size_t number() const throw(){return measure_number;}
  size_t page() const throw(){return page_number;}
  void push_back(char c) throw(){back()+=c;}
  void push_back(const brl_token& s) throw(){std::vector<brl_token>::push_back(s);}
  short row() const throw(){return position[0];}
  void set_measure_number(size_t n) throw(){measure_number=n;}
  void set_page_number(size_t n) throw(){page_number=n;}
  void set_position(int, int) throw();
};
class brl_staff{
 private:
  std::vector<brl_measure> data;
public:
  std::vector<std::vector<brl_measure>::size_type> page_bound;
  std::string name; // Name of the part which the staff belongs to.
  int part, staff;  // Metadata.
 public:
  typedef std::vector<brl_measure>::const_iterator const_iterator;
  typedef std::vector<brl_measure>::iterator iterator;
  typedef std::vector<brl_measure>::size_type size_type;
  brl_staff(std::string n, int p, int s) throw(): name(n), part(p), staff(s){}
  virtual ~brl_staff() throw(){}
  bool add(char, brl_token::type_t, bool=false) throw();
  bool add(const std::string&, brl_token::type_t, bool=false) throw();
  void add_page_bound() throw(){if(size()>0 &&(page_bound.empty() || page_bound.back()!=size()-1)) page_bound.push_back(size()-1);}
  const brl_measure& at(size_type i) const throw(){return data.at(i);}
  brl_measure& at(size_type i) throw(){return data.at(i);}
  const brl_measure& back() const throw(){return data.back();}
  brl_measure& back() throw(){return data.back();}
  const_iterator begin() const throw(){return data.begin();}
  iterator begin() throw(){return data.begin();}
  void clear() throw();
  bool empty() const throw(){return data.empty();}
  const_iterator end() const throw(){return data.end();}
  iterator end() throw(){return data.end();}
  std::string label() const throw();
  brl_staff& operator<<(const brl_measure&) throw();
  const brl_measure& operator[](size_type i) const throw(){return data[i];}
  brl_measure& operator[](size_type i) throw(){return data[i];}
  size_type page_begin(size_t i) const throw(){return i<page_bound.size() ? page_bound[i] : size();}
  size_type page_end(size_t i) const throw(){return page_begin(i+1);}
  size_t part_index() const throw(){return part-1;}
  size_type size() const throw(){return data.size();}
};
class brl_output{
 private:
  std::vector<std::pair<std::string, std::string> > part_table;
  std::vector<brl_staff> score;
  std::string work_data[5];
 public:
  typedef std::vector<brl_staff>::const_iterator const_iterator;
  typedef std::vector<brl_staff>::iterator iterator;
  brl_output() throw(){}
  virtual ~brl_output() throw(){}
  void add_new_staff(std::string, int, int) throw();
  std::string arranger() const throw(){return work_data[4];}
  const brl_measure& at(int i, int j) const throw(){return score[i][j];}
  brl_measure& at(int i, int j) throw(){return score[i][j];}
  const brl_staff& back() const throw(){return score.back();}
  brl_staff& back() throw(){return score.back();}
  const_iterator begin() const throw(){return score.begin();}
  iterator begin() throw(){return score.begin();}
  void clear() throw();
  std::string composer() const throw(){return work_data[2];}
  bool empty() const throw(){return score.empty();}
  const_iterator end() const throw(){return score.end();}
  iterator end() throw(){return score.end();}
  std::string lyricist() const throw(){return work_data[3];}
  size_t measure() const throw(){return score.empty() ? 0 : score.front().size();}
  brl_staff& operator[](size_t i) throw(){return score[i];}
  const brl_staff& operator[](size_t i) const throw(){return score[i];}
  std::string part(const const_iterator& p) const throw(){return part_table[p->part_index()].second;}
  void reset_work(const bms_score&) throw();
  void set_part_pair(const std::string&, const std::string&) throw();
  size_t size() const throw(){return score.size();}
  size_t size(size_t i) const throw(){return i<size() ? score[i].size() : 0;}
  std::string work_number() const throw(){return work_data[0];}
  std::string work_title() const throw(){return work_data[1];}
};
#endif
