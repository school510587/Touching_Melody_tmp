#include <math.h>
#include <stdlib.h>
#include "midi_data.h"
#include "score.h"
using namespace std;
class trill_function{
 private:
  int counter;
  unsigned char main_pitch;
  char downward_step, upward_step;
 public:
  trill_function(unsigned char, char, char) throw();
  virtual ~trill_function() throw(){}
  unsigned char next() throw();
  void start(int) throw();
};
const int MidiStream::default_tempo=120;
const float MidiStream::default_ffffff=139.f, MidiStream::default_fffff=135.f, MidiStream::default_ffff=131.f, MidiStream::default_fff=127.f, MidiStream::default_ff=112.f, MidiStream::default_f=98.f, MidiStream::default_mf=83.f, MidiStream::default_mp=69.f, MidiStream::default_p=54.f, MidiStream::default_pp=40.f, MidiStream::default_ppp=26.f, MidiStream::default_pppp=18.f, MidiStream::default_ppppp=10.f, MidiStream::default_pppppp=2.f;
trill_function::trill_function(unsigned char m, char u, char d) throw(): counter(0){
 main_pitch=m;
 upward_step=u;
 downward_step=d;
}
unsigned char trill_function::next() throw(){
 switch(counter++%4){ // The shape of output must be like a wave.
  case 1: return main_pitch+upward_step;
  case 3: return main_pitch-downward_step;
  default: return main_pitch;
 }
}
void trill_function::start(int phase) throw(){
 while(phase<0) phase+=4;
 counter=phase%4;
}
#if 0
static long auto_long_grace_percent(long duration, long divisions, int tempo) throw(){
 double result=(8.0*divisions*tempo)/duration/MidiStream::default_tempo;
 if(result>16) result=16; // An upper bound of 50%.
 return 50+(long)ceil(result);
}
#endif
static long auto_short_grace_percent(long duration, long divisions, int tempo) throw(){
 double result=(12.0*divisions*tempo)/duration/MidiStream::default_tempo;
 if(result>50) result=50; // An upper bound of 50%.
 return (long)ceil(result);
}
static long default_trill_beats(long default_grace_percent) throw(){
 switch(default_grace_percent){
  case 1:
  case 2:
  case 3:
  case 4: return 33;
  case 5:
  case 6: return 25;
  case 7:
  case 8:
  case 9: return 11;
  case 10:
  case 11:
  case 12: return 9;
  default:
   if(13<=default_grace_percent && default_grace_percent<=16) return 7;
   else if(17<=default_grace_percent && default_grace_percent<=25) return 5;
   else if(26<=default_grace_percent) return 3;
  break;
 }
 return 0; // No trill-beats can be defined.
}
MidiCommandVector add_skills(const MidiStream& stream, int tempo) throw(){
 MidiCommandVector result;
 result.vector<midi_command_t>::operator=(stream);
 return result;
}
void MidiCommandVector::add_boundary(long point) throw(){
 size_t p=find_position_of(point*factor);
 if(p>0) at(p).m_start=(at(p).m_stop=at(p-1).m_start)+1;
 else at(p).m_start=1, at(p).m_stop=0;
}
void MidiCommandVector::add_controller(unsigned char channel, unsigned char id, unsigned char rate, long point) throw(){
 midi_message_t message={0};
 message.part=message.staff=0; // This value is only for controllers.
 message.command_byte[0]=0xb0|channel;
 message.command_byte[1]=id;
 message.command_byte[2]=rate;
 point*=factor;
 if(empty() || point>back().time_point){
  resize(size()+1);
  back().time_point=point;
  back().message.push_back(message);
  back().m_start=back().m_stop=0;
 }
 else{
  int position=size()-1;
  for(; position>0 && at(position).time_point>point; position--);
  if(at(position).time_point==point){
   list<midi_message_t>::iterator bound=at(position).message.begin();
   while(bound!=at(position).message.end() && (bound->command_byte[0]&0xf0)==0xc0 && bound->command_byte[2]==0) bound++;
   while(bound!=at(position).message.end() && (bound->command_byte[0]&0xf0)==0xb0) bound++;
   at(position).message.insert(bound, message);
  }
  else{
   midi_command_t new_command; // Note: {0} initializer is forbidden.
   insert(begin()+(++position), new_command);
   at(position).time_point=point;
   at(position).message.push_back(message);
  }
  at(position).m_start=at(position).m_stop=0;
 }
}
void MidiCommandVector::add_note(const bms_note& center, const midi_instrument_map_t& m, long begin) throw(){
 map<const bms_note*, long> prefix_percent, suffix_percent;
 const midi_instrument_t* i=NULL;
 midi_message_t message={0};
 long range[3]={0}, grace_time[2]={0}, default_grace=0, trill_beats=0;
 int current_tempo=tempo(begin);
 //default_grace= center.is_long_grace() ? auto_long_grace_percent(center.duration(), divisions, current_tempo) : auto_short_grace_percent(center.duration(), divisions, current_tempo);
 default_grace= auto_short_grace_percent(center.duration(), divisions, current_tempo);
 if(center.trill_kind()==1){ // Special assignment for trill-marks.
  if(center.trill_beats()<2){
   long g=101;
   if(center.suffix()>0){
    g=center.next_suffix_grace()->steal_time_previous();
    if(g<=100) default_grace=g;
   }
   if(g>100 && center.prefix()>0){
    g=center.next_prefix_grace()->steal_time_following();
    if(g<=100) default_grace=g;
   }
   if(g>100) adjust(center.duration()*default_grace, 100); // No grace for scaling.
   adjust(center.duration(), trill_beats=default_trill_beats(default_grace));
  } // The number of beats is defined.
  else adjust(center.duration(), trill_beats=center.trill_beats());
 }
 else if(center.trill_kind()>0){ // Mordents and turns.
  adjust(center.duration()*gcd(center.trill_second_beat(), center.trill_last_beat()), 100);
  adjust(center.duration()*(center.trill_last_beat()-center.trill_second_beat()), 100*(center.trill_beats()-2));
 }
 if(center.prefix()>0 || center.suffix()>0){
  long g=0;
  for(const bms_note* l=center.next_prefix_grace(); l!=NULL; l=l->next_prefix_grace()){
   prefix_percent[l]=l->steal_time_following();
   if(prefix_percent[l]>100) prefix_percent[l]=default_grace;
   g=gcd(g, prefix_percent[l]);
  }
  for(const bms_note* l=center.next_suffix_grace(); l!=NULL; l=l->next_suffix_grace()){
   suffix_percent[l]=l->steal_time_previous();
   if(suffix_percent[l]>100) suffix_percent[l]=default_grace;
   g=gcd(g, suffix_percent[l]);
  }
  if(g>0) adjust(g*center.duration(), 100); // There exists some grace notes.
 }
 for(const bms_note* c=&center; c!=NULL; c=c->next_chord()) if(c->arpeggiate_order()>0 && !c->attack_set()) adjust(center.duration()*default_grace, 100);
 if(!center.release_set()){
  if(center.articulation_flag()&bms_note::note_staccato) adjust(center.duration(), 2);
  if(center.articulation_flag()&bms_note::note_staccatissimo || center.articulation_flag()&bms_note::note_detached_legato) adjust(center.duration(), 4);
 }
 range[0]=begin*=factor; // Scaling.
 message.part=center._part();
 message.staff=center._staff();
 if(center.prefix()>0){ // Duration of the root note is stolen.
  long s=center.duration()*factor;
  for(const bms_note* l=center.next_prefix_grace(); l!=NULL; l=l->next_prefix_grace()){
   range[1]=range[0]+s*prefix_percent[l]/100;
   i=&m.find(l->instrument())->second;
   message.command_byte[0]=0x90|(i->channel-1);
   message.command_byte[1]= i->channel==10 ? i->unpitched : l->midi_pitch();
   message.command_byte[2]=l->dynamics();
#ifdef AMPLIFY
   if(message.command_byte[2]<108) message.command_byte[2]+=20;
   else message.command_byte[2]=127;
#endif
   add_pitch(message, range[0], range[1], center._measure(), l->tied_type());
   range[0]=range[1];
  }
  grace_time[0]=range[0];
 }
 range[0]+=center.attack()*factor;
 range[2]=center.duration()*factor;
 if(center.suffix()>0){ // Duration of the root note is stolen.
  long s=suffix_percent[0];
  for(const bms_note* l=center.next_suffix_grace(); l!=NULL; l=l->next_suffix_grace()) s+=suffix_percent[l];
  range[2]-=(grace_time[1]=s*center.duration()*factor/100);
 }
 range[1]=begin+range[2]+center.release()*factor;
 if(!center.release_set()){
  if(center.articulation_flag()&bms_note::note_staccato) range[2]/=2;
  if(center.articulation_flag()&bms_note::note_staccatissimo) range[2]/=4;
  if(center.articulation_flag()&bms_note::note_detached_legato) range[2]=3*range[2]/4;
 }
 i=&m.find(center.instrument())->second;
 message.command_byte[0]=0x90|(i->channel-1);
 message.command_byte[1]= i->channel==10 ? i->unpitched : center.midi_pitch();
 message.command_byte[2]=center.dynamics();
#ifdef AMPLIFY
 if(message.command_byte[2]<108) message.command_byte[2]+=20;
 else message.command_byte[2]=127;
#endif
 if(center.trill_kind()>0){
  long end=range[1], s=center.duration()*factor;
  char w[2]={0};
  unsigned char dynamics=message.command_byte[2];
  w[0]=center.trill_step();
  if(center.trill_kind()==5 || center.trill_kind()==7) w[0]=-w[0];
  if(center.trill_kind()<12){
   w[1]=center.trill_step();
   if(center.trill_kind()!=5 && center.trill_kind()!=7) w[1]=-w[1];
  }
  else w[1]=center.trill_two_note_turn();
  if(center.trill_kind()==1){ // Performance of trill-marks.
   trill_function trill_wave(message.command_byte[1], w[0], w[1]);
   trill_wave.start(center.trill_start_note());
   for(long j=0; j<trill_beats && range[0]<end; j++){
    range[1]=range[0]+s/trill_beats;
    if(range[1]>end) range[1]=end;
    message.command_byte[1]=trill_wave.next();
    message.command_byte[2]=dynamics; // To reset its dynamics.
    add_pitch(message, range[0], range[1], center._measure(), center.tied_type());
    range[0]=range[1];
   }
  }
  else{ // Performance of mordents and turns.
   trill_function trill_wave(message.command_byte[1], w[0], w[1]);
   trill_wave.start(center.trill_start_note());
   for(long j=0; j<center.trill_beats() && range[0]<end; j++){
    if(j==0) range[1]=range[0]+s*center.trill_second_beat()/100;
    else if(j==center.trill_beats()-1) range[1]=range[0]+s*(100-center.trill_last_beat())/100;
    else range[1]=range[0]+s*(center.trill_last_beat()-center.trill_second_beat())/100/(center.trill_beats()-2);
    if(range[1]>end) range[1]=end;
    message.command_byte[1]=trill_wave.next();
    message.command_byte[2]=dynamics; // To reset its dynamics.
    add_pitch(message, range[0], range[1], center._measure(), center.tied_type());
    range[0]=range[1];
   }
  }
 }
 else{ // No ornaments.
  if(!center.attack_set()) range[0]+=center.arpeggiate_order()*center.duration()*factor*default_grace/100;
  if(!center.release_set()) range[1]=begin+range[2];
  add_pitch(message, range[0], range[1], center._measure(), center.tied_type());
  for(const bms_note* j=center.next_chord(); j!=NULL; j=j->next_chord()){
   range[0]=begin+(j->attack_set() ?(j->attack()*factor):(j->arpeggiate_order()*j->duration()*factor*default_grace/100))+grace_time[0];
   range[2]=j->duration()*factor-grace_time[1];
   range[1]=begin+range[2];
   if(j->release_set()) range[1]+=j->release()*factor;
   else{
    if(center.articulation_flag()&bms_note::note_staccato) range[2]/=2;
    if(center.articulation_flag()&bms_note::note_staccatissimo) range[2]/=4;
    if(center.articulation_flag()&bms_note::note_detached_legato) range[2]=3*range[2]/4;
    range[1]=begin+range[2];
   }
   i=&m.find(j->instrument())->second;
   message.command_byte[0]=0x90|(i->channel-1);
   message.command_byte[1]= i->channel==10 ? i->unpitched : j->midi_pitch();
   message.command_byte[2]=j->dynamics();
#ifdef AMPLIFY
   if(message.command_byte[2]<108) message.command_byte[2]+=20;
   else message.command_byte[2]=127;
#endif
   add_pitch(message, range[0], range[1], center._measure(), j->tied_type());
  }
 }
 if(center.suffix()>0){ // Playback for suffix grace notes.
  long s=center.duration()*factor;
  range[0]=begin+s-grace_time[1];
  for(const bms_note* l=center.next_suffix_grace(); l!=NULL; l=l->next_suffix_grace()){
   range[1]=range[0]+s*suffix_percent[l]/100;
   i=&m.find(l->instrument())->second;
   message.command_byte[0]=0x90|(i->channel-1);
   message.command_byte[1]= i->channel==10 ? i->unpitched : l->midi_pitch();
   message.command_byte[2]=l->dynamics();
#ifdef AMPLIFY
   if(message.command_byte[2]<108) message.command_byte[2]+=20;
   else message.command_byte[2]=127;
#endif
   add_pitch(message, range[0], range[1], center._measure(), l->tied_type());
   range[0]=range[1];
  }
 }
}
void MidiCommandVector::add_pitch(midi_message_t& message, long start, long stop, int measure, int tied_type) throw(){
 if(message.command_byte[2]!=0){ // Some notes may have dynamics of 0.
  size_t position=0;
  if(tied_type==0 || tied_type==1){
   position=find_position_of(start);
   if(position<size()){
    at(position).message.push_back(message);
    at(position).m_start=measure;
   }
  }
  if(tied_type==0 || tied_type==3){
   message.command_byte[2]=0; // Close-note message.
   position=find_position_of(stop);
   if(position<size()){
    at(position).message.push_front(message);
    at(position).m_stop=measure;
    if(at(position).m_start==0) at(position).m_start=at(position).m_stop;
   }
  }
 } // After this call, the dynamics of message becomes 0.
}
void MidiCommandVector::add_skill(const sound_entity_t& skill, long start) throw(){
 switch(skill.kind){
  case 0: // Trill-marks, mordents, and turns.
   if(skill.trill.second_beat==0 || skill.trill.last_beat==0){
   }
   else{
   }
  break;
 }
}
void MidiCommandVector::adjust(long numerator, long denominator) throw(){
 long g=gcd(numerator*factor, denominator);
 denominator/=g;
 factor*=denominator;
 for(size_t i=0; i<size(); i++) at(i).time_point*=denominator;
}
size_t MidiCommandVector::find_position_of(long point) throw(){
 if(empty() || point>back().time_point){
  resize(size()+1); // The inserted point is beyond the original ending.
  back().time_point=point;
  return size()-1; // The new object is a insertion point.
 }
 else{ // The object is previously created, or its position is in the middle.
  size_t position=size()-1;
  for(; position>0 && at(position).time_point>point; position--);
  if(at(position).time_point!=point){ // This point does not previously exist.
   midi_command_t new_command; // Note: {0} initializer is forbidden.
   insert(begin()+(++position), new_command);
   at(position).time_point=point;
  }
  return position;
 }
}
void MidiCommandVector::set_tempo(long time_point, int tempo) throw(){
 vector<pair<long, int> >::iterator p=tempo_map.begin();
 while(p!=tempo_map.end() && p->first<time_point) p++;
 if(p==tempo_map.end() || p->first>time_point) tempo_map.insert(p, pair<long, int>(time_point, tempo));
} // Note: The time index is not scaled.
int MidiCommandVector::tempo(long time_point) const throw(){
 vector<pair<long, int> >::const_iterator p=tempo_map.begin();
 while(p!=tempo_map.end() && p->first<=time_point) p++;
 return p==tempo_map.begin() ? MidiStream::default_tempo : (p-1)->second;
}
void MidiStream::clear() throw(){
 vector<midi_command_t>::clear();
 tempo_map.clear();
}
void MidiStream::set_tempo(long time_point, int tempo) throw(){
 vector<pair<long, int> >::iterator p=tempo_map.begin();
 while(p!=tempo_map.end() && p->first<time_point) p++;
 if(p==tempo_map.end() || p->first>time_point) tempo_map.insert(p, pair<long, int>(time_point, tempo));
}
int MidiStream::tempo(long time_point) const throw(){
 vector<pair<long, int> >::const_iterator p=tempo_map.begin();
 while(p!=tempo_map.end() && p->first<=time_point) p++;
 return p==tempo_map.begin() ? default_tempo : (p-1)->second;
}
