/*
   Touching Melody - A Braille Score Assist System
   Copyright (C) 2010 Fingers
                 2010~2013 Bo-Cheng Jhan
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
#include <math.h>
#include <time.h>
#include <windows.h>
#include "player.h"
using namespace std;
static HMIDIOUT outDev=NULL; // MIDI output device.
int ScorePlayer::instance_count=0;
static void close_midi_device() throw(){
 if(outDev!=NULL){
  midiOutReset(outDev); // To stop the playback.
  midiOutClose(outDev);
 }
}
static void open_midi_device(DWORD n) throw(){
 char error_text[40]={0};
 MMRESULT error=0;
 int decision=IDRETRY;
 do{
  if(midiOutGetNumDevs()==0){ // There is no output device.
   decision=MessageBoxA(NULL, "If [Cancel], playback will have no effect.", "No midi device", MB_RETRYCANCEL|MB_ICONWARNING);
   outDev=NULL;
   continue;
  }
  error=midiOutOpen(&outDev, n, 0, 0, 0);
  if(error){
   midiOutGetErrorTextA(error, error_text, sizeof(error_text));
   decision=MessageBoxA(NULL, error_text, "Cannot open midi device", MB_RETRYCANCEL|MB_ICONWARNING);
   outDev=NULL;
   continue;
  }
  decision=IDCANCEL; // Success in opening.
 }while(decision==IDRETRY);
}
bool ScorePlayer::sound(unsigned long message) throw(){
 return outDev==NULL ? false : (midiOutShortMsg(outDev, message)==MMSYSERR_NOERROR);
}
ScorePlayer::ScorePlayer() throw(){
 //if(instance_count++==0) open_midi_device();
 range.begin=1;
 range.end=numeric_limits<int>::max();
 state.customer_tempo=state.original_tempo=120;
 state.volume=MAX_PLAYER_VOLUME;
}
ScorePlayer::~ScorePlayer() throw(){
 //if(--instance_count==0) close_midi_device();
}
void ScorePlayer::initialize_arguments(const bms_score& M) throw(){
 if(!range.staff_open.empty()) range.staff_open.clear();
 if(M.empty()) return;
 else{
  range.staff_open.reserve(M.getPartNum());
  for(int i=0; i<M.getPartNum(); i++){
   range.staff_open.push_back(vector<bool>(M.getStaffNum(i+1)));
   for(size_t j=0; j<range.staff_open[i].size(); j++) range.staff_open[i][j]=false;
  }
  range.begin=1;
  range.end=M.getMeasureNum();
  range.key_shift=0;
 }
}
void ScorePlayer::play_score(Front& face) throw(){
 open_midi_device(0);
 if(outDev!=NULL){
  const double scalar=60.0*CLOCKS_PER_SEC*state.original_tempo/(face.score().midi_stream.get_divisions()*state.customer_tempo);
  double volume_rate=(double)state.volume/MAX_PLAYER_VOLUME;
  DWORD volume=(DWORD)round((numeric_limits<DWORD>::max()>>16)*volume_rate);
  MIDIHDR header={0};
  MidiCommandVector v=add_skills(face.score().midi_stream, state.customer_tempo);
  MidiCommandVector::iterator bp, ep;
  clock_t end_wait=0, pause_start=0;
  key_t input=0;
  bool support_volume=true;
  for(bp=v.begin(); bp!=v.end() && bp->m_start<range.begin; bp++);
  for(ep=bp; ep+1!=v.end() && ep->m_start<range.end+1; ep++);
  for(unsigned char c=0; c<0x10; c++) sound(ALL_CONTROLLERS_OFF(c));
  if(midiOutSetVolume(outDev, (volume<<16)|volume)!=MMSYSERR_NOERROR){
   HMIXER midi_mixer=NULL;
   UINT y=-5;
   int x=0;
   if(x=midiOutGetID(outDev, &y)) printf("id %d ", x);
   if(x=mixerOpen(&midi_mixer, y, 0, 0, MIXER_OBJECTF_MIDIOUT)) printf("%d %d", x, y);
   else puts("mix");
   mixerClose(midi_mixer);
   support_volume=false;
   midiOutSetVolume(outDev, numeric_limits<unsigned long>::max());
  }
  close_midi_device(); // The default device is closed.
  open_midi_device(MIDI_MAPPER);
  for(size_t i=0; i<face.score().instrument_count(); i++){
   midi_message_t instrument_message={0};
   instrument_message.command_byte[0]=0xc0|(face.score().midi_instrument(i).channel-1);
   instrument_message.command_byte[1]=face.score().midi_instrument(i).program-1;
   sound(instrument_message.command_word);
   instrument_message.command_byte[0]=0xb0|(face.score().midi_instrument(i).channel-1);
   instrument_message.command_byte[1]=7; // Volume controller.
   instrument_message.command_byte[2]=face.score().midi_instrument(i).volume;
   if(!support_volume) instrument_message.command_byte[2]=(unsigned char)round(instrument_message.command_byte[2]*volume_rate);
   sound(instrument_message.command_word);
   instrument_message.command_byte[1]=10; // Pan controller.
   instrument_message.command_byte[2]=(unsigned char)round(127*(90-face.score().midi_instrument(i).pan)/180.0);
   sound(instrument_message.command_word);
  }
  end_wait=clock(); // An initial value.
  for(vector<midi_command_t>::iterator it=bp;; it++){
   HANDLE buffer=NULL;
   UINT error=MMSYSERR_NOERROR;
   if(buffer=GlobalAlloc(GHND, it->message.size()*sizeof(it->message.front().command_word))){
    if(header.lpData=(char*)GlobalLock(buffer)){
     header.dwBufferLength=0;
     for(list<midi_message_t>::iterator m_it=it->message.begin(); m_it!=it->message.end(); m_it++){
      midi_message_t x=*m_it;
      if(range.key_shift!=0 && (x.command_byte[0]&0xf0)==0x90 && (x.command_byte[0]&0x0f)!=0x09) x.command_byte[1]+=range.key_shift;
      if(m_it->staff==0 ||(m_it->command_byte[2]==0 && it!=bp && range.staff_open[m_it->part-1][m_it->staff-1])||(m_it->command_byte[2]!=0 && it!=ep && range.staff_open[m_it->part-1][m_it->staff-1])){
       memcpy(header.lpData+header.dwBufferLength, &x.command_word, sizeof(x.command_word));
       header.dwBufferLength+=sizeof(m_it->command_word);
      }
     }
     if(header.dwBufferLength>0 && midiOutPrepareHeader(outDev, &header, sizeof(header))==MMSYSERR_NOERROR){
      error=midiOutLongMsg(outDev, &header, sizeof(header));
      if(error){
       char error_message[120]={0};
       midiOutGetErrorText(error, error_message, sizeof(error_message));
       MessageBoxA(NULL, error_message, "Error", MB_OK|MB_ICONERROR);
       goto the_end;
      }
      while(MIDIERR_STILLPLAYING==midiOutUnprepareHeader(outDev, &header, sizeof(header)));
     }
     GlobalUnlock(buffer);
    }
    GlobalFree(buffer);
   }

   if(it==ep) break; // Termination.
   end_wait+=(long)round(scalar*((it+1)->time_point-it->time_point)/face.score().midi_stream.tempo(it->time_point));
   while((input=face.browse(end_wait))!=0){
    switch(input){
     case PERIOD_KEY:
     case CTRL_P_KEY: // Pause function.
      for(unsigned char c=0; c<0x10; c++){
       if(input==PERIOD_KEY) sound(ALL_CONTROLLERS_OFF(c));
       sound(ALL_NOTES_OFF(c));
      }
      if(input==PERIOD_KEY) goto the_end;
      else{
       pause_start=clock();
       do{ // Infinity of waiting until manual breaking.
        input=face.browse();
        if(input==PERIOD_KEY) goto the_end;
       }while(input!=CTRL_P_KEY);
       end_wait+=clock()-pause_start; // The end is delayed.
      }
     break;
    }
   }
  }
 }
 the_end:
 close_midi_device(); // MIDI_MAPPER is closed.
}
