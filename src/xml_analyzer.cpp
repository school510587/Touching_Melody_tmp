#include <algorithm>
#include <stack>
#include <math.h>
#include <memory.h>
#include "elements.h"
#include "xml_analyzer.h"
#define SLUR_PLACE_UNKNOWN 0
#define SLUR_PLACE_OVER 1
#define SLUR_PLACE_UNDER 2
#define SLUR_PLACE_BOTH (SLUR_PLACE_OVER|SLUR_PLACE_UNDER)
#define SOUND_DACAPO 32768
#define SOUND_DAMPER_PEDAL 16384
#define SOUND_SOFT_PEDAL 8192
#define SOUND_SOSTENUTO_PEDAL 2048
#define TIED_STATE_UNKNOWN 0
#define TIED_STATE_SINGLE 1
#define TIED_STATE_DOUBLE 2
using namespace std;
using namespace MusicXML2;
typedef struct{
 list<midi_message_t>* command_list;
 list<midi_message_t>::iterator note;
} midi_note_record_t;
typedef struct{
 int actual_notes, counter, tuplet_size;
 bms_note* note;
} time_modification_t;
const unsigned short XMLAnalyzer::damper_pedal_change=32768, XMLAnalyzer::damper_pedal=16384, XMLAnalyzer::soft_pedal_change=8192, XMLAnalyzer::soft_pedal=4096, XMLAnalyzer::sostenuto_pedal_change=2048, XMLAnalyzer::sostenuto_pedal=1024;
static int alter_from_accidental(int a) throw(){
 switch(a){
  case 0:
  case 1: return -2;
  case 2:
  case 3:
  case 4: return -1;
  case 6:
  case 7:
  case 8: return 1;
  case 9:
  case 10: return 2;
  default: return 0; // 5 included.
 }
}
static int encode_bar_style(const string& str){
 if(str=="regular") return 0;
 else if(str=="dotted") return 1;
 else if(str=="dashed") return 2;
 else if(str=="heavy") return 3;
 else if(str=="light-light") return 4;
 else if(str=="light-heavy") return 5;
 else if(str=="heavy-light") return 6;
 else if(str=="heavy-heavy") return 7;
 else if(str=="tick") return 8;
 else if(str=="short") return 9;
 else if(str=="none") return 10;
 else return 11;
}
static size_t encode_controller(const string& text) throw(){
 if(text=="yes") return 100;
 else if(text=="no") return 0;
 else{
  size_t result=0;
  for(string::const_iterator p=text.begin(); p!=text.end(); p++){
   if('0'<=*p && *p<='9') result=10*result+(*p-'0');
   else return 0;
  }
  return result;
 }
}
static int encode_mode(const string& text) throw(){
 if(text.empty()) return 0;
 else if(text=="major") return 1;
 else if(text=="minor") return 2;
 else if(text=="aeolian") return 3;
 else if(text=="dorian") return 4;
 else if(text=="ionain") return 5;
 else if(text=="locrian") return 6;
 else if(text=="lydian") return 7;
 else if(text=="mixolydian") return 8;
 else if(text=="phrygian") return 9;
 else return 0;
}
static int encode_sign(string s) throw(){
 for(string::iterator it=s.begin(); it!=s.end(); it++) *it|=0x20;
 if(s=="g") return 1;
 else if(s=="c") return 2;
 else if(s=="f") return 3;
 else if(s=="percussion") return 4;
 else if(s=="tab") return 5;
 else if(s=="none") return 6;
 else{
  puts("Exception: unknown sign");
  return -1;
 }
}
static int encode_start_note(string s) throw(){
 if(s.empty()) return 2;
 else if(s=="main") return 0;
 else if(s=="upper") return 1;
 else if(s=="below") return 3;
 else return 2;
}
static int encode_trill_step(string s) throw(){
 if(s.empty()) return 3; // Automatic trill-step.
 else if(s=="whole") return 2;
 else if(s=="half") return 1;
 else if(s=="unison") return 0;
 else return 3;
}
static int encode_two_note_turn(string s) throw(){
 if(s.empty()) return 3; // Automatic two-note-turn.
 else if(s=="whole") return 2;
 else if(s=="half") return 1;
 else if(s=="none") return 0;
 else return 3;
}
static bms_note::note_type_t encode_note_type(const string& type) throw(){
 if(type=="whole") return bms_note::whole;
 else if(type=="half") return bms_note::half;
 else if(type=="quarter") return bms_note::quarter;
 else if(type=="eighth") return bms_note::eighth;
 else if(type=="16th") return bms_note::_16th;
 else if(type=="32nd") return bms_note::_32nd;
 else if(type=="64th") return bms_note::_64th;
 else if(type=="128th") return bms_note::_128th;
 else if(type=="256th") return bms_note::_256th;
 else if(type=="breve") return bms_note::breve;
 else if(type=="long") return bms_note::longa;
 else return bms_note::whole;
}
/*   A convertion from accidental or 
 * accidental-mark symbol value to an 
 * integer. This result will matches the 
 * index of accidental_word array in the 
 * file "out_words.h", except what returned 
 * in the last else block.
 */
static int encode_accidental(const string& str) throw(){
 if(str=="sharp") return 8;
 else if(str=="flat") return 2;
 else if(str=="natural") return 5;
 else if(str=="natural-sharp") return 6;
 else if(str=="natural-flat") return 4;
 else if(str=="double-sharp" || str=="sharp-sharp") return 10;
 else if(str=="flat-flat") return 0;
 else if(str=="quarter-sharp") return 7;
 else if(str=="quarter-flat") return 3;
 else if(str=="three-quarter-sharp") return 9;
 else if(str=="three-quarter-flat") return 1;
 else return 15; // Invalid information.
}
static int encode_type(const string& t){
 if(t=="stop") return TYPE_STOP;
 else if(t=="start") return TYPE_START;
 else if(t=="down") return TYPE_DOWN;
 else if(t=="change") return TYPE_CHANGE;
 else if(t=="up") return TYPE_UP;
 else if(t=="continue") return TYPE_CONTINUE;
 else return TYPE_UNKNOWN;
}
static bool default_pitch_direction(int s, int l) throw(){
 return s==1 ||(s==2 && l<4); // G clef, 1st-, 2nd- and 3rd-line C clef.
}
long gcd(long a, long b) throw(){
 long r=0;
 if(a<b){
  r=a;
  a=b;
  b=r;
 } // After this block, it is ensured that a>b.
 while(b>0){
  r=a%b;
  a=b;
  b=r;
 } // After the loop, a is the GCD of 2 inputs.
 return a;
}
static void join_midi_parts(MidiStream& stream, vector<MidiCommandVector>& midi_part) throw(){
 map<unsigned char, midi_note_record_t> channel[16]; 
 if(midi_part.size()>1){ // Multi-part music score.
  long a=0, lcm=midi_part.front().get_divisions();
  for(size_t i=1; i<midi_part.size(); i++){
   a=midi_part[i].get_divisions();
   lcm*=a/gcd(lcm, a);
  }
  stream.set_divisions(lcm);
  for(size_t i=0; i<midi_part.size(); i++){
   long m=lcm/midi_part[i].get_divisions();
   for(size_t j=0; j<midi_part[i].size(); j++) midi_part[i][j].time_point*=m;
   for(MidiCommandVector::tempo_iterator p=midi_part[i].tempo_begin(); p!=midi_part[i].tempo_end(); p++) stream.set_tempo(p->first*m, p->second);
  }
  while(!midi_part.empty()){
   a=midi_part.front().front().time_point;
   for(size_t i=1; i<midi_part.size(); i++) if(a>midi_part[i].front().time_point) a=midi_part[i].front().time_point;
   stream.resize(stream.size()+1);
   stream.back().time_point=a;
   stream.back().m_start=0;
   stream.back().m_stop=0;
   for(size_t i=0; i<midi_part.size(); i++){
    if(midi_part[i].front().time_point==a){
     for(list<midi_message_t>::iterator m_it=midi_part[i].front().message.begin(); m_it!=midi_part[i].front().message.end(); m_it++) stream.back().message.push_back(*m_it);
     if(stream.back().m_start==0) stream.back().m_start=midi_part[i].front().m_start;
     if(stream.back().m_stop==0) stream.back().m_stop=midi_part[i].front().m_stop;
     midi_part[i].erase(midi_part[i].begin());
     if(midi_part[i].empty()) midi_part.erase(midi_part.begin()+i);
    }
   }
  }
 }
 else{ // Single-part music score.
  stream.set_divisions(midi_part.front().get_divisions());
  for(MidiCommandVector::tempo_iterator p=midi_part[0].tempo_begin(); p!=midi_part[0].tempo_end(); p++) stream.set_tempo(p->first, p->second);
  stream.vector<midi_command_t>::operator=(midi_part.front());
 }
 for(MidiStream::iterator p=stream.begin(); p!=stream.end(); p++){
  list<midi_message_t> extra_off;
  for(list<midi_message_t>::iterator q=p->message.begin(); q!=p->message.end(); q++){
   if((q->command_byte[0]&0xf0)==0x90){
    midi_note_record_t& r=channel[q->command_byte[0]&0x0f][q->command_byte[1]];
    if(q->command_byte[2]!=0){ // Note-on commands.
     if(r.command_list==NULL){
      r.command_list=&p->message;
      r.note=p->message.end();
     }
     else if(r.note==r.command_list->end()){
      midi_message_t m={0};
      m.command_word=q->command_word;
      m.command_byte[2]=0; // m must be note-off.
      m.part=q->part, m.staff=q->staff;
      extra_off.push_back(m);
     }
     else r.note=r.command_list->end();
    }
   }
  }
  p->message.insert(p->message.begin(), extra_off.begin(), extra_off.end());
 } // Redundant commands are erased after this loop.
}
static int step_number(char step) throw(){
 if(step>='C') return step-'C';
 else if(step=='A') return 5;
 else if(step=='B') return 6;
 else{
  puts("Exception: unknown step");
  return -1;
 }
}
bool XMLAnalyzer::chord_lower_than(const bms_note* const& p, const bms_note* const& q) throw(){
 return p->chord_common_block()<q->chord_common_block();
}
bool XMLAnalyzer::lower_than(XMLAnalyzer::note_vertex* const& p, XMLAnalyzer::note_vertex* const& q) throw(){
 if(p->time<q->time) return true;
 else if(p->time>q->time) return false;
 switch(p->note->stem()){
  case STEM_UP:
   if(q->note->stem()!=STEM_UP) return false;
   else goto pitch_comparison;
  case STEM_NONE:
   if(q->note->stem()!=STEM_NONE) return q->note->stem()==STEM_UP;
   else if(q->note->rest()) return false;
   else goto pitch_comparison;
  case STEM_DOWN:
   if(q->note->stem()!=STEM_DOWN) return true;
   else goto pitch_comparison;
 }
 pitch_comparison: // Two notes have the same stem value.
 if(p->note->diatonic_pitch()<q->note->diatonic_pitch()) return true;
 else if(p->note->diatonic_pitch()==q->note->diatonic_pitch()) return p->note->midi_pitch()<q->note->midi_pitch();
 return false;
}
bool XMLAnalyzer::pitch_lower_than(const bms_note* const& p, const bms_note* const& q) throw(){
 return *p<*q;
}
bool XMLAnalyzer::prior_than(const music_event& i, const music_event& j) throw(){
 return i.first<j.first;
}
bool XMLAnalyzer::upper_than(XMLAnalyzer::note_vertex* const& p, XMLAnalyzer::note_vertex* const& q) throw(){
 if(p->time<q->time) return true;
 else if(p->time>q->time) return false;
 switch(p->note->stem()){
  case STEM_UP:
   if(q->note->stem()!=STEM_UP) return true;
   else goto pitch_comparison;
  case STEM_NONE:
   if(q->note->stem()!=STEM_NONE) return q->note->stem()==STEM_DOWN;
   else if(p->note->rest()) return false;
   else goto pitch_comparison;
  case STEM_DOWN:
   if(q->note->stem()!=STEM_DOWN) return false;
   else goto pitch_comparison;
 }
 pitch_comparison: // Two notes have the same stem value.
 if(p->note->diatonic_pitch()>q->note->diatonic_pitch()) return true;
 else if(p->note->diatonic_pitch()==q->note->diatonic_pitch()) return p->note->midi_pitch()>q->note->midi_pitch();
 return false;
}
XMLAnalyzer::XMLAnalyzer(bms_score& music_score) throw(): score(&music_score), divisions(0){
 buffer.attributes.set_data(score->element.attributes);
 buffer.barline.set_data(score->element.barline);
 buffer.voice=NULL;
}
XMLAnalyzer::~XMLAnalyzer() throw(){}
void XMLAnalyzer::add(bms_note* s, unsigned long t) throw(){
 if(s->chord()<0) t-=s->duration();
 else if(s->grace()){
  if(s->steal_time_previous()>0){
   int n=buffer.note.size()-1;
   for(; n>=0; n--) if(!buffer.note[n]->grace() || buffer.note[n]->steal_time_previous()>0) break;
   for(int i=n+1; i<buffer.note.size(); i++){
    buffer.note[i]->set_steal_time_following(0);
    buffer.note[i]->set_steal_time_previous(101);
   }
  }
 }
 buffer.note.push_back(s, t);
}
void XMLAnalyzer::analyze(bms_part& part) throw(){
 for(struct{size_t g, i, r;} p={0}; p.i<buffer.note.size(); p.i++){ // Configuration of chords and graces.
  if(buffer.note[p.i]->chord()<0){
   buffer.note[p.r]->increase_chord();
   buffer.note[p.i]->set_chord(-buffer.note[p.r]->chord());
  }
  else if(buffer.note[p.i]->grace()){
   if(buffer.note[p.i]->grace_not_set()){
    if(buffer.note.time(p.i)==measure_end) buffer.note[p.i]->set_steal_time_previous(101);
    else buffer.note[p.i]->set_steal_time_following(101);
   }
   if(buffer.note[p.i]->steal_time_following()>0) p.g++;
   else if(buffer.note[p.i]->steal_time_previous()>0) buffer.note[p.r]->increase_suffix();
  }
  else{ // Regular notes.
   p.r=p.i;
   for(size_t i=0; i<p.g; i++) buffer.note[p.r]->increase_prefix();
   p.g=0;
  }
 }
 for(buffer_type<bms_note*>::iterator n=buffer.note.begin(); n!=buffer.note.end(); n++){
  if((*n)->chord()>0){ // Chord configuration by appearance and instruments.
   char s=(*n)->chord();
   stable_sort(n, n+s+1, chord_lower_than);
   for(bms_part::note_iterator q=n, r=q+1; q!=n+s+1; q=r++){
    while(r!=n+s+1 && (*q)->_staff()==(*r)->_staff() && (*q)->type()==(*r)->type() && (*q)->stem()==(*r)->stem() && (*q)->instrument()==(*r)->instrument()) r++;
    (*q)->set_chord((r-q)-1);
    for(bms_part::note_iterator t=q+1; t!=r; t++) (*t)->set_chord(q-t);
   } // Staff, type, stem, and instrument must be the same in a chord.
   n+=s;
  }
 }
 for(buffer_type<bms_note*>::iterator n=buffer.note.begin(); n!=buffer.note.end(); n++){
  if((*n)->chord()>0){ // Chord configuration by pitch.
   char s=(*n)->chord();
   stable_sort(n, n+s+1, pitch_lower_than);
   if(part[(*n)->_staff()-1].upmost_pitch) reverse(n, n+s+1);
   (*n)->set_chord(s);
   for(bms_part::note_iterator q=n+1; q!=n+s+1; q++){
    (*n)->set_articulation_flag((*q)->articulation_flag());
    if((*q)->fermata()<(*n)->fermata()) (*n)->set_fermata((*q)->fermata());
    if((*q)->prefix()>0){
     (*n)->set_prefix((*q)->prefix());
     (*q)->set_prefix(0);
    }
    if((*q)->suffix()>0){
     (*n)->set_suffix((*q)->suffix());
     (*q)->set_suffix(0);
    }
    (*q)->set_articulation_flag(0, true);
    (*q)->set_fermata(3); // Any invalid notation is erased.
    (*q)->set_chord(n-q);
   }
   n+=s;
  }
 }
 for(struct{size_t g, i, r;} p={0}; p.i<buffer.note.size(); p.i++){ // Note network construction.
  if(buffer.note[p.i]->chord()<0){
   buffer.note[p.i-1]->set_next_chord(buffer.note[p.i]);
   buffer.note[p.i]->set_center_note(buffer.note[p.r]);
  }
  else if(buffer.note[p.i]->grace()){
   if(buffer.note[p.i]->steal_time_following()>0) p.g++;
   else if(buffer.note[p.i]->steal_time_previous()>0){
    buffer.note[p.i-1]->set_next_suffix_grace(buffer.note[p.i]);
    buffer.note[p.i]->set_center_note(buffer.note[p.r]);
   }
  }
  else{ // Regular notes.
   p.r=p.i;
   if(p.g>0){
    buffer.note[p.r]->set_next_prefix_grace(buffer.note[p.r-p.g]);
    for(size_t i=p.r-p.g; i<p.r; i++){
     buffer.note[i]->set_next_prefix_grace(i+1==p.r ? NULL : buffer.note[i+1]);
     buffer.note[i]->set_center_note(buffer.note[p.r]);
    }
    p.g=0;
   }
  }
 }
 buffer.event.reserve(buffer.event.size()+buffer.attributes.size()+buffer.barline.size()+buffer.direction.size()+buffer.note.size());
 for(size_t i=0; i<buffer.barline.size(); i++){
  buffer.event.resize(buffer.event.size()+1);
  buffer.event.back().first=buffer.barline.time(i);
  buffer.event.back().second.set_barline_index(buffer.barline.actual_index(i));
 }
 for(size_t i=0; i<buffer.attributes.size(); i++){
  buffer.event.resize(buffer.event.size()+1);
  buffer.event.back().first=buffer.attributes.time(i);
  buffer.event.back().second.set_attributes_index(buffer.attributes.actual_index(i));
 }
 for(size_t i=0; i<buffer.direction.size(); i++){
  buffer.event.resize(buffer.event.size()+1);
  buffer.event.back().first=buffer.direction.time(i);
  buffer.event.back().second.set_direction_index(buffer.direction.actual_index(i));
 }
 for(size_t i=0; i<buffer.note.size(); i++){
  buffer.event.resize(buffer.event.size()+1);
  buffer.event.back().first=buffer.note.time(i);
  buffer.event.back().second.set_note(buffer.note[i]);
 }
 for(size_t s=0; s<part.size(); s++){
  list<note_vertex> all_vertex;
  part[s].resize(part[s].size()+1); // New measure.
  root.time_range[0]=measure_begin;
  root.downward_interval=part[s].upmost_pitch;
  for(size_t i=0; i<buffer.note.size(); i++){
   if(buffer.note[i]->_staff()==s+1 && buffer.note[i]->regular()){
    all_vertex.resize(all_vertex.size()+1);
    all_vertex.back().note=buffer.note[i];
    all_vertex.back().time=buffer.note.time(i);
   }
  }
  for(size_t b=0; b<buffer.barline.size(); b++){
   for(list<note_vertex>::iterator p=all_vertex.begin(); p!=all_vertex.end();){
    if(p->time<buffer.barline.time(b)){
     root.vertex.push_back(*p);
     p=all_vertex.erase(p);
    } // The vertex is adapted and erased from the list.
    else p++; // This vertex is skipped.
   }
   root.time_range[1]=buffer.barline.time(b);
   if(!root.vertex.empty()){
    parallel_cut(&root);
    write_into(part[s].back());
   }
   part[s].back().resize(part[s].back().size()+1);
   part[s].back().back().set_barline_index(buffer.barline.actual_index(b));
   root.time_range[0]=root.time_range[1];
  }
  root.vertex.assign(all_vertex.begin(), all_vertex.end());
  root.time_range[1]=measure_end;
  if(!root.vertex.empty()){
   parallel_cut(&root);
   write_into(part[s].back());
  }
  for(size_t a=0; a<buffer.attributes.size(); a++){
   long t=measure_begin;
   if(buffer.attributes[a]._staff()!=s+1 && buffer.attributes[a]._staff()!=0) continue;
   if(buffer.attributes[a]._staff()==0){
    bool confliction=false; // Confliction means a local signature overrides the global signature.
    for(size_t b=0; b<buffer.attributes.size(); b++) if(confliction=(buffer.attributes.time(b)==buffer.attributes.time(a) && buffer.attributes[b].kind()==buffer.attributes[a].kind() && buffer.attributes[b]._staff()==s+1)) break;
    if(confliction) continue;
   }
   for(vector<bms_measure_element>::iterator p=part[s].back().begin(); p!=part[s].back().end(); p++){
    if(p->is_voice_index()){
     if(t!=buffer.attributes.time(a)) t+=buffer.voice->at(p->index2voice()).duration();
     if(t==buffer.attributes.time(a)){
      p=part[s].back().insert(p, bms_measure_element());
      p->set_attributes_index(buffer.attributes.actual_index(a));
      break;
     }
     else if(t>buffer.attributes.time(a)){
      bool inserted=false;
      t-=buffer.voice->at(p->index2voice()).duration();
      for(bms_note_area::iterator q=buffer.voice->at(p->index2voice()).begin(); q!=buffer.voice->at(p->index2voice()).end(); q++){
       bms_note_area::row_iterator r=q->begin();
       long d[2]={t, 0};
       for(; r!=q->end() && d[0]<=buffer.attributes.time(a); r++) d[0]+=r->duration();
       d[0]-=(--r)->duration();
       for(bms_in_accord_trait::iterator i=r->begin(); i!=r->end(); i++){
        d[1]=d[0];
        for(bms_in_accord_trait::row_iterator n=i->begin(); n!=i->end(); n++){
         if(n->is_note_pointer()){
          if(d[1]>=buffer.attributes.time(a)){
           n=i->insert(n, bms_measure_element());
           n->set_attributes_index(buffer.attributes.actual_index(a));
           inserted=true;
          }
          else d[1]+=n->as_note()->duration();
         }
         if(inserted) break;
        }
        if(inserted) break;
       }
       if(inserted) break;
      }
      break;
     }
    }
   }
  }
  for(size_t a=0; a<buffer.direction.size(); a++){
   long t=measure_begin;
   if(buffer.direction[a]._staff()!=s+1) continue;
   if(buffer.direction.time(a)<measure_end){
    for(vector<bms_measure_element>::iterator p=part[s].back().begin(); p!=part[s].back().end(); p++){
     if(p->is_voice_index()){
      if(t==buffer.direction.time(a)){
       p=part[s].back().insert(p, bms_measure_element());
       p->set_direction_index(buffer.direction.actual_index(a));
       break;
      }
      else if((t+=buffer.voice->at(p->index2voice()).duration())>buffer.direction.time(a)){
       bool inserted=false;
       t-=buffer.voice->at(p->index2voice()).duration();
       for(bms_note_area::iterator q=buffer.voice->at(p->index2voice()).begin(); q!=buffer.voice->at(p->index2voice()).end(); q++){
        bms_note_area::row_iterator r=q->begin();
        long d[2]={t, 0};
        for(; r!=q->end() && d[0]<=buffer.direction.time(a); r++) d[0]+=r->duration();
        d[0]-=(--r)->duration();
        for(bms_in_accord_trait::iterator i=r->begin(); i!=r->end(); i++){
         d[1]=d[0];
         for(bms_in_accord_trait::row_iterator n=i->begin(); n!=i->end(); n++){
          if(n->is_note_pointer()){
           if(d[1]==buffer.direction.time(a)) inserted=true;
           else{
            d[1]+=n->as_note()->duration();
            if(d[1]>buffer.direction.time(a)){
             inserted=true;
             if(buffer.direction[a].is_stop()) n++;
            }
           }
          }
          if(inserted){ // There is a position (n) for insertion.
           n=i->insert(n, bms_measure_element());
           n->set_direction_index(buffer.direction.actual_index(a));
           break;
          }
         }
         if(inserted) break;
        }
        if(inserted) break;
       }
       break;
      }
     }
    }
   }
   else{ // The direction is at the end.
    part[s].back().push_back(bms_measure_element());
    part[s].back().back().set_direction_index(buffer.direction.actual_index(a));
   }
  }
 }
 for(struct{size_t g, i; unsigned short v;} p={0}; p.i<buffer.note.size(); p.i++){
  if(buffer.note[p.i]->chord()<0) buffer.note[p.i]->set_voice(p.v);
  else if(buffer.note[p.i]->grace()){
   if(buffer.note[p.i]->steal_time_following()>0) p.g++;
   else if(buffer.note[p.i]->steal_time_previous()>0) buffer.note[p.i]->set_voice(p.v);
  }
  else{ // Regular notes.
   p.v=buffer.note[p.i]->voice();
   for(size_t i=p.i-p.g; i<p.i; i++) buffer.note[i]->set_voice(p.v);
   p.g=0;
  }
 } // Distribution of voice information.
 buffer.attributes.reset_time();
 buffer.barline.reset_time();
 buffer.direction.reset_time();
 buffer.note.reset_time();
}
void XMLAnalyzer::analyze_midi(MidiCommandVector& midi, size_t staff_count) throw(){
 dynamics_list.sort(earlier_than);
 if(!buffer.event.empty()){
  list<sound_dynamics_t>::const_iterator d=dynamics_list.begin();
  list<music_event*> l;
  struct{long time; int state;} history={0};
  unsigned char* dynamics=new unsigned char[staff_count];
  memset(dynamics, 83, staff_count*sizeof(*dynamics));
  stable_sort(buffer.event.begin(), buffer.event.end(), prior_than);
  for(vector<music_event>::iterator p=buffer.event.begin(); p!=buffer.event.end(); p++){
   if(p->second.is_note_pointer() && !p->second.as_note()->rest() && p->second.as_note()->regular()){
    for(; d!=dynamics_list.end() && d->time<=p->first; d++){
     switch(history.state){
      case 0: // Initialization.
       for(size_t i=0; i<staff_count; i++) dynamics[i]=d->value;
       history.time=d->time;
       history.state=1;
      break;
      case 1: // Initialization.
       if(history.time<d->time || d->staff==0){
        for(size_t i=0; i<staff_count; i++) dynamics[i]=d->value;
        history.time=d->time;
       }
       else dynamics[d->staff-1]=d->value;
      break;
     }
    }
    for(list<music_event*>::iterator q=l.begin(); q!=l.end();){
     if(d!=dynamics_list.end() && d->time<(*q)->first+(*q)->second.as_note()->duration()) q++;
     else{
      for(bms_note* g=(*q)->second.as_note()->next_suffix_grace(); g!=NULL; g=g->next_suffix_grace()) for(bms_note* c=g; c!=NULL; c=c->next_chord()) c->set_dynamics(dynamics[c->_staff()-1]);
      midi.add_note(*(*q)->second.as_note(), score->midi_instrument_map(), (*q)->first);
      q=l.erase(q);
     }
    }
    if(d!=dynamics_list.end() && p->second.as_note()->next_suffix_grace()!=NULL && d->time<p->first+p->second.as_note()->duration()) l.push_back(&*p);
    else{
    for(bms_note* g=p->second.as_note()->group_begin(); g!=NULL; g=g->group_next()) for(bms_note* c=g; c!=NULL; c=c->next_chord()) c->set_dynamics(dynamics[c->_staff()-1]);
    midi.add_note(*p->second.as_note(), score->midi_instrument_map(), p->first);
    }
   }
  }
  delete[] dynamics;
 }
 dynamics_list.clear();
}
void XMLAnalyzer::analyze_skills(size_t staff_count) throw(){
 map<int, map<int, int> > accidental_map;
 for(map<long, vector<bms_note*> >::iterator p=note_map.at.begin(); p!=note_map.at.end(); p++){
  map<int, vector<bms_note*> > arpeggiate_state;
  for(vector<bms_note*>::iterator q=p->second.begin(); q!=p->second.end(); q++){
   if(accidental_map.count((*q)->_staff())==0) accidental_map[(*q)->_staff()];
   if((*q)->arpeggiate_number()>0) arpeggiate_state[((*q)->arpeggiate_number()<<2)|(*q)->arpeggiate_kind()].push_back(*q);
  }
  for(map<int, vector<bms_note*> >::iterator q=arpeggiate_state.begin(); q!=arpeggiate_state.end(); q++){
   int order=0;
   bool cross_staff=false;
   if(q->second.empty()) continue; // Unexpected exception.
   stable_sort(q->second.begin(), q->second.end(), pitch_lower_than);
   if(q->second.front()->arpeggiate_kind()&1) reverse(q->second.begin(), q->second.end());
   for(vector<bms_note*>::iterator r=q->second.begin(); r+1!=q->second.end(); r++) if(cross_staff=((*r)->_staff()!=(*(r+1))->_staff())) break;
   for(vector<bms_note*>::iterator r=q->second.begin(); r!=q->second.end(); r++){
    if(cross_staff) (*r)->set_arpeggiate_kind(2);
    (*r)->set_arpeggiate_order(order++);
   }
  }
 }
 for(map<long, vector<bms_note*> >::iterator p=note_map.at.begin(); p!=note_map.at.end(); p++){
  for(vector<bms_note*>::iterator q=p->second.begin(); q!=p->second.end(); q++){
   if(!(*q)->rest() && (*q)->trill_kind()>0){
    int d[4]={0}, m[2]={15, 15}, w=(*q)->diatonic_pitch();
    d[2]=((*q)->step()==2 || (*q)->step()==6 ? 1 : 2)-(*q)->alter();
    d[3]=((*q)->step()==0 || (*q)->step()==3 ? 1 : 2)+(*q)->alter();
    if(accidental_map[(*q)->_staff()].count(w+1)>0) m[0]=accidental_map[(*q)->_staff()][w+1];
    else{ // Alternation of step w+1.
     switch(alter[(w+1)%7]){
      case 1: m[0]=encode_accidental("sharp"); break;
      case 0: m[0]=encode_accidental("natural"); break;
      case -1: m[0]=encode_accidental("flat"); break;
     }
    }
    if(w>0 && accidental_map[(*q)->_staff()].count(w-1)>0) m[1]=accidental_map[(*q)->_staff()][w-1];
    else{ // Alternation of step w-1.
     switch(alter[(w+6)%7]){
      case 1: m[1]=encode_accidental("sharp"); break;
      case 0: m[1]=encode_accidental("natural"); break;
      case -1: m[1]=encode_accidental("flat"); break;
     }
    }
    d[0]=d[2]+alter_from_accidental(m[0]);
    d[1]=d[3]-alter_from_accidental(m[1]);
    if(note_map.uncorrected_trill.count(*q)>0){
     int c=encode_accidental("natural");
     if((*q)->accidental_mark_above()>=11){
      (*q)->set_accidental_mark_above((*q)->accidental_mark_below());
      (*q)->set_accidental_mark_below(15);
     }
     if((*q)->accidental_mark_above()<c){
      if(d[0]<2 || d[1]==0){ // The flat must be below the main note.
       unsigned char t=(*q)->accidental_mark_above();
       (*q)->set_accidental_mark_above((*q)->accidental_mark_below());
       (*q)->set_accidental_mark_below(t);
      }
     }
     else if((*q)->accidental_mark_above()>c){
      if(d[0]>0 && d[1]>=2){ // The sharp must be below the main note.
       unsigned char t=(*q)->accidental_mark_above();
       (*q)->set_accidental_mark_above((*q)->accidental_mark_below());
       (*q)->set_accidental_mark_below(t);
      }
     }
     else{
      bool exchange=false;
      if(m[0]==c) exchange=(m[1]!=c);
      else if(m[1]==c) exchange=false;
      else if(d[0]>2 || d[1]>2) exchange=(d[1]>2);
      else exchange=(fifths<0);
      if(exchange){
       unsigned char t=(*q)->accidental_mark_above();
       (*q)->set_accidental_mark_above((*q)->accidental_mark_below());
       (*q)->set_accidental_mark_below(t);
      }
     }
     note_map.uncorrected_trill.erase(*q);
    }
    if((*q)->accidental_mark_above()<11){
     accidental_map[(*q)->_staff()][w+1]=(*q)->accidental_mark_above();
     d[0]=d[2]+alter_from_accidental((*q)->accidental_mark_above());
    }
    if((*q)->accidental_mark_below()<11){
     accidental_map[(*q)->_staff()][w-1]=(*q)->accidental_mark_below();
     d[1]=d[3]-alter_from_accidental((*q)->accidental_mark_below());
    }
    switch((*q)->trill_kind()){ // To determine undefined default fields.
     case 1: // Trill-marks.
      if((*q)->trill_start_note()==2) (*q)->set_trill_start_note(encode_start_note("main"));
     goto single_above_accidental_mark;
     case 4: // Mordents.
      if((*q)->trill_start_note()==2) (*q)->set_trill_start_note(encode_start_note("main"));
      if((*q)->trill_beats()==0) (*q)->set_trill_beats(3);
      if((*q)->trill_second_beat()>100) (*q)->set_trill_second_beat(12);
      if((*q)->trill_last_beat()>100) (*q)->set_trill_last_beat(24);
     goto single_above_accidental_mark;
     case 6: // Long mordents.
      if((*q)->trill_start_note()==2) (*q)->set_trill_start_note(encode_start_note("main"));
      if((*q)->trill_beats()==0) (*q)->set_trill_beats(5);
      if((*q)->trill_second_beat()>100) (*q)->set_trill_second_beat(12);
      if((*q)->trill_last_beat()>100) (*q)->set_trill_last_beat(48);
     goto single_above_accidental_mark;
     single_above_accidental_mark:
      if((*q)->trill_step()==3){
       (*q)->set_trill_step(d[0]);
      }
      if((*q)->trill_two_note_turn()==3) (*q)->set_trill_two_note_turn(0);
     break;
     case 5: // Inverted mordents.
      if((*q)->trill_start_note()==2) (*q)->set_trill_start_note(encode_start_note("main"));
      if((*q)->trill_beats()==0) (*q)->set_trill_beats(3);
      if((*q)->trill_second_beat()>100) (*q)->set_trill_second_beat(12);
      if((*q)->trill_last_beat()>100) (*q)->set_trill_last_beat(24);
     goto single_below_accidental_mark;
     case 7: // Long inverted mordents.
      if((*q)->trill_start_note()==2) (*q)->set_trill_start_note(encode_start_note("main"));
      if((*q)->trill_beats()==0) (*q)->set_trill_beats(5);
      if((*q)->trill_second_beat()>100) (*q)->set_trill_second_beat(12);
      if((*q)->trill_last_beat()>100) (*q)->set_trill_last_beat(48);
     goto single_below_accidental_mark;
     single_below_accidental_mark:
      if((*q)->trill_step()==3){
       (*q)->set_trill_step(d[1]);
      }
      if((*q)->trill_two_note_turn()==3) (*q)->set_trill_two_note_turn(0);
     break;
     case 12: // Turns.
      if((*q)->trill_start_note()==2) (*q)->set_trill_start_note(encode_start_note("upper"));
      if((*q)->trill_beats()==0) (*q)->set_trill_beats(4);
      if((*q)->trill_second_beat()>100) (*q)->set_trill_second_beat(25);
      if((*q)->trill_last_beat()>100) (*q)->set_trill_last_beat(75);
     goto double_accidental_marks;
     case 13: // Inverted turns.
      if((*q)->trill_start_note()==2) (*q)->set_trill_start_note(encode_start_note("below"));
      if((*q)->trill_beats()==0) (*q)->set_trill_beats(4);
      if((*q)->trill_second_beat()>100) (*q)->set_trill_second_beat(25);
      if((*q)->trill_last_beat()>100) (*q)->set_trill_last_beat(75);
     goto double_accidental_marks;
     case 14: // Delayed turns.
      if((*q)->trill_start_note()==2) (*q)->set_trill_start_note(encode_start_note("main"));
      if((*q)->trill_beats()==0) (*q)->set_trill_beats(4);
      if((*q)->trill_second_beat()>100) (*q)->set_trill_second_beat(50);
      if((*q)->trill_last_beat()>100) (*q)->set_trill_last_beat(83);
     goto double_accidental_marks;
     case 15: // Delayed inverted turns.
      if((*q)->trill_start_note()==2) (*q)->set_trill_start_note(encode_start_note("below"));
      if((*q)->trill_beats()==0) (*q)->set_trill_beats(4);
      if((*q)->trill_second_beat()>100) (*q)->set_trill_second_beat(50);
      if((*q)->trill_last_beat()>100) (*q)->set_trill_last_beat(83);
     goto double_accidental_marks;
     double_accidental_marks:
      if((*q)->trill_step()==3){
       (*q)->set_trill_step(d[0]);
      }
      if((*q)->trill_two_note_turn()==3) (*q)->set_trill_two_note_turn(d[1]);
     break;
    }
   }
   if((*q)->accidental()<11) accidental_map[(*q)->_staff()][(*q)->diatonic_pitch()]=(*q)->accidental();
  }
 }
 if(!note_map.uncorrected_trill.empty()) note_map.uncorrected_trill.clear();
}
void XMLAnalyzer::analyze_slur() throw(){
 if(slur_record.empty()) return;
 for(vector<slur_record_t>::iterator p=slur_record.begin(); p!=slur_record.end(); p++){
  vector<slur_trait_t> slur_bridge;
  slur_trait_t trait={0};
  if(p->path.size()<2) continue; // Unexpected exception.
  trait.type=TYPE_CONTINUE;
  for(vector<slur_trait_t>::iterator q=p->path.begin(), r=q+1; q!=p->path.end(); q=r++){
   if(q->nptr->chord()<0) q->nptr=q->nptr->center_note();
   while(r!=p->path.end() && r->nptr->group_id()==q->nptr->group_id()) r++;
   for(bms_note* s=q->nptr->group_begin(); s!=NULL; s=s->group_next()){
    trait.nptr=s;
    if(q!=r){
     if(trait.nptr==q->nptr) q++;
     else if(q->type!=TYPE_START) q=p->path.insert(q, trait)+1;
    }
    else if((q-1)->type!=TYPE_STOP) r=q=p->path.insert(q, trait)+1;
   }
  }
  for(vector<slur_trait_t>::iterator q=p->path.begin(), r=q+1; r!=p->path.end(); q=r++){
   if(r->nptr->group_id()==q->nptr->group_id()) continue;
   slur_bridge.clear();
   root.time_range[0]=note_map.time[q->nptr];
   root.time_range[1]=note_map.time[r->nptr];
   if(q->nptr->_staff()==r->nptr->_staff()){
    if(q->nptr->voice_abs()==0 && r->nptr->voice_abs()==0){ // Primary-voice slurs.
     for(map<long, vector<bms_note*> >::iterator s=note_map.at.begin(); s!=note_map.at.end(); s++){
      if(root.time_range[0]<s->first && s->first<root.time_range[1]){
       for(vector<bms_note*>::iterator t=s->second.begin(); t!=s->second.end(); t++){
        if((*t)->_staff()==q->nptr->_staff() && (*t)->voice_abs()==0 && (*t)->regular()){
         for(trait.nptr=(*t)->next_prefix_grace(); trait.nptr!=NULL; trait.nptr=trait.nptr->next_prefix_grace()) slur_bridge.push_back(trait);
         trait.nptr=*t;
         slur_bridge.push_back(trait);
         for(trait.nptr=(*t)->next_suffix_grace(); trait.nptr!=NULL; trait.nptr=trait.nptr->next_suffix_grace()) slur_bridge.push_back(trait);
        }
       }
      }
     }
    }
    else{
     vector<bms_note*>::iterator t=note_map.at[root.time_range[0]].begin();
     bool secondary=true;
     do{ // The moment is ensured to have at least a note.
      if((*t)->_staff()==q->nptr->_staff()) secondary=((*t)->voice_abs()<=q->nptr->voice_abs());
     }while(secondary && ++t!=note_map.at[root.time_range[0]].end());
     if(secondary){ // The start of the slur passes secondary-test.
      t=note_map.at[root.time_range[1]].begin();
      do{ // The moment is ensured to have at least a note.
       if((*t)->_staff()==r->nptr->_staff()) secondary=((*t)->voice_abs()<=r->nptr->voice_abs());
      }while(secondary && ++t!=note_map.at[root.time_range[1]].end());
     }
     if(secondary){ // Secondary-voice slurs.
      unsigned short voice=0;
      for(map<long, vector<bms_note*> >::iterator s=note_map.at.begin(); s!=note_map.at.end(); s++){
       if(root.time_range[0]<s->first && s->first<root.time_range[1]){
        vector<bms_note*> v;
        v.reserve(s->second.size());
        voice=0;
        for(t=s->second.begin(); t!=s->second.end(); t++){
         if((*t)->_staff()==q->nptr->_staff() && (*t)->regular()){
          if(voice<(*t)->voice_abs()) voice=(*t)->voice_abs();
          v.push_back(*t); // Collection of notes of the same staff.
         }
        }
        for(t=v.begin(); t!=v.end(); t++){
         if((*t)->voice_abs()==voice){
          for(vector<bms_note*>::iterator u=t-(*t)->prefix(); u!=t; u++){
           trait.nptr=*u;
           slur_bridge.push_back(trait);
          }
          trait.nptr=*t;
          slur_bridge.push_back(trait);
          for(vector<bms_note*>::iterator u=t+(*t)->chord()+1, v=u+(*t)->suffix(); u!=v; u++){
           trait.nptr=*u;
           slur_bridge.push_back(trait);
          }
         }
        }
       }
      }
     }
     else{ // Cross-voice slurs.
     }
    }
   }
   else{ // Cross-staff slurs.
   }
   if(!slur_bridge.empty()){
    ptrdiff_t x=p->path.end()-r;
    p->path.insert(r, slur_bridge.begin(), slur_bridge.end());
    r=p->path.end()-x;
/*
//if(p->path[0].nptr->_measure()==22){
FILE* h=fopen("h.txt", "a");
for(int i=0; i<p->path.size(); i++){
fprintf(h, "%d %d %d %d\n", p->path[i].nptr->_measure(), p->path[i].nptr->_staff(), p->path[i].nptr->midi_pitch(), p->path[i].nptr->type());
}
fputs("$$\n", h);
fclose(h);
//}
*/
   }
  }
  for(vector<slur_trait_t>::iterator q=p->path.begin(); q!=p->path.end(); q++){
   q->nptr->increase_slur_number(q->type);
   if((p->attribute_flag&1)|| p->path.size()<=4){ // Short slurs.
    vector<slur_trait_t>::iterator r=q-1, s=q+1;
    if(q!=p->path.begin()){ // r is valid.
     if(q->nptr->_staff()!=r->nptr->_staff()) q->nptr->set_short_slur_type(SLUR_CROSS_STAFF, true);
     else if(q->nptr->voice_abs()!=r->nptr->voice_abs()) q->nptr->set_short_slur_type(SLUR_CROSS_VOICE, true);
    }
    if(s!=p->path.end()){
     if(q->nptr->_staff()!=s->nptr->_staff()) q->nptr->set_short_slur_type(SLUR_CROSS_STAFF);
     else if(q->nptr->voice_abs()!=s->nptr->voice_abs()) q->nptr->set_short_slur_type(SLUR_CROSS_VOICE);
     else q->nptr->set_short_slur_type(SLUR_GENERAL);
     if(s->nptr->grace()) q->nptr->set_short_slur_attribute(SLUR_GRACE);
    }
    if(q->nptr->grace()) q->nptr->set_short_slur_attribute(SLUR_GRACE);
    if(p->attribute_flag&2){
     q->nptr->set_short_slur_attribute(SLUR_DISCONTINUOUS);
     p->attribute_flag&=~2;
    }
   }
   else{ // Long slurs.
    switch(q->type){
     case TYPE_START: q->nptr->set_long_slur_type(SLUR_START_LONG, false); break;
     case TYPE_STOP: q->nptr->set_long_slur_type(SLUR_STOP_LONG, false); break;
    }
   }
  }
 }
 slur_record.clear();
}
void XMLAnalyzer::analyze_tied() throw(){
 if(tied_record.empty()) return;
 for(vector<tied_record_t>::reverse_iterator p=tied_record.rbegin(); p!=tied_record.rend(); p++){
  if(p->type==TYPE_STOP){ // To find in-staff pairs.
   vector<tied_record_t>::reverse_iterator q=p+1;
   for(; q!=tied_record.rend(); q++) if(q->staff==p->staff && q->pitch==p->pitch && q->type==TYPE_START) break;
   if(q==tied_record.rend()) p->state=TIED_STATE_SINGLE; // There is no start at the same staff.
   else p->state=q->state=TIED_STATE_DOUBLE;
  }
 }
 for(vector<tied_record_t>::reverse_iterator p=tied_record.rbegin(); p!=tied_record.rend(); p++){
  if(p->state==TIED_STATE_SINGLE && p->type==TYPE_STOP){
   vector<tied_record_t>::reverse_iterator q=p+1;
   for(; q!=tied_record.rend(); q++) if(q->state==TIED_STATE_UNKNOWN && q->pitch==p->pitch && q->type==TYPE_START) break;
   if(q==tied_record.rend()) p->state=TIED_STATE_UNKNOWN; // There is no matching tied-start.
   else p->state=q->state=TIED_STATE_DOUBLE;
  } // To find inter-staff pairs.
 }
 for(vector<tied_record_t>::iterator p=tied_record.begin(); p!=tied_record.end(); p++){
  if(p->state==TIED_STATE_UNKNOWN && p->type==TYPE_START){
   unsigned long t=note_map.time[p->nptr]+p->nptr->duration();
   for(vector<bms_note*>::iterator q=note_map.at[t].begin(); q!=note_map.at[t].end(); q++){
    if(*q!=p->nptr && (*q)->midi_pitch()==p->nptr->midi_pitch()){
     (*q)->set_tied_type(TYPE_STOP);
     p->state=TIED_STATE_SINGLE;
     break;
    }
   } // To find implicit pairs (start only).
  }
 }
 for(vector<tied_record_t>::iterator p=tied_record.begin(); p!=tied_record.end(); p++) if(p->state!=TIED_STATE_UNKNOWN) p->nptr->set_tied_type(p->type);
 tied_record.clear();
}
void XMLAnalyzer::analyze_tuplet() throw(){
}
void XMLAnalyzer::clear_voice_tree(voice_tree_node* node) throw(){
 for(vector<voice_tree_node*>::iterator p=node->child.begin(); p!=node->child.end(); p++){
  clear_voice_tree(*p);
  delete *p;
 }
 node->child.clear();
 node->vertex.clear();
}
vector<XMLAnalyzer::music_event>::iterator XMLAnalyzer::find_event(const bms_note* p) throw(){
 vector<music_event>::iterator answer=buffer.event.begin();
 for(; answer!=buffer.event.end() && answer->second.as_note()!=p; answer++);
 return answer;
}
bool XMLAnalyzer::find_path(list<note_vertex*>& path, note_vertex* v, long t) throw(){
 path.push_back(v);
 if(v->next.empty()) v->visited=((v->time+v->note->duration()==t)||(t==0));
 else{
  for(list<note_vertex*>::iterator p=v->next.begin(); !v->visited && p!=v->next.end(); p++) if(!(*p)->visited) v->visited=find_path(path, *p, t);
  if(t==0) v->visited=true;
 }
 if(!v->visited) path.pop_back();
 return v->visited;
}
void XMLAnalyzer::parallel_cut(voice_tree_node* node) throw(){
 list<note_vertex*> head, path[2];
 if(node==&root){ // Notes may not fill entire duration at the 1st call.
  node->time_range[1]=(node->time_range[0]=node->vertex.front().time)+node->vertex.front().note->duration();
  for(vector<note_vertex>::iterator p=node->vertex.begin(); p!=node->vertex.end(); p++){
   long end_time=p->time+p->note->duration();
   if(end_time>node->time_range[1]) node->time_range[1]=end_time;
   if(p->time<=node->time_range[0]){
    if(p->time<node->time_range[0]){
     head.clear();
     node->time_range[0]=p->time;
    }
    head.push_back(&*p); // Head selection.
   }
  }
 }
 else for(vector<note_vertex>::iterator p=node->vertex.begin(); p!=node->vertex.end(); p++) if(p->time==node->time_range[0]) head.push_back(&*p);
 for(vector<note_vertex>::iterator p=node->vertex.begin(); p!=node->vertex.end(); p++){
  p->visited=false;
  p->next.clear();
  for(vector<note_vertex>::iterator q=node->vertex.begin(); q!=node->vertex.end(); q++) if(q->time==p->time+p->note->duration()) p->next.push_back(&*q);
  p->next.sort(node->downward_interval ? upper_than : lower_than);
 } // Construction of adjacency relation.
 if(head.size()>1) head.sort(node->downward_interval ? upper_than : lower_than);
 if(find_path(path[0], head.front(), node->time_range[1])){ // The primary melody.
  node->child.push_back(new voice_tree_node);
  for(list<note_vertex*>::iterator p=path[0].begin(); p!=path[0].end(); p++) node->child.back()->vertex.push_back(**p);
  memcpy(node->child.back()->time_range, node->time_range, sizeof(node->time_range));
  node->child.back()->downward_interval=node->downward_interval;
  path[0].clear();
 }
 if(head.size()>1){ // The secondary melody.
  for(vector<note_vertex>::iterator p=node->vertex.begin(); p!=node->vertex.end(); p++) if(!p->visited) p->next.reverse();
  find_path(path[0], head.back(), node->time_range[1]);
  if(head.size()>2){ // Other melodies.
   list<note_vertex*>::iterator r[2]={head.begin(), head.end()};
   r[0]++, r[1]--;
   for(vector<note_vertex>::iterator p=node->vertex.begin(); p!=node->vertex.end(); p++) if(!p->visited) p->next.reverse();
   for(; r[0]!=r[1]; r[0]++){
    if(find_path(path[1], *r[0], node->time_range[1])){
     node->child.push_back(new voice_tree_node);
     for(list<note_vertex*>::iterator p=path[1].begin(); p!=path[1].end(); p++) node->child.back()->vertex.push_back(**p);
     memcpy(node->child.back()->time_range, node->time_range, sizeof(node->time_range));
     node->child.back()->downward_interval=node->downward_interval;
     path[1].clear();
    }
   }
  }
  if(!path[0].empty()){ // The path is found.
   node->child.push_back(new voice_tree_node);
   for(list<note_vertex*>::iterator p=path[0].begin(); p!=path[0].end(); p++) node->child.back()->vertex.push_back(**p);
   memcpy(node->child.back()->time_range, node->time_range, sizeof(node->time_range));
   node->child.back()->downward_interval=node->downward_interval;
   path[0].clear();
  }
 }
 if(node==&root){ // The 1st parallel cut.
  list<voice_tree_node**> complex_voice;
  for(vector<note_vertex>::iterator p=node->vertex.begin(); p!=node->vertex.end(); p++) if(!p->visited) path[0].push_back(&*p);
  if(!path[0].empty()){
   bool compatible[2]={false, false};
   if(head.size()==2 && node->downward_interval) for(list<note_vertex*>::iterator p=path[0].begin(); p!=path[0].end(); p++) (*p)->next.reverse();
   do{
    note_vertex* s=path[0].front(); // The "earliest" note in path[0].
    for(list<note_vertex*>::iterator p=path[0].begin(); p!=path[0].end(); p++) if((*p)->time<s->time) s=*p;
    path[1].clear();
    find_path(path[1], s); // FIND anyway.
    for(vector<voice_tree_node*>::reverse_iterator p=node->child.rbegin(); p!=node->child.rend(); p++){
     compatible[0]=compatible[1]=false;
     for(vector<note_vertex>::iterator q=(*p)->vertex.begin(); q!=(*p)->vertex.end(); q++){
      if(path[1].front()->time==q->time) compatible[0]=true;
      if(path[1].back()->time+path[1].back()->note->duration()==q->time+q->note->duration()) compatible[1]=true;
     }
     if(compatible[0] && compatible[1]){
      for(list<note_vertex*>::iterator q=path[1].begin(); q!=path[1].end(); q++) (*p)->vertex.push_back(**q);
      complex_voice.push_back(&*p);
      break;
     }
    }
    if(!compatible[0] || !compatible[1]){ // Rest-note padding.
    }
    for(list<note_vertex*>::iterator p=path[0].begin(); p!=path[0].end();){
     if((*p)->visited) p=path[0].erase(p);
     else p++;
    }
   }while(!path[0].empty());
  }
  if(!complex_voice.empty()){ // Vertical (timewise) cut is needed.
   complex_voice.sort();
   complex_voice.unique();
   for(list<voice_tree_node**>::iterator p=complex_voice.begin(); p!=complex_voice.end(); p++) vertical_cut(**p);
  }
 }
 else{ // Tree of in-accords cannot grow deeper.
  for(vector<note_vertex>::iterator p=node->vertex.begin(); p!=node->vertex.end(); p++) if(!p->visited) path[1].push_back(&*p);
  path[1].sort(node->downward_interval ? upper_than : lower_than);
 }
}
void XMLAnalyzer::parse(Sxmlelement& Sxmlelt) throw(runtime_error){
 xml_iter=Sxmlelt->begin();
 map<string, const bms_note*> tuplet_start;
 map<size_t, size_t> wedge_map;
 string value; // A buffer storing the value of either a symbol or an attribute.
 size_t subtree_size=0; // Size of the subtree of a nested symbol.
 int element_type=0;
 bool attribute_change=false, grace_steal_previous=false;
 measure_begin=measure_end=time_axis=0;
 score->set_page(0, "");
 for(xml_iter=Sxmlelt->begin(); xml_iter!=Sxmlelt->end(); xml_iter++){ // Tree traversal.
  if(*xml_iter){
   element_type=(*xml_iter)->getType();
   subtree_size=(*xml_iter)->size();     //此標籤下的 subtree 數量
   if(element_type==k_work_number) score->set_work_number((*xml_iter)->getValue());
   else if(element_type==k_work_title) score->set_work_title((*xml_iter)->getValue());
   else if(element_type==k_creator){
    value=(*xml_iter)->getAttributeValue("type");
    if(value.empty() || value=="composer") score->set_composer((*xml_iter)->getValue(), false);
    else if(value=="lyricist") score->set_lyricist((*xml_iter)->getValue(), false);
    else if(value=="arranger") score->set_arranger((*xml_iter)->getValue(), false);
   }
   else if(element_type==k_score_part){
    score->score.resize(score->score.size()+1);
    score->score.back().set_id((*xml_iter)->getAttributeValue("id"));
   }
   else if(element_type==k_part_name) score->score.back().set_name((*xml_iter)->getValue());
   else if(element_type==k_part_abbreviation) score->score.back().set_abbreviation((*xml_iter)->getValue());
   else if(element_type==k_score_instrument){
    value=(*xml_iter)->getAttributeValue("id");
    if(instrument.id.count(value)==0) instrument.id[value]=pair<int, size_t>(position._part(), score->add_instrument());
    if(instrument.part.count(score->score.size())==0) instrument.part[score->score.size()]=instrument.id[value].second;
   }
   else if(element_type==k_instrument_name) score->instrument[instrument.id[value].second].name=(*xml_iter)->getValue();
   else if(element_type==k_instrument_abbreviation) score->instrument[instrument.id[value].second].abbreviation=(*xml_iter)->getValue();
   else if(element_type==k_midi_instrument){
    value=(*xml_iter)->getAttributeValue("id");
    if(instrument.id.count(value)==0) instrument.id[value]=pair<int, size_t>(position._part(), score->add_instrument());
    if(instrument.part.count(score->score.size())==0) instrument.part[score->score.size()]=instrument.id[value].second;
   }
   else if(element_type==k_midi_channel){
    int channel=atoi((*xml_iter)->getValue().c_str());
    if(1<=channel && channel<=16) score->instrument[instrument.id[value].second].midi.channel=channel;
   }
   else if(element_type==k_midi_program){
    int m_program=atoi((*xml_iter)->getValue().c_str());
    score->instrument[instrument.id[value].second].midi.program=m_program;
   }
   else if(element_type==k_midi_unpitched) score->instrument[instrument.id[value].second].midi.unpitched=atoi((*xml_iter)->getValue().c_str());
   else if(element_type==k_volume){
    double v=atof((*xml_iter)->getValue().c_str());
    score->instrument[instrument.id[value].second].midi.volume=(unsigned char)round(1.27*v);
   }
   else if(element_type==k_pan){
    int pan=atoi((*xml_iter)->getValue().c_str());
    score->instrument[instrument.id[value].second].midi.pan=pan;
   }
   else if(element_type==k_elevation){
    //int elevation=atoi((*xml_iter)->getValue().c_str());
   }
   else if(element_type==k_part){
    if(position._part()>0){
     analyze(score->part(position._part()));
     prepare_note_map();
     analyze_skills(score->part(position._part()).size());
     analyze_slur();
     analyze_tied();
     analyze_midi(midi_part.back(), score->part(position._part()).size());
     wedge_map.clear();
    }
    position.set_measure(0);
    score->part(position.increase_part()).resize(1);
    buffer.direction.set_data(score->part(position._part()).element.direction);
    buffer.note.set_data(score->part(position._part()).element.note);
    buffer.voice=&score->part(position._part()).voice;
    buffer.event.clear();
    measure_begin=measure_end=time_axis=0; // The time is counted from 0.
    midi_part.resize(midi_part.size()+1);
    attribute_change=grace_steal_previous=false;
   }
   else if(element_type==k_measure){
    if(position._measure()>0) analyze(score->part(position._part()));
    position.increase_measure();
    midi_part.back().add_boundary(measure_begin=time_axis=measure_end);
   }
   else if(element_type==k_print){
    if((*xml_iter)->getAttributeValue("new-page")=="yes") score->set_page(score->part(position._part()).front().size(), (*xml_iter)->getAttributeValue("page-number"));
   }
   else if(element_type==k_sound) parse_sound(position._part(), 0);
   else if(element_type==k_attributes){
    list<bms_attributes> data;
    for(size_t i=0; i<subtree_size; i++){
     xml_iter++; // Global iterator.
     switch((*xml_iter)->getType()){
      case k_divisions: // Duration of a quarter note.
       midi_part.back().set_divisions(divisions=atol((*xml_iter)->getValue().c_str()));
      break;
      case k_key:
       data.push_back(bms_attributes(position));
       data.back().initialize_key();
       data.back().set_staff((*xml_iter)->getAttributeIntValue("number", 0));
       data.back().set_change(attribute_change);
      break;
      case k_cancel:
       data.back().set_cancel(atoi((*xml_iter)->getValue().c_str()));
      break;
      case k_fifths:
       data.back().set_fifths(atoi((*xml_iter)->getValue().c_str()));
       set_fifths(data.back().fifths());
      break;
      case k_mode:
       data.back().set_mode(encode_mode((*xml_iter)->getValue().c_str()));
      break;
      case k_time:
       data.push_back(bms_attributes(position));
       data.back().initialize_time();
       data.back().set_staff((*xml_iter)->getAttributeIntValue("number", 0));
       data.back().set_change(attribute_change);
       value=(*xml_iter)->getAttributeValue("symbol");
       if(value=="common") data.back().set_symbol(bms_attributes::common);
       else if(value=="cut") data.back().set_symbol(bms_attributes::cut);
       else if(value=="single-number") data.back().set_symbol(bms_attributes::single_number);
       else if(value=="normal") data.back().set_symbol(bms_attributes::normal);
      break;
      case k_beats:
       value=(*xml_iter)->getValue();
       data.back().set_beats(value.empty() ? 0 : atoi(value.c_str()));
      break;
      case k_beat_type:
       value=(*xml_iter)->getValue();
       data.back().set_beat_type(value.empty() ? 0 : atoi(value.c_str()));
      break;
      case k_staves:
       score->part(position._part()).resize(atoi((*xml_iter)->getValue().c_str()));
      break;
      case k_clef:
       data.push_back(bms_attributes(position));
       data.back().initialize_clef();
       data.back().set_staff((*xml_iter)->getAttributeIntValue("number", 1));
       data.back().set_change(attribute_change);
      break;
      case k_sign:
       data.back().set_sign(encode_sign((*xml_iter)->getValue()));
      break;
      case k_line:
       data.back().set_line(atoi((*xml_iter)->getValue().c_str()));
      break;
      case k_clef_octave_change:
       data.back().set_clef_octave_change(atoi((*xml_iter)->getValue().c_str()));
      break;
     }
     subtree_size+=(*xml_iter)->size();
    }
    data.sort();
    for(list<bms_attributes>::iterator p=data.begin(); p!=data.end(); p++){
     if(p->kind()==bms_attributes::clef && !attribute_change){
      if(score->part(p->_part()).size()>1) score->part(p->_part()).staff(p->_staff()).upmost_pitch=(p->_staff()==1);
      else score->part(p->_part()).staff(p->_staff()).upmost_pitch=default_pitch_direction(p->sign(), p->line());
     }
     if(!p->empty()) buffer.attributes.push_back(*p, time_axis);
    }
    attribute_change=true; // Next attribute at the same part will be a change.
   }
   else if(element_type==k_note){
    bms_note* new_node=new bms_note(position);
    bool delay=true;
    new_node->set_staff(1);
    new_node->set_instrument(instrument.part[new_node->_part()]);
    value=(*xml_iter)->getAttributeValue("attack");
    if(!value.empty()) new_node->set_attack(atol(value.c_str()));
    value=(*xml_iter)->getAttributeValue("release");
    if(!value.empty()) new_node->set_release(atol(value.c_str()));
    if(!(value=(*xml_iter)->getAttributeValue("dynamics")).empty()){
     int d=(int)round(atoi(value.c_str())*0.9);
     new_node->set_dynamics((unsigned char)d);
    }
    if(!(value=(*xml_iter)->getAttributeValue("end_dynamics")).empty()){
     int d=(int)round(atoi(value.c_str())*0.9);
     new_node->set_end_dynamics((unsigned char)d);
    }
    new_node->set_pizzicato((*xml_iter)->getAttributeValue("pizzicato")=="yes");
    for(size_t i=0; i<subtree_size; i++){ // To get all information about the note.
     xml_iter++; // Global iterator
     switch((*xml_iter)->getType()){
      case k_grace: // This note is a grace note.
       new_node->set_grace(true);
       value=(*xml_iter)->getAttributeValue("slash");
       new_node->set_slash(value=="yes" ? 1 : 0);
       value=(*xml_iter)->getAttributeValue("steal-time-previous");
       if(!value.empty()) new_node->set_steal_time_previous(atoi(value.c_str()));
       value=(*xml_iter)->getAttributeValue("steal-time-following");
       if(!value.empty()) new_node->set_steal_time_following(atoi(value.c_str()));
       value=(*xml_iter)->getAttributeValue("make-time");
       if(!value.empty()) new_node->set_make_time(atoi(value.c_str()));
       delay=false;
      break;
      case k_chord: // This note is part of a chord.
       new_node->set_chord(-1); // A temporary negative value.
       delay=false;
      break;
      case k_rest:
       new_node->set_rest(true);
      break;
      case k_unpitched: // Initialization of an unpitched note.
       new_node->set_step(6); // Step B.
       new_node->set_octave(4);
      break;
      case k_display_step:
      case k_step: // Step, either pitched or rest/unpitched (display-step).
       new_node->set_step(step_number((*xml_iter)->getValue().at(0)));
      break;
      case k_alter: // A half-step alternation.
       new_node->set_alter(atoi((*xml_iter)->getValue().c_str()));
      break;
      case k_display_octave:
      case k_octave: // Octave either pitched or rest/unpitched (display-octave).
       new_node->set_octave(atoi((*xml_iter)->getValue().c_str()));
      break;
      case k_duration: // Actual playing time in divisions.
       new_node->set_duration(atol((*xml_iter)->getValue().c_str()));
      break;
      case k_type:
       new_node->set_type(encode_note_type((*xml_iter)->getValue()));
      break;
      case k_dot:
       new_node->increase_dot();
      break;
      case k_voice: // MIDI track number.
       new_node->set_voice(atoi((*xml_iter)->getValue().c_str()));
      break;
      case k_instrument:
       value=(*xml_iter)->getAttributeValue("id");
       new_node->set_instrument(instrument.id[value].second);
      break;
      case k_stem:
       value=(*xml_iter)->getValue();
       if(value=="up") new_node->set_stem(STEM_UP);
       else if(value=="down") new_node->set_stem(STEM_DOWN);
       else if(value=="double") new_node->set_stem(STEM_DOUBLE);
       else if(value=="none") new_node->set_stem(STEM_NONE);
      break;
      case k_notehead:
       value=(*xml_iter)->getValue();
       if(value=="x") new_node->set_stem(NOTEHEAD_X);
       else if(value=="slash") new_node->set_stem(NOTEHEAD_SLASH);
       else if(value=="diamond") new_node->set_stem(NOTEHEAD_DIAMOND);
       else if(value=="triangle") new_node->set_stem(NOTEHEAD_TRIANGLE);
       else if(value=="inverted triangle") new_node->set_stem(NOTEHEAD_INVERTED_TRIANGLE);
       else if(value=="square") new_node->set_stem(NOTEHEAD_SQUARE);
       else if(value=="circle-x") new_node->set_stem(NOTEHEAD_CIRCLE_X);
       else if(value=="slashed") new_node->set_stem(NOTEHEAD_SLASHED);
       else if(value=="back slashed") new_node->set_stem(NOTEHEAD_BACK_SLASHED);
       else if(value=="cross") new_node->set_stem(NOTEHEAD_CROSS);
       else if(value=="cluster") new_node->set_stem(NOTEHEAD_CLUSTER);
       else if(value=="arrow up") new_node->set_stem(NOTEHEAD_ARROW_UP);
       else if(value=="arrow down") new_node->set_stem(NOTEHEAD_ARROW_DOWN);
       else if(value=="none") new_node->set_stem(NOTEHEAD_NONE);
       else if(value=="normal") new_node->set_stem(NOTEHEAD_NORMAL);
      break;
      case k_staff: // 如果有兩個以上的staff才會出現的標籤,staff初始化為1
       new_node->set_staff(atoi((*xml_iter)->getValue().c_str()));
      break;
      case k_beam:
      break;
      case k_actual_notes:
       value="1";
      goto tuplet_actual; // The two elements offer the same information.
      case k_normal_notes:
       value="1";
      goto tuplet_normal; // The two elements offer the same information.
      case k_slur:
       value=(*xml_iter)->getAttributeValue("type");
       if(!value.empty()){ // To make a log of the slur.
        int t=encode_type(value);
        value=(*xml_iter)->getAttributeValue("number");
        if(value.empty()) value="1"; // Default number.
        if(t==TYPE_START){
         slur_map[value]=slur_record.size();
         slur_record.resize(slur_record.size()+1);
         slur_record.back().attribute_flag=0;
         slur_record.back().number=atoi(value.c_str());
        }
        if(slur_map.count(value)>0){
         slur_record[slur_map[value]].path.resize(slur_record[slur_map[value]].path.size()+1);
         slur_record[slur_map[value]].path.back().type=t;
         slur_record[slur_map[value]].path.back().nptr=new_node;
         if(t==TYPE_STOP){
          slur_map.erase(value);
          grace_steal_previous=true;
         }
        }
       }
      break;
      case k_tied:
       value=(*xml_iter)->getAttributeValue("type");
       if(!value.empty()){ // To make a log of the tied.
        tied_record.resize(tied_record.size()+1);
        tied_record.back().type=encode_type(value);
        tied_record.back().number=(*xml_iter)->getAttributeIntValue("number", 0);
        tied_record.back().nptr=new_node;
        tied_record.back().pitch=new_node->midi_pitch();
        tied_record.back().staff=new_node->_staff();
        tied_record.back().state=TIED_STATE_UNKNOWN;
       }
      break;
      case k_tuplet:
       value=(*xml_iter)->getAttributeValue("number");
       if(value.empty()) value="1";
       new_node->set_tuplet_type(atoi(value.c_str())-1, encode_type((*xml_iter)->getAttributeValue("type")));
       if(tuplet_start.count(value)>0){
        size_t n=atoi(value.c_str())-1;
        new_node->set_tuplet_number(n, tuplet_start[value]->tuplet_number(n, false), false);
        new_node->set_tuplet_number(n, tuplet_start[value]->tuplet_number(n, true), true);
        tuplet_start.erase(value);
       }
       else tuplet_start[value]=new_node;
      break;
      case k_tuplet_number:
       if(xml_iter.getParent()->getType()==k_tuplet_actual){
        tuplet_actual:
        new_node->set_tuplet_number(atoi(value.c_str())-1, atoi((*xml_iter)->getValue().c_str()), false);
       }
       else{
        tuplet_normal:
        new_node->set_tuplet_number(atoi(value.c_str())-1, atoi((*xml_iter)->getValue().c_str()), true);
       }
      break;
      case k_arpeggiate:
       new_node->set_arpeggiate_number((*xml_iter)->getAttributeIntValue("number", 1));
       value=(*xml_iter)->getAttributeValue("direction");
       if(value=="down") new_node->set_arpeggiate_kind(1);
      break;
      case k_mordent:
       new_node->set_trill_kind(4);
       value=(*xml_iter)->getAttributeValue("long");
       if(value=="yes") new_node->set_trill_kind(6);
      goto trill_data;
      case k_inverted_mordent:
       new_node->set_trill_kind(5);
       value=(*xml_iter)->getAttributeValue("long");
       if(value=="yes") new_node->set_trill_kind(7);
      goto trill_data;
      case k_trill_mark: new_node->set_trill_kind(1); goto trill_data;
      case k_turn: new_node->set_trill_kind(12); goto trill_data;
      case k_delayed_turn: new_node->set_trill_kind(14); goto trill_data;
      case k_inverted_turn: new_node->set_trill_kind(13); goto trill_data;
      case k_delayed_inverted_turn: new_node->set_trill_kind(15); goto trill_data;
      trill_data: // Attributes of entity trill-sound.
       value=(*xml_iter)->getAttributeValue("start-note");
       new_node->set_trill_start_note(encode_start_note(value));
       value=(*xml_iter)->getAttributeValue("trill-step");
       new_node->set_trill_step(encode_trill_step(value));
       value=(*xml_iter)->getAttributeValue("two-note-turn");
       new_node->set_trill_two_note_turn(encode_two_note_turn(value));
       value=(*xml_iter)->getAttributeValue("accelerate");
       new_node->set_trill_accelerate(value=="yes");
       value=(*xml_iter)->getAttributeValue("beats");
       new_node->set_trill_beats(value.empty() ? 0 : atoi(value.c_str()));
       value=(*xml_iter)->getAttributeValue("second-beat");
       if(!value.empty()) new_node->set_trill_second_beat(atoi(value.c_str()));
       value=(*xml_iter)->getAttributeValue("last-beat");
       if(!value.empty()) new_node->set_trill_last_beat(atoi(value.c_str()));
      break;
      case k_accidental_mark:
      if(xml_iter.getParent()->getType()==k_ornaments){
       value=(*xml_iter)->getAttributeValue("placement");
       if(value=="above"){
        int a=encode_accidental((*xml_iter)->getValue());
        if(a<11){ // Valid accidental-mark value.
         if(note_map.uncorrected_trill.count(new_node)>0){
          new_node->set_accidental_mark_below(new_node->accidental_mark_above());
          new_node->set_accidental_mark_above(a);
          note_map.uncorrected_trill.erase(new_node);
         }
         else if(new_node->accidental_mark_above()>=11) new_node->set_accidental_mark_above(a);
        }
       }
       else if(value=="below"){
        int a=encode_accidental((*xml_iter)->getValue());
        if(a<11){ // Valid accidental-mark value.
         if(note_map.uncorrected_trill.count(new_node)>0){
          new_node->set_accidental_mark_above(new_node->accidental_mark_below());
          new_node->set_accidental_mark_below(a);
          note_map.uncorrected_trill.erase(new_node);
         }
         else if(new_node->accidental_mark_below()>=11) new_node->set_accidental_mark_below(a);
        }
       }
       else{ // Placement is not given.
        int a=encode_accidental((*xml_iter)->getValue());
        if(a<11){ // Valid accidental-mark value.
         if(new_node->accidental_mark_above()>=11){
          new_node->set_accidental_mark_above(a);
          if(new_node->accidental_mark_below()>=11) note_map.uncorrected_trill.insert(new_node);
         }
         else if(new_node->accidental_mark_below()>=11) new_node->set_accidental_mark_below(a);
        }
       }
       break;
      }
      else{ // A non-ornament accidental-mark is the same as an accidental.
      case k_accidental:
       new_node->set_accidental(encode_accidental((*xml_iter)->getValue()));
       break;
      }
      case k_fingering:
       value=(*xml_iter)->getAttributeValue("substitution");
       if(value=="yes") new_node->set_fingering(2, atoi((*xml_iter)->getValue().c_str()));
       else{
        value=(*xml_iter)->getAttributeValue("alternate");
        if(value=="yes") new_node->set_fingering(1, atoi((*xml_iter)->getValue().c_str()));
        else new_node->set_fingering(0, atoi((*xml_iter)->getValue().c_str()));
       }
      break;
      case k_accent:
       new_node->set_articulation_flag(bms_note::note_accent);
      break;
      case k_strong_accent:
       new_node->set_articulation_flag(bms_note::note_strong_accent);
      break;
      case k_staccato:
       new_node->set_articulation_flag(bms_note::note_staccato);
      break;
      case k_staccatissimo:
       new_node->set_articulation_flag(bms_note::note_staccatissimo);
      break;
      case k_spiccato:
       new_node->set_articulation_flag(bms_note::note_spiccato);
      break;
      case k_tenuto:
       new_node->set_articulation_flag(bms_note::note_tenuto);
      break;
      case k_detached_legato:
       new_node->set_articulation_flag(bms_note::note_detached_legato);
      break;
      case k_up_bow:
       new_node->set_up_bow(true);
      break;
      case k_down_bow:
       new_node->set_down_bow(true);
      break;
      case k_fermata:
       value=(*xml_iter)->getValue();
       if(value.empty()) new_node->set_fermata(0);
       else new_node->set_fermata(value=="angled" ? 1 : value=="square" ? 2 : 0);
      break;
      case k_lyric:
       score->part(new_node->_part()).set_song_music();
      break;
      case k_syllabic:
       value=(*xml_iter)->getValue();
       if(value=="single") new_node->set_syllabic(1);
       else if(value=="begin") new_node->set_syllabic(2);
       else if(value=="middle") new_node->set_syllabic(3);
       else if(value=="end") new_node->set_syllabic(4);
      break;
      case k_text:
       new_node->set_lyric((*xml_iter)->getValue());
      break;
     }
     subtree_size+=(*xml_iter)->size();
    }
    if(new_node->grace_not_set() && grace_steal_previous) new_node->set_steal_time_previous(101);
    for(size_t i=0; i<6; i++){
     if(new_node->tuplet_type(i)!=TYPE_UNKNOWN){
      if(new_node->tuplet_number(i, false)==0) new_node->set_tuplet_number(i, new_node->tuplet_number(0, false), false);
      if(new_node->tuplet_number(i, true)==0) new_node->set_tuplet_number(i, new_node->tuplet_number(0, true), true);
     }
    }
    if(new_node->empty()) delete new_node; // Empty notes should be cleared.
    add(new_node, time_axis);
    grace_steal_previous=(new_node->trill_kind()==1 ||(new_node->grace() && grace_steal_previous));
    if(delay){
     time_axis+=new_node->duration();
     if(measure_end<time_axis) measure_end=time_axis;
    }
   }
   else if(element_type==k_backup){
    long duration=0;
    for(size_t i=0; i<subtree_size; i++){
     xml_iter++; // Global iterator.
     if((*xml_iter)->getType()==k_duration) duration=atoi((*xml_iter)->getValue().c_str());
     subtree_size+=(*xml_iter)->size();
    }
    time_axis-=duration;
   }
   else if(element_type==k_forward){
    long forward_duration=0;
    for(size_t i=0; i<subtree_size; i++){
     xml_iter++; // Global iterator
     switch((*xml_iter)->getType()){
      case k_duration:
       forward_duration=atol((*xml_iter)->getValue().c_str());
      break;
      case k_voice: // The information is useless so far.
      break;
     }
     subtree_size+=(*xml_iter)->size();
    }
    time_axis+=forward_duration;
    if(measure_end<time_axis) measure_end=time_axis;
   }
   else if(element_type==k_barline){
    bms_barline new_node(position);
    for(size_t i=0; i<subtree_size; i++){
     xml_iter++; // Global iterator
     switch((*xml_iter)->getType()){
      case k_bar_style:
       new_node.set_bar_style(encode_bar_style((*xml_iter)->getValue()));
      break;
      case k_coda:
       new_node.set_notations(1);
      break;
      case k_ending:
       if((*xml_iter)->getAttributeValue("type")=="start"){ // Only the start is recorded.
        new_node.set_ending_number((*xml_iter)->getAttributeValue("number"));
        new_node.set_ending_text((*xml_iter)->getValue());
       }
      break;
      case k_fermata:
       new_node.set_notations(2);
      break;
      case k_repeat:
       value=(*xml_iter)->getAttributeValue("direction");
       new_node.set_notations(4);
       if(value=="forward") new_node.set_notations(8);
      break;
      case k_segno:
       new_node.set_notations(16);
      break;
     }
     subtree_size+=(*xml_iter)->size();
    }
    new_node.set_staff(0);
    if(!new_node.empty()) buffer.barline.push_back(new_node, time_axis);
   } // There is only one barline element for all staves.
   else if(element_type==k_direction){
    bms_direction new_node(position);
    long direction_offset=0, sound_offset=0; // Relative position of the direction in divisions.
    size_t original_dynamics_count=dynamics_list.size();
    value=(*xml_iter)->getAttributeValue("placement");
    new_node.set_above(value!="below");
    value=(*xml_iter)->getAttributeValue("directive");
    new_node.set_directive(value=="yes");
    new_node.set_staff(0);
    for(size_t i=0; i<subtree_size; i++){
     xml_iter++; // Global iterator
     switch((*xml_iter)->getType()){
      case k_metronome:
       new_node.set_parentheses(xml_iter->getAttributeValue("parentheses")=="yes");
      break;
      case k_beat_unit:
       new_node.set_beat_unit(encode_note_type(xml_iter->getValue()));
      break;
      case k_beat_unit_dot:
       new_node.increase_beat_unit_dot();
      break;
      case k_per_minute:
       new_node.set_per_minute(xml_iter->getValue());
      break;
      case k_dynamics:
       parse_dynamics((*xml_iter)->size(), new_node);
      break;
      case k_offset:
       direction_offset=atol((*xml_iter)->getValue().c_str());
       value=(*xml_iter)->getAttributeValue("sound");
       if(value=="yes") sound_offset=direction_offset;
      break;
      case k_pedal:
       new_node.set_line((*xml_iter)->getAttributeValue("line")=="yes");
       new_node.set_pedal_type(encode_type((*xml_iter)->getAttributeValue("type")));
      break;
      case k_sound:
       if(new_node.direction_kind()==bms_direction::pedal){
        size_t depth=encode_controller((*xml_iter)->getAttributeValue("damper-pedal"));
        if(depth==0 && new_node.type()==TYPE_START) depth=100;
        new_node.set_pedal_depth(depth);
       }
       parse_sound(new_node._part(), sound_offset);
      break;
      case k_staff:
       new_node.set_staff(atoi((*xml_iter)->getValue().c_str()));
      break;
      case k_wedge:
       new_node.set_wedge_number((*xml_iter)->getAttributeIntValue("number", 1));
       value=(*xml_iter)->getAttributeValue("type");
       if(value=="stop"){
        if(wedge_map.count(new_node.number())>0){
         switch(wedge_map[new_node.number()]){
          case TYPE_CRESCENDO_START: new_node.set_wedge_type(TYPE_CRESCENDO_STOP); break;
          case TYPE_DIMINUENDO_START: new_node.set_wedge_type(TYPE_DIMINUENDO_STOP); break;
         }
         wedge_map.erase(new_node.number());
        }
        else;
       }
       else if(value=="crescendo"){
        if(wedge_map.count(new_node.number())>0);
        new_node.set_wedge_type(wedge_map[new_node.number()]=TYPE_CRESCENDO_START);
       }
       else if(value=="diminuendo"){
        if(wedge_map.count(new_node.number())>0);
        new_node.set_wedge_type(wedge_map[new_node.number()]=TYPE_DIMINUENDO_START);
       }
      break;
      case k_dashes:
       new_node.set_dashes_number((*xml_iter)->getAttributeIntValue("number", 1));
       value=(*xml_iter)->getAttributeValue("type");
       new_node.set_dashes_type(encode_type(value));
      break;
      case k_words:
       new_node.set_text((*xml_iter)->getValue().c_str());
      break;
      case k_octave_shift:
       value=(*xml_iter)->getAttributeValue("number");
       new_node.set_shift_number(value.empty() ? 1 : atoi(value.c_str()));
       value=(*xml_iter)->getAttributeValue("size");
       if(!value.empty()) new_node.set_shift_size(atoi(value.c_str()));
       new_node.set_shift_type(encode_type((*xml_iter)->getAttributeValue("type")));
      break;
     }
     subtree_size+=(*xml_iter)->size();
    }
    if(new_node._staff()==0 && score->part(new_node._part()).size()==1) new_node.set_staff(1);
    if(new_node._staff()>0){ // This direction contains staff specification.
     if(new_node.direction_kind()!=bms_direction::unknown){
      if(original_dynamics_count<dynamics_list.size()){
       dynamics_list.back().above=new_node.above();
       dynamics_list.back().staff=new_node._staff();
      }
      if(!new_node.empty()) buffer.direction.push_back(new_node, time_axis+direction_offset);
     }
    }
   }
  }
 }
 analyze(score->part(position._part()));
 midi_part.back().add_boundary(measure_end);
 prepare_note_map();
 analyze_skills(score->part(position._part()).size());
 analyze_slur();
 analyze_tied();
 analyze_midi(midi_part.back(), score->part(position._part()).size());
 join_midi_parts(score->midi_stream, midi_part); // After the function midi_part is cleared.
}
void XMLAnalyzer::parse_dynamics(size_t element_count, bms_direction& new_node) throw(){
 for(; element_count>0; element_count--){
  xml_iter++; // Global iterator.
  switch((*xml_iter)->getType()){
   case k_p:
    new_node.set_text("p");
   break;
   case k_pp:
    new_node.set_text("pp");
   break;
   case k_ppp:
    new_node.set_text("ppp");
   break;
   case k_pppp:
    new_node.set_text("pppp");
   break;
   case k_ppppp:
    new_node.set_text("ppppp");
   break;
   case k_pppppp:
    new_node.set_text("pppppp");
   break;
   case k_f:
    new_node.set_text("f");
   break;
   case k_ff:
    new_node.set_text("ff");
   break;
   case k_fff:
    new_node.set_text("fff");
   break;
   case k_ffff:
    new_node.set_text("ffff");
   break;
   case k_fffff:
    new_node.set_text("fffff");
   break;
   case k_ffffff:
    new_node.set_text("ffffff");
   break;
   case k_mp:
    new_node.set_text("mp");
   break;
   case k_mf:
    new_node.set_text("mf");
   break;
   case k_sf:
    new_node.set_text("sf");
   break;
   case k_sfp:
    new_node.set_text("sfp");
   break;
   case k_sfpp:
    new_node.set_text("sfpp");
   break;
   case k_fp:
    new_node.set_text("fp");
   break;
   case k_rf:
    new_node.set_text("rf");
   break;
   case k_rfz:
    new_node.set_text("rfz");
   break;
   case k_sfz:
    new_node.set_text("sfz");
   break;
   case k_sffz:
    new_node.set_text("sffz");
   break;
   case k_fz:
    new_node.set_text("fz");
   break;
   case k_other_dynamics:
    new_node.set_text((*xml_iter)->getValue().c_str());
   break;
  }
 }
}
void XMLAnalyzer::parse_sound(int part, long offset) throw(){
 struct{string pan, elevation, damper_pedal, soft_pedal, sostenuto_pedal;} controller;
 vector<size_t> changed_instrument;
 string dynamics, tempo, value;
 size_t element_count=(*xml_iter)->size();
 int v=0;
 dynamics=(*xml_iter)->getAttributeValue("dynamics");
 tempo=(*xml_iter)->getAttributeValue("tempo");
 controller.damper_pedal=(*xml_iter)->getAttributeValue("damper-pedal");
 controller.soft_pedal=(*xml_iter)->getAttributeValue("soft-pedal");
 controller.sostenuto_pedal=(*xml_iter)->getAttributeValue("sostenuto-pedal");
 for(size_t i=0; i<element_count; i++){
  element_count+=(*++xml_iter)->size();
  switch((*xml_iter)->getType()){
   case k_midi_instrument:
    value=(*xml_iter)->getAttributeValue("id");
    if(!value.empty());
    else if(instrument.id[value].first!=part);
    else changed_instrument.push_back(instrument.id[value].second);
   break;
   case k_offset: // The sound offset element overrides the direction offset element.
    value=(*xml_iter)->getAttributeValue("id");
    if(!value.empty()) offset=atoi(value.c_str());
   break;
  }
 }
 if(changed_instrument.empty()) changed_instrument.push_back(instrument.part[part]);
 if(!dynamics.empty()){
  int d=(int)round(atof(dynamics.c_str())*0.9);
  d=bms_note::normalize_dynamics((unsigned char)d);
  dynamics_list.resize(dynamics_list.size()+1);
  dynamics_list.back().time=time_axis+offset;
  dynamics_list.back().value=(unsigned char)d;
  dynamics_list.back().above=false;
  dynamics_list.back().staff=0; // It is used to recognize single sound element.
 }
 if(!tempo.empty()){
  v=atoi(tempo.c_str());
  if(20<=v && v<=208) midi_part.back().set_tempo(time_axis+offset, v);
 }
 for(vector<size_t>::iterator p=changed_instrument.begin(); p!=changed_instrument.end(); p++){
  if(!controller.damper_pedal.empty()){
   if(controller.damper_pedal=="yes") v=127;
   else if(controller.damper_pedal=="no") v=0;
   else v=(int)round(atof(controller.damper_pedal.c_str())*1.27);
   midi_part.back().add_controller(score->midi_instrument(*p).channel-1, 64, v, time_axis+offset);
  }
  if(!controller.soft_pedal.empty()){
   if(controller.soft_pedal=="yes") v=127;
   else if(controller.soft_pedal=="no") v=0;
   else v=(int)round(atof(controller.soft_pedal.c_str())*1.27);
   midi_part.back().add_controller(score->midi_instrument(*p).channel-1, 67, v, time_axis+offset);
  }
  if(!controller.sostenuto_pedal.empty()){
   if(controller.sostenuto_pedal=="yes") v=127;
   else if(controller.sostenuto_pedal=="no") v=0;
   else v=(int)round(atof(controller.sostenuto_pedal.c_str())*1.27);
   midi_part.back().add_controller(score->midi_instrument(*p).channel-1, 66, v, time_axis+offset);
  }
 }
}
void XMLAnalyzer::prepare_note_map() throw(){
 note_map.at.clear();
 note_map.time.clear();
 for(vector<music_event>::iterator p=buffer.event.begin(); p!=buffer.event.end(); p++){
  if(p->second.is_note_pointer()){
   note_map.at[p->first].push_back(p->second.as_note());
   note_map.time[p->second.as_note()]=p->first;
  }
 }
}
void XMLAnalyzer::set_fifths(int fifths) throw(){
 this->fifths=fifths;
 if(fifths>0) for(int i=0, s=6; i<fifths; i++) alter[s=(s+4)%7]=1;
 else if(fifths<0) for(int i=0, s=3; i<-fifths; i++) alter[s=(s+3)%7]=-1;
 else for(int i=0; i<7; i++) alter[i]=0;
}
void XMLAnalyzer::vertical_cut(voice_tree_node* node) throw(){
 map<unsigned long, vector<note_vertex*> > moment;
 size_t current_count=0;
 for(vector<note_vertex>::iterator p=node->vertex.begin(); p!=node->vertex.end(); p++) moment[p->time].push_back(&*p);
 for(map<unsigned long, vector<note_vertex*> >::iterator p=moment.begin(); p!=moment.end(); p++){
  for(vector<note_vertex*>::iterator q=p->second.begin(); q!=p->second.end(); q++){
   if(*q!=NULL){ // The element is not previously appended by following loop.
    map<unsigned long, vector<note_vertex*> >::iterator r=p;
    for(r++; r!=moment.end(); r++) if(r->first<p->first+(*q)->note->duration()) r->second.push_back(NULL);
   }
  }
 }
 for(map<unsigned long, vector<note_vertex*> >::iterator p=moment.begin(); p!=moment.end(); p++){
  if(p->second.size()!=current_count){
   if(!node->child.empty()) node->child.back()->time_range[1]=p->first;
   node->child.push_back(new voice_tree_node);
   node->child.back()->time_range[0]=p->first;
   node->child.back()->downward_interval=node->downward_interval;
   current_count=p->second.size();
  }
  for(vector<note_vertex*>::iterator q=p->second.begin(); q!=p->second.end(); q++) if(*q!=NULL) node->child.back()->vertex.push_back(**q);
 }
 node->child.back()->time_range[1]=node->time_range[1];
 for(vector<voice_tree_node*>::iterator p=node->child.begin(); p!=node->child.end(); p++) parallel_cut(*p);
}
void XMLAnalyzer::write_into(vector<bms_measure_element>& m) throw(){
 m.resize(m.size()+1);
 m.back().set_voice_index(buffer.voice->size());
 buffer.voice->push_back(bms_note_area(root.time_range[1]-root.time_range[0]));
 for(vector<voice_tree_node*>::const_iterator p=root.child.begin(); p!=root.child.end(); p++){
  buffer.voice->back().resize(buffer.voice->back().size()+1);
  if((*p)->child.empty()){ // The measure was just cut once parallelly.
   buffer.voice->back().back().push_back(bms_in_accord_trait((*p)->time_range[1]-(*p)->time_range[0]));
   buffer.voice->back().back().back().resize(1);
   for(vector<note_vertex>::const_iterator s=(*p)->vertex.begin(); s!=(*p)->vertex.end(); s++){
    buffer.voice->back().back().back().back().resize(buffer.voice->back().back().back().back().size()+1);
    buffer.voice->back().back().back().back().back().set_note(s->note);
    s->note->set_voice(0, p-root.child.begin());
    s->note->set_voice(1, 0);
   }
  }
  else{ // The measure was parallelly and vertically cut.
   for(vector<voice_tree_node*>::const_iterator q=(*p)->child.begin(); q!=(*p)->child.end(); q++){
    buffer.voice->back().back().push_back(bms_in_accord_trait((*q)->time_range[1]-(*q)->time_range[0]));
    for(vector<voice_tree_node*>::const_iterator r=(*q)->child.begin(); r!=(*q)->child.end(); r++){
     buffer.voice->back().back().back().resize(buffer.voice->back().back().back().size()+1);
     for(vector<note_vertex>::const_iterator s=(*r)->vertex.begin(); s!=(*r)->vertex.end(); s++){
      buffer.voice->back().back().back().back().resize(buffer.voice->back().back().back().back().size()+1);
      buffer.voice->back().back().back().back().back().set_note(s->note);
      s->note->set_voice(0, p-root.child.begin());
      s->note->set_voice(1, r-(*q)->child.begin());
     }
    }
   }
  }
 }
 clear_voice_tree(&root);
}
bool XMLAnalyzer::read(const string& filename) throw(){
 xmlreader source_reader;
 score->source=source_reader.read(filename.c_str());
 if(score->source){ // Success.
  Sxmlelement Sxmlelt=score->source->elements();
  parse(Sxmlelt);
  return true;
 }
 else return false; // Failure.
}
