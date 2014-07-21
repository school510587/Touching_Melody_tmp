#ifndef READER_H
#define READER_H
#include "front.h"
class ScoreReader{
 private:
  void read_attribute(std::ostringstream&, const bms_attributes&) const throw();
  void read_barline(std::ostringstream&, const bms_barline&, const bms_note*&) const throw();
  void read_direction(std::ostringstream&, const bms_direction&) const throw();
  void read_note(std::ostringstream&, const bms_note&, const bms_note*&) const throw();
 public:
  OralOutArg range;
  OralSysArg state;
  ScoreReader() throw();
  virtual ~ScoreReader() throw();
  void convert(const bms_score&, std::vector<std::string>&) const throw();
  void initialize_range() throw();
  key_t read(Front&) const throw();
};
#endif // READER_H_INCLUDED
