#ifndef OT_TIMER_GATE_HPP_
#define OT_TIMER_GATE_HPP_

#include <ot/liberty/celllib.hpp>
#include "ot/timer/pin.hpp"


namespace ot {

// Forward declaration
class Pin;
class Arc;
class Test;

// ------------------------------------------------------------------------------------------------

// Class: Gate
class Gate {

  friend class Timer;

  public:
    
    Gate(const std::string&, CellView);

    inline const std::string& name() const;

    const std::string& cell_name() const;
    // inline const std::vector<Pin*>& get_pins();
    std::vector<std::string> print_pins() const {
      std::vector<std::string> pin_names;
      // std::cout << "Pins for gate " << _name << ":" << std::endl;
      for (const auto* pin : _pins) {
        pin_names.push_back(pin->name());
        // std::cout << pin->name() << std::endl;
      }
      return pin_names;
    }


  private:

    std::string _name;

    CellView _cell;

    std::vector<Pin*> _pins;
    std::vector<Arc*> _arcs;
    std::vector<Test*> _tests;

    //void _repower(CellView);
}; 

// Function: name
inline const std::string& Gate::name() const {
  return _name;
}
// inline const std::vector<Pin*>& Gate::get_pins(){
//   return _pins;
// }

};  // end of namespace ot. -----------------------------------------------------------------------

#endif






