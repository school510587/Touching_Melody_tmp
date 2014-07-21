#ifndef ANALYZER_H_INCLUDED
#define ANALYZER_H_INCLUDED
#include <set>
#include "score.h"
#include "xmlreader.h"
class XMLAnalyzer{
 private:
  template<class kind>
  class buffer_type{
   private:
    typedef class std::vector<kind> vector_type;
    vector_type* data;
    std::vector<long> time_point;
   public:
    typedef class vector_type::iterator iterator;
    typedef typename vector_type::size_type size_type;
    buffer_type() throw(): data(NULL){}
    size_type actual_index(size_type i) throw(){return data->size()-(size()-i);}
    kind& actual_element(size_type i) throw(){return data->at(i);}
    kind& back() throw(){return data->back();}
    iterator begin() throw(){return data_begin()+(data->size()-size());}
    iterator data_begin() throw(){return data->begin();}
    iterator end() throw(){return data->end();}
    bool empty() const throw(){return time_point.empty();}
    void insert(iterator& p, const kind& x, long t) throw(){
     time_point.insert(time_point.begin()+((p-data->begin())-(data->size()-size())), t);
     data->insert(p, x);
    }
    kind& operator[](size_type i) throw(){return data->at(actual_index(i));}
    void push_back(const kind& k, long t) throw(){
     time_point.push_back(t);
     data->push_back(k);
    }
    void reset_time() throw(){time_point.clear();}
    void set_data(vector_type& new_data) throw(){
     data=&new_data;
     reset_time();
    }
    size_type size() const throw(){return (size_type)time_point.size();}
    long time(size_type i) throw(){return time_point[i];}
  };
  typedef std::pair<long, bms_measure_element> music_event;
  typedef struct{
   long time;
   int staff;
   bool above;
   unsigned char value;
  } sound_dynamics_t;
  typedef struct{
   int type;
   bms_note* nptr;
  } slur_trait_t; // Record of a single slur element.
  typedef struct{
   std::vector<slur_trait_t> path;
   int number;
   unsigned char attribute_flag;
  } slur_record_t;
  typedef struct{
   int number, pitch, staff, state, type;
   bms_note* nptr;
  } tied_record_t;
  struct note_vertex{ // Vertices for graphic analysis of in-accords.
   std::list<note_vertex*> next; // Edges.
   bms_note* note; // The pointer can be adjusted within a chord.
   long time;
   bool visited;
  };
  static const unsigned short damper_pedal_change, damper_pedal, soft_pedal_change, soft_pedal, sostenuto_pedal_change, sostenuto_pedal;
  static bool chord_lower_than(const bms_note* const&, const bms_note* const&) throw();
  static bool earlier_than(const sound_dynamics_t& p, const sound_dynamics_t& q) throw(){return p.time<q.time;}
  static bool lower_than(note_vertex* const&, note_vertex* const&) throw();
  static bool pitch_lower_than(const bms_note* const&, const bms_note* const&) throw();
  static bool prior_than(const music_event&, const music_event&) throw();
  static bool upper_than(note_vertex* const&, note_vertex* const&) throw();
  struct{
   buffer_type<bms_attributes> attributes;
   buffer_type<bms_barline> barline;
   buffer_type<bms_direction> direction;
   buffer_type<bms_note*> note;
   std::vector<music_event> event;
   std::vector<bms_note_area>* voice;
  } buffer;
  struct{
   std::map<long, std::vector<bms_note*> > at;
   std::map<bms_note*, unsigned long> time;
   std::set<const bms_note*> uncorrected_trill;
  } note_map; // Implementation of note-to-time and note-to-group relationships.
  struct voice_tree_node{
   std::vector<note_vertex> vertex;
   std::vector<voice_tree_node*> child;
   long time_range[2];
   bool downward_interval;
  } root;
  struct{
   std::map<std::string, std::pair<int, size_t> > id; // The id-(part, instrument index) relation.
   std::map<int, size_t> part; // The part-the 1st instrument index relation.
  } instrument;
  bms_node position;
  std::list<sound_dynamics_t> dynamics_list;
  std::vector<MidiCommandVector> midi_part;
  std::map<std::string, size_t> slur_map;
  std::vector<slur_record_t> slur_record;
  std::vector<tied_record_t> tied_record;
  MusicXML2::ctree<MusicXML2::xmlelement>::iterator xml_iter;
  bms_score* score;
  int fifths, alter[7]; // A declaration of pre-defined alternations.
  long divisions, measure_begin, measure_end, time_axis;
  void add(bms_note*, unsigned long) throw();
  void analyze(bms_part&) throw();
  void analyze_midi(MidiCommandVector&, size_t) throw();
  void analyze_skills(size_t) throw();
  void analyze_slur() throw();
  void analyze_tied() throw();
  void analyze_tuplet() throw();
  void clear_voice_tree(voice_tree_node*) throw();
  std::vector<music_event>::iterator find_event(const bms_note*) throw();
  bool find_path(std::list<note_vertex*>&, note_vertex*, long=0) throw();
  void parallel_cut(voice_tree_node*) throw();
  void parse(MusicXML2::Sxmlelement&) throw(std::runtime_error);
  void parse_dynamics(size_t, bms_direction&) throw();
  void parse_sound(int, long) throw();
  void prepare_note_map() throw();
  void set_fifths(int) throw();
  void vertical_cut(voice_tree_node*) throw();
  void write_into(std::vector<bms_measure_element>&) throw();
 public:
  XMLAnalyzer(bms_score&) throw();
  virtual ~XMLAnalyzer() throw();
  bool read(const std::string&) throw();
};
#endif  //ANALYZER_H_INCLUDED
