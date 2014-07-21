#ifndef SCORE_H
#define SCORE_H
#include <ostream>
#include "common.h"
#include "midi_data.h"
#include "xmlreader.h"
template<class content>
class time_slot: public std::vector<std::vector<content> >{
 private:
  long delta_time;
 public:
  typedef class std::vector<content>::const_iterator const_row_iterator;
  typedef class std::vector<content>::iterator row_iterator;
  time_slot() throw(): delta_time(0){}
  explicit time_slot(long t) throw(): delta_time(t){}
  long duration() const throw(){return delta_time;}
};
class bms_measure_element; // Declaration for type definitions.
typedef time_slot<bms_measure_element> bms_in_accord_trait;
class bms_note_area: public time_slot<bms_in_accord_trait>{
 public:
  explicit bms_note_area(long duration): time_slot<bms_in_accord_trait>(duration){}
};
class bms_measure_element{
 friend class bms_part;
 friend class bms_score;
 private:
  enum{a_i, a_p, b_i, b_p, d_i, d_p, n_i, n_p, v_i, v_p, unknown} type;
  union{
   union{
    std::vector<bms_attributes>::size_type attributes;
    std::vector<bms_barline>::size_type barline;
    std::vector<bms_direction>::size_type direction;
    std::vector<bms_note>::size_type note;
    std::vector<bms_note_area>::size_type voice;
   } index;
   union{
    bms_attributes* attributes;
    bms_barline* barline;
    bms_direction* direction;
    bms_note* note;
    bms_note_area* voice;
   } pointer;
  };
 public:
  bms_measure_element() throw(): type(unknown){}
  virtual ~bms_measure_element() throw(){}
  bms_note* as_note() const throw(){return type==n_p ? pointer.note : NULL;}
  bool become_pointer_mode(std::vector<bms_attributes>&) throw();
  bool become_pointer_mode(std::vector<bms_barline>&) throw();
  bool become_pointer_mode(std::vector<bms_note_area>&) throw();
  std::vector<bms_attributes>::size_type index2attributes() const throw(){return index.attributes;}
  std::vector<bms_barline>::size_type index2barline() const throw(){return index.barline;}
  std::vector<bms_direction>::size_type index2direction() const throw(){return index.direction;}
  std::vector<bms_note_area>::size_type index2voice() const throw(){return index.voice;}
  bool is_attributes_index() const throw(){return type==a_i;}
  bool is_barline_index() const throw(){return type==b_i;}
  bool is_direction_index() const throw(){return type==d_i;}
  bool is_note_index() const throw(){return type==n_i;}
  bool is_note_pointer() const throw(){return type==n_p;}
  bool is_voice_index() const throw(){return type==v_i;}
  bms_measure_element operator+(int) const throw();
  bms_measure_element operator-(int) const throw();
  bms_measure_element& operator+=(int) throw();
  void set_attributes_index(std::vector<bms_attributes>::size_type) throw();
  void set_barline_index(std::vector<bms_barline>::size_type) throw();
  void set_direction_index(std::vector<bms_direction>::size_type) throw();
  void set_note_index(std::vector<bms_note>::size_type) throw();
  void set_note(bms_note*) throw();
  void set_voice_index(std::vector<bms_note_area>::size_type) throw();
};
typedef std::vector<bms_measure_element> bms_measure;
class bms_staff: public std::vector<bms_measure>{
 public:
  bool upmost_pitch;
  bms_staff() throw(): upmost_pitch(false){}
  virtual ~bms_staff() throw(){}
};
class bms_part: public std::vector<bms_staff>{
 friend class voice_analyzer;
 friend class XMLAnalyzer;
 private:
  struct{
   std::vector<bms_direction> direction;
   std::vector<bms_note*> note;
  } element;
  std::vector<bms_note_area> voice;
  std::string abbreviation, id, name;
  bool song_music;
 public:
  static const int multiple_voices=4, no_whole_measure_rest=0, whole_measure_rest_start=2, whole_measure_rest_stop=1;
  typedef std::vector<bms_note*>::const_iterator const_note_iterator;
  typedef std::vector<bms_note*>::iterator note_iterator;
  bms_part(const std::string& i="") throw(): id(i), song_music(false){}
  virtual ~bms_part() throw(){}
  void clear() throw();
  const bms_direction& direction_at(const bms_measure_element& i) const throw(){return element.direction[i.index.barline];}
  const_note_iterator find_note(const bms_note*) const throw();
  bool found(const bms_staff::iterator&) throw();
  bool found(const bms_staff::const_iterator&) const throw();
  std::string get_id() const throw(){return id;}
  bool get_song_music() const throw(){return song_music;}
  const bms_note_area& note_area_at(const bms_measure_element& i) const throw(){return voice[i.index.voice];}
  const_note_iterator note_begin() const throw(){return element.note.begin();}
  note_iterator note_begin() throw(){return element.note.begin();}
  const_note_iterator note_end() const throw(){return element.note.end();}
  note_iterator note_end() throw(){return element.note.end();}
  std::string part_abbreviation() const throw(){return abbreviation;}
  std::string part_name() const throw(){return name;}
  void set_abbreviation(const std::string&) throw();
  void set_id(const std::string& n_id) throw(){id=n_id;}
  void set_song_music() throw(){song_music=true;}
  void set_name(const std::string& n) throw(){name=n;}
  bms_staff& staff(size_t index) throw(){return at(index-1);}
  const bms_staff& staff(size_t index) const throw(){return at(index-1);}
  void unset_song_music() throw(){song_music=false;}
  int measure_attribute(const bms_staff::const_iterator&) const throw();
};
class bms_score: public page_map{
 friend class XMLAnalyzer;
 friend class ScorePlayer;
 private:
  typedef struct{
   std::string abbreviation, name;
   midi_instrument_t midi;
  } instrument_t;
  std::vector<instrument_t> instrument;
  struct{
   std::vector<bms_attributes> attributes;
   std::vector<bms_barline> barline;
  } element;
  MusicXML2::SXMLFile source;
  std::string work_number, work_title, composer, lyricist, arranger;
  std::vector<bms_part> score;
  MidiStream midi_stream;
  int measure_alignment; // Measure-number of index-0 measure.
 public:
  typedef std::vector<bms_part>::const_iterator const_iterator;
  typedef std::vector<bms_part>::iterator iterator;
  bms_score() throw(): measure_alignment(1){}
  virtual ~bms_score() throw(){clear();}
  size_t add_instrument() throw();
  const_iterator begin() const throw(){return score.begin();}
  iterator begin() throw(){return score.begin();}
  const_iterator end() const throw(){return score.end();}
  void clear() throw();
  bool empty() const throw(){return score.empty();}
  iterator end() throw(){return score.end();}
  std::string get_arranger() const throw(){return arranger;}
  const bms_attributes& get_attributes(const bms_measure_element& i) const throw(){return element.attributes[i.index.attributes];}
  const bms_barline& get_barline(const bms_measure_element& i) const throw(){return element.barline[i.index.barline];}
  std::string get_composer() const throw(){return composer;}
  std::string get_lyricist() const throw(){return lyricist;}
  int getMeasureNum() const throw(){return score[0][0].size();}
  int getPartNum() const throw(){return score.size();}
  int getStaffNum(int p) const throw(){return part(p).size();}
  int get_tempo(long point) const throw(){return midi_stream.tempo(point);}
  std::string get_work_number() const throw(){return work_number;}
  std::string get_work_title() const throw(){return work_title;}
  std::string instrument_abbreviation(size_t index) const throw(){return instrument[index].abbreviation;}
  size_t instrument_count() const throw(){return instrument.size();}
  std::string instrument_name(size_t index) const throw(){return instrument[index].name;}
  size_t page_begin(page_table_const_iterator) const throw();
  size_t page_end(page_table_const_iterator) const throw();
  const midi_instrument_t& midi_instrument(size_t index) const throw(){return instrument[index].midi;}
  midi_instrument_map_t midi_instrument_map() const throw();
  void outputXML(std::ostream&);
  bms_part& part(size_t index) throw(){return score[index-1];}
  const bms_part& part(size_t index) const throw(){return score[index-1];}
  bms_part& part(const OralOutArg& range) throw(){return part(range.part);}
  const bms_part& part(const OralOutArg& range) const throw(){return part(range.part);}
  void set_arranger(const std::string&, bool=true) throw();
  void set_composer(const std::string&, bool=true) throw();
  void set_lyricist(const std::string&, bool=true) throw();
  void set_work_number(const std::string& n) throw(){work_number=n;}
  void set_work_title(const std::string& t) throw(){work_title=t;}
  bms_staff& staff(const OralOutArg& range) throw(){return part(range.part).staff(range.staff);}
  const bms_staff& staff(const OralOutArg& range) const throw(){return part(range.part).staff(range.staff);}
};
bool Colonge_key(const bms_note&, const bms_note&) throw(std::invalid_argument);
#endif
