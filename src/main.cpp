#include <stdlib.h>
#include <string.h>
#include "braille_outputer.h"
#include "reader.h"
#include "player.h"
using namespace std;
static Front& my_front=Front::console;
static BrailleOutputer my_braille_outputer;
static ScoreReader SR;
static ScorePlayer playback;
int GUI_main(){ // GUI mode.
 enum{S_start, S_main_menu, S_get_filename, S_output_braille_score, S_oral_reading, S_playing_music, S_options} status=S_start;
 for(;;){
  switch(status){
   case S_start:
    if(my_front.start_message(false)!=ENTER_KEY) goto quit;
    status=S_main_menu;
   break;
   case S_main_menu:
    switch(my_front.main_menu()){
     case 0:
      status=S_get_filename;
     break;
     case 1:
      status=S_output_braille_score;
     break;
     case 2:
      status=S_oral_reading;
     break;
     case 3:
      status=S_playing_music;
     break;
     case 4:
      status=S_options;
     break;
     case 5:
      if(my_front.start_message(true)!=ENTER_KEY) goto quit;
      status=S_main_menu;
     break;
     case 6: goto quit;
     case 7:
      Front::midi_debug(ScorePlayer::sound, false);
     break;
     case 8:
      Front::midi_debug(ScorePlayer::sound, true);
     break;
    }
   break;
   case S_get_filename:
    while(my_front.choose_input(NULL) && my_front.want_retry("File import failed!"));
    Front::clear_key_queue();
    SR.initialize_range();
    playback.initialize_arguments(my_front.score());
    playback.state.customer_tempo=playback.state.original_tempo=my_front.score().get_tempo(0);
    status=S_main_menu;
   break;
   case S_output_braille_score:
    if(my_front.score().empty()) Front::no_score_message();
    else{
     key_t input=0;
     my_braille_outputer.update_output(my_front.score(), my_front.sheet());
     my_front.print_score(my_front.state.max_char_screen);
     while((input=my_front.browse())!=BACK_KEY){
      if(input==F3_KEY){
       my_front.save(my_front.state.max_char_file);
       Front::clear_key_queue();
      }
     }
    }
    status=S_main_menu;
   break;
   case S_oral_reading:
    if(!my_front.score().empty()){
     if(my_front.set_oral_range(SR.range)){
      my_braille_outputer.update_output(my_front.score(), my_front.sheet());
      my_front.print_score(my_front.state.max_char_screen);
      if(SR.read(my_front)==ESC_KEY) goto quit;
     }
    }
    else Front::no_score_message();
    status=S_main_menu;
   break;
   case S_playing_music:
    if(!my_front.score().empty()){
     if(my_front.set_play_range(playback.range)){
      my_braille_outputer.update_output(my_front.score(), my_front.sheet());
      my_front.print_score(my_front.state.max_char_screen);
      playback.play_score(my_front);
     }
    }
    else Front::no_score_message();
    status=S_main_menu;
   break;
   case S_options:
    my_front.option_menu(my_front.state, SR.state, playback.state);
    status=S_main_menu;
   break;
   default: goto quit; // Unused default case.
  }
 }
 quit:
 return 0;
}
int main(int argc, char* argv[]){
 if(argc==1) return GUI_main();
 else{ // Command-line (testing) mode.
  if(!strcmp(argv[1], "brl")){
   if(argc!=3) goto error;
   if(my_front.choose_input(argv[2])){
    printf("%s: failed to read %s\n", argv[0], argv[2]);
    return 1;
   }
   my_braille_outputer.update_output(my_front.score(), my_front.sheet());
   my_front.export_text(stdout, my_front.state.max_char_screen);
  }
  else if(!strcmp(argv[1], "oral")){
   vector<string> content;
   if(argc!=5 && argc!=6) goto error;
   if(my_front.choose_input(argv[2])){
    printf("%s: failed to read %s\n", argv[0], argv[2]);
    return 1;
   }
   SR.initialize_range();
   SR.range.part=atoi(argv[3]);
   SR.range.staff=atoi(argv[4]);
   if(argc!=6) SR.range.end=my_front.score().getMeasureNum();
   else if(sscanf(argv[5], "%d:%d", &SR.range.begin, &SR.range.end)!=2) goto error;
   SR.convert(my_front.score(), content);
   for(vector<string>::const_iterator p=content.begin(); p!=content.end(); p++) puts(p->c_str());
  }
  else if(!strcmp(argv[1], "play")){
   if(argc<3) goto error;
   if(my_front.choose_input(argv[2])){
    printf("%s: failed to read %s\n", argv[0], argv[2]);
    return 1;
   }
   playback.initialize_arguments(my_front.score());
   if(argc>3){ // Additional arguments are given.
    int i=3, r[3]={0};
    if(sscanf(argv[i], "%d:%d", &r[0], &r[1])==2){
     playback.range.begin=r[0], playback.range.end=r[1], i++;
     if(i==argc) goto full_open; // No part-specific argument.
    }
    for(; i<argc; i++){
     if(sscanf(argv[i], "%d,%d:%d", &r[0], &r[1], &r[2])!=3){
      if(sscanf(argv[i], "%d,%d:", &r[0], &r[1])==2) r[2]=my_front.score().getStaffNum(r[0]);
      else if(sscanf(argv[i], "%d,:%d", &r[0], &r[2])==2) r[1]=1;
      else if(sscanf(argv[i], "%d,:", &r[0])==1) r[1]=1, r[2]=my_front.score().getStaffNum(r[0]);
      else goto error;
     }
     for(int j=r[1]-1; j<r[2]; j++) playback.range.staff_open[r[0]-1][j]=true;
    }
   }
   else{
    full_open:
    for(size_t i=0; i<playback.range.staff_open.size(); i++) for(size_t j=0; j<playback.range.staff_open[i].size(); j++) playback.range.staff_open[i][j]=true;
   }
   playback.state.customer_tempo=playback.state.original_tempo=my_front.score().get_tempo(0);
   playback.play_score(my_front);
  }
  else goto error; // Invalid argument 2.
  return 0;
 }
 error:
 puts("Usage:");
 printf("%s brl <xml_path>\n", argv[0]);
 printf("%s oral <xml_path> <part> <staff> [begin:end]\n", argv[0]);
 printf("%s play <xml_path> [begin:end] [part,begin:end]...\n", argv[0]);
 return 1;
}
