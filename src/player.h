#ifndef PLAYER_H
#define PLAYER_H
#include "front.h"
class ScorePlayer{
 private:
  static int instance_count;
 public:
  PlayOutArg range;
  PlaySysArg state;
  static bool sound(unsigned long) throw();
  ScorePlayer() throw();
  virtual ~ScorePlayer() throw();
  void initialize_arguments(const bms_score&) throw();
  void play_score(Front&) throw();
};
#endif // PLAYER_H_INCLUDED
