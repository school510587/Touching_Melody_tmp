/*
    Touching Melody - A Braille Score Assist System
    Copyright (C) 2010 Fingers
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
    NCKU CSIE Scream Lab(成功大學資訊工程學系- 音樂與多媒體實驗室)
    http://screamlab-ncku-2008.blogspot.com/
    hardyyeh00031@gmail.com
*/
#ifndef MUSIC_DATA_H
#define MUSIC_DATA_H
#include <stdexcept>
#define NOTEHEAD_NORMAL 0
#define NOTEHEAD_CIRCLE_X 1
#define NOTEHEAD_SLASH 2
#define NOTEHEAD_X 3
#define NOTEHEAD_ARROW_DOWN 4
#define NOTEHEAD_ARROW_UP 5
#define NOTEHEAD_CLUSTER 6
#define NOTEHEAD_DIAMOND 7
#define NOTEHEAD_INVERTED_TRIANGLE 8
#define NOTEHEAD_SQUARE 9
#define NOTEHEAD_TRIANGLE 10
#define NOTEHEAD_BACK_SLASHED 11
#define NOTEHEAD_CROSS 12
#define NOTEHEAD_SLASHED 13
#define NOTEHEAD_NONE 14
#define notehead_is_normal(h) (h<=NOTEHEAD_SLASH)
#define notehead_is_vertical(h) (h==NOTEHEAD_ARROW_DOWN || h==NOTEHEAD_ARROW_UP)
#define notehead_is_diamond(h) (NOTEHEAD_CLUSTER<=h && h<=NOTEHEAD_TRIANGLE)
#define SLUR_CROSS_STAFF 1 // These three are short-slur types.
#define SLUR_CROSS_VOICE 2
#define SLUR_GENERAL 3
#define SLUR_GRACE 1 // These two are short-slur attributes.
#define SLUR_DISCONTINUOUS 2
#define SLUR_START_LONG 1 // These three are long-slur types.
#define SLUR_STOP_LONG 2
#define SLUR_DISCONTINUOUS_LONG (SLUR_START_LONG|SLUR_STOP_LONG)
#define STEM_NONE 0
#define STEM_DOWN 1
#define STEM_UP 2
#define STEM_DOUBLE (STEM_DOWN|STEM_UP)
#define TYPE_CHANGE 2
#define TYPE_CONTINUE 2
#define TYPE_CRESCENDO_START 0
#define TYPE_CRESCENDO_STOP 1
#define TYPE_DIMINUENDO_START 2
#define TYPE_DIMINUENDO_STOP 3
#define TYPE_DOWN 1
#define TYPE_START 1
#define TYPE_STOP 3
#define TYPE_UP 2
#define TYPE_UNKNOWN 0
class bms_node{
 private:
  int measure, part, staff;
 protected:
  typedef struct{unsigned char dot:4, type:4;} bms_note_type_t;
  virtual bool empty() const throw(){return false;} // No-output valid value.
 public:
  bms_node() throw(): measure(0), part(0), staff(1){}
  virtual ~bms_node() throw(){}
  int increase_measure() throw(){return ++measure;}
  int increase_part() throw(){return ++part;}
  int _measure() const throw(){return measure;}
  int _part() const throw(){return part;}
  void set_measure(int m) throw(std::invalid_argument){measure=m;}
  void set_part(int p) throw(std::invalid_argument){part=p;}
  void set_staff(int s) throw(std::invalid_argument){staff=s;}
  int _staff() const throw(){return staff;}
};
class bms_attributes: public bms_node{
 private:
  union{
   struct{int sign, line, octave_change;} clef_data;
   struct{int cancel, fifths, mode;} key_signature;
   struct{
    union{
     std::pair<int, int>* beat_pair;
     int* beat_number;
    }; // Normal symbols and single-number symbols refer to beat_pair and beat_number respectively.
    size_t count; // Both normal and single-number symbols need a length counter.
    int symbol;
   } time_signature;
  } data;
  int attributes_kind;
  bool attributes_change; // It is true when the element is not first appearance.
 public:
  enum{clef, key, time, unknown}; // Kinds of attributes.
  enum{common, cut, normal, single_number}; // Types of time signatures.
  bms_attributes(const bms_node& p=bms_node()) throw();
  bms_attributes(const bms_attributes& another) throw(){*this=another;}
  virtual ~bms_attributes() throw();
  int beats(size_t) const throw(std::invalid_argument);
  size_t beat_pair_count() const throw(){return data.time_signature.count;}
  int beat_type(size_t) const throw(std::invalid_argument);
  int cancel() const throw(){return kind()==key ? data.key_signature.cancel : 0;}
  bool change() const throw(){return attributes_change;}
  int clef_octave_change() const throw(){return kind()==clef ? data.clef_data.octave_change : 0;}
  bool empty() const throw(){return kind()==unknown;}
  int fifths() const throw(){return kind()==key ? data.key_signature.fifths : 0;}
  void initialize_clef() throw();
  void initialize_key() throw();
  void initialize_time() throw();
  int kind() const throw(){return attributes_kind;}
  int line() const throw(){return kind()==clef ? data.clef_data.line : 0;}
  int mode() const throw(){return kind()==key ? data.key_signature.mode : 0;}
  bms_attributes& operator=(const bms_attributes&) throw();
  bool operator<(const bms_attributes&) const throw();
  void set_beats(int) throw(std::invalid_argument);
  void set_beat_type(int) throw(std::invalid_argument);
  void set_cancel(int) throw(std::invalid_argument);
  void set_change(bool b) throw(){attributes_change=b;}
  void set_clef_octave_change(int) throw(std::invalid_argument);
  void set_fifths(int) throw(std::invalid_argument);
  void set_line(int) throw(std::invalid_argument);
  void set_mode(int) throw(std::invalid_argument);
  void set_sign(int) throw(std::invalid_argument);
  void set_symbol(int) throw(std::invalid_argument);
  int sign() const throw(){return kind()==clef ? data.clef_data.sign : 0;}
  int symbol() const throw(){return kind()==time ? data.time_signature.symbol : 0;}
};
class bms_barline: public bms_node{
 private:
  struct{std::string number, text;} ending;
  int style;
  unsigned char notation_flag; // Segno, coda, fermata, repeat mark.
 public:
  bms_barline(const bms_node& p=bms_node()) throw(): bms_node(p), style(0), notation_flag('\0'){}
  virtual ~bms_barline() throw(){}
  int bar_style() const throw(){return style;}
  bool empty() const throw(){return false;}
  std::string ending_number() const throw(){return ending.number;}
  std::string ending_text() const throw(){return ending.text;}
  unsigned char notations() const throw(){return notation_flag;}
  void set_bar_style(int) throw(std::invalid_argument);
  void set_ending_number(const std::string&) throw(std::invalid_argument);
  void set_ending_text(const std::string&) throw(std::invalid_argument);
  void set_notations(unsigned char) throw(std::invalid_argument);
};
class bms_direction: public bms_node{
 private:
  typedef struct{ // Metronomes.
   bms_note_type_t beat_unit; // The l-value.
   union{ // The r-value.
    bms_note_type_t r_unit;
    char per_minute[14];
   };
   bool parentheses;
  } bms_metronome_t;
  struct{size_t above:1, directive:1; int type:30;} attribute;
  union{
   struct{size_t number:30, type:2;} dashes_type;
   bms_metronome_t* metronome_type;
   struct{
    int size:27;
    size_t number:3, type:2;
   } octave_shift_type;
   struct{size_t depth:29, line:1, type:2;} pedal_type;
   struct{size_t number:3, spread:27, type:2;} wedge_type;
   std::string* words_type;
  };
 public:
  enum{dashes, metronome, pedal, octave_shift, wedge, words, unknown};
  bms_direction(const bms_node& =bms_node()) throw();
  virtual ~bms_direction() throw(){clear();}
  bms_direction(const bms_direction&) throw();
  bool above() const throw();
  void clear(int=unknown) throw();
  int diatonic_step_change() const throw(std::invalid_argument);
  int direction_kind() const throw();
  bool empty() const throw(){return direction_kind()==unknown;}
  void increase_beat_unit_dot() throw(std::invalid_argument);
  bool is_directive() const throw();
  bool is_half_pedaling() const throw(std::invalid_argument);
  bool is_stop() const throw();
  bool is_text() const throw(); // It has a 3-4-5-point prefix.
  bool is_trivial() const throw(); // It has trivial braille notations.
  bool long_text() const throw(std::invalid_argument);
  size_t number() const throw(); // The number attribute (number-level).
  int octave_change() const throw(std::invalid_argument);
  bms_direction& operator=(const bms_direction&) throw();
  size_t pedal_depth() const throw(std::invalid_argument);
  void set_above(bool) throw();
  void set_beat_unit(unsigned char, bool=false) throw(std::invalid_argument);
  void set_dashes_number(size_t) throw(std::invalid_argument);
  void set_dashes_type(size_t) throw(std::invalid_argument);
  void set_directive(bool) throw();
  void set_line(bool) throw();
  void set_parentheses(bool) throw();
  void set_pedal_depth(size_t) throw(std::invalid_argument);
  void set_pedal_type(size_t) throw(std::invalid_argument);
  void set_per_minute(const std::string&) throw();
  void set_shift_size(int) throw(std::invalid_argument);
  void set_shift_number(size_t) throw(std::invalid_argument);
  void set_shift_type(size_t) throw(std::invalid_argument);
  void set_text(const std::string&, bool=false) throw();
  void set_wedge_number(size_t) throw(std::invalid_argument);
  void set_wedge_type(size_t) throw(std::invalid_argument);
  std::string text() const throw(std::invalid_argument);
  size_t type() const throw(); // The type attribute (not direction-type).
};
class bms_note: public bms_node{
 private:
  typedef struct{
   unsigned char number[3], attribute_flag:2, long_type:2, short_left:2, short_right:2;
  } slur_t;
  typedef struct{
   short type;
   unsigned char attribute_flag;
  } tied_t;
  typedef struct{
   const bms_note* partner; // The tremolo partner (may be itself).
   int value; // 3 represents a 32nd tremolo.
  } tremolo_t;
  typedef struct{
   int kind, number, order;
  } arpeggiate_t; // Number 0 stands for no arpeggiate.
  struct{
   unsigned char instrument, dynamics:7, dynamics_set:1, end_dynamics:7, end_dynamics_set:1;
   struct{unsigned char pizzicato:1, up_bow:1, down_bow:1, harmonic:3, other:2;} string_technical;
   char chord; // Positive value for main notes, negative for chord notes.
   bool rest, grace, printed;
  } attribute;
  struct{int step, alter, octave;} pitch;
  struct{
   long attack:31, release:31;
   unsigned long attack_set:1, release_set:1;
   union{ // Duration block: Meaning of memory relies on `grace' boolean variable.
    struct{ // A normal note keeps its duration and number of relative grace notes.
     unsigned long prefix:3, suffix:3;
     long duration:26;
    };
    struct{unsigned long slash:1, steal_time_previous:7, steal_time_following:7, make_time:17;};
   }; // A grace note keeps its slash and relationship to the nearest normal note.
  } time;
  struct{
   bms_note* link[3]; // [0]: chord link; the other depends on note attributes.
   int accidental;
   union{
    unsigned char number[2];
    unsigned short state;
   } voice;
   struct{
    unsigned char head:4, parentheses:1, set_stem:1, stem:2;
   } shape;
   bms_note_type_t note_type;
  } appearance;
  struct{
   struct{unsigned long actual_note_number:15, normal_note_number:15, type:2;} tuplet[6];
   int fermata;
   unsigned char fingering[4];
   slur_t slur;
   tied_t tied;
   tremolo_t tremolo;
   arpeggiate_t arpeggiate;
   unsigned short articulation_flag;
   struct{
    unsigned long kind:4, start_note:2, trill_step:2, two_note_turn:2, accelerate:1, beats:7, second_beat:7, last_beat:7;
    unsigned char accidental_mark_above:4, accidental_mark_below:4;
   } trill;
   int syllabic;
   std::string lyric;
  } notations;
 public:
  typedef enum{longa, breve, whole, half, quarter, eighth, _16th, _32nd, _64th, _128th, _256th} note_type_t;
  enum{
   note_accent=1, note_strong_accent=2, note_staccato=4, note_tenuto=8, note_detached_legato=16, 
   note_staccatissimo=32, note_spiccato=64, note_scoop=128, note_plop=256, note_doit=512, 
   note_falloff=1024, note_breath_mark=2048, note_caesura=4096, note_stress=8192, note_unstress=16384
  };
  static unsigned char normalize_dynamics(int d) throw(){return d>127 ? 127 : d<0 ? 0 : d;}
  bms_note(const bms_node& =bms_node()) throw();
  virtual ~bms_note() throw(){}
  int accidental() const throw(){return appearance.accidental;}
  unsigned char accidental_mark_above() const throw(){return notations.trill.accidental_mark_above;}
  unsigned char accidental_mark_below() const throw(){return notations.trill.accidental_mark_below;}
  int alter() const throw(){return pitch.alter;}
  int arpeggiate_kind() const throw(){return notations.arpeggiate.kind;}
  int arpeggiate_number() const throw(){return notations.arpeggiate.number;}
  int arpeggiate_order() const throw(){return notations.arpeggiate.order;}
  unsigned short articulation_flag() const throw(){return notations.articulation_flag;}
  long attack() const throw(){return time.attack;}
  bool attack_set() const throw(){return time.attack_set==1;}
  bms_note* center_note() throw();
  const bms_note* center_note() const throw();
  char chord() const throw(){return attribute.chord;}
  unsigned long long chord_common_block() const throw();
  bool contains_tied_start() const throw(){return tied_type()==TYPE_START || tied_type()==TYPE_CONTINUE;}
  int diatonic_pitch() const throw(std::invalid_argument);
  unsigned char dot() const throw(){return appearance.note_type.dot;}
  bool down_bow() const throw(){return attribute.string_technical.down_bow==1;}
  long duration() const throw(){return grace() ? 0 : time.duration;}
  unsigned char dynamics() const throw(){return attribute.dynamics;}
  bool empty() const throw(){return false;}
  unsigned char end_dynamics() const throw(){return attribute.end_dynamics;}
  int fermata() const throw(){return notations.fermata;}
  unsigned char fingering(size_t) const throw(std::invalid_argument);
  bool grace() const throw(){return attribute.grace;}
  bool grace_not_set() const throw(std::invalid_argument);
  bms_note* group_begin() throw();
  const bms_note* group_begin() const throw();
  bms_note* group_next() throw();
  const bms_note* group_next() const throw();
  const void* group_id() const throw();
  void increase_chord() throw(){attribute.chord++;}
  void increase_dot() throw(){appearance.note_type.dot++;}
  void increase_prefix() throw(){time.prefix++;}
  void increase_slur_number(int t) throw(){notations.slur.number[t%3]++;}
  void increase_suffix() throw(){time.suffix++;}
  unsigned char instrument() const throw(){return attribute.instrument;}
  bool is_breve_long() const throw(); // A breve or longa note.
  bool is_long_grace() const throw(); // Longer-than-eighth types.
  bool is_long_note() const throw(){return type()==eighth || is_long_grace();}
  unsigned char long_slur_type() const throw(){return notations.slur.long_type;}
  std::string lyric() const throw(){return notations.lyric;}
  unsigned long make_time() const throw(){return grace() ? time.make_time : 0;}
  int midi_pitch() const throw(std::invalid_argument);
  int midi_step() const throw(std::invalid_argument);
  bms_note* next_chord() throw(){return appearance.link[0];}
  const bms_note* next_chord() const throw(){return appearance.link[0];}
  bms_note* next_prefix_grace() throw(){return appearance.link[1];}
  const bms_note* next_prefix_grace() const throw(){return appearance.link[1];}
  bms_note* next_suffix_grace() throw(){return appearance.link[2];}
  const bms_note* next_suffix_grace() const throw(){return appearance.link[2];}
  unsigned char notehead() const throw(std::invalid_argument);  int number() const throw(){return appearance.shape.head;}
  int octave() const throw(){return pitch.octave;}
  bool operator<(const bms_note&) const throw(std::invalid_argument);
  bool parentheses() const throw(){return appearance.shape.parentheses==1;}
  unsigned long prefix() const throw(){return grace() ? 0 : time.prefix;}
  bool printed() const throw(){return attribute.printed;}
  long release() const throw(){return time.release;}
  bool release_set() const throw(){return time.release_set==1;}
  bool regular() const throw(){return chord()>=0 && !grace();}
  bool rest() const throw(){return attribute.rest;}
  void set_accidental(int) throw(std::invalid_argument);
  void set_accidental_mark_above(unsigned char) throw(std::invalid_argument);
  void set_accidental_mark_below(unsigned char) throw(std::invalid_argument);
  void set_alter(int) throw(std::invalid_argument);
  void set_arpeggiate_kind(int, bool=false) throw(std::invalid_argument);
  void set_arpeggiate_number(int) throw(std::invalid_argument);
  void set_arpeggiate_order(int) throw(std::invalid_argument);
  void set_articulation_flag(unsigned short, bool=false) throw(std::invalid_argument);
  void set_attack(long) throw(std::invalid_argument);
  void set_center_note(bms_note*) throw(std::invalid_argument);
  void set_chord(char) throw(std::invalid_argument);
  void set_down_bow(bool) throw();
  void set_duration(long) throw(std::invalid_argument);
  void set_dynamics(unsigned char, bool=false) throw(std::invalid_argument);
  void set_end_dynamics(unsigned char, bool=false) throw(std::invalid_argument);
  void set_fermata(int) throw(std::invalid_argument);
  void set_fingering(size_t, unsigned char) throw(std::invalid_argument);
  void set_grace(bool) throw(std::invalid_argument);
  void set_grace_link(bms_note*, bms_note*) throw(std::invalid_argument);
  void set_instrument(unsigned char) throw(std::invalid_argument);
  void set_long_slur_type(unsigned char, bool=true) throw(std::invalid_argument);
  void set_lyric(const std::string&) throw(std::invalid_argument);
  void set_make_time(unsigned long) throw(std::invalid_argument);
  void set_next_chord(bms_note*) throw(std::invalid_argument);
  void set_next_prefix_grace(bms_note*) throw(std::invalid_argument);
  void set_next_suffix_grace(bms_note*) throw(std::invalid_argument);
  void set_notehead(unsigned char) throw(std::invalid_argument);
  void set_octave(int) throw(std::invalid_argument);
  void set_parentheses(bool) throw();
  void set_pizzicato(bool) throw();
  void set_prefix(unsigned long) throw(std::invalid_argument);
  void set_printed(bool) throw(std::invalid_argument);
  void set_release(long) throw(std::invalid_argument);
  void set_rest(bool) throw(std::invalid_argument);
  void set_short_slur_attribute(unsigned char, bool=false) throw(std::invalid_argument);
  void set_short_slur_type(unsigned char, bool=false) throw(std::invalid_argument);
  void set_slash(bool) throw(std::invalid_argument);
  void set_steal_time_following(unsigned long) throw(std::invalid_argument);
  void set_steal_time_previous(unsigned long) throw(std::invalid_argument);
  void set_stem(unsigned char, bool=false) throw(std::invalid_argument);
  void set_step(int) throw(std::invalid_argument);
  void set_suffix(unsigned long) throw(std::invalid_argument);
  void set_syllabic(int) throw(std::invalid_argument);
  void set_tied_type(int, bool=false) throw(std::invalid_argument);
  void set_trill_accelerate(bool) throw(std::invalid_argument);
  void set_trill_beats(unsigned long) throw(std::invalid_argument);
  void set_trill_kind(unsigned long) throw(std::invalid_argument);
  void set_trill_last_beat(unsigned long) throw(std::invalid_argument);
  void set_trill_second_beat(unsigned long) throw(std::invalid_argument);
  void set_trill_start_note(unsigned long) throw(std::invalid_argument);
  void set_trill_step(unsigned long) throw(std::invalid_argument);
  void set_trill_two_note_turn(unsigned long) throw(std::invalid_argument);
  void set_tuplet_number(size_t, unsigned long, bool) throw(std::invalid_argument);
  void set_tuplet_type(size_t, unsigned char) throw(std::invalid_argument);
  void set_type(note_type_t) throw(std::invalid_argument);
  void set_up_bow(bool) throw();
  void set_voice(int) throw(std::invalid_argument);
  void set_voice(size_t, unsigned char) throw(std::invalid_argument);
  unsigned char short_slur_attribute() const throw(){return notations.slur.attribute_flag;}
  unsigned char short_slur_type(bool=false) const throw(std::invalid_argument);
  int shape() const throw(std::invalid_argument);
  bool slash() const throw(){return time.slash==1;}
  unsigned char slur_number(int) const throw(std::invalid_argument);
  unsigned long steal_time_following() const throw(){return grace() ? time.steal_time_following : 0;}
  unsigned long steal_time_previous() const throw(){return grace() ? time.steal_time_previous : 0;}
  unsigned char stem() const throw(){return rest() ? STEM_NONE : appearance.shape.stem;}
  int step() const throw(){return pitch.step;}
  unsigned long suffix() const throw(){return grace() ? 0 : time.suffix;}
  int syllabic() const throw(){return notations.syllabic;}
  int tied_type() const throw(){return notations.tied.type;}
  bool trill_accelerate() const throw(){return notations.trill.accelerate==1;}
  unsigned long trill_beats() const throw(){return notations.trill.beats;}
  unsigned long trill_kind() const throw(){return notations.trill.kind;}
  unsigned long trill_last_beat() const throw(){return notations.trill.last_beat;}
  unsigned long trill_second_beat() const throw(){return notations.trill.second_beat;}
  unsigned long trill_start_note() const throw(){return notations.trill.start_note;}
  unsigned long trill_step() const throw(){return notations.trill.trill_step;}
  unsigned long trill_two_note_turn() const throw(){return notations.trill.two_note_turn;}
  unsigned long tuplet_number(size_t, bool) const throw(std::invalid_argument);
  unsigned long tuplet_show_number(size_t i) const throw(std::invalid_argument){return tuplet_number(i, false);}
  unsigned char tuplet_type(size_t) const throw(std::invalid_argument);
  note_type_t type() const throw(){return (note_type_t)appearance.note_type.type;}
  bool up_bow() const throw(){return attribute.string_technical.up_bow==1;}
  unsigned short voice() const throw(){return appearance.voice.state;}
  unsigned char voice(size_t) const throw(std::invalid_argument);
  unsigned short voice_abs() const throw(std::invalid_argument);
};
#endif
