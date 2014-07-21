#include <stdlib.h>
#include "score.h"
using namespace std;
bool bms_measure_element::become_pointer_mode(vector<bms_attributes>& data) throw(){
 if(type==a_i){
  pointer.attributes=&data[index.attributes];
  type=a_p;
  return true;
 }
 else return false;
}
bool bms_measure_element::become_pointer_mode(std::vector<bms_barline>& data) throw(){
 if(type==b_i){
  pointer.barline=&data[index.barline];
  type=b_p;
  return true;
 }
 else return false;
}
bool bms_measure_element::become_pointer_mode(std::vector<bms_note_area>& data) throw(){
 if(type==v_i){
  pointer.voice=&data[index.voice];
  type=v_p;
  return true;
 }
 else return false;
}
bms_measure_element bms_measure_element::operator+(int i) const throw(){
 bms_measure_element result;
 switch(type){
  case a_i: result.index.attributes=index.attributes+i; break;
  case b_i: result.index.barline=index.barline+i; break;
  case d_i: result.index.direction=index.direction+i; break;
  case n_i: result.index.note=index.note+i; break;
  case v_i: result.index.voice=index.voice+i; break;
  default: break;
 }
 return result;
}
bms_measure_element bms_measure_element::operator-(int i) const throw(){
 bms_measure_element result;
 switch(type){
  case a_i: result.index.attributes=index.attributes-i; break;
  case b_i: result.index.barline=index.barline-i; break;
  case d_i: result.index.direction=index.direction-i; break;
  case n_i: result.index.note=index.note-i; break;
  case v_i: result.index.voice=index.voice-i; break;
  default: break;
 }
 return result;
}
bms_measure_element& bms_measure_element::operator+=(int i) throw(){
 switch(type){
  case a_i: index.attributes+=i; break;
  case b_i: index.barline+=i; break;
  case d_i: index.direction+=i; break;
  case n_i: index.note+=i; break;
  case v_i: index.voice+=i; break;
  default: break;
 }
 return *this;
}
void bms_measure_element::set_attributes_index(std::vector<bms_attributes>::size_type i) throw(){
 index.attributes=i;
 type=a_i;
}
void bms_measure_element::set_barline_index(std::vector<bms_barline>::size_type i) throw(){
 index.barline=i;
 type=b_i;
}
void bms_measure_element::set_direction_index(std::vector<bms_direction>::size_type i) throw(){
 index.direction=i;
 type=d_i;
}
void bms_measure_element::set_note_index(std::vector<bms_note>::size_type i) throw(){
 index.note=i;
 type=n_i;
}
void bms_measure_element::set_note(bms_note* p) throw(){
 pointer.note=p;
 type=n_p;
}
void bms_measure_element::set_voice_index(std::vector<bms_note_area>::size_type i) throw(){
 index.voice=i;
 type=v_i;
}
void bms_part::clear() throw(){
 vector<bms_staff>::clear(); // Inherited from parent class.
 element.direction.clear();
 for(note_iterator p=note_begin(); p!=note_end(); p++) delete *p;
 element.note.clear();
 voice.clear();
}
bms_part::const_note_iterator bms_part::find_note(const bms_note* a) const throw(){
 const_note_iterator p=note_begin();
 for(; p!=note_end() && *p!=a; p++);
 return p;
}
int bms_part::measure_attribute(const bms_staff::const_iterator& m) const throw(){
 const bms_measure_element* p=NULL;
 int c=~(whole_measure_rest_start|whole_measure_rest_stop); // Conceptually inverted result.
 for(bms_measure::const_iterator q=m->begin(); q!=m->end(); q++){
  if(p==NULL){
   if(q->is_note_pointer() || q->is_voice_index()) p=&*q;
   else c|=whole_measure_rest_stop; // This measure CANNOT be a stop.
  }
  else if(!q->is_direction_index() || direction_at(*q).direction_kind()!=8) c|=whole_measure_rest_start;
 }
 if(~c==0 || p==NULL) return 0;
 else if(p->is_voice_index()){
  const bms_note_area& a=note_area_at(*p);
  const bms_note* w=NULL;
  if(a.size()>1 || a.front().size()>1 || a.front().front().size()>1) return 0;
  for(bms_in_accord_trait::const_row_iterator n=a.front().front().front().begin(); n!=a.front().front().front().end(); n++){
   if(w==NULL){
    if(n->is_note_pointer()) w=n->as_note();
    else c|=whole_measure_rest_stop;
   }
   else if(!n->is_direction_index() || direction_at(*n).direction_kind()!=8) c|=whole_measure_rest_start;
  }
  return w->rest() && w->type()==bms_note::whole && w->printed() ? ~c : 0;
 }
 else return p->as_note()->rest() && p->as_note()->type()==bms_note::whole && p->as_note()->printed() ? ~c : 0;
}
void bms_part::set_abbreviation(const string& a) throw(){
 string::size_type range[2]={0};
 if(a.empty()) return;
 while(a[range[0]]==' ' || a[range[0]]=='\t' || a[range[0]]=='\n') if(++range[0]==a.length()) return;
 for(range[1]=a.length()-1; a[range[1]]==' ' || a[range[1]]=='\t' || a[range[1]]=='\n'; range[1]--);
 abbreviation=a.substr(range[0], range[1]+1);
}
size_t bms_score::add_instrument() throw(){
 instrument.resize(instrument.size()+1);
 instrument.back().midi.channel=1;
 instrument.back().midi.program=1;
 instrument.back().midi.volume=80;
 instrument.back().midi.pan=0;
 return instrument.size()-1;
}
void bms_score::clear() throw(){
 page_map::clear();
 instrument.clear();
 score.clear();
 midi_stream.clear();
 work_number.clear();
 work_title.clear();
 composer.clear();
 lyricist.clear();
 arranger.clear();
}
size_t bms_score::page_begin(page_table_const_iterator p) const throw(){
 return (p!=page_table.end() ? p->first : getMeasureNum())+measure_alignment;
}
size_t bms_score::page_end(page_table_const_iterator p) const throw(){
 return (p!=page_table.end() && ++p!=page_table.end() ? p->first : getMeasureNum())+measure_alignment;
}
midi_instrument_map_t bms_score::midi_instrument_map() const throw(){
 midi_instrument_map_t m;
 for(size_t i=0; i<instrument.size(); i++) m[i]=midi_instrument(i);
 return m;
}
void bms_score::set_arranger(const string& a, bool m) throw(){
 if(m) arranger=a; // Write-and-clear mode.
 else if(a.length()>0){ // Append mode.
  if(arranger.length()>0) arranger+=", ";
  arranger+=a;
 }
}
void bms_score::set_composer(const string& c, bool m) throw(){
 if(m) composer=c;
 else if(c.length()>0){
  if(composer.length()>0) composer+=", ";
  composer+=c;
 }
}
void bms_score::set_lyricist(const string& l, bool m) throw(){
 if(m) lyricist=l;
 else if(l.length()>0){
  if(lyricist.length()>0) lyricist+=", ";
  lyricist+=l;
 }
}
bool Colonge_key(const bms_note& now, const bms_note& last) throw(invalid_argument){
 int distance=abs(now.diatonic_pitch()-last.diatonic_pitch());
 switch(distance+1){
  case 1:
  case 2:
  case 3: return false;
  case 4:
  case 5: return now.octave()!=last.octave();
  default: return true;
 }
}
