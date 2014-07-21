#ifndef BRAILLE_SYMBOLS_H
#define BRAILLE_SYMBOLS_H
const char NumberPrefix = '#';
const char *UpperNumber="jabcdefghi", *LowerNumber="0123456789";
const char *common_time=".c", *cut_time="_c";
const char* step_symbol[5]={"~c", "yz&=(!)", "nopqrst", "\?:$}|{w", "defghij"};
const char rest_symbol[4]={'m', 'u', 'v', 'x'};
const char dot_symbol='\'';
const char OctaveSymbol[8] = {'\0', '`', '~', '_', '"', '.', ';', ','};
const char* slur_symbol[8]={"c", ";b", "~c", "_c", "\"c", "~2", ";c", ",c"};
const char* TiedSymbol[3]={"`c", ".c", "~c"};
const char* arpeggiate_symbol[4]={">k", ">kk", "\">k", "\">kk"};
const char* ChordSymbol="-/+#903";
const char* SignSymbol[6]={
 ">/l", // G sign - treble clef
 ">/k", // 左手部分的G
 ">+l", // C sign
 ">+k", // 第四線的C
 ">#l", // F sign - bass clef
 ">#k" // 右手部分的F
};
const char* AlterSymbol[11]={"<<", "_<", "<", "`<", "*<", "*", "*%", "`%", "%", "_%", "%%"};
const char* barline[7]={" ", " l ", " k ", "<k", "<k'", "<7", "<2"};
const char *coda_symbol="+l", *segno_symbol="+";
const char* articulation[7]={"8", "`8", "_8", "\"8", ".8", ";8", ",8"};
const char* fermata_symbol[3]={"<l", "~<l", ";<l"};
const char* finger=" abl1k2"; // Index 0 is a space.
const char* in_accord_symbol[3]={"<>", "\"1", ".k"};
const char* dashes_sign[4]={"''", "'", "--", "-"};
const char* wedge_sign="c3d4"; // The start/stop of cresc/dim wedges.
const char* pedal_sign[5]={"<c", "*c", "*<c", "\"<c", "\"*c"};
const char *music_prefix=",'", *lyric_prefix=";2", *music_parentheses=",'";
const char* grace_symbol[2]={"5", "\"5"};
const char* ornament_sign[16]={"", "6", "", "", "\"6", "\"6l", ";6", ";6l", "", "", "", "", ",4", ",4l", "4", "4l"};
const char* note_shape_symbol[6]={"", "5a", "5b", "5l", "5k", "5'"};
const char* bow[2]={"<'", "<b"};
const char* page_number_sign="\"3";
#endif
