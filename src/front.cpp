#include <sstream>
#include <conio.h>
#include <io.h>
#include <shlwapi.h>
#include <time.h>
#include <windows.h>
#include "braille_outputer.h" // "front.h" is included.
#include "pseudo_GUI_module.h"
#include "xml_analyzer.h"
#define clrscr() 	system("cls")
using namespace std;
static const char* octave_expression[3]={"Octave number", "Braille symbol", "Absolute pitch"};
/* Temporarily be unused.
static const char* chord_direction[3]={"From low to high", "From high to low", "According to the braille"};
static const char* chord_pitch[3]={"Note-style Braille Score", "Interval-style Braille Score", "Absolute interval"};
*/
static const char* sound_gender[2]={"Male", "Female"};
const choice_t Front::main_menu_range=7, Front::option_menu_entries=13;
Front Front::console=Front();
void Front::cursor_control_block::append_step(short row, short column) throw(){
 if(step.count(row)==0){
  step[row]=line_content(1);
  step[row][0]=column;
 }
 else step[row].push_back(column);
}
short Front::cursor_control_block::column() const throw(){
 return index==0 ? 0 : step.find(line)->second.at(index);
}
void Front::cursor_control_block::go_down(short bottom) throw(){
 if(line<bottom){
  if(step.count(line)>0){
   if(step.count(line+1)>0){
    size_t i=0;
    while(i<step[line+1].size() && step[line+1][i]<step[line][index]) i++;
    if(i>=step[line+1].size()) i--;
    index=i;
   }
   else index=0;
  }
  else index=0;
  line++;
 }
}
void Front::cursor_control_block::go_end() throw(){
 index= step.count(line)>0 ? (step[line].size()-1) : 0;
}
void Front::cursor_control_block::go_first_home() throw(){
 line=0;
 go_home();
}
void Front::cursor_control_block::go_home() throw(){
 index=0;
}
void Front::cursor_control_block::go_last_end(short bottom) throw(){
 line=bottom;
 go_end();
}
void Front::cursor_control_block::go_left() throw(){
 if(step.count(line)){
  if(index>0) index--;
 }
 else index=0;
}
void Front::cursor_control_block::go_right() throw(){
 if(step.count(line)){
  if(index+1<(int)step[line].size()) index++;
 }
 else index=0;
}
void Front::cursor_control_block::go_up() throw(){
 if(line>0){
  if(step.count(line)){
   if(step.count(line-1)){
    size_t i=0;
    while(i<step[line-1].size() && step[line-1][i]<step[line][index]) i++;
    if(i>=step[line-1].size()) i--;
    index=i;
   }
   else index=0;
  }
  else index=0;
  line--;
 }
}
Front::Front() throw(): main_choice(0){
 SetConsoleTitleA("Touching Melody");
 state.layout=LAYOUT_DEFAULT;
 state.max_char_screen=state.max_char_file=40;
}
void Front::assign_step(short row, size_t column) throw(){
 div_t location=div(row, 25);
 if((size_t)location.quot>=page_frame.size()) page_frame.resize(location.quot+1);
 page_frame[location.quot].append_step(location.rem, column);
}
void Front::assign_text(short row, string text, string::size_type alignment) throw(){
 div_t location=div(row, 25);
 if((size_t)location.quot>=page_frame.size()) page_frame.resize(location.quot+1);
 while(text.length()<alignment) text=' '+text;
 page_frame[location.quot].put(location.rem, text);
}
void Front::clear_key_queue() throw(){
 while(kbhit()) getch();
}
void Front::midi_debug(bool (*sound)(unsigned long) throw(), bool percussion) throw(){
 FILE* instrument_data=NULL;
 if(sound==NULL) return;
 instrument_data=fopen(percussion ? "synthesizer/MIDI_percussion.mnu" : "synthesizer/MIDI_instrument.mnu", "rb");
 if(instrument_data!=NULL){
  struct{char* buffer; const char** entry; size_t size[2];} data={0};
  fseek(instrument_data, 0, SEEK_END);
  data.size[0]=ftell(instrument_data);
  if(data.buffer=(char*)malloc(data.size[0]+1)){
   rewind(instrument_data);
   fread(data.buffer, 1, data.size[0], instrument_data);
   data.buffer[data.size[0]]='\0';
   for(char* p=data.buffer; *p!='\0'; p++){
    if(*p=='\r'){
     *p='\n';
     data.size[1]++;
    }
   }
   if(data.entry=(const char**)malloc((data.size[1]+1)*sizeof(*data.entry))){
    GUI_element* element[3]={NULL};
    size_t element_count= percussion ? 2 : 3;
    midi_message_t x;
    data.size[1]=0;
    for(char* p=strtok(data.buffer, "\n"); p!=NULL; p=strtok(NULL, "\n")) data.entry[data.size[1]++]=p;
    element[0]=new select_box(2, 0, "Instrument", 0, data.size[1], data.entry);
    if(percussion) element[1]=new number_box(4, 0, "Dynamics", 0, 127, 127);
    else{
     element[1]=new number_box(3, 0, "Pitch", 12, 108, 60);
     element[2]=new number_box(4, 0, "Dynamics", 0, 127, 127);
    }
    clrscr();
    _cputs("MIDI-debug panel:");
    for(size_t i=0; i<element_count; i++){
     element[i]->add_escape_key(3, BACK_KEY, ENTER_KEY, TAB_KEY);
     element[i]->paint();
    }
    for(int i=0, t=0;;){
     switch(element[i]->focus()){
      case BACK_KEY: goto quit_debug;
      case ENTER_KEY:
       if(x.command_byte[2]!='\0') sound(ALL_NOTES_OFF(0));
       if(percussion){
        t=atoi(data.entry[element[0]->get_value().as_integer]);
        x.command_byte[0]=0x99; // NoteOn message on channel 10.
        x.command_byte[1]=(unsigned char)t;
        t=element[1]->get_value().as_integer;
        x.command_byte[2]=(unsigned char)t;
       }
       else{
        t=atoi(data.entry[element[0]->get_value().as_integer]);
        x.command_byte[0]=0xc0; // ProgramChange message on channel 1.
        x.command_byte[1]=(unsigned char)(t-1);
        x.command_byte[2]=0;
        sound(x.command_word);
        t=element[1]->get_value().as_integer;
        x.command_byte[0]=0x90; // NoteOn message on channel 1.
        x.command_byte[1]=(unsigned char)t;
        t=element[2]->get_value().as_integer;
        x.command_byte[2]=(unsigned char)t;
       }
       sound(x.command_word);
      break;
      case TAB_KEY: i=(i+1)%element_count; break;
     }
    }
    quit_debug:
    sound(ALL_NOTES_OFF(0));
    sound(ALL_NOTES_OFF(0x09));
    for(size_t i=0; i<element_count; i++) delete element[i];
    free(data.entry);
   }
   free(data.buffer);
  }
  fclose(instrument_data);
 }
}
void Front::no_score_message() throw(){
 MessageBoxA(NULL, "No imported score!", "Error", MB_OK|MB_ICONERROR);
}
key_t Front::browse(clock_t end_wait) throw(){
 key_t input=0;
 for(;;){
  if(kbhit()){
   switch(input=get_input()){
    case DOWN_KEY: cursor.go_down(bottom); goto update_cursor;
    case UP_KEY: cursor.go_up(); goto update_cursor;
    case LEFT_KEY: cursor.go_left(); goto update_cursor;
    case RIGHT_KEY: cursor.go_right(); goto update_cursor;
    case HOME_KEY: cursor.go_home(); goto update_cursor;
    case END_KEY: cursor.go_end(); goto update_cursor;
    case CTRL_HOME_KEY: cursor.go_first_home(); goto update_cursor;
    case CTRL_END_KEY: cursor.go_last_end(bottom); goto update_cursor;
    default: return input;
    update_cursor: gotoxy(cursor.column(), cursor.row()); break;
   }
  }
  if(clock()>=end_wait) break;
 }
 return 0;
}
bool Front::choose_input(const char* given_path) throw(){
 path[0]=file_title[0]='\0';
 basic_data.clear();
 page_frame.clear();
 if(given_path){ // The path is given in command-line mode.
  strcpy(path, given_path);
  strcpy(file_title, PathFindFileNameA(given_path));
 }
 else{
  OPENFILENAME ofn; // Common dialog box structure.
  char cwd[MAX_PATH]={0};
  getcwd(cwd, sizeof(cwd));
  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize=sizeof(ofn);
  ofn.hwndOwner=NULL;
  ofn.lpstrFile=path;
  ofn.nMaxFile=sizeof(path);
  ofn.lpstrFilter="Music XML (*.xml)\0*.xml\0All files\0*.*\0";
  ofn.nFilterIndex=1;
  ofn.lpstrFileTitle=file_title;
  ofn.nMaxFileTitle=sizeof(file_title);
  ofn.lpstrInitialDir=NULL;
  ofn.Flags=OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
  if(GetOpenFileName(&ofn)==TRUE){
   char console_title[sizeof(file_title)+20]={0};
   sprintf(console_title, "%s - Touching Melody", file_title);
   SetConsoleTitleA(console_title);
   chdir(cwd);
  }
 }
 if(path[0]!='\0'){
  XMLAnalyzer analyzer(basic_data);
  return !analyzer.read(path);
 }
 else return false;
}
void Front::export_text(FILE* stream, int width) const throw(){
 bool header=false;
 if(!sheet().work_title().empty() || !sheet().work_number().empty()){
  if(!sheet().work_title().empty()) fprintf(stream, "  %s", sheet().work_title().c_str());
  if(sheet().work_number().empty()) fputc('\n', stream);
  else fprintf(stream, "  %s\n", sheet().work_number().c_str());
  header=true;
 }
 if(!sheet().composer().empty()){
  fprintf(stream, "Composer: %s\n", sheet().composer().c_str());
  header=true;
 }
 if(!sheet().lyricist().empty()){
  fprintf(stream, "Lyricist: %s\n", sheet().lyricist().c_str());
  header=true;
 }
 if(!sheet().arranger().empty()){
  fprintf(stream, "Arranger: %s\n", sheet().arranger().c_str());
  header=true;
 }
 if(header) fputc('\n', stream);
 for(brl_output::const_iterator u=sheet().begin(); u!=sheet().end(); u++){
  int length=width; // This assignment makes a '\n' is printed following the header.
  fputs(u->label().c_str(), stream);
  for(brl_staff::const_iterator v=u->begin(); v!=u->end(); v++){
   if(!v->empty()){
    if(v->at(0).substr(0, 3)!=";2 "){
     if(length>0 && length+v->length()>width){
      fputc('\n', stream);
      length=fprintf(stream, "#%u ", v->number());
     }
     length+=v->length()+1;
     for(brl_measure::const_iterator w=v->begin(); w!=v->end(); w++) fputs(w->c_str(), stream);
     fputc(' ', stream);
    }
    else{
     istringstream lyrics;
     fputc('\n', stream);
     length=0;
     lyrics.str(v->c_str());
     for(string lyric; lyrics>>lyric;){
      if(length>0 && length+lyric.length()>width){
       fputc('\n', stream);
       length=0;
      }
      length+=fprintf(stream, "%s ", lyric.c_str());
     }
    }
   }
  }
  fputc('\n', stream);
 }
}
choice_t Front::main_menu() throw(){
 clrscr();
 printf("XML File: %s\n", file_title[0]=='\0' ? "None." : file_title);
 puts("  Import music XML file... (I)");
 puts("  Braille music score (B)");
 puts("  Oral reading (O)");
 puts("  Play (P)");
 puts("  Settings... (S)");
 puts("  Help... (H)");
 puts("  Exit (Esc)");
 gotoxy(0, 1+main_choice);
 _cputs("*\b");
 while(true){
  switch(get_input()){
   case DOWN_KEY: // Go to the next choice.
    putch(' ');
    main_choice=(main_choice+1)%main_menu_range;
    gotoxy(0, 1+main_choice);
    _cputs("*\b");
   break;
   case UP_KEY:
    putch(' ');
    main_choice=(main_choice+main_menu_range-1)%main_menu_range;
    gotoxy(0, 1+main_choice);
    _cputs("*\b");
   break;
   case HOME_KEY:  // Go to the first choice of main menu.
    putch(' ');
    gotoxy(0, 1+(main_choice=0));
    _cputs("*\b");
   break;
   case END_KEY:   // Go to the last choice of main menu.
    putch(' ');
    gotoxy(0, 1+(main_choice=main_menu_range-1));
    _cputs("*\b");
   break;
   case ENTER_KEY:
   return main_choice;
   case I_KEY:
   case i_KEY: return main_choice=0;
   case B_KEY:
   case b_KEY: return main_choice=1;
   case O_KEY:
   case o_KEY: return main_choice=2;
   case P_KEY:
   case p_KEY: return main_choice=3;
   case S_KEY:
   case s_KEY: return main_choice=4;
   case H_KEY:
   case h_KEY: return main_choice=5;
   case ESC_KEY: return 6; // The program is expected to terminate in main function.
   case M_KEY:
   case m_KEY: return 7; // Midi debug.
   case N_KEY:
   case n_KEY: return 8; // Midi debug (percussion).
  }
 }
}
void Front::option_menu(BrlSysArg& args_b, OralSysArg& args_o, PlaySysArg& args_p) throw(){
 GUI_element* element[13]={NULL};
 GUI_element::value_t value={0};
 clrscr();
 _cputs("Preference settings:\n\n");
 _cputs("Braille music score outputs");
 element[0]=new number_box(2, 0, "Maximum characters a line on screen", 20, 75, args_b.max_char_screen);
 element[1]=new number_box(3, 0, "Maximum characters a line in file", 20, 75, args_b.max_char_file);
 gotoxy(0, 5);
 _cputs("Oral reading settings");
 element[2]=new check_box(6, 4, "Clef", args_o.flags&ORAL_CLEF);
 element[3]=new check_box(7, 4, "Key signature", args_o.flags&ORAL_KEY);
 element[4]=new check_box(8, 4, "Time signature\n", args_o.flags&ORAL_TIME);
 element[5]=new check_box(9, 4, "Pitch for each note", args_o.flags&ORAL_PITCH);
 element[6]=new check_box(10, 4, "Duration for each note", args_o.flags&ORAL_DURATION);
 element[7]=new check_box(11, 4, "Octave for each note\n", args_o.flags&ORAL_OCTAVE_EVERYTIME);
 switch(args_o.flags&ORAL_OCTAVE_REPRESENTATION_BITS){
  default:
  case ORAL_OCTAVE_NUM: element[8]=new select_box(12, 0, "Octave expression", 0, 3, octave_expression); break;
  case ORAL_OCTAVE_BRL: element[8]=new select_box(12, 0, "Octave expression", 1, 3, octave_expression); break;
  case ORAL_OCTAVE_ABS: element[8]=new select_box(12, 0, "Octave expression", 2, 3, octave_expression); break;
 }
 element[9]=new select_box(13, 0, "Sound", args_o.flags&ORAL_MALE ? 0 : 1, 2, sound_gender);
 element[10]=new magnitude_box(14, 0, "Volume", args_o.volume/100, 10); // Unit: 1000/10=100.
 gotoxy(0, 16);
 _cputs("Playing settings");
 element[11]=new number_box(17, 0, "Tempo", 20, 208, args_p.customer_tempo);
 element[12]=new magnitude_box(18, 0, "Volume", args_p.volume, MAX_PLAYER_VOLUME);
 for(int i=0; i<13; i++){
  element[i]->add_escape_key(3, BACK_KEY, ENTER_KEY, TAB_KEY);
  element[i]->paint();
 }
 for(int i=0;; i=(i+1)%13){
  switch(element[i]->focus()){
   case BACK_KEY: goto destruct_all;
   case ENTER_KEY: goto set_result;
  } // TAB_KEY causes the next element to be focused.
 }
 set_result:
 value=element[0]->get_value();
 if(value.as_integer!=args_b.max_char_screen){
  args_b.max_char_screen=value.as_integer;
  args_b.layout|=LAYOUT_DIRTY;
 }
 value=element[1]->get_value();
 if(value.as_integer!=args_b.max_char_file){
  args_b.max_char_file=value.as_integer;
  args_b.layout|=LAYOUT_DIRTY;
 }
 value=element[2]->get_value();
 if((~args_o.flags&ORAL_CLEF)&& value.as_boolean) args_o.flags|=ORAL_CLEF|ORAL_DIRTY;
 else if((args_o.flags&ORAL_CLEF)&& !value.as_boolean){
  args_o.flags^=ORAL_CLEF;
  args_o.flags|=ORAL_DIRTY;
 }
 value=element[3]->get_value();
 if((~args_o.flags&ORAL_KEY)&& value.as_boolean) args_o.flags|=ORAL_KEY|ORAL_DIRTY;
 else if((args_o.flags&ORAL_KEY)&& !value.as_boolean){
  args_o.flags^=ORAL_KEY;
  args_o.flags|=ORAL_DIRTY;
 }
 value=element[4]->get_value();
 if((~args_o.flags&ORAL_TIME)&& value.as_boolean) args_o.flags|=ORAL_TIME|ORAL_DIRTY;
 else if((args_o.flags&ORAL_TIME)&& !value.as_boolean){
  args_o.flags^=ORAL_TIME;
  args_o.flags|=ORAL_DIRTY;
 }
 value=element[5]->get_value();
 if((~args_o.flags&ORAL_PITCH)&& value.as_boolean) args_o.flags|=ORAL_PITCH|ORAL_DIRTY;
 else if((args_o.flags&ORAL_PITCH)&& !value.as_boolean){
  args_o.flags^=ORAL_PITCH;
  args_o.flags|=ORAL_DIRTY;
 }
 value=element[6]->get_value();
 if((~args_o.flags&ORAL_DURATION)&& value.as_boolean) args_o.flags|=ORAL_DURATION|ORAL_DIRTY;
 else if((args_o.flags&ORAL_DURATION)&& !value.as_boolean){
  args_o.flags^=ORAL_DURATION;
  args_o.flags|=ORAL_DIRTY;
 }
 value=element[7]->get_value();
 if((~args_o.flags&ORAL_OCTAVE_EVERYTIME)&& value.as_boolean) args_o.flags|=ORAL_OCTAVE_EVERYTIME|ORAL_DIRTY;
 else if((args_o.flags&ORAL_OCTAVE_EVERYTIME)&& !value.as_boolean){
  args_o.flags^=ORAL_OCTAVE_EVERYTIME;
  args_o.flags|=ORAL_DIRTY;
 }
 switch(element[8]->get_value().as_unsigned_integer){
  case 0:
   if((args_o.flags&ORAL_OCTAVE_REPRESENTATION_BITS)!=ORAL_OCTAVE_NUM){
    args_o.flags=(args_o.flags&~ORAL_OCTAVE_REPRESENTATION_BITS)|ORAL_OCTAVE_NUM;
    goto octave_expression_dirty;
   }
  break;
  case 1:
   if((args_o.flags&ORAL_OCTAVE_REPRESENTATION_BITS)!=ORAL_OCTAVE_BRL){
    args_o.flags=(args_o.flags&~ORAL_OCTAVE_REPRESENTATION_BITS)|ORAL_OCTAVE_BRL;
    goto octave_expression_dirty;
   }
  break;
  case 2:
   if((args_o.flags&ORAL_OCTAVE_REPRESENTATION_BITS)!=ORAL_OCTAVE_ABS){
    args_o.flags=(args_o.flags&~ORAL_OCTAVE_REPRESENTATION_BITS)|ORAL_OCTAVE_ABS;
    goto octave_expression_dirty;
   }
  break;
  octave_expression_dirty: args_o.flags|=ORAL_DIRTY;
 }
 value=element[9]->get_value();
 if(value.as_unsigned_integer==0 &&(~args_o.flags&ORAL_MALE)) args_o.flags|=ORAL_MALE|ORAL_DIRTY;
 else if(value.as_unsigned_integer==1 &&(args_o.flags&ORAL_MALE)){
  args_o.flags^=ORAL_MALE;
  args_o.flags|=ORAL_DIRTY;
 }
 value=element[10]->get_value();
 if(value.as_unsigned_long!=args_o.volume/100){
  args_o.volume=value.as_unsigned_long*100;
  args_o.flags|=ORAL_DIRTY;
 }
 value=element[11]->get_value();
 if(value.as_integer!=args_p.customer_tempo) args_p.customer_tempo=value.as_integer;
 value=element[12]->get_value();
 if(value.as_unsigned_long!=args_p.volume) args_p.volume=value.as_unsigned_long;
 destruct_all:
 for(int i=0; i<13; i++) delete element[i];
}
void Front::print_score(int width, bool reset_cursor) throw(){
 bottom=0;
 cursor.reset_step_map();
 clrscr();
 if(!sheet().work_title().empty() || !sheet().work_number().empty()){
  if(!sheet().work_title().empty()) printf("  %s", sheet().work_title().c_str());
  if(sheet().work_number().empty()) putchar('\n');
  else printf("  %s\n", sheet().work_number().c_str());
  bottom++;
 }
 if(!sheet().composer().empty()){
  printf("Composer: %s\n", sheet().composer().c_str());
  bottom++;
 }
 if(!sheet().lyricist().empty()){
  printf("Lyricist: %s\n", sheet().lyricist().c_str());
  bottom++;
 }
 if(!sheet().arranger().empty()){
  printf("Arranger: %s\n", sheet().arranger().c_str());
  bottom++;
 }
 if(bottom>0) putchar('\n'), bottom++;
 for(brl_output::iterator u=sheet().begin(); u!=sheet().end(); u++){
  int length=width; // This assignment makes a '\n' is printed following the header.
  fputs(u->label().c_str(), stdout);
  for(brl_staff::iterator v=u->begin(); v!=u->end(); v++){
   if(!v->empty()){
    if(v->at(0).substr(0, 3)!=";2 "){
     if(length>0 && length+v->length()>width){
      putchar('\n');
      length=printf("#%u ", v->number());
      cursor.append_step(++bottom, 0);
     }
     v->set_position(bottom, length);
     for(brl_measure::iterator w=v->begin(); w!=v->end(); length+=(w++)->length()){
      cursor.append_step(bottom, length);
      w->set_position(bottom, length);
      fputs(w->c_str(), stdout);
     }
     putchar(' ');
     length++;
    }
    else{
     istringstream lyrics;
     putchar('\n');
     length=0;
     bottom++;
     lyrics.str(v->c_str());
     for(string lyric; lyrics>>lyric;){
      if(length>0 && length+lyric.length()>width){
       putchar('\n');
       length=0;
       bottom++;
      }
      cursor.append_step(bottom, length);
      length+=printf("%s ", lyric.c_str());
     }
    }
   }
  }
  putchar('\n');
  bottom++;
 }
 if(bottom>0) bottom--;
 if(reset_cursor){
  cursor.reset_position();
  gotoxy(cursor.column(), cursor.row());
 }
}
void Front::print_total_score(int width, bool reset_cursor) throw(){
 ostringstream line;
 size_t page_index=0;
 bottom=0;
 for(bms_score::page_table_const_iterator p=score().page_table_begin(); p!=score().page_table_end(); p++){
  line.str("");
  line<<'#'<<score().page_begin(p)<<'-'<<score().page_end(p)<<' '<<BrailleOutputer::page_number_string(page_index+1, p->second);
  assign_text(bottom++, line.str(), width);
  if(page_index==0){ // Score headers.
   if(!sheet().work_title().empty()) assign_text(bottom++, "  "+sheet().work_title());
   if(!sheet().composer().empty()) assign_text(bottom++, "Composer: "+sheet().composer(), width);
   if(!sheet().lyricist().empty()) assign_text(bottom++, "Lyricist: "+sheet().lyricist(), width);
   if(!sheet().arranger().empty()) assign_text(bottom++, "Arranger: "+sheet().arranger(), width);
   if(!sheet().work_number().empty()) assign_text(bottom++, "OP. "+sheet().work_number(), width);
   if(bottom>0) bottom++; // An empty line between header and score.
  }
  for(brl_output::iterator i=sheet().begin(); i!=sheet().end(); i++){
   size_t length=0;
   line.str("");
   for(size_t m=i->page_begin(page_index); m<i->page_end(page_index); m++){
    if(line.str().length()>0 && line.str().length()+i->at(m).length()>width){
     assign_text(bottom++, line.str());
     line.str(""); // A new empty line.
     line<<'#'<<i->at(m).number()<<' ';
     length=line.str().length();
     assign_step(bottom, 0);
    }
    i->at(m).set_position(bottom, length);
    for(brl_measure::iterator j=i->at(m).begin(); j!=i->at(m).end(); length+=(j++)->length()){
     assign_step(bottom, length);
     j->set_position(bottom, length);
     line<<*j;
    }
    line<<' '; // Default barline.
    length++;
   }
  }
  if(line.str().length()>0) assign_text(bottom, line.str());
 }
 if(bottom>0) bottom--;
FILE* h=fopen("h.txt", "a");
/*
for(size_t i=0; i<sheet.size(); i++){
for(size_t j=0; j<sheet[i].page_bound.size(); j++){
fprintf(h, "%u ", sheet[i].page_bound[j]);
}
fputc(10, h);
}
*/
fclose(h);
}
void Front::save(int line_width) const throw(){
 OPENFILENAME ofn; // Common dialog box structure.
 char cwd[MAX_PATH]={0}, target[MAX_PATH]={0};
 getcwd(cwd, sizeof(cwd));
 score_identifier(target, sizeof(target), false);
 strcat(target, ".txt");
 ZeroMemory(&ofn, sizeof(ofn));
 ofn.lStructSize=sizeof(ofn);
 ofn.hwndOwner=NULL;
 ofn.lpstrFile=target;
 ofn.nMaxFile=sizeof(target)-4;
 ofn.lpstrFilter="Text files (*.txt)\0*.txt\0All files\0*.*\0";
 ofn.nFilterIndex=1;
 ofn.lpstrFileTitle=NULL;
 ofn.nMaxFileTitle=0;
 ofn.lpstrInitialDir=NULL;
 ofn.Flags=OFN_PATHMUSTEXIST;
 if(GetSaveFileName(&ofn)==TRUE){
  FILE* output=NULL;
  if(ofn.nFilterIndex==1 &&(strlen(target)<4 || strcmp(target+strlen(target)-4, ".txt"))) strcat(target, ".txt");
  output=fopen(target, "wt");
  export_text(output, line_width);
  fclose(output);
 }
 chdir(cwd);
}
void Front::score_identifier(char* buffer, size_t size, bool quotation) const throw(){
 if(buffer!=NULL && size>0){
  size_t begin= quotation ? 1 : 0, end=strlen(file_title);
  if(end>4 && !stricmp(file_title+end-4, ".xml")) end-=4;
  if(quotation) size--; // A preserved charactor for '\"'.
  if(end>size) end=size;
  if(begin>=end) return;
  strncpy(buffer+begin, file_title, end-begin);
  if(quotation) buffer[0]=buffer[end]='\"';
 }
}
bool Front::set_oral_range(OralOutArg& a) const throw(){
 GUI_element* element[4]={NULL};
 number_box* range[3]={NULL};
 bool result=false;
 if(a.end>score().getMeasureNum()) a.end=score().getMeasureNum();
 element[0]=new number_box(1, 0, "Part", 1, score().getPartNum(), a.part);
 element[1]=range[0]=new number_box(2, 0, "Staff", 1, score().getStaffNum(a.part), a.staff);
 element[2]=range[1]=new number_box(3, 0, "From measure", 1, a.end, a.begin);
 element[3]=range[2]=new number_box(4, 0, "To measure", a.begin, score().getMeasureNum(), a.end);
 clrscr();
 _cputs("Please select a working range:");
 for(int i=0; i<4; i++){
  element[i]->add_escape_key(3, BACK_KEY, ENTER_KEY, TAB_KEY);
  element[i]->paint();
 }
 for(int i=0;; i=(i+1)%4){
  switch(element[i]->focus()){
   case BACK_KEY: goto destruct_all;
   case ENTER_KEY: goto set_result;
   case TAB_KEY:
    switch(i){
     case 0:
      range[0]->reset_upper_bound(score().getStaffNum(element[0]->get_value().as_integer));
      range[0]->paint();
     break;
     case 2:
      range[2]->reset_lower_bound(element[2]->get_value().as_integer);
      range[2]->paint();
     break;
     case 3:
      range[1]->reset_upper_bound(element[3]->get_value().as_integer);
      range[1]->paint();
     break;
    }
   break;
  }
 }
 set_result:
 a.part=element[0]->get_value().as_integer;
 a.staff=element[1]->get_value().as_integer;
 a.begin=element[2]->get_value().as_integer;
 a.end=element[3]->get_value().as_integer;
 result=true;
 destruct_all:
 for(int i=0; i<4; i++) delete element[i];
 return result;
}
bool Front::set_play_range(PlayOutArg& a) const throw(){
 if(a.staff_open.empty()) return false;
 else{
  typedef struct{
   size_t index[2];
   check_box* box;
  } staff_box_pair_t;
  char buffer[20]={0}, format[23]={0}, key_shift=a.key_shift;;
  GUI_element* element[4]={NULL};
  element_list part_list, staff_list;
  check_box** part_switch=new check_box*[a.staff_open.size()];
  number_box* range[2]={NULL};
  staff_box_pair_t* staff_switch=NULL;
  size_t index=0, max_part_length=1, staff_count=a.staff_open.front().size();
  bool result=false;
  for(size_t factor=10; factor<=a.staff_open.size(); factor*=10) max_part_length++;
  sprintf(format, "Part %%0%ud ->", max_part_length);
  element[0]=range[0]=new number_box(1, 0, "From measure", 1, a.end, a.begin);
  element[1]=range[1]=new number_box(2, 0, "To measure", a.begin, score().getMeasureNum(), a.end);
  element[2]=&part_list, element[3]=&staff_list;
  element[0]->add_escape_key(5, BACK_KEY, CTRL_LEFT_KEY, CTRL_RIGHT_KEY, ENTER_KEY, TAB_KEY);
  element[1]->add_escape_key(5, BACK_KEY, CTRL_LEFT_KEY, CTRL_RIGHT_KEY, ENTER_KEY, TAB_KEY);
  element[2]->add_escape_key(6, BACK_KEY, CTRL_A_KEY, CTRL_X_KEY, ENTER_KEY, RIGHT_KEY, TAB_KEY);
  element[3]->add_escape_key(4, BACK_KEY, ENTER_KEY, LEFT_KEY, TAB_KEY);
  for(size_t i=0; i<a.staff_open.size(); i++){
   sprintf(buffer, format, i+1);
   part_switch[i]=new check_box(3+i, 4, buffer, false);
   part_switch[i]->add_escape_key(6, BACK_KEY, CTRL_A_KEY, CTRL_X_KEY, ENTER_KEY, RIGHT_KEY, TAB_KEY);
   part_list.add_element(*part_switch[i]);
   staff_count+=a.staff_open[i].size();
  }
  staff_switch=new staff_box_pair_t[staff_count];
  staff_count=0;
  for(size_t i=0; i<a.staff_open.size(); i++){
   for(size_t j=0; j<a.staff_open[i].size(); j++){
    sprintf(buffer, "Staff %u", j+1);
    staff_switch[staff_count].index[0]=i, staff_switch[staff_count].index[1]=j;
    staff_switch[staff_count].box=new check_box(3+i+j, 17+max_part_length, buffer, a.staff_open[i][j]);
    staff_switch[staff_count++].box->add_escape_key(4, BACK_KEY, ENTER_KEY, LEFT_KEY, TAB_KEY);
   }
  }
  clrscr();
  _cputs("Please select a working range:");
  for(int i=0; i<3; i++) element[i]->paint();
  for(;;){
   switch(element[index]->focus()){
    case BACK_KEY: result=false; goto destruct_all;
    case CTRL_A_KEY:
     for(size_t i=0; i<a.staff_open.size(); i++){
      part_switch[i]->set_value(true);
      part_switch[i]->paint();
     }
    break;
    case CTRL_LEFT_KEY: key_shift--; break;
    case CTRL_RIGHT_KEY: key_shift++; break;
    case CTRL_X_KEY:
     for(size_t i=0; i<a.staff_open.size(); i++){
      part_switch[i]->set_value(false);
      part_switch[i]->paint();
     }
    break;
    case ENTER_KEY: result=true; goto set_result;
    case LEFT_KEY: // Program comes here only when index=3.
     staff_list.vanish(); // It vanishes from the console.
     staff_list.clear();
     index=2;
    break;
    case RIGHT_KEY: // Program comes here only with index=2.
     if(!part_switch[part_list.get_value().as_unsigned_integer]->get_value().as_boolean){
      for(size_t i=0; i<staff_count; i++){
       if(staff_switch[i].index[0]==part_list.get_value().as_unsigned_integer){
        staff_list.add_element(*staff_switch[i].box);
       }
      }
      staff_list.paint();
      index=3;
     }
    break;
    case TAB_KEY:
     switch(index){
      case 0:
       range[1]->reset_lower_bound(element[0]->get_value().as_integer);
       range[1]->paint();
       index=1;
      break;
      case 1:
       range[0]->reset_upper_bound(element[1]->get_value().as_integer);
       range[0]->paint();
       index=2;
      break;
      case 2: index=0; break;
      case 3:
       staff_list.vanish();
       staff_list.clear();
       index=0;
      break;
     }
    break;
   }
  }
  set_result:
  a.begin=element[0]->get_value().as_integer;
  a.end=element[1]->get_value().as_integer;
  for(size_t i=0; i<staff_count; i++){
   if(part_switch[staff_switch[i].index[0]]->get_value().as_boolean) a.staff_open[staff_switch[i].index[0]][staff_switch[i].index[1]]=true;
   else a.staff_open[staff_switch[i].index[0]][staff_switch[i].index[1]]=staff_switch[i].box->get_value().as_boolean;
  }
  a.key_shift=key_shift;
  destruct_all:
  delete element[0];
  delete element[1];
  for(size_t i=0; i<a.staff_open.size(); i++) delete part_switch[i];
  delete[] part_switch;
  for(size_t i=0; i<staff_count; i++) delete staff_switch[i].box;
  delete[] staff_switch;
  return result;
 }
}
key_t Front::start_message(bool pause) const throw(){
 int bound=4, l=0;
 clock_t endwait=clock()+3000L;
 clrscr();
 puts("Welcome to Touch Melody. (Beta 3.1)");
 puts("This is a software to produce braille score for visually disabled people.");
 puts("It also provides score reading & music playing functions.");
 puts("Press Enter To Continue...");
 gotoxy(0, l);
 while(pause || clock()<endwait){
  if(kbhit()){
   switch(get_input()){
    case DOWN_KEY:
     l=(l+1)%bound;
     gotoxy(0, l);
    break;
    case UP_KEY:
     l=(l+bound-1)%bound;
     gotoxy(0, l);
    break;
    case ENTER_KEY: return ENTER_KEY;
    case ESC_KEY: return ESC_KEY;
   }
  }
 }
 return ENTER_KEY;
}
bool Front::want_retry(const char* message) throw(){
 if(MessageBoxA(NULL, message, "Error", MB_RETRYCANCEL|MB_ICONWARNING)==IDRETRY) return true;
 else{
  SetConsoleTitleA("Touching Melody");
  file_title[0]='\0';
  return false;
 }
}
