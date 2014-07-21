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
#include <sstream>
#include <conio.h>
#include <process.h>
#define __MIDL_user_allocate_free_DEFINED__
#include <windows.h>
#include <Servprov.h>
#include <sapi.h>
#include "reader.h"
#include "reader_words.h"
#define READING_LENGTH_LIMIT 172
using namespace std;
using namespace MusicXML2;
typedef struct{unsigned short kind:4, above:4, below:4;} ornament_t;
static map<string, string> dictionary;
static const char* accidental_mark_string(int a) throw(){
 if(0<=a && a<11) return accidental_word[a];
 else return "";
}
static string articulation_string(unsigned short a) throw(){
 string result;
 if(a&bms_note::note_staccato){
  if(!result.empty()) result+="、";
  result+=staccato_word;
 }
 if(a&bms_note::note_spiccato){
  if(!result.empty()) result+="、";
  result+=spiccato_word;
 }
 if(a&bms_note::note_staccatissimo){
  if(!result.empty()) result+="、";
  result+=staccatissimo_word;
 }
 if(a&bms_note::note_accent){
  if(!result.empty()) result+="、";
  result+=accent_word;
 }
 if(a&bms_note::note_strong_accent){
  if(!result.empty()) result+="、";
  result+=strong_accent_word;
 }
 if(a&bms_note::note_detached_legato){
  if(!result.empty()) result+="、";
  result+=detached_legato_word;
 }
 if(a&bms_note::note_tenuto){
  if(!result.empty()) result+="、";
  result+=tenuto_word;
 }
 return result;
}
static ornament_t convert_ornament(const bms_note& n) throw(){
 ornament_t result={0};
 if(n.trill_kind()>0){
  result.kind=n.trill_kind();
  result.above=n.accidental_mark_above();
  result.below=n.accidental_mark_below();
 }
 return result;
}
static string direction_string(const string& text) throw(){
 if(!text.empty()){
  char* buffer=(char*)malloc(text.length()+1);
  string phrase, result;
  strcpy(buffer, text.c_str());
  for(char* p=strtok(buffer, " .\n\r\t"); p!=NULL; p=strtok(NULL, " .\n\r\t")){
   for(char* q=p; *q!='\0'; q++) if('A'<=*q && *q<='Z') *q|=0x20;
   phrase=dictionary[p];
   if(!phrase.empty()) result+=phrase;
  }
  if(!result.empty()) result=("文字「"+result+"」")+Comma;
  free(buffer);
  return result;
 }
 else return "";
}
static string dot_string(int dot) throw(){
 string result;
 if(dot>0){
  if(dot==2) result+="複";
  else if(dot>2) result+=number_word[dot];
  result+="附點";
 }
 return result;
}
static void initialize_dictionary() throw(){
 dictionary["allegro"]="快板";
 dictionary["andante"]="行板";
 dictionary["cresc"]="漸強";
 dictionary["dim"]=dictionary["dimin"]="漸弱";
 dictionary["dolce"]="柔美的";
 dictionary["f"]="強";
 dictionary["ff"]="甚強";
 dictionary["legato"]="圓滑的";
 dictionary["mf"]="中強";
 dictionary["mp"]="中弱";
 dictionary["p"]="弱";
 dictionary["poco"]="稍微";
 dictionary["pp"]="甚弱";
 dictionary["rit"]="漸慢";
 dictionary["sf"]="突強";
 dictionary["sfp"]="突強後弱";
 dictionary["sfpp"]="突強後甚弱";
 dictionary["simile"]="相似的";
 dictionary["ten"]="持音";
}
static string note_type_string(bms_note::note_type_t t) throw(){
 switch(t){
  case bms_note::longa: return type_word[0];
  case bms_note::breve: return type_word[1];
  case bms_note::whole: return type_word[2];
  case bms_note::half: return type_word[3];
  case bms_note::quarter: return type_word[4];
  case bms_note::eighth: return type_word[5];
  case bms_note::_16th: return type_word[6];
  case bms_note::_32nd: return type_word[7];
  case bms_note::_64th: return type_word[8];
  case bms_note::_128th: return type_word[9];
  case bms_note::_256th: return type_word[10];
 }
 return "";
}
static string octave_string(unsigned long flags, int octave) throw(){
 string sentence;
 switch(flags&ORAL_OCTAVE_REPRESENTATION_BITS){
  default: // This selection is adapted when an exception arises.
  case ORAL_OCTAVE_NUM: // The n-th interval for 0<=n<=9.
   sentence="第";
   sentence+=number_word[octave];
   sentence+="音層";
  break;
  case ORAL_OCTAVE_BRL: // Point numbers in brl score.
   sentence=Ocatave_word[octave];
  break;
  case ORAL_OCTAVE_ABS: // Absolute octave.
   sentence=absolute_octave_word[octave];
  break;
 }
 return sentence;
}
static string ornament_string(ornament_t o) throw(){
 if(o.kind>0){ // There is an ornament specification.
  string result;
  if(o.above<11) result+=string("上面")+accidental_word[o.above];
  if(o.below<11){
   if(!result.empty()) result+="、";
   result+=string("下面")+accidental_word[o.below];
  }
  if(!result.empty()) result+="的";
  return result+ornament_word[o.kind];
 }
 else return "";
}
static string number_string(int n) throw(){
 string result;
 if(n<0){ // Negative numbers.
  if(n==1<<31){ // Special case: The result is -2^31 after 2's complement.
   result=number_word[15]+number_string(~n);
   result.replace(result.length()-2, result.length(), number_word[8]);
  }
  else result=number_word[15]+number_string(-n);
 }
 else if(n==0) return number_word[0];
 else{
  if(n<10000){
   int e3=n/1000, e2=(n%1000)/100, e1=(n%100)/10;
   n%=10;
   if(n>0) result=number_word[n];
   if(e1>0){ // Ten(s).
    result=number_word[10]+result;
    if(e1>1 || e2>=1 || e3>=1) result=number_word[e1]+result;
   }
   if(e2>0){ // Hundred(s).
    if(n>0 && e1==0) result=number_word[0]+result;
    result=number_word[e2]+(number_word[11]+result);
   }
   if(e3>0){ // Thousand(s).
    if((n>0 || e1>0)&& e2==0) result=number_word[0]+result;
    result=number_word[e3]+(number_word[12]+result);
   }
  }
  else{
   int e8=n/100000000, e4=(n/10000)%10000;
   n%=10000;
   if(n>0) result+=number_string(n);
   if(e4>0){
    if(0<n && n<1000) result=number_word[0]+result;
    result=number_string(e4)+number_word[13]+result;
   }
   if(e8>0){
    if(n>0 && e4==0) result+=number_word[0];
    result=number_string(e8)+number_word[14]+result;
   }
  }
 }
 return result;
}
static string skill_string(unsigned short a, ornament_t o, int fermata, const char* tag) throw(){
 string result[2];
 result[0]=articulation_string(a);
 result[1]=ornament_string(o);
 if(!result[0].empty() && !result[1].empty()) result[0]+="及";
 result[0]+=result[1];
 if(fermata<3){
  if(!result[0].empty()) result[0]+="及";
  result[0]+=fermata_word[fermata];
 }
 if(result[0].empty()) return "";
 else{
  if(fermata==3) result[0]+="記號";
  result[0]="旁邊有"+result[0];
  return tag+result[0]+Comma;
 }
}
static string slur_and_tied_string(const bms_note& n, size_t tied_stop, size_t tied_start) throw(){ // For chords.
 string result[2];
 if(n.slur_number(TYPE_STOP)>0) result[0]=(number_string(n.slur_number(TYPE_STOP))+"條")+slur_word;
 if(tied_stop>0){
  if(!result[0].empty()) result[0]+="和";
  result[0]+=(number_string(tied_stop)+"條")+tied_word;
 }
 if(!result[0].empty()){
  result[0]+="在剛才的和弦結束";
  result[0]+=Comma;
 }
 if(n.slur_number(TYPE_START)>0) result[1]=(number_string(n.slur_number(TYPE_START))+"條")+slur_word;
 if(tied_start>0){
  if(!result[1].empty()) result[1]+="和";
  result[1]+=(number_string(tied_start)+"條")+tied_word;
 }
 if(!result[1].empty()){
  result[1]+="從剛才的和弦開始";
  result[1]+=Comma;
 }
 return result[0]+result[1];
}
static string slur_and_tied_string(const bms_note& n) throw(){ // For single notes.
 string result[2];
 if(n.slur_number(TYPE_STOP)>0) result[0]=slur_word;
 if(n.tied_type()==TYPE_STOP){
  if(!result[0].empty()) result[0]+="和";
  result[0]+=tied_word;
 }
 if(!result[0].empty()){
  result[0]+="在剛才的音符結束";
  result[0]+=Comma;
 }
 if(n.slur_number(TYPE_START)>0) result[1]=slur_word;
 if(n.tied_type()==TYPE_START){
  if(!result[1].empty()) result[1]+="和";
  result[1]+=tied_word;
 }
 if(!result[1].empty()){
  result[1]+="從剛才的音符開始";
  result[1]+=Comma;
 }
 return result[0]+result[1];
}
static string total_clef_string(const bms_attributes& a) throw(){
 string result;
 if(0<a.sign()){
  if(a.clef_octave_change()!=0) result+=(a.clef_octave_change()>0 ? "高" : "低")+number_string(7*abs(a.clef_octave_change())+1)+"度記譜的";
  if(1<=a.sign() && a.sign()<=3 && 1<=a.line() && a.line()<=6) result+="第"+number_string(a.line())+"線";
  result+=clef_word[a.sign()];
 }
 return result+Comma;
}
ScoreReader::ScoreReader() throw(){
 CoInitialize(NULL); // Initialize COM.
 if(dictionary.empty()) initialize_dictionary();
 initialize_range();
 state.rate=0; // Undefined now.
 state.volume=1000; // The maximum volume defined in MCI.
 state.flags=ORAL_CLEF|ORAL_KEY|ORAL_TIME|ORAL_RIGHT_HAND|ORAL_STAFFWISE|ORAL_PITCH|ORAL_OCTAVE_NUM|ORAL_DURATION|ORAL_CHORD_DIRECTION_FOLLOWING_BRL;
}
ScoreReader::~ScoreReader() throw(){
 CoUninitialize();
}
void ScoreReader::convert(const bms_score& score, vector<string>& content) const throw(){
 ostringstream sentence;
 const bms_note* last_note=NULL;
 bool multiple_voices=false, previous_multiple_voices=false;
 for(bms_staff::const_iterator m=score.staff(range).begin(); m!=score.staff(range).end(); m++){
  if(m-score.staff(range).begin()+1<range.begin) continue;
  else if(m-score.staff(range).begin()+1>range.end) break;
  sentence.str(""); // String data in the stream is cleared.
  sentence<<"第"<<number_string(m-score.staff(range).begin()+1)<<"小節"<<Comma;
  for(bms_measure::const_iterator e=m->begin(); e!=m->end(); e++){
   if(e->is_attributes_index()){
    read_attribute(sentence, score.get_attributes(*e));
    last_note=NULL; // The previous-note state is reset.
   }
   else if(e->is_barline_index()) read_barline(sentence, score.get_barline(*e), last_note);
   else if(e->is_direction_index()) read_direction(sentence, score.part(range).direction_at(*e));
   else if(e->is_voice_index()){
    const bms_note_area& v=score.part(range).note_area_at(*e);
    multiple_voices=v.size()>1;
    if(v.size()>1) sentence<<"接下來全小節分成"<<number_string(v.size())<<"個"<<in_accord_word<<Comma;
    else if(previous_multiple_voices) sentence<<"接下來變成"<<number_string(1)<<"個"<<in_accord_word<<Comma;
    for(bms_note_area::const_iterator q=v.begin(); q!=v.end(); q++){
     if(v.size()>1) sentence<<"第"<<number_string(q-v.begin()+1)<<"個"<<in_accord_word<<Comma;
     if(q->size()>1){
      if(v.size()>1) sentence<<"該"<<in_accord_word;
      else sentence<<"這個小節";
      sentence<<"分成"<<number_string(q->size())<<"個段落"<<Comma;
     }
     for(bms_note_area::const_row_iterator r=q->begin(); r!=q->end(); r++){
      if(v.size()>1) multiple_voices=r->size()>1;
      if(q->size()>1) sentence<<"第"<<number_string(r-q->begin()+1)<<"個段落"<<Comma;
      if(r->size()>1) sentence<<"再分成"<<number_string(r->size())<<"個"<<in_accord_word<<Comma;
      for(bms_in_accord_trait::const_iterator i=r->begin(); i!=r->end(); i++){
       if(r->size()>1) sentence<<"第"<<number_string(i-r->begin()+1)<<"個"<<in_accord_word<<Comma;
       if(multiple_voices) last_note=NULL;
       for(bms_in_accord_trait::const_row_iterator n=i->begin(); n!=i->end(); n++){
        if(n->is_note_pointer()){
         if(n->as_note()->grace()) continue;
         for(const bms_note* g=n->as_note()->group_begin(); g!=NULL; g=g->group_next()) read_note(sentence, *g, last_note);
        }
        else if(n->is_direction_index()) read_direction(sentence, score.part(range).direction_at(*n));
        else if(n->is_attributes_index()){
         read_attribute(sentence, score.get_attributes(*n));
         last_note=NULL; // The previous-note state is reset.
        }
       }
      }
     }
    }
    if(multiple_voices) last_note=NULL;
    previous_multiple_voices=multiple_voices;
   }
  }
  sentence<<"小節結束。";
  content.push_back(sentence.str());
  sentence.clear();
 }
}
void ScoreReader::initialize_range() throw(){
 range.part=range.staff=1;
 range.begin=1;
 range.end=numeric_limits<int>::max();
}
key_t ScoreReader::read(Front& face) const throw(){
 vector<string> W; // A string vector of measures.
 SPVOICESTATUS status={0}; // Running state of speaker.
 struct{wchar_t* buffer; size_t length;} text={0}; // Text buffer for speaker.
 ISpVoice* speaker=NULL; // The voice interface.
 FILE* g=fopen("guide.txt", "wt");
 key_t input=0;
 convert(face.score(), W);
 if(g!=NULL){
  for(size_t i=0; i<W.size(); i++) fprintf(g, "%s\n", W[i].c_str());
  fclose(g);
  spawnl(P_NOWAIT, "C:\\WINDOWS\\system32\\notepad.exe", "notepad.exe", "guide.txt", NULL);
 }
 CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&speaker); // Create the voice interface object.
 for(vector<string>::iterator it=W.begin(); it!=W.end(); it++){
  if(it->length()>READING_LENGTH_LIMIT){
   int eos=READING_LENGTH_LIMIT; // End of sentence.
   for(; eos>=0; eos--) if((*it)[eos]==Comma[0] && (*it)[eos+1]==Comma[1]) break;
   it=W.insert(it, it->substr(0, eos<0 ? READING_LENGTH_LIMIT : eos));
   *(it+1)=(it+1)->substr(eos<0 ? READING_LENGTH_LIMIT : eos, (it+1)->length());
  }
  text.length=MultiByteToWideChar(CP_ACP, 0, it->c_str(), it->length(), NULL, 0);
  text.buffer=(wchar_t*)realloc(text.buffer, (text.length+1)*sizeof(*text.buffer));
  MultiByteToWideChar(CP_ACP, 0, it->c_str(), it->length(), text.buffer, text.length);
  text.buffer[text.length]=0;
  speaker->Speak(text.buffer, SPF_ASYNC|SPF_IS_XML, NULL);
  do{//~~
   switch(input=face.browse(-1L)){
    case CTRL_P_KEY:
    speaker->Pause();
    do input=face.browse();
    while(input!=CTRL_P_KEY && input!=ESC_KEY && input!=PERIOD_KEY);
    if(input==CTRL_P_KEY){
     speaker->Resume();
     break;
    } // Another 2 keys cause the following result.
    case ESC_KEY:
    case PERIOD_KEY:
    goto stop_reading;
   }
   speaker->GetStatus(&status, NULL);
  }while(status.dwRunningState!=SPRS_DONE);
 }
 stop_reading:
 speaker->Release();
 remove("guide.txt");
 return input;
}
void ScoreReader::read_attribute(ostringstream& sentence, const bms_attributes& a) const throw(){
 switch(a.kind()){
  case bms_attributes::clef: // The clef information.
   if(state.flags&ORAL_CLEF){
    if(a.change()){
     if(a.sign()>0) sentence<<"譜號轉變成"<<total_clef_string(a);
    }
    else sentence<<total_clef_string(a);
   }
   else if(a.sign()<=3 && a.sign()!=2) sentence<<hand_word[a.sign()]<<Comma;
  break;
  case bms_attributes::key: // Key signature.
   if(state.flags&ORAL_KEY){
    if(a.change() &&(a.fifths()!=0 || a.cancel()!=0)) sentence<<"調號轉變成";
    if(a.fifths()>0) sentence<<Num_word[a.fifths()]<<"個"<<accidental_word[8]<<Comma;
    else if(a.fifths()<0) sentence<<Num_word[-a.fifths()]<<"個"<<accidental_word[2]<<Comma;
    else if(a.change() && a.cancel()!=0) sentence<<"無任何升降"<<Comma;
   }
  break;
  case bms_attributes::time: // Time signature.
   if(state.flags&ORAL_TIME){
    if(a.change()) sentence<<"拍號轉變成";
    if(a.beat_pair_count()>1) sentence<<"混合節拍依序為";
    switch(a.symbol()){
     case bms_attributes::normal: // Normal time signature.
      if(a.beat_pair_count()>0){
       for(size_t i=0; i<a.beat_pair_count(); i++){
        if(a.beats(i)<10 && a.beat_type(i)<10) sentence<<number_word[a.beats(i)]<<number_word[a.beat_type(i)]<<"拍";
        else sentence<<"每小節"<<number_string(a.beats(i))<<"個"<<number_string(a.beat_type(i))<<"分音符";
        sentence<<Comma;
       }
      }
      else sentence<<special_time_word[2]<<Comma;
     break;
     case bms_attributes::common: // Special "common" time signature.
      sentence<<special_time_word[0]<<Comma;
     break;
     case bms_attributes::cut: // Special "cut" time signature.
      sentence<<special_time_word[1]<<Comma;
     break;
     case bms_attributes::single_number: // Single-number time signature.
      for(size_t i=0; i<a.beat_pair_count(); i++) sentence<<"一個數字"<<number_string(a.beats(i))<<"的拍號"<<Comma;
     break;
    }
   }
  break;
 }
}
void ScoreReader::read_barline(ostringstream& sentence, const bms_barline& b, const bms_note*& p) const throw(){
 if(b.notations()&4){ // Repeat marks.
  sentence<<barline_word[b.notations()&8 ? 5 : 6]<<Comma;
  p=NULL;
 }
 else if(b.bar_style()==4){
  sentence<<barline_word[4]<<Comma;
  p=NULL;
 }
 if(b.notations()&(16|2|1)){ // Coda, fermata, or segno.
  if(b.notations()&1) sentence<<coda_word<<Comma;
  else if(b.notations()&2) sentence<<fermata_word<<Comma;
  else if(b.notations()&16) sentence<<segno_word<<Comma;
 }
 if(!b.ending_number().empty()){ // Endings.
  string number=b.ending_number();
  size_t n1=0, n2=0;
  sentence<<"第";
  n1=number.find_first_of(",");
  if(n1==string::npos){
   sscanf(number.c_str(), "%u", &n2);
   sentence<<number_string(n2);
  }
  else{ // Multiple ending numbers.
   sscanf(number.substr(0, n1).c_str(), "%u", &n2);
   sentence<<number_string(n2)<<"到";
   n2=number.find_last_of(",");
   sscanf(number.substr(n2, number.length()).c_str(), "%u", &n1);
   sentence<<number_string(n1);
  }
  sentence<<"次結束記號"<<Comma;
  p=NULL;
 }
}
void ScoreReader::read_direction(std::ostringstream& sentence, const bms_direction& d) const throw(){
 switch(d.direction_kind()){
  case bms_direction::dashes:
   switch(d.type()){
    case TYPE_START: sentence<<dashes_word[0]<<Comma; break;
    case TYPE_STOP: sentence<<dashes_word[1]<<Comma; break;
   }
  break;
  case bms_direction::octave_shift:
   switch(d.type()){
    case TYPE_DOWN: sentence<<octave_shift_word[0]; goto shift_size;
    case TYPE_UP: sentence<<octave_shift_word[1]; goto shift_size;
    shift_size: sentence<<number_string(d.diatonic_step_change())<<octave_shift_word[2]<<Comma; break;
    case TYPE_STOP: sentence<<octave_shift_word[3]<<Comma; break;
   }
  break;
  case bms_direction::pedal:
   switch(d.type()){
    case TYPE_START: sentence<<pedal_word[d.is_half_pedaling() ? 3 : 0]<<Comma; break;
    case TYPE_STOP: sentence<<pedal_word[1]<<Comma; break;
    case TYPE_CHANGE: sentence<<pedal_word[2]<<Comma; break;
   }
  break;
  case bms_direction::wedge: sentence<<wedge_word[d.type()]<<Comma; break;
  case bms_direction::words: sentence<<direction_string(d.text()); break;
 }
}
void ScoreReader::read_note(ostringstream& sentence, const bms_note& n, const bms_note*& p) const throw(){
 list<const bms_note*> chord;
 for(const bms_note* c=&n; c!=NULL; c=c->next_chord()) chord.push_back(c);
 for(size_t i=0; i<6; i++) if(n.tuplet_type(i)==TYPE_START) sentence<<number_string(n.tuplet_number(i, false))<<"連音開始"<<Comma;
 if(chord.size()>1){ // Chord notes.
  if(n.arpeggiate_number()>0) sentence<<"有"<<arpeggiate_word[n.arpeggiate_kind()]<<"的";
  sentence<<"和弦包含"<<Num_word[chord.size()]<<"個音"<<Comma;
  if(state.flags&ORAL_PITCH){
   sentence<<"主音為";
   switch(state.flags&ORAL_CHORD_DIRECTION_BITS){
    case ORAL_CHORD_DOWNWARD_ONLY:
     if(*chord.front()<*chord.back()) chord.reverse();
    break; // The chord becomes non-increasing order.
    case ORAL_CHORD_UPWARD_ONLY:
     if(*chord.back()<*chord.front()) chord.reverse();
    break; // The chord becomes non-decreasing order.
   }
  }
 }
 if(state.flags&ORAL_PITCH){ // The pitch must be shown in the reading.
  const char* acci_str=accidental_mark_string(n.accidental());
  if(acci_str[0]!='\0') sentence<<acci_str<<Comma;
  if(!n.rest() &&(state.flags&ORAL_OCTAVE_EVERYTIME || p==NULL || Colonge_key(n, *p))) sentence<<octave_string(state.flags, n.octave())<<Comma;
 } // Octave part of pitch.
 if(!n.rest()){ // Pitched notes. They have steps and types.
  if(state.flags&ORAL_PITCH) sentence<<step_word[n.step()]<<Comma;
  if(state.flags&ORAL_DURATION){
   if(n.grace()){ // This note group has grace(s).
    const char* form=NULL;
    switch(n.steal_time_following()>0 ? n.center_note()->prefix() : n.center_note()->suffix()){
     case 0: break; // Unwanted exception.
     case 1: form=grace_form[n.is_long_grace() ? 0 : 1]; break;
     case 2: form=grace_form[n.steal_time_following()>0 ? 2 : 3]; break;
     default: form=grace_form[4]; break;
    }
    sentence<<form<<grace_word<<Comma;
   }
   else sentence<<dot_string(n.dot())<<note_type_string(n.type())<<"音符"<<Comma;
  }
 }
 else{ // Rest notes have types only.
  if(state.flags&ORAL_DURATION) sentence<<dot_string(n.dot())<<note_type_string(n.type());
  sentence<<rest_word<<Comma;
 }
 if(state.flags&ORAL_PITCH && chord.size()>1){ // Chord notes.
  size_t tied_count[2]={0};
  int root_pitch=chord.front()->diatonic_pitch(), fermata=chord.front()->fermata();
  unsigned short articulations=chord.front()->articulation_flag();
  ornament_t ornament=convert_ornament(*chord.front());
  switch(chord.front()->tied_type()){
   case 1: tied_count[0]=1; break;
   case 2:
    tied_count[0]=1;
    tied_count[1]=1;
   break;
   case 3: tied_count[1]=1; break;
  }
  p=chord.front(); // Index of the previous note.
  chord.pop_front(); // The main note is discarded.
  for(list<const bms_note*>::const_iterator i=chord.begin(); i!=chord.end(); i++){
   const char* acci_str=accidental_mark_string((*i)->accidental());
   if(acci_str[0]!='\0') sentence<<acci_str<<Comma;
   switch(state.flags&ORAL_CHORD_DIRECTION_BITS){
    default: // Standard mode.
    case ORAL_CHORD_DIRECTION_FOLLOWING_BRL:
     if(root_pitch<chord.back()->diatonic_pitch()) sentence<<"往上";
     else sentence<<"往下";
     sentence<<number_string(abs((*i)->diatonic_pitch()-root_pitch)+1)<<"度"<<Comma;
    break;
    case ORAL_CHORD_DOWNWARD_ONLY:
     if(state.flags&ORAL_OCTAVE_EVERYTIME) sentence<<octave_string(state.flags, (*i)->octave())<<"的";
     sentence<<step_word[(*i)->step()]<<Comma;
    break;
    case ORAL_CHORD_UPWARD_ONLY:
     if(state.flags&ORAL_OCTAVE_EVERYTIME) sentence<<octave_string(state.flags, (*i)->octave())<<"的";
     sentence<<step_word[(*i)->step()]<<Comma;
    break;
   }
   switch((*i)->tied_type()){
    case 1: tied_count[0]++; break;
    case 2:
     tied_count[0]++;
     tied_count[1]++;
    break;
    case 3: tied_count[1]++; break;
   }
   if((*i)->fermata()<fermata) fermata=(*i)->fermata();
   articulations|=(*i)->articulation_flag();
   if(ornament.kind==0) ornament=convert_ornament(**i);
  }
  sentence<<skill_string(articulations, ornament, fermata, "和弦");
  sentence<<slur_and_tied_string(n, tied_count[1], tied_count[0]);
 }
 else{ // Non-chord notes.
  if(!n.rest()) p=&n; // Index of the previous note.
  if(n.arpeggiate_number()>0) sentence<<"為"<<arpeggiate_word[n.arpeggiate_kind()]<<"的一部份"<<Comma;
  sentence<<skill_string(n.articulation_flag(), convert_ornament(n), n.fermata(), "音符");
  sentence<<slur_and_tied_string(n);
 }
 for(size_t i=0; i<6; i++) if(n.tuplet_type(i)==TYPE_STOP) sentence<<number_string(n.tuplet_number(i, false))<<"連音結束"<<Comma;
}
