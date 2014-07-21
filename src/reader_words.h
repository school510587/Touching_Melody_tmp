#ifndef READER_WORDS_H
#define READER_WORDS_H

const char* Comma="，";

//數字-量詞
const char Num_word[11][4] =
{
    "零","一","兩","三","四","五","六","七","八","九","十"
};

//數字
const char* number_word[17]={"零", "一", "二", "三", "四", "五", "六", "七", "八", "九", "十", "百", "千", "萬", "億", "負", "兩"};
const char* hand_word[4]={"無譜號資訊", "右手", NULL, "左手"};
const char* clef_word[6]={"無譜號資訊", "高音譜號", "中音譜號", "低音譜號", "打擊樂譜表", "六線譜譜號"};
const char* key_mode[10]={"調式", "教會", "教會", "教會", "教會", "教會", "教會", "教會", "大調", "小調"};
const char* special_time_word[3]={"普通拍號", "切半拍號", "任意節拍"};
const char* accidental_word[11]={"重降記號", "四分之三降記號", "降記號", "四分之一降記號", "還原後降記號", "還原記號", "還原後升記號", "四分之一生記號", "升記號", "四分之三生記號", "重升記號"};
const char* Ocatave_word[9]={"兩方第四點音層", "第四點音層", "第四五點音層", "第四五六點音層", "第五點音層", "第四六點音層", "第五六點音層", "第六點音層", "兩方第六點音層"};
const char* absolute_octave_word[9]={"大字二組", "大字一組", "大字組", "小字組", "小字一組", "小字二組", "小字三組", "小字四組", "小字五組"};
const char* step_word[7]={"兜", "蕤", "瞇", "發", "蒐", "拉", "希"};
const char* rest_word="休止符";
const char* type_word[11]={"長", "倍全", "全", "二分", "四分", "八分", "十六分", "三十二分", "六十四分", "一二八分", "二五六分"};
const char* in_accord_word="分部";
const char* slur_word="圓滑線";
const char* tied_word="連結線";
const char* arpeggiate_word[4]={"琶音", "下琶音", "跨譜表琶音", "跨譜表下琶音"};
const char* accent_word="強音";
const char* dashes_word[2]={"持續線開始", "持續線結束"};
const char* octave_shift_word[4]={"低", "高", "度記譜開始", "記譜回到正確音高"};
const char* wedge_word[4]={"開始漸強", "停止漸強", "開始漸弱", "停止漸弱"};
const char* detached_legato_word="次斷音";
const char* fermata_word[3]={"停留記號", "三角形停留記號", "方形停留記號"};
const char *grace_word="倚音", *grace_form[5]={"長", "短", "雙", "後", "滑"};
const char* ornament_word[16]={"", "震音", "", "", "漣音", "逆漣音", "長漣音", "長逆漣音", "", "", "", "", "迴音", "逆迴音", "後迴音", "後逆迴音"};
const char* pedal_word[4]={"踩下踏板", "放開踏板", "放開再踩下踏板", "半踏板"};
const char* spiccato_word="跳弓";
const char* staccatissimo_word="大頓音";
const char* staccato_word="斷奏";
const char* strong_accent_word="大強音";
const char* tenuto_word="持音";
const char* barline_word[7]={"一般小節線", "點字小節線", "虛點小節線", "結尾複縱線", "兩條細的小節線", "反覆記號左端", "反覆記號右端"};
const char *coda_word="尾聲記號", *segno_word="連續記號";
#endif // OUTWORDS_H_INCLUDED
