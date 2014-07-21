#ifndef BRAILLE_OUTPUTER_H
#define BRAILLE_OUTPUTER_H
#include "front.h"
class BrailleOutputer{
 private:
  typedef enum{normal_note, octave_chord, simple_chord} note_body_type_t;
  const bms_note* last_note;
  size_t dashes_number;
  int global_octave_shift, extra_octave_shift;
  bool octave_shift_switch;
  void clear() throw();
  void output_attributes(brl_staff&, const bms_attributes&, bool) throw();
  void output_barline(brl_staff&, const bms_barline&) throw();
  std::string output_clef(const bms_attributes&, bool) throw();
  void output_direction(brl_staff&, const bms_direction&, const bms_direction*) throw();
  std::string output_key_signature(const bms_attributes&) throw();
  void output_note(brl_staff&, const bms_note&) throw();
  void output_note_body(brl_staff&, const bms_note&, const bms_note*, note_body_type_t) throw();
  std::string output_time_signature(const bms_attributes&) throw();
 public:
  static std::string page_number_string(size_t, std::string="") throw();
  static void to_upper_number(std::string&) throw();
  BrailleOutputer() throw();
  virtual ~BrailleOutputer() throw(){}
  void update_output(const bms_score&, brl_output&) throw(); // Main function of output.
};
#endif
