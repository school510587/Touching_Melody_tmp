#include <conio.h>
#include <windows.h>
#include "front.h"
#include "pseudo_GUI_module.h"
using namespace std;
GUI_element::GUI_element(short r, short c, const std::string& l) throw(): label(l), base_row(r), base_column(c){
 focus_row=base_row, focus_column=base_column;
 focus_column+=label.length()+2;
}
void GUI_element::add_escape_key(size_t n, ...) throw(){
 va_list key_list;
 if(n>0){
  va_start(key_list, n);
  for(size_t i=0; i<n; i++) escape_key.push_back((key_t)va_arg(key_list, int));
  va_end(key_list);
  escape_key.sort();
  escape_key.unique();
 }
}
bool GUI_element::is_escape_key(key_t k) const throw(){
 for(list<key_t>::const_iterator p=escape_key.begin(); p!=escape_key.end(); p++) if(*p==k) return true;
 return false;
}
void GUI_element::paint() const throw(){
 gotoxy(base_column, base_row);
 if(!label.empty()) _cputs(label.c_str());
 update_display();
}
void GUI_element::vanish() const throw(){
 gotoxy(base_column, base_row);
 if(!label.empty()) for(size_t i=0; i<label.length(); i++) putch(' ');
 remove_display();
}
check_box::check_box(short r, short c, const std::string& l, bool b) throw(): GUI_element(r, c= c<4?4:c, l), state(b){
 focus_column=c-3; // The value is overwritten.
}
key_t check_box::focus() throw(){
 key_t input=0;
 for(gotoxy(focus_column, focus_row);;){
  input=get_input();
  if(is_escape_key(input)) return input;
  else if(input==BLANK_KEY){
   state=!state;
   _cprintf("%c\b", state ? '*' : ' ');
  }
  else if(input==ESC_KEY) exit(1);
 }
}
GUI_element::value_t check_box::get_value() const throw(){
 GUI_element::value_t value={0};
 value.as_boolean=state;
 return value;
}
void check_box::remove_display() const throw(){
 gotoxy(focus_column-1, focus_row);
 _cputs("   ");
}
void check_box::update_display() const throw(){
 gotoxy(focus_column-1, focus_row);
 _cprintf("(%c) \b\b\b", state ? '*' : ' ');
}
void element_list::add_element(GUI_element& new_element) throw(){
 new_element.add_escape_key(2, DOWN_KEY, UP_KEY);
 entry.push_back(&new_element);
}
void element_list::clear() throw(){
 entry.clear();
 index=0;
}
key_t element_list::focus() throw(){
 key_t input=0;
 for(;;){
  if(is_escape_key(input=entry[index]->focus())) return input;
  switch(input){
   case DOWN_KEY: index=(index+1)%entry.size(); break;
   case UP_KEY: index=(index+entry.size()-1)%entry.size(); break;
   case ESC_KEY: exit(1);
  }
 }
}
GUI_element::value_t element_list::get_value() const throw(){
 GUI_element::value_t value={0};
 value.as_unsigned_integer=index;
 return value;
}
void element_list::remove_display() const throw(){
 for(std::vector<GUI_element*>::const_iterator p=entry.begin(); p!=entry.end(); p++) (*p)->vanish();
}
void element_list::update_display() const throw(){
 for(std::vector<GUI_element*>::const_iterator p=entry.begin(); p!=entry.end(); p++) (*p)->paint();
}
magnitude_box::magnitude_box(short r, short c, const std::string& l, unsigned long x, unsigned long m) throw(): GUI_element(r, c, l), max(m){
 if(x>m) x=m;
 length=x;;
}
key_t magnitude_box::focus() throw(){
 key_t input=0;
 gotoxy(focus_column+length, focus_row);
 for(;;){
  if(is_escape_key(input=get_input())) return input;
  switch(input){
   case LEFT_KEY:
    if(length>0){
     length--;
     _cputs("\b \b");
    }
   break;
   case RIGHT_KEY:
    if(length<max){
     length++;
     putch('=');
    }
   break;
   case ESC_KEY: exit(1);
  }
 }
}
GUI_element::value_t magnitude_box::get_value() const throw(){
 GUI_element::value_t value={0};
 value.as_unsigned_long=length;
 return value;
}
void magnitude_box::remove_display() const throw(){
 _cputs("  ");
 for(unsigned long i=0; i<length; i++) putch(' ');
}
void magnitude_box::update_display() const throw(){
 _cputs(": ");
 for(unsigned long i=0; i<length; i++) putch('=');
}
number_box::number_box(short r, short c, const std::string& l, int b, int t, int v) throw(): GUI_element(r, c, l){
 if(b>t){
  int x=t;
  t=b;
  b=x;
 }
 if(v>t) v=t;
 else if(v<b) v=b;
 min=b, max=t, number=v;
}
key_t number_box::focus() throw(){
 char input[12]={0}, *p=input;
 clock_t previous_input=0;
 key_t input_key=0;
 gotoxy(focus_column, focus_row);
 for(;;){
  if(p!=input && previous_input+1000<clock()) *(p=input)='\0';
  if(kbhit()){
   if(is_escape_key(input_key=get_input())) return input_key;
   switch(input_key){
    case UP_KEY:
     if(number>min) _cprintf("%-10d\b\b\b\b\b\b\b\b\b\b", --number);
    break;
    case DOWN_KEY:
     if(number<max) _cprintf("%-10d\b\b\b\b\b\b\b\b\b\b", ++number);
    break;
    case HOME_KEY:
     if(number!=min) _cprintf("%-10d\b\b\b\b\b\b\b\b\b\b", number=min);
    break;
    case END_KEY:
     if(number!=max) _cprintf("%-10d\b\b\b\b\b\b\b\b\b\b", number=max);
    break;
    case ESC_KEY: exit(1);
    default:
     if(0x3000<=input_key && input_key<=0x3900 && p<input+11){
      if(p==input || previous_input+1000>=clock()){
       *p++=input_key>>8;
       *p='\0';
       number=atoi(input);
       if(number>max) number=max;
       else if(number<min) number=min;
       _cprintf("%-10d\b\b\b\b\b\b\b\b\b\b", number);
       previous_input=clock();
      }
     }
    break;
   }
  }
 }
}
GUI_element::value_t number_box::get_value() const throw(){
 GUI_element::value_t value={0};
 value.as_integer=number;
 return value;
}
void number_box::remove_display() const throw(){
 _cputs("            ");
}
void number_box::reset_lower_bound(int l) throw(){
 if(number<l) number=l;
 min=l;
}
void number_box::reset_upper_bound(int u) throw(){
 if(number>u) number=u;
 max=u;
}
void number_box::update_display() const throw(){
 _cprintf(": %-10d", number);
}
select_box::select_box(short r, short c, const std::string& l, size_t i, size_t n, const char** v) throw(): GUI_element(r, c, l){
 selection.assign(v, v+n);
 if(i>=n) i=n;
 index=i;
}
key_t select_box::focus() throw(){
 key_t input=0;
 gotoxy(focus_column, focus_row);
 for(size_t next=0;;){
  if(is_escape_key(input=get_input())) return input;
  switch(input){
   case DOWN_KEY: next=(index+1)%selection.size(); goto update_screen;
   case END_KEY: next=selection.size()-1; goto update_screen;
   case HOME_KEY: next=0; goto update_screen;
   case UP_KEY: next=(index+selection.size()-1)%selection.size(); goto update_screen;
   update_screen:
    _cputs(selection[next].c_str());
    if(selection[next].length()<selection[index].length()) for(size_t i=0; i<selection[index].length()-selection[next].length(); i++) putch(' ');
    gotoxy(focus_column, focus_row);
    index=next;
   break;
   case ESC_KEY: exit(1);
  }
 }
}
GUI_element::value_t select_box::get_value() const throw(){
 GUI_element::value_t value={0};
 value.as_unsigned_integer=index;
 return value;
}
void select_box::remove_display() const throw(){
 _cputs("  ");
 for(size_t i=0; i<selection[index].length(); i++) putch(' ');
}
void select_box::update_display() const throw(){
 _cprintf(": %s", selection[index].c_str());
}
key_t get_input() throw(){
 unsigned char c=getch();
 key_t input=c<<8;
 if(c==0 || c>0x7f) input+=getch();
 return input;
}
void gotoxy(short x, short y) throw(){
 static HANDLE hConsole=0; // Declaring a variable to handle hardwares.
 static int InstanceCount=0;
 COORD coord; // Declaring a coordinate.
 if(!InstanceCount){ // To get the control of console.
     hConsole=GetStdHandle(STD_OUTPUT_HANDLE);
     InstanceCount=1;
 }
 coord.X=x, coord.Y=y;
 // To move the cursor to the indicated coordinate.
 SetConsoleCursorPosition(hConsole, coord);
}
