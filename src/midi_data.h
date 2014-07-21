#ifndef MIDI_DATA_H
#define MIDI_DATA_H
#include <list>
#include <map>
#include <vector>
#include "music_data.h"
#define ALL_CONTROLLERS_OFF(channel) (0x000079b0|channel)
#define ALL_NOTES_OFF(channel) (0x00007bb0|channel)
#define SKILL_INVERTED_MORDENT 2
#define SKILL_MORDENT 1
#define SKILL_TRILL 0
#define TRILL_START_BELOW 1
#define TRILL_START_MAIN 2
#define TRILL_START_UPPER 3
#define TRILL_STEP_HALF 1
#define TRILL_STEP_WHOLE 2
#define TRILL_STEP_UNISON 3
#define TRILL_2_TURN_HALF 1
#define TRILL_2_TURN_NONE 0
#define TRILL_2_TURN_WHOLE 2
#define TRILL_ACCELERATE_NO 0
#define TRILL_ACCELERATE_YES 1
class midi_command_t;
class MidiCommandVector;
class MidiStream;
typedef struct{
 unsigned char channel, pan, volume;
 union{unsigned char program, unpitched;};
} midi_instrument_t;
typedef std::map<size_t, midi_instrument_t> midi_instrument_map_t;
typedef struct{
 union{
  unsigned long command_word;
  unsigned char command_byte[4];
 };
 int part, staff;
} midi_message_t;
typedef struct{
 unsigned char start_note:2, trill_step:2, two_note_turn:2, accelerate:1;
 unsigned char beats, second_beat, last_beat, kind;
} trill_sound_t;
typedef struct{
 trill_sound_t trill;
 long duration; // In divisions.
 int part, measure, staff;
 unsigned char channel, main_pitch, intensity, kind;
} sound_entity_t;
class midi_command_t{
 public:
  std::list<midi_message_t> message;
  long time_point, m_start, m_stop;
  midi_command_t() throw(): time_point(0), m_start(0), m_stop(0){}
  virtual ~midi_command_t() throw(){}
};
class MidiCommandVector: public std::vector<midi_command_t>{
 friend MidiCommandVector add_skills(const MidiStream&, int) throw();
 private:
  std::vector<std::pair<long, int> > tempo_map;
  long divisions, factor;
  void add_pitch(midi_message_t&, long, long, int, int) throw();
  void adjust(long, long) throw();
  int tempo(long) const throw();
 public:
  typedef std::vector<midi_command_t>::iterator iterator;
  typedef std::vector<std::pair<long, int> >::iterator tempo_iterator;
  MidiCommandVector() throw(): divisions(1), factor(1){}
  virtual ~MidiCommandVector() throw(){}
  void add_boundary(long) throw();
  void add_controller(unsigned char, unsigned char, unsigned char, long) throw();
  void add_note(const bms_note&, const midi_instrument_map_t&, long) throw();
  void add_skill(const sound_entity_t&, long) throw();
  size_t find_position_of(long) throw();
  long get_divisions() const throw(){return divisions*factor;}
  void set_divisions(long d) throw(){if(d>0) divisions=d;}
  void set_tempo(long, int) throw();
  tempo_iterator tempo_begin() throw(){return tempo_map.begin();}
  tempo_iterator tempo_end() throw(){return tempo_map.end();}
};
class MidiStream: public std::vector<midi_command_t>{
 friend MidiCommandVector add_skills(const MidiStream&, int) throw();
 private:
  static const float default_ffffff, default_fffff, default_ffff, default_fff, default_ff, default_f, default_mf, default_mp, default_p, default_pp, default_ppp, default_pppp, default_ppppp, default_pppppp;
  std::vector<std::pair<long, int> > tempo_map;
  long common_divisions;
 public:
  static const int default_tempo;
  MidiStream(): common_divisions(1){}
  virtual ~MidiStream() throw(){}
  void clear() throw();
  long get_divisions() const throw(){return common_divisions;}
  void set_divisions(long divisions) throw(){if(divisions>0) common_divisions=divisions;}
  void set_tempo(long, int) throw();
  int tempo(long) const throw();
};
#endif
