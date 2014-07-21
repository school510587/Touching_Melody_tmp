#include <sstream>
#include <stdlib.h>
#include "braille_outputer.h"
#include "braille_symbols.h"
using namespace std;
using namespace MusicXML2;
static bool contain_123(char c) throw(){
 static const char* inverse_pattern="`~_\".;,";
 if(c=='\0' || c==' ') return false;
 else{
  for(const char* p=inverse_pattern; *p!='\0'; p++) if(*p==c) return false;
  return true;
 }
}
static string measure_rest_string(int n) throw(){
 char result[16]={0};
 switch(n){
  case 1:
  case 2:
  case 3:
   for(char* p=result; p<result+n; p++) *p=rest_symbol[0];
  break; 
  default:
   if(n>0){ // Negative n is unwanted exception.
    sprintf(result, "#%d%c", n, rest_symbol[0]);
    for(char* p=result+1; *p!=rest_symbol[0]; p++) *p=UpperNumber[*p&0x0f];
   }
  break; 
 }
 return result;
}
static bool is_text_type(brl_token::type_t t) throw(){
 return t==brl_token::words || t==brl_token::wedge || t==brl_token::dashes;
}
static bool long_words(const std::string& w) throw(){
 return w.length()>2 && w[0]=='>' && w[w.length()-1]=='>';
}
static char rest_char(bms_note::note_type_t t) throw(){
 switch(t){
  case bms_note::whole: return rest_symbol[0];
  case bms_note::half: return rest_symbol[1];
  case bms_note::quarter: return rest_symbol[2];
  case bms_note::eighth: return rest_symbol[3];
  case bms_note::_16th: return rest_symbol[0];
  case bms_note::_32nd: return rest_symbol[1];
  case bms_note::_64th: return rest_symbol[2];
  case bms_note::_128th: return rest_symbol[3];
  case bms_note::_256th: return rest_symbol[0];
  default: break;
 }
 return '\0';
}
static string short_slur_string(unsigned char type, unsigned char attribute) throw(){
 string result;
 switch(type){
  case SLUR_CROSS_STAFF: result+="\""; goto regular_short_slur;
  case SLUR_CROSS_VOICE: result+="_"; goto regular_short_slur;
  case SLUR_GENERAL:
  regular_short_slur:
   if(attribute&SLUR_GRACE) result+=";";
   if(attribute&SLUR_DISCONTINUOUS) result+=",";
   result+=slur_symbol[0];
  break;
 }
 return result;
}
static char step_char(bms_note::note_type_t t, int s) throw(){
 switch(t){
  case bms_note::whole: return step_symbol[1][s];
  case bms_note::half: return step_symbol[2][s];
  case bms_note::quarter: return step_symbol[3][s];
  case bms_note::eighth: return step_symbol[4][s];
  case bms_note::_16th: return step_symbol[1][s];
  case bms_note::_32nd: return step_symbol[2][s];
  case bms_note::_64th: return step_symbol[3][s];
  case bms_note::_128th: return step_symbol[4][s];
  case bms_note::_256th: return step_symbol[1][s];
  default: break;
 }
 return '\0';
}
static string tuplet_string(size_t number, int show_number) throw(){
 if(show_number==3 && number==1) return "2"; // Normal triplet.
 else{ // Tuplets of other show-number or triplets in nested tuplets.
  ostringstream tuplet;
  tuplet<<'_'<<show_number<<'\'';
  return tuplet.str();
 }
}
bool brl_token::followed_dot(type_t next) const throw(){
 if(empty()) return false;
 switch(type){
  case clef:
  case ending: return true;
  case wedge: return !is_text_type(next);
  case words: return !is_text_type(next) && at(length()-1)!='\'' && at(length()-1)!='>';
 }
 return false;
}
void brl_token::set_position(int r, int c) throw(){
 position[0]=(short)r;
 position[1]=(short)c;
}
const char* brl_measure::c_str() const throw(){
 string answer;
 for(const_iterator p=begin(); p!=end(); p++) answer+=*p;
 return answer.c_str();
}
size_t brl_measure::length() const throw(){
 size_t answer=0;;
 for(const_iterator p=begin(); p!=end(); p++) answer+=p->length();
 return answer;
}
void brl_measure::set_position(int r, int c) throw(){
 position[0]=(short)r;
 position[1]=(short)c;
}
bool brl_staff::add(char c, brl_token::type_t t, bool dot) throw(){
 char s[2]={c, '\0'};
 return add(s, t);
}
bool brl_staff::add(const string& s, brl_token::type_t t, bool dot) throw(){
 if(s.empty()) return false; // No newly inserted string.
 if(empty()) *this<<brl_measure(1);
 else if(!back().empty()){
  if(data.back().back().followed_blank() && t!=brl_token::time) data.back().back().add_blank();
  if(long_words(data.back().back())) data.back().push_back(brl_token(" ", brl_token::blank_cell));
  if(long_words(s) && !data.back().back().end_blank()) data.back().push_back(brl_token("\" ", brl_token::measure_continuity));
  if(data.back().back().followed_dot(t) && contain_123(s[0])) data.back().back().add_dot();
 }
 data.back().push_back(brl_token(s, t));
 return true;
}
void brl_staff::clear() throw(){
 data.clear();
 page_bound.clear();
 name.clear();
}
string brl_staff::label() const throw(){
 ostringstream label_content;
 label_content<<"Part #"<<part<<": staff #"<<staff<<' '<<name;
 return label_content.str();
}
brl_staff& brl_staff::operator<<(const brl_measure& m) throw(){
 data.push_back(m);
 return *this;
}
void brl_output::add_new_staff(string name, int part, int staff) throw(){
 score.push_back(brl_staff(name, part, staff));
}
void brl_output::clear() throw(){
 part_table.clear();
 score.clear();
 for(int i=0; i<5; i++) work_data[i].clear();
}
/*
size_t brl_output::page_begin(page_table_const_iterator p) const throw(){
 return p!=page_table.end() ? p->first : measure();
}
size_t brl_output::page_end(page_table_const_iterator p) const throw(){
 return p!=page_table.end() && ++p!=page_table.end() ? p->first : measure();
}
*/
void brl_output::reset_work(const bms_score& m) throw(){
 work_data[0]=m.get_work_number();
 work_data[1]=m.get_work_title();
 work_data[2]=m.get_composer();
 work_data[3]=m.get_lyricist();
 work_data[4]=m.get_arranger();
}
void brl_output::set_part_pair(const string& name, const string& abbreviation) throw(){
 part_table.push_back(pair<string, string>(name, abbreviation));
 if(part_table.back().second[0]!='>') part_table.back().second='>'+part_table.back().second;
 if(part_table.back().second[part_table.back().second.length()-1]!='.') part_table.back().second+='\'';
 else part_table.back().second[part_table.back().second.length()-1]='\'';
}
BrailleOutputer::BrailleOutputer() throw(){
 clear();
}
void BrailleOutputer::clear() throw(){
 dashes_number=global_octave_shift=extra_octave_shift=0;
 octave_shift_switch=false;
}
void BrailleOutputer::output_attributes(brl_staff& staff, const bms_attributes& a, bool upmost_pitch) throw(){
 switch(a.kind()){
  case bms_attributes::clef:
   if(staff.add(output_clef(a, upmost_pitch), brl_token::clef)) last_note=NULL;
  break;
  case bms_attributes::key:
   if(staff.add(output_key_signature(a), brl_token::key)) last_note=NULL;
  break;
  case bms_attributes::time:
   if(staff.add(output_time_signature(a), brl_token::time)) last_note=NULL;
  break;
 }
}
void BrailleOutputer::output_barline(brl_staff& staff, const bms_barline& b) throw(){
 if(b.notations()&4){ // Repeat marks.
  staff.add(barline[b.notations()&8 ? 5 : 6], brl_token::repeat);
  last_note=NULL; // There is a necessary octave sign after each repeat mark.
 }
 else if(b.bar_style()==4){
  staff.add(barline[4], brl_token::barline);
  last_note=NULL; // There is a necessary octave sign after each light-light barline.
 }
 if(b.notations()&(16|2|1)){ // Coda, fermata, or segno.
  staff.add(" ", brl_token::blank_cell);
  if(b.notations()&1) staff.add(coda_symbol, brl_token::coda);
  else if(b.notations()&2) staff.add(fermata_symbol[0], brl_token::fermata);
  else if(b.notations()&16) staff.add(segno_symbol, brl_token::segno);
 }
 if(!b.ending_number().empty()){
  string result;
  if(b.ending_text().empty()){
   string number=b.ending_number();
   size_t n1=0, n2=0;
   result+='#';
   n1=number.find_first_of(",");
   if(n1==string::npos) result+=number;
   else{ // Multiple ending numbers.
    result+=number.substr(0, n1)+'-';
    n2=number.find_last_of(",");
    result+=number.substr(n2, number.length());
   }
  }
  else{
   result+='>';
   result+=b.ending_text();
  }
  staff.add(result, brl_token::ending);
  last_note=NULL;
 }
}
string BrailleOutputer::output_clef(const bms_attributes& a, bool upmost_pitch) throw(){
 string result;
 if(a.sign()!=0){ // This attribute consains a clef.
  int l=a.line();
  switch(a.sign()){
   case 1: // G clef.
    result=SignSymbol[upmost_pitch ? 0 : 1];
    if(l!=2) result.insert(2, 1, OctaveSymbol[l]);
   break;
   case 2: // C clef.
    switch(l){
     case 3: result=SignSymbol[2]; break;
     case 4: result=SignSymbol[3]; break;
     default:
      result=SignSymbol[2];
      result.insert(2, 1, OctaveSymbol[l]);
     break;
    }
   break;
   case 3: // F clef.
    result=SignSymbol[upmost_pitch ? 5 : 4];
    if(l!=4) result.insert(2, 1, OctaveSymbol[l]);
   break;
  }
  if(a.clef_octave_change()!=0){
   int number=1+7*abs(a.clef_octave_change());
   result+='#';
   if(a.clef_octave_change()>0){
    if(number>9) result+=UpperNumber[(number/10)%10];
    result+=UpperNumber[number%10];
   }
   else{
    if(number>9) result+='0'%(number/10)%10;
    result+='0'+number%10;
   }
  }
 }
 global_octave_shift=a.clef_octave_change();
 return result;
}
void BrailleOutputer::output_direction(brl_staff& staff, const bms_direction& d, const bms_direction* l) throw(){
 string result;
 switch(d.direction_kind()){
  case bms_direction::dashes:
   if(d.type()==TYPE_STOP || l==NULL || !l->is_text()) result+=">";
   switch(d.type()){
    case TYPE_START:
     if(dashes_number==0){
      result+=dashes_sign[0];
      dashes_number=d.number();
     }
     else result+=dashes_sign[2];
    break;
    case TYPE_STOP:
     if(d.number()==dashes_number){
      result+=dashes_sign[1];
      dashes_number=0;
     }
     else result+=dashes_sign[3];
    break;
   }
   if(staff.add(result, brl_token::dashes)) last_note=NULL;
  break;
  case bms_direction::octave_shift: // Octave-shift (8va or 8ba).
   extra_octave_shift=d.octave_change();
   octave_shift_switch=true; // A switch for control is on.
  break;
  case bms_direction::pedal:
   switch(d.type()){
    case TYPE_START: staff.add(pedal_sign[d.is_half_pedaling() ? 3 : 0], brl_token::pedal); break;
    case TYPE_STOP: staff.add(pedal_sign[1], brl_token::pedal); break;
    case TYPE_CHANGE: staff.add(pedal_sign[2], brl_token::pedal); break;
   }
  break;
  case bms_direction::wedge:
   result=">";
   result+=wedge_sign[d.type()];
   if(staff.add(result, brl_token::wedge)) last_note=NULL;
  break;
  case bms_direction::words:
   result=d.text();
   for(size_t i=0; i<result.size(); i++) if(result[i]=='.') result[i]='\'';
   if(d.long_text()) result=">"+result+">"; // Long text must be enclosed by 2 text-prefices.
   else{
    for(size_t i=0; i<result.size(); i++) if(result[i]==' ') result[i]='>';
    if(result[0]!='>') result=">"+result;
   }
   if(staff.add(result, brl_token::words)) last_note=NULL;
  break;
 }
}
string BrailleOutputer::output_key_signature(const bms_attributes& k) throw(){
 string result;
 if(k.cancel()>0){ // Natural(s) in a key signature.
  int cancel=k.cancel();
  if(k.cancel()>3){
   result+='#'; // Number sign.
   if(cancel>=10) result+=UpperNumber[cancel/10];
   result+=UpperNumber[cancel%10];
   result+=AlterSymbol[5];
  }
  else for(int i=0; i<cancel; i++) result+=AlterSymbol[5];
 }
 if(k.fifths()!=0){
  int absf=abs(k.fifths());
  if(k.fifths()>0){ // Sharps.
   if(absf>3){
    result+='#'; // Number sign.
    if(absf>=10) result+=UpperNumber[absf/10];
    result+=UpperNumber[absf%10];
    result+=AlterSymbol[8];
   }
   else for(int i=0; i<absf; i++) result+=AlterSymbol[8];
  }
  else{ // Flats.
   if(absf>3){
    result+='#'; // Number sign.
    if(absf>=10) result+=UpperNumber[absf/10];
    result+=UpperNumber[absf%10];
    result+=AlterSymbol[2];
   }
   else for(int i=0; i<absf; i++) result+=AlterSymbol[2];
  }
 }
 return result;
}
void BrailleOutputer::output_note(brl_staff& staff, const bms_note& n) throw(){
 if(n.parentheses()) staff.add(music_parentheses, brl_token::parentheses);
 if(n.long_slur_type()==SLUR_START_LONG) staff.add(slur_symbol[1], brl_token::slur);
 else if(n.long_slur_type()==SLUR_DISCONTINUOUS_LONG) staff.add(";b~2", brl_token::slur);
 staff.add(short_slur_string(n.short_slur_type(true), 0), brl_token::slur);
 for(size_t i=0, j=1; i<6; i++) if(n.tuplet_type(i)==TYPE_START) staff.add(tuplet_string(j++, n.tuplet_number(i, false)), brl_token::tuplet);
 if(n.rest()){ // Rest note.
  string result;
  if(!n.printed()) result+='\"';
  if(n.is_breve_long()){ // Breve and longa notes.
   result+=rest_symbol[0];
   result+=step_symbol[0];
   if(n.type()==bms_note::longa) result+=step_symbol[0]; // Longa notes.
   result+=rest_symbol[0];
  }
  else result+=rest_char(n.type());
  for(int i=0; i<n.dot(); i++) result+=dot_symbol;
  staff.add(result, brl_token::rest);
  if(n.fermata()<3) staff.add(fermata_symbol[n.fermata()], brl_token::fermata);
 }
 else{ // Non-rest notes.
  if(n.arpeggiate_number()>0) staff.add(arpeggiate_symbol[n.arpeggiate_kind()], brl_token::arpeggiate);
  if(n.up_bow()) staff.add(bow[0], brl_token::up_bow);
  if(n.down_bow()) staff.add(bow[1], brl_token::down_bow);
  if(n.trill_kind()>0){ // Ornaments.
   int a=n.accidental_mark_above();
   if(a<11) staff.add(AlterSymbol[a], brl_token::accidental);
   a=n.accidental_mark_below();
   if(a<11) staff.add(string(",")+AlterSymbol[a], brl_token::accidental);
   staff.add(ornament_sign[n.trill_kind()], brl_token::turn);
  }
  if(n.articulation_flag()&bms_note::note_staccato) staff.add(articulation[0], brl_token::staccato);
  if(n.articulation_flag()&bms_note::note_spiccato) staff.add(articulation[6], brl_token::spiccato);
  if(n.articulation_flag()&bms_note::note_staccatissimo) staff.add(articulation[5], brl_token::staccatissimo);
  if(n.articulation_flag()&bms_note::note_accent) staff.add(articulation[4], brl_token::accent);
  if(n.articulation_flag()&bms_note::note_strong_accent) staff.add(articulation[1], brl_token::strong_accent);
  if(n.articulation_flag()&bms_note::note_detached_legato) staff.add(articulation[3], brl_token::detached_legato);
  if(n.articulation_flag()&bms_note::note_tenuto) staff.add(articulation[2], brl_token::tenuto);
  if(n.grace()) staff.add(grace_symbol[0], brl_token::grace);
  output_note_body(staff, n, last_note, normal_note);
  staff.add(short_slur_string(n.short_slur_type(), n.short_slur_attribute()), brl_token::slur);
  if(n.next_chord()!=NULL){
   bool all_tied=(n.tied_type()==1 || n.tied_type()==2);
   for(const bms_note* c=n.next_chord(); all_tied && c!=NULL; c=c->next_chord()) all_tied=c->contains_tied_start();
   if(n.contains_tied_start() && !all_tied) staff.add(TiedSymbol[n.grace() ? 2 : 0], brl_token::tied); // Main note tied.
   for(const bms_note* c[2]={n.next_chord(), &n}; c[0]!=NULL; c[0]=c[0]->next_chord()){
    int interval=abs(c[0]->diatonic_pitch()-c[1]->diatonic_pitch());
    output_note_body(staff, *c[0], &n, 0<interval && interval<8 ? simple_chord : octave_chord);
    if(c[0]->contains_tied_start() && !all_tied) staff.add(TiedSymbol[0], brl_token::tied); // Tied of chord notes.
    c[1]=c[0]; // A reference to the previous chord note.
   }
   if(n.fermata()<3) staff.add(fermata_symbol[n.fermata()], brl_token::fermata);
   if(all_tied) staff.add(TiedSymbol[1], brl_token::tied);
  }
  else{
   if(n.fermata()<3) staff.add(fermata_symbol[n.fermata()], brl_token::fermata);
   if(n.contains_tied_start()) staff.add(TiedSymbol[0], brl_token::tied); // Tied of a single note.
  }
  if(n.long_slur_type()==SLUR_STOP_LONG) staff.add(slur_symbol[5], brl_token::slur);
  last_note=&n;
 }
}
void BrailleOutputer::output_note_body(brl_staff& staff, const bms_note& note, const bms_note* reference, note_body_type_t type) throw(){
 int accidental=note.accidental();
 if(0<=accidental && accidental<11) staff.add(AlterSymbol[accidental], brl_token::accidental);
 switch(type){
  case octave_chord: // The pitch is represented by a chord symbol.
  case simple_chord:
   if(reference!=NULL){
    int interval=abs(note.diatonic_pitch()-reference->diatonic_pitch());
    if(type==octave_chord) staff.add(OctaveSymbol[note.octave()], brl_token::octave);
    if(interval>=7) interval%=7;
    staff.add(ChordSymbol[interval], brl_token::chord);
   }
  break;
  case normal_note: // A complete representation including pitch, duration, and dots.
   if(octave_shift_switch){
    int real_octave=note.octave(), display_octave=real_octave+extra_octave_shift-global_octave_shift;
    if(1<=display_octave && display_octave<=7) staff.add(OctaveSymbol[display_octave], brl_token::octave_shift);
    staff.add(OctaveSymbol[real_octave], brl_token::octave);
    octave_shift_switch=false;
   }
   else if(reference==NULL || Colonge_key(note, *reference)) staff.add(OctaveSymbol[note.octave()], brl_token::octave);
   if(note.is_breve_long()){ // Breve and longa notes.
    string result;
    result+=step_symbol[1][note.step()];
    result+=step_symbol[0];
    if(note.type()==bms_note::longa) result+=step_symbol[0]; // Longa notes.
    result+=step_symbol[1][note.step()];
    staff.add(result, brl_token::note);
   }
   else staff.add(step_char(note.type(), note.step()), brl_token::note);
   for(int i=0; i<note.dot(); i++) staff.back().back().add_dot();
  break;
 }
 if(note.fingering(0)>0) staff.add(finger[note.fingering(0)], brl_token::fingering);
 if(note.fingering(1)>0) staff.add(finger[note.fingering(1)], brl_token::fingering);
 if(note.fingering(2)>0){
  staff.add(slur_symbol[0], brl_token::fingering); // Substitution of fingerings.
  staff.add(finger[note.fingering(2)], brl_token::fingering);
 }
}
string BrailleOutputer::output_time_signature(const bms_attributes& t) throw(){
 ostringstream buffer;
 string result;
 switch(t.symbol()){
  case bms_attributes::normal: // Normal time signature.
   for(size_t i=0; i<t.beat_pair_count(); i++){
    buffer<<t.beats(i);
    result=buffer.str();
    for(size_t j=0; j<result.length(); j++) result[j]=UpperNumber[result[j]&0x0f];
    buffer.str("");
    buffer<<t.beat_type(i);
    result='#'+result+buffer.str();
   }
  break;
  case bms_attributes::common: // Special "common" time signature.
   result=common_time;
  break;
  case bms_attributes::cut: // Special "cut" time signature.
   result=cut_time;
  break;
  case bms_attributes::single_number: // Single-number time signature.
   for(size_t i=0; i<t.beat_pair_count(); i++){
    buffer<<t.beats(i);
    result=buffer.str();
    for(size_t i=0; i<result.length(); i++) result[i]=UpperNumber[result[i]&0x0f];
    result='#'+result;
   }
  break;
 }
 return result;
}
string BrailleOutputer::page_number_string(size_t i, string n) throw(){
 bool all_digits=true;
 if(n.empty()){
  char buffer[12]={0};
  sprintf(buffer, "%u", i);
  n=buffer; // n becomes the string representation of i.
 } // Note: all_digits remains true when n is empty.
 else for(string::const_iterator p=n.begin(); all_digits && p!=n.end(); p++) if(*p<'0' || '9'<*p) all_digits=false;
 if(all_digits) to_upper_number(n);
 return page_number_sign+n;
}
void BrailleOutputer::to_upper_number(string& n) throw(){
 for(string::iterator p=n.begin(); p!=n.end(); p++) if('0'<=*p && *p<='9') *p=UpperNumber[*p&0x0f];
}
void BrailleOutputer::update_output(const bms_score& score, brl_output& sheet) throw(){
 if(!sheet.empty()) sheet.clear();
 sheet.reset_work(score);
 for(bms_score::const_iterator p=score.begin(); p!=score.end(); p++){
  sheet.set_part_pair(p->part_name(), p->part_abbreviation());
  for(bms_part::const_iterator s=p->begin(); s!=p->end(); s++){
   int all_rest_state=bms_part::no_whole_measure_rest;
   string lyrics;
   clear();
   last_note=NULL; // The previous note.
   sheet.add_new_staff(p->part_name(), p-score.begin()+1, s-p->begin()+1);
   for(bms_staff::const_iterator m=s->begin(); m!=s->end(); m++){
    if(m==s->begin() && p->get_song_music()){
     sheet.back().add(",'", brl_token::music_prefix);
     lyrics+=";2 ";
    }
    if(score.is_new_page(m-s->begin()) && all_rest_state!=bms_part::no_whole_measure_rest && !sheet.back().empty()){
     sheet.back().add(measure_rest_string((m-s->begin())-sheet.back().back().number()+1), brl_token::whole_rests);
     all_rest_state=bms_part::no_whole_measure_rest;
    } // Counting of measure-rests must stop at page boundaries.
    switch(p->measure_attribute(m)){
     case bms_part::whole_measure_rest_start:
      if(all_rest_state&bms_part::whole_measure_rest_start){
       sheet.back().add(measure_rest_string((m-s->begin())-sheet.back().back().number()+1), brl_token::whole_rests);
      }
      all_rest_state=bms_part::whole_measure_rest_start;
     break;
     case bms_part::whole_measure_rest_start|bms_part::whole_measure_rest_stop:
      switch(all_rest_state){
       case bms_part::no_whole_measure_rest: all_rest_state=bms_part::whole_measure_rest_start; break;
       case bms_part::whole_measure_rest_start: all_rest_state|=bms_part::whole_measure_rest_stop; break;
      }
     break;
     case bms_part::whole_measure_rest_stop:
      if(all_rest_state&bms_part::whole_measure_rest_start){
       sheet.back().add(measure_rest_string((m-s->begin())-sheet.back().back().number()+2), brl_token::whole_rests);
       all_rest_state=bms_part::whole_measure_rest_stop;
      }
      else all_rest_state=bms_part::no_whole_measure_rest;
     break;
     default: // This measure is not empty (whole-measure rest).
      if(all_rest_state&bms_part::whole_measure_rest_start){
       sheet.back().add(measure_rest_string((m-s->begin())-sheet.back().back().number()+1), brl_token::whole_rests);
      }
      all_rest_state=bms_part::no_whole_measure_rest;
     break;
    }
    if(~all_rest_state&bms_part::whole_measure_rest_stop) sheet.back()<<brl_measure(m-s->begin()+1); // Adding an empty string(measure).
    if(score.is_new_page(m-s->begin())) sheet.back().add_page_bound(); // Previously added measure becomes the bound.
    for(bms_measure::const_iterator e=m->begin(); e!=m->end(); e++){
     if(e->is_attributes_index()) output_attributes(sheet.back(), score.get_attributes(*e), s->upmost_pitch);
     else if(e->is_barline_index()) output_barline(sheet.back(), score.get_barline(*e));
     else if(e->is_direction_index()) output_direction(sheet.back(), p->direction_at(*e), (e-1)->is_direction_index() ? &p->direction_at(*(e-1)) : NULL);
     else if(e->is_voice_index()){
      const bms_note_area& v=p->note_area_at(*e);
      bool multiple_voices=v.size()>1;
      if(all_rest_state&(bms_part::whole_measure_rest_start|bms_part::whole_measure_rest_stop)) continue;
      for(bms_note_area::const_iterator q=v.begin(); q!=v.end(); q++){
       if(q!=v.begin()) sheet.back().add(in_accord_symbol[0], brl_token::in_accord);
       for(bms_note_area::const_row_iterator r=q->begin(); r!=q->end(); r++){
        if(r!=q->begin()) sheet.back().add(in_accord_symbol[2], brl_token::in_accord);
        if(v.size()==1) multiple_voices=r->size()>1;
        for(bms_in_accord_trait::const_iterator i=r->begin(); i!=r->end(); i++){
         if(i!=r->begin()) sheet.back().add(in_accord_symbol[1], brl_token::in_accord);
         if(multiple_voices) last_note=NULL;
         for(bms_in_accord_trait::const_row_iterator n=i->begin(); n!=i->end(); n++){
          if(n->is_note_pointer()){
           if(!n->as_note()->lyric().empty()){
            lyrics+=n->as_note()->lyric();
            if(n->as_note()->syllabic()==1 || n->as_note()->syllabic()==4) lyrics+=' ';
           }
           for(const bms_note* g=n->as_note()->group_begin(); g!=NULL; g=g->group_next()) output_note(sheet.back(), *g);
          }
          else if(n->is_direction_index()) output_direction(sheet.back(), p->direction_at(*n), (n-1)->is_direction_index() ? &p->direction_at(*(n-1)) : NULL);
          else if(n->is_attributes_index()) output_attributes(sheet.back(), score.get_attributes(*n), s->upmost_pitch);
         }
        }
       }
      }
      if(multiple_voices) last_note=NULL;
     }
    }
   } // Buffered whole-measure rests.
   if(all_rest_state&bms_part::whole_measure_rest_start) sheet.back().add(measure_rest_string(s->size()-sheet.back().back().number()+1), brl_token::whole_rests);
   sheet.back().add(barline[3], brl_token::barline); // Ending barline.
   if(!lyrics.empty()){
    sheet.back()<<brl_measure(0);
    sheet.back().add(lyrics, brl_token::lyric);
   }
   global_octave_shift=extra_octave_shift=0;
  }
 }
}
