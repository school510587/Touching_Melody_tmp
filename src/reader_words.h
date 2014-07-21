#ifndef READER_WORDS_H
#define READER_WORDS_H

const char* Comma="�A";

//�Ʀr-�q��
const char Num_word[11][4] =
{
    "�s","�@","��","�T","�|","��","��","�C","�K","�E","�Q"
};

//�Ʀr
const char* number_word[17]={"�s", "�@", "�G", "�T", "�|", "��", "��", "�C", "�K", "�E", "�Q", "��", "�d", "�U", "��", "�t", "��"};
const char* hand_word[4]={"�L�и���T", "�k��", NULL, "����"};
const char* clef_word[6]={"�L�и���T", "�����и�", "�����и�", "�C���и�", "�������Ъ�", "���u���и�"};
const char* key_mode[10]={"�զ�", "�з|", "�з|", "�з|", "�з|", "�з|", "�з|", "�з|", "�j��", "�p��"};
const char* special_time_word[3]={"���q�縹", "���b�縹", "���N�`��"};
const char* accidental_word[11]={"�����O��", "�|�����T���O��", "���O��", "�|�����@���O��", "�٭�᭰�O��", "�٭�O��", "�٭��ɰO��", "�|�����@�ͰO��", "�ɰO��", "�|�����T�ͰO��", "���ɰO��"};
const char* Ocatave_word[9]={"���ĥ|�I���h", "�ĥ|�I���h", "�ĥ|���I���h", "�ĥ|�����I���h", "�Ĥ��I���h", "�ĥ|���I���h", "�Ĥ����I���h", "�Ĥ��I���h", "���Ĥ��I���h"};
const char* absolute_octave_word[9]={"�j�r�G��", "�j�r�@��", "�j�r��", "�p�r��", "�p�r�@��", "�p�r�G��", "�p�r�T��", "�p�r�|��", "�p�r����"};
const char* step_word[7]={"��", "�B", "�N", "�o", "�`", "��", "��"};
const char* rest_word="����";
const char* type_word[11]={"��", "����", "��", "�G��", "�|��", "�K��", "�Q����", "�T�Q�G��", "���Q�|��", "�@�G�K��", "�G������"};
const char* in_accord_word="����";
const char* slur_word="��ƽu";
const char* tied_word="�s���u";
const char* arpeggiate_word[4]={"�]��", "�U�]��", "���Ъ�]��", "���Ъ�U�]��"};
const char* accent_word="�j��";
const char* dashes_word[2]={"����u�}�l", "����u����"};
const char* octave_shift_word[4]={"�C", "��", "�װO�ж}�l", "�O�Ц^�쥿�T����"};
const char* wedge_word[4]={"�}�l���j", "����j", "�}�l���z", "����z"};
const char* detached_legato_word="���_��";
const char* fermata_word[3]={"���d�O��", "�T���ΰ��d�O��", "��ΰ��d�O��"};
const char *grace_word="�ʭ�", *grace_form[5]={"��", "�u", "��", "��", "��"};
const char* ornament_word[16]={"", "�_��", "", "", "����", "�f����", "������", "���f����", "", "", "", "", "�j��", "�f�j��", "��j��", "��f�j��"};
const char* pedal_word[4]={"��U��O", "��}��O", "��}�A��U��O", "�b��O"};
const char* spiccato_word="���}";
const char* staccatissimo_word="�j�y��";
const char* staccato_word="�_��";
const char* strong_accent_word="�j�j��";
const char* tenuto_word="����";
const char* barline_word[7]={"�@��p�`�u", "�I�r�p�`�u", "���I�p�`�u", "�������a�u", "����Ӫ��p�`�u", "���аO������", "���аO���k��"};
const char *coda_word="���n�O��", *segno_word="�s��O��";
#endif // OUTWORDS_H_INCLUDED
