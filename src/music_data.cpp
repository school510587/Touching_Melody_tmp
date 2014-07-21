#include <stdlib.h>
#include <string.h>
#include "music_data.h"
#define ERROR(place, name) std::invalid_argument(#place": `"#name"' argument is invalid. ")
using namespace std;
bms_attributes::bms_attributes(const bms_node& p) throw(): bms_node(p){
 memset(&data, 0, sizeof(data));
 attributes_kind=unknown;
 attributes_change=false;
}
bms_attributes::~bms_attributes() throw(){
 if(kind()==time){
  switch(symbol()){
   case single_number: if(data.time_signature.beat_number!=NULL) free(data.time_signature.beat_number); break;
   default: if(data.time_signature.beat_pair!=NULL) delete[] data.time_signature.beat_pair; break;
  } // Default case maintains inconsistency between the symbol and beat-pairs.
 }
}
int bms_attributes::beats(size_t i) const throw(invalid_argument){
 switch(symbol()){
  case normal: return data.time_signature.beat_pair[i].first;
  case common: return 4;
  case cut: return 2;
  case single_number: return data.time_signature.beat_number[i];
 }
 return 0;
}
int bms_attributes::beat_type(size_t i) const throw(invalid_argument){
 return symbol()==normal ? data.time_signature.beat_pair[i].second : beats(i);
}
void bms_attributes::initialize_clef() throw(){
 attributes_kind=clef;
 set_sign(0);
 set_line(0);
 set_clef_octave_change(0);
}
void bms_attributes::initialize_key() throw(){
 attributes_kind=key;
 set_cancel(0);
 set_fifths(0);
}
void bms_attributes::initialize_time() throw(){
 attributes_kind=time;
 data.time_signature.beat_pair=NULL;
 data.time_signature.count=0;
 set_symbol(normal);
}
bms_attributes& bms_attributes::operator=(const bms_attributes& another) throw(){
 bms_node::operator=(another);
 if((attributes_kind=another.kind())==time){
  switch(another.symbol()){
   case single_number:
    data.time_signature.beat_number=(int*)malloc((data.time_signature.count=another.beat_pair_count())*sizeof(int));
    memcpy(data.time_signature.beat_number, another.data.time_signature.beat_number, beat_pair_count()*sizeof(*data.time_signature.beat_number));
   break;
   default:
    data.time_signature.beat_pair=new pair<int, int>[data.time_signature.count=another.beat_pair_count()];
    memcpy(data.time_signature.beat_pair, another.data.time_signature.beat_pair, beat_pair_count()*sizeof(*data.time_signature.beat_pair));
   break;
  }
  data.time_signature.symbol=another.data.time_signature.symbol;
 }
 else memcpy(&data, &another.data, sizeof(data));
 attributes_change=another.attributes_change;
 return *this;
}
bool bms_attributes::operator<(const bms_attributes& another) const throw(){
 switch(kind()){
  case clef: return another.kind()!=clef;
  case key: return another.kind()==clef ? false : another.kind()==time ? true : (_staff()>another._staff());
  case time: return another.kind()==time ? (_staff()>another._staff()) : false;
 }
 return false;
}
void bms_attributes::set_beats(int n) throw(invalid_argument){
 if(kind()==time){
  if(symbol()==single_number){
   int* p=data.time_signature.beat_number;
   data.time_signature.beat_number=(int*)malloc((beat_pair_count()+1)*sizeof(int));
   memcpy(data.time_signature.beat_number, p, beat_pair_count()*sizeof(*data.time_signature.beat_number));
   data.time_signature.beat_number[data.time_signature.count++]=n;
   if(p!=NULL) free(p);
  }
  else{
   pair<int, int>* p=data.time_signature.beat_pair;
   data.time_signature.beat_pair=new pair<int, int>[beat_pair_count()+1];
   memcpy(data.time_signature.beat_pair, p, beat_pair_count()*sizeof(*data.time_signature.beat_pair));
   data.time_signature.beat_pair[data.time_signature.count++].first=n;
   if(p!=NULL) delete[] p;
  }
 }
}
void bms_attributes::set_beat_type(int n) throw(invalid_argument){
 if(kind()==time && symbol()!=single_number) data.time_signature.beat_pair[beat_pair_count()-1].second=n;
}
void bms_attributes::set_cancel(int c) throw(invalid_argument){
 if(kind()==key) data.key_signature.cancel=c;
}
void bms_attributes::set_clef_octave_change(int n) throw(invalid_argument){
 if(kind()==clef) data.clef_data.octave_change=n;
}
void bms_attributes::set_fifths(int f) throw(invalid_argument){
 if(kind()==key) data.key_signature.fifths=f;
}
void bms_attributes::set_line(int l) throw(invalid_argument){
 if(kind()==clef) data.clef_data.line=l;
}
void bms_attributes::set_mode(int m) throw(invalid_argument){
 if(kind()==key) data.key_signature.mode=m;
}
void bms_attributes::set_sign(int s) throw(invalid_argument){
 if(kind()==clef) data.clef_data.sign=s;
}
void bms_attributes::set_symbol(int s) throw(invalid_argument){
 if(kind()==time) {
  if(symbol()==single_number){
   if(s!=single_number){
    int* p=data.time_signature.beat_number;
    data.time_signature.beat_pair=new pair<int, int>[beat_pair_count()];
    for(size_t i=0; i<beat_pair_count(); i++) data.time_signature.beat_pair[i].first=data.time_signature.beat_pair[i].second=p[i];
    if(p!=NULL) free(p);
   }
  }
  else if(s==single_number){
   pair<int, int>* p=data.time_signature.beat_pair;
   data.time_signature.beat_number=(int*)malloc(beat_pair_count()*sizeof(int));
   for(size_t i=0; i<beat_pair_count(); i++) data.time_signature.beat_number[i]=p[i].first;
   if(p!=NULL) delete[] p;
  }
  data.time_signature.symbol=s;
 }
}
void bms_barline::set_bar_style(int s) throw(invalid_argument){style=s;}
void bms_barline::set_ending_number(const string& n) throw(invalid_argument){ending.number=n;}
void bms_barline::set_ending_text(const string& t) throw(invalid_argument){ending.text=t;}
void bms_barline::set_notations(unsigned char n) throw(invalid_argument){notation_flag|=n;}
bms_direction::bms_direction(const bms_node& p) throw(): bms_node(p), words_type(NULL){
 attribute.type=unknown;
 attribute.above=1; // Value of "above".
 attribute.directive=0;
}
bms_direction::bms_direction(const bms_direction& another) throw(): words_type(NULL){
 attribute.type=unknown;
 *this=another;
}
bool bms_direction::above() const throw(){return attribute.above==1;}
void bms_direction::clear(int type) throw(){
 switch(direction_kind()){
  case metronome: if(metronome_type!=NULL) delete metronome_type; break;
  case words: if(words_type!=NULL) delete words_type; break;
 }
 switch(attribute.type=type){ // Initialization list.
  case dashes: dashes_type.number=1; dashes_type.type=TYPE_UNKNOWN; break;
  case metronome:
   metronome_type=new bms_metronome_t;
   metronome_type->beat_unit.type=0;
   metronome_type->beat_unit.dot=0;
   memset(metronome_type->per_minute, 0, sizeof(metronome_type->per_minute));
   metronome_type->parentheses=false;
  break;
  case octave_shift: octave_shift_type.number=1; octave_shift_type.size=8; octave_shift_type.type=TYPE_UNKNOWN; break;
  case pedal: pedal_type.line=0; pedal_type.type=TYPE_UNKNOWN; break;
  case wedge: wedge_type.number=0; wedge_type.type=TYPE_CRESCENDO_START; break;
  case words: words_type=NULL; break;
 }
}
int bms_direction::diatonic_step_change() const throw(invalid_argument){return direction_kind()==octave_shift ? octave_shift_type.size : 0;}
int bms_direction::direction_kind() const throw(){return attribute.type;}
void bms_direction::increase_beat_unit_dot() throw(invalid_argument){
 if(direction_kind()!=metronome) clear(metronome);
 (metronome_type->r_unit.type>0 ? metronome_type->r_unit.dot : metronome_type->beat_unit.dot)++;
}
bool bms_direction::is_directive() const throw(){return attribute.directive;}
bool bms_direction::is_half_pedaling() const throw(invalid_argument){return 34<=pedal_depth() && pedal_depth()<=66;}
bool bms_direction::is_stop() const throw(){
 switch(direction_kind()){
  case wedge: return type()==TYPE_CRESCENDO_STOP || type()==TYPE_DIMINUENDO_STOP;
  case pedal: return type()==TYPE_STOP;
  case octave_shift: return octave_shift_type.type==TYPE_STOP;
  default: return false;
 }
}
bool bms_direction::is_text() const throw(){
 switch(direction_kind()){
  case dashes:
  case wedge:
  case words: return true;
 }
 return false;
}
bool bms_direction::is_trivial() const throw(){return is_text() || direction_kind()==pedal;}
bool bms_direction::long_text() const throw(invalid_argument){
 if(direction_kind()==words && words_type!=NULL){
  size_t blank_count=0;
  for(string::const_iterator p=words_type->begin(); p!=words_type->end(); p++) if(*p==' ') blank_count++;
  return blank_count>1; // I.E. It contains at least 3 words.
 }
 else return false;
}
size_t bms_direction::number() const throw(){
 switch(direction_kind()){
  case dashes: return dashes_type.number;
  case octave_shift: return octave_shift_type.number;
  case wedge: return wedge_type.number;
 }
 return 1; // Default value when number-level is implied.
}
int bms_direction::octave_change() const throw(invalid_argument){
 if(octave_shift_type.type!=TYPE_STOP){
  int size=(octave_shift_type.size-1)/7;
  return octave_shift_type.type==TYPE_DOWN ? -size : size;
 }
 else return 0; // No change in a type-stop octave-shift direction.
}
bms_direction& bms_direction::operator=(const bms_direction& another) throw(){
 bms_node::operator=(another); // Copy of the parent.
 clear(another.direction_kind());
 attribute.above=another.attribute.above, attribute.directive=another.attribute.directive;
 switch(another.direction_kind()){ // Copy list.
  case dashes: dashes_type=another.dashes_type; break;
  case metronome: *metronome_type=*another.metronome_type; break;
  case octave_shift: octave_shift_type=another.octave_shift_type; break;
  case pedal: pedal_type=another.pedal_type; break;
  case wedge: wedge_type=another.wedge_type; break;
  case words: if(another.words_type!=NULL) words_type=new string(*another.words_type); break;
 }
 return *this;
}
size_t bms_direction::pedal_depth() const throw(invalid_argument){return direction_kind()==pedal ? pedal_type.depth : 0;}
void bms_direction::set_above(bool a) throw(){attribute.above=a ? 1 : 0;}
void bms_direction::set_beat_unit(unsigned char u, bool left) throw(invalid_argument){
 if(direction_kind()!=metronome) clear(metronome);
 (left || metronome_type->beat_unit.type==0 ? metronome_type->beat_unit.type : metronome_type->r_unit.type)=u;
}
void bms_direction::set_dashes_number(size_t n) throw(invalid_argument){
 if(direction_kind()!=dashes) clear(dashes);
 dashes_type.number=n;
}
void bms_direction::set_dashes_type(size_t t) throw(invalid_argument){
 if(direction_kind()!=dashes) clear(dashes);
 dashes_type.type=t;
}
void bms_direction::set_directive(bool d) throw(){attribute.directive=d ? 1 : 0;}
void bms_direction::set_line(bool l) throw(){
 if(direction_kind()!=pedal) clear(pedal);
 pedal_type.line= l ? 1 : 0;
}
void bms_direction::set_parentheses(bool p) throw(){
 if(direction_kind()!=metronome) clear(metronome);
 metronome_type->parentheses=p;
}
void bms_direction::set_pedal_depth(size_t d) throw(invalid_argument){
 if(direction_kind()!=pedal) clear(pedal);
 pedal_type.depth=d;
}
void bms_direction::set_pedal_type(size_t t) throw(invalid_argument){
 if(direction_kind()!=pedal) clear(pedal);
 pedal_type.type=t;
}
void bms_direction::set_per_minute(const string& m) throw(){
 if(direction_kind()!=metronome) clear(metronome);
 strncpy(metronome_type->per_minute, m.c_str(), sizeof(metronome_type->per_minute));
 for(char* p=metronome_type->per_minute; *p!='\0'; p++){
  if('1'<=*p && *p<='9') *p+=0x30;
  else if(*p=='0') *p='j';
 }
}
void bms_direction::set_shift_size(int s) throw(invalid_argument){
 if(direction_kind()!=octave_shift) clear(octave_shift);
 octave_shift_type.size=s;
}
void bms_direction::set_shift_number(size_t n) throw(invalid_argument){
 if(direction_kind()!=octave_shift) clear(octave_shift);
 octave_shift_type.number=n;
}
void bms_direction::set_shift_type(size_t t) throw(invalid_argument){
 if(direction_kind()!=octave_shift) clear(octave_shift);
 octave_shift_type.type=t;
}
void bms_direction::set_text(const string& t, bool reset) throw(){
 int space_state=0;
 if(direction_kind()!=words || reset) clear(words);
 if(words_type==NULL) words_type=new string;
 for(string::const_iterator p=t.begin(); p!=t.end(); p++){
  if(*p!=' ' && *p!='\t' && *p!='\n'){
   if(space_state!=1){
    if(space_state==2) *words_type+=' ';
    space_state=1;
   }
   *words_type+=*p;
  }
  else if(space_state==1) space_state=2;
 }
}
void bms_direction::set_wedge_number(size_t n) throw(invalid_argument){
 if(direction_kind()!=wedge) clear(wedge);
 wedge_type.number=n;
}
void bms_direction::set_wedge_type(size_t t) throw(invalid_argument){
 if(direction_kind()!=wedge) clear(wedge);
 wedge_type.type=t;
}
string bms_direction::text() const throw(invalid_argument){return direction_kind()==words && words_type!=NULL ? *words_type : "";}
size_t bms_direction::type() const throw(){
 switch(direction_kind()){
  case dashes: return dashes_type.type;
  case octave_shift: return octave_shift_type.type;
  case pedal: return pedal_type.type;
  case wedge: return wedge_type.type;
 }
 return TYPE_UNKNOWN;
}
bms_note::bms_note(const bms_node& p) throw(): bms_node(p){
 attribute.instrument='\0';
 attribute.dynamics=attribute.end_dynamics=0;
 attribute.dynamics_set=attribute.end_dynamics_set=0;
 memset(&attribute.string_technical, 0, sizeof(attribute.string_technical));
 attribute.chord=0;
 attribute.rest=attribute.grace=false;
 attribute.printed=true;
 pitch.octave=-1;
 pitch.alter=0;
 pitch.step=-1;
 memset(&time, 0, sizeof(time));
 appearance.link[0]=appearance.link[1]=appearance.link[2]=NULL;
 appearance.accidental=-1;
 appearance.note_type.type=(unsigned char)whole;
 appearance.note_type.dot=0;
 appearance.voice.state=0;
 memset(&appearance.shape, 0, sizeof(appearance.shape));
 memset(&notations.slur, 0, sizeof(notations.slur));
 notations.fermata=3;
 memset(&notations.fingering, 0, sizeof(notations.fingering));
 notations.tied.type=TYPE_UNKNOWN;
 notations.tied.attribute_flag=0;
 notations.tremolo.partner=NULL;
 notations.tremolo.value=0;
 memset(notations.tuplet, 0, sizeof(notations.tuplet));
 memset(&notations.arpeggiate, 0, sizeof(notations.arpeggiate));
 notations.articulation_flag=0;
 notations.trill.kind=0;
 notations.trill.start_note=2;
 notations.trill.trill_step=3;
 notations.trill.two_note_turn=3;
 notations.trill.accelerate=0;
 notations.trill.beats=0;
 notations.trill.second_beat=101;
 notations.trill.last_beat=101;
 notations.trill.accidental_mark_above=notations.trill.accidental_mark_below=0x0f;
 notations.syllabic=0;
}
bms_note* bms_note::center_note() throw(){
 if(chord()<0 || steal_time_previous()>0) return appearance.link[1];
 else if(steal_time_following()>0) return appearance.link[2];
 else return this;
}
const bms_note* bms_note::center_note() const throw(){
 if(chord()<0 || steal_time_previous()>0) return appearance.link[1];
 else if(steal_time_following()>0) return appearance.link[2];
 else return this;
}
unsigned long long bms_note::chord_common_block() const throw(){
 unsigned long long value=_staff();
 value=(value<<4)|type();
 value=(value<<4)|dot();
 value=(value<<2)|stem();
 return (value<<8)|instrument();
}
int bms_note::diatonic_pitch() const throw(invalid_argument){
 try{return octave()*7+step();}
 catch(invalid_argument& i){throw invalid_argument(string("bms_note::diatonic_pitch(): Error:\n")+i.what());}
}
unsigned char bms_note::fingering(size_t i) const throw(invalid_argument){
 return notations.fingering[i];
}
bool bms_note::grace_not_set() const throw(invalid_argument){
 return grace() && steal_time_previous()==0 && steal_time_following()==0 && make_time()==0;
}
bms_note* bms_note::group_begin() throw(){
 bms_note* center=center_note();
 while(center->center_note()!=center) center=center->center_note();
 return center->next_prefix_grace()==NULL ? center : center->next_prefix_grace();
}
const bms_note* bms_note::group_begin() const throw(){
 const bms_note* center=center_note();
 while(center->center_note()!=center) center=center->center_note();
 return center->next_prefix_grace()==NULL ? center : center->next_prefix_grace();
}
bms_note* bms_note::group_next() throw(){
 if(steal_time_following()>0) return next_prefix_grace()==NULL ? center_note() : next_prefix_grace();
 else return next_suffix_grace();
}
const bms_note* bms_note::group_next() const throw(){
 if(steal_time_following()>0) return next_prefix_grace()==NULL ? center_note() : next_prefix_grace();
 else return next_suffix_grace();
}
const void* bms_note::group_id() const throw(){
 const bms_note* center=center_note();
 while(center->center_note()!=center) center=center->center_note();
 return center;
}
bool bms_note::is_breve_long() const throw(){
 switch(type()){
  case longa:
  case breve: return true;
  default: return false;
 }
}
bool bms_note::is_long_grace() const throw(){
 switch(type()){
  case whole:
  case half:
  case quarter: return true;
  default: return is_breve_long();
 }
}
int bms_note::midi_pitch() const throw(invalid_argument){
 try{
  if(rest()) return 0;
  else return midi_step()+alter();
 }
 catch(invalid_argument& i){throw invalid_argument(string("bms_note::midi_pitch(): Error:\n")+i.what());}
}
int bms_note::midi_step() const throw(invalid_argument){
 static const int midi_step[7]={0, 2, 4, 5, 7, 9, 11};
 try{return (octave()+1)*12+midi_step[step()];}
 catch(invalid_argument& i){throw invalid_argument(string("bms_note::midi_step(): Error:\n")+i.what());}
}
bool bms_note::operator<(const bms_note& another) const throw(invalid_argument){
 int pitch[2]={diatonic_pitch(), another.diatonic_pitch()};
 if(pitch[0]<pitch[1]) return true;
 else if(pitch[0]==pitch[1]) return midi_pitch()<another.midi_pitch();
 else return false;
}
void bms_note::set_accidental(int a) throw(invalid_argument){
 appearance.accidental=a;
}
void bms_note::set_accidental_mark_above(unsigned char a) throw(invalid_argument){
 notations.trill.accidental_mark_above=a;
}
void bms_note::set_accidental_mark_below(unsigned char a) throw(invalid_argument){
 notations.trill.accidental_mark_below=a;
}
void bms_note::set_alter(int a) throw(invalid_argument){
 pitch.alter=a;
}
void bms_note::set_arpeggiate_kind(int k, bool reset) throw(invalid_argument){
 if(reset) notations.arpeggiate.kind=k;
 else notations.arpeggiate.kind|=k;
}
void bms_note::set_arpeggiate_number(int n) throw(invalid_argument){
 notations.arpeggiate.number=n;
}
void bms_note::set_arpeggiate_order(int o) throw(invalid_argument){
 notations.arpeggiate.order=o;
}
void bms_note::set_articulation_flag(unsigned short f, bool reset) throw(invalid_argument){
 if(reset) notations.articulation_flag=f;
 else notations.articulation_flag|=f;
}
void bms_note::set_attack(long a) throw(invalid_argument){
 time.attack=a;
 time.attack_set=1;
}
void bms_note::set_center_note(bms_note* c) throw(std::invalid_argument){
 if(chord()<0 || steal_time_previous()>0) appearance.link[1]=c;
 else if(steal_time_following()>0) appearance.link[2]=c;
}
void bms_note::set_chord(char c) throw(invalid_argument){
 attribute.chord=c;
}
void bms_note::set_down_bow(bool b) throw(){
 attribute.string_technical.down_bow=b ? 1 : 0;
}
void bms_note::set_duration(long d) throw(invalid_argument){
 time.duration=d;
}
void bms_note::set_dynamics(unsigned char d, bool reset) throw(invalid_argument){
 if(attribute.dynamics_set==0 || reset) attribute.dynamics=normalize_dynamics(d);
 attribute.dynamics_set=1;
}
void bms_note::set_end_dynamics(unsigned char d, bool reset) throw(invalid_argument){
 if(attribute.end_dynamics_set==0 || reset) attribute.end_dynamics=d;
 attribute.end_dynamics_set=1;
}
void bms_note::set_fermata(int f) throw(invalid_argument){
 notations.fermata=f;
}
void bms_note::set_fingering(size_t i, unsigned char f) throw(invalid_argument){
 if(1<=f && f<=5) notations.fingering[i]=f;
}
void bms_note::set_grace(bool g) throw(invalid_argument){
 attribute.grace=g;
}
void bms_note::set_instrument(unsigned char i) throw(invalid_argument){
 attribute.instrument=i;
}
void bms_note::set_long_slur_type(unsigned char t, bool reset) throw(invalid_argument){
 if(reset) notations.slur.long_type=t;
 else notations.slur.long_type|=t;
}
void bms_note::set_lyric(const string& l) throw(invalid_argument){
 notations.lyric=l;
}
void bms_note::set_make_time(unsigned long t) throw(invalid_argument){
 time.make_time=t;
}
void bms_note::set_next_chord(bms_note* l) throw(std::invalid_argument){
 appearance.link[0]=l;
}
void bms_note::set_next_prefix_grace(bms_note* g) throw(std::invalid_argument){
 if(regular() || steal_time_following()>0) appearance.link[1]=g;
}
void bms_note::set_next_suffix_grace(bms_note* g) throw(std::invalid_argument){
 if(regular() || steal_time_previous()>0) appearance.link[2]=g;
}
void bms_note::set_notehead(unsigned char h) throw(invalid_argument){
 appearance.shape.head=h;
}
void bms_note::set_octave(int o) throw(invalid_argument){
 pitch.octave=o;
}
void bms_note::set_parentheses(bool b) throw(){
 appearance.shape.parentheses=b;
}
void bms_note::set_pizzicato(bool b) throw(){
 attribute.string_technical.pizzicato=b ? 1 : 0;
}
void bms_note::set_prefix(unsigned long n) throw(invalid_argument){
 time.prefix=n;
}
void bms_note::set_printed(bool p) throw(invalid_argument){
 attribute.printed=p;
}
void bms_note::set_release(long r) throw(invalid_argument){
 time.release=r;
 time.release_set=1;
}
void bms_note::set_rest(bool r) throw(invalid_argument){
 if(attribute.rest=r) appearance.shape.set_stem=1;
}
void bms_note::set_short_slur_attribute(unsigned char a, bool reset) throw(invalid_argument){
 if(reset) notations.slur.attribute_flag=a;
 else notations.slur.attribute_flag|=a;
}
void bms_note::set_short_slur_type(unsigned char t, bool left) throw(invalid_argument){
 (left ? notations.slur.short_left : notations.slur.short_right)=t;
}
void bms_note::set_slash(bool s) throw(invalid_argument){
 time.slash= s ? 1 : 0;
}
void bms_note::set_steal_time_following(unsigned long t) throw(invalid_argument){
 time.steal_time_following=t;
}
void bms_note::set_steal_time_previous(unsigned long t) throw(invalid_argument){
 time.steal_time_previous=t;
}
void bms_note::set_stem(unsigned char s, bool reset) throw(invalid_argument){
 if(reset || appearance.shape.set_stem==0){
  appearance.shape.stem=s;
  appearance.shape.set_stem=1;
 }
}
void bms_note::set_step(int s) throw(invalid_argument){
 pitch.step=s;
}
void bms_note::set_suffix(unsigned long n) throw(invalid_argument){
 time.suffix=n;
}
void bms_note::set_syllabic(int s) throw(invalid_argument){
 notations.syllabic=s;
}
void bms_note::set_tied_type(int t, bool reset) throw(invalid_argument){
 if(!reset && tied_type()!=TYPE_UNKNOWN){ // There is an other state of tied.
  switch(tied_type()){
   case TYPE_START:
    notations.tied.type= t==TYPE_STOP ? TYPE_CONTINUE : t;
   break;
   case TYPE_STOP:
    notations.tied.type= t==TYPE_START ? TYPE_CONTINUE : t;
   break;
  } // There is a default case of 2, which must not be rewritten.
 }
 else notations.tied.type=t;
}
void bms_note::set_trill_accelerate(bool a) throw(invalid_argument){
 notations.trill.accelerate= a ? 1 : 0;
}
void bms_note::set_trill_beats(unsigned long b) throw(invalid_argument){
 notations.trill.beats=b;
}
void bms_note::set_trill_kind(unsigned long k) throw(invalid_argument){
 notations.trill.kind=k;
}
void bms_note::set_trill_last_beat(unsigned long l) throw(invalid_argument){
 notations.trill.last_beat=l;
}
void bms_note::set_trill_second_beat(unsigned long s) throw(invalid_argument){
 notations.trill.second_beat=s;
}
void bms_note::set_trill_start_note(unsigned long n) throw(invalid_argument){
 notations.trill.start_note=n;
}
void bms_note::set_trill_step(unsigned long s) throw(invalid_argument){
 notations.trill.trill_step=s;
}
void bms_note::set_trill_two_note_turn(unsigned long t) throw(invalid_argument){
 notations.trill.two_note_turn=t;
}
void bms_note::set_tuplet_number(size_t i, unsigned long n, bool normal) throw(invalid_argument){
 if(i<6) (normal ? notations.tuplet[i].normal_note_number : notations.tuplet[i].actual_note_number)=n;
}
void bms_note::set_tuplet_type(size_t i,unsigned char t) throw(invalid_argument){
 if(i<6) notations.tuplet[i].type=t;
}
void bms_note::set_type(note_type_t t) throw(invalid_argument){
 appearance.note_type.type=(unsigned char)t;
}
void bms_note::set_up_bow(bool b) throw(){
 attribute.string_technical.up_bow=b ? 1 : 0;
}
void bms_note::set_voice(int v) throw(invalid_argument){
 appearance.voice.state=v;
}
void bms_note::set_voice(size_t l, unsigned char v) throw(invalid_argument){
 if(l<2) appearance.voice.number[l]=v;
}
unsigned char bms_note::short_slur_type(bool left) const throw(invalid_argument){
 if(left) return notations.slur.short_left;
 else return notations.slur.short_right;
}
int bms_note::shape() const throw(invalid_argument){
 if(notehead_is_normal(appearance.shape.head)) return !appearance.shape.set_stem?1:appearance.shape.stem==STEM_NONE?1:0;
 else{
  if(appearance.shape.head==NOTEHEAD_X) return 2;
  else if(notehead_is_vertical(appearance.shape.head)) return 3;
  else if(notehead_is_diamond(appearance.shape.head)) return 4;
  else return 5;
 }
}
unsigned char bms_note::slur_number(int t) const throw(invalid_argument){
 return notations.slur.number[t%3];
}
unsigned long bms_note::tuplet_number(size_t i, bool normal) const throw(invalid_argument){
 return i<6 ? (normal ? notations.tuplet[i].normal_note_number : notations.tuplet[i].actual_note_number) : 0;
}
unsigned char bms_note::tuplet_type(size_t i) const throw(invalid_argument){
 return i<6 ? notations.tuplet[i].type : TYPE_UNKNOWN;
}
unsigned char bms_note::voice(size_t l) const throw(invalid_argument){
 return l<2 ? appearance.voice.number[l] : '\0';
}
unsigned short bms_note::voice_abs() const throw(invalid_argument){
 return (((unsigned short)appearance.voice.number[0])<<8)|appearance.voice.number[1];
}
