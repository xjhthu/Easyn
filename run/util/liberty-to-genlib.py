# Attempts to process only combinational gates!
# Sequential gates *should* be skipped, but may be improperly processed instead!
# Pin loads and delays are *not* processed! They are ignored, and arbitrary defaults are written to the output.
from sys import stderr, stdin
import argparse
from liberty.parser import parse_liberty
from enum import Enum
import re
import numpy as np

argparser = argparse.ArgumentParser(description='Convert a Synopsys Liberty library to a SIS Genlib library.')
argparser.add_argument('filename', help='Filename of the Synopsys Liberty file. Use "-" for stdin')
argparser.add_argument('-v', '--verbose', action='store_true', help='Print more info to stderr')
argparser.add_argument('-u', '--always-use', action='append', type=lambda s: re.split(',', s), 
        help='A list of cells to use, regardless of "dont care" status in the input library. Accepts regex.')
args = argparser.parse_args()

if args.filename == '-':
    lib = parse_liberty(stdin.read())
else:
    lib = parse_liberty(open(args.filename, 'r').read())

def debug (message):
    if args.verbose:
        print(message, file=stderr)

class TimingSense(Enum):
    NONE            = 0
    POSITIVE_UNATE  = 1
    NEGATIVE_UNATE  = 2
    UNKNOWN         = 3

def cellIsDontuse (cell):
    if cell['dont_use'] is None:
        return None
    else:
        return cell['dont_use']

def cellIsComb (cell):
    return not cellIsSeq(cell)

def cellIsSeq (cell):
    for pin in cell.get_groups('pin'):
        if ((pin['clock'] is not None and pin['clock'] == 'true')
            or (pin['clock_gate_out_pin'] is not None and pin['clock_gate_out_pin'] == 'true')):
            return True
    return False

def cellIsTie (cell):
    cell_sense = cellTimingSense(cell)
    if cell_sense == TimingSense.NONE and cellOutputPins(cell) == 1:
        # Probably a tie cell
        return True
    else:
        return False

def cellIsUnate (cell):
    sense = TimingSense.NONE
    cell_sense = cellTimingSense(cell)
    # if cell_sense == TimingSense.POSITIVE_UNATE or cell_sense == TimingSense.NEGATIVE_UNATE:
    if cell_sense == TimingSense.POSITIVE_UNATE or cell_sense == TimingSense.NEGATIVE_UNATE or cell_sense == TimingSense.UNKNOWN:
        return True
    elif cellIsTie(cell):
        return True
    else:
        return False

def pinIsOutput (pin):
    if pin['direction'] == 'output':
        return True
    else:
        return False

def cellOutputPins (cell):
    output_pins = 0
    output_pin = None
    for pin in cell.get_groups('pin'):
        if pinIsOutput(pin):
            output_pins += 1
            output_pin = pin
    return output_pins

def cellSingleOutput (cell):
    output_pins = cellOutputPins(cell)
    if output_pins != 1:
        return False
    else:
        for pin in cell.get_groups('pin'):
            if pinIsOutput(pin):
                return pin

# Return timing sense of a pin from a timing group as a TimingSense object
def timingGetTimingSense (timing) -> TimingSense:
    sense = timing['timing_sense']
    switcher = {
        'positive_unate': TimingSense.POSITIVE_UNATE,
        'negative_unate': TimingSense.NEGATIVE_UNATE,
        'unknown_unate': TimingSense.UNKNOWN,
    }
    return switcher.get(sense, TimingSense.NONE)

def outputpinTimingSense (pin) -> TimingSense:
    # If sense of all timing groups of output pin are equal, this determines the timing sense of the pin.
    # If sense of timing groups differ, sense is binate or unknown.
    if not pinIsOutput(pin):
        raise Exception('Cannot find timing sense of a non-output pin {}'.format(pin.args[0]))
    sense = TimingSense.NONE
    for timing in pin.get_groups('timing'):
        timing_sense = timingGetTimingSense(timing)
        if sense == TimingSense.NONE:
            sense = timing_sense
        elif sense != timing_sense:
            return TimingSense.UNKNOWN
        
    return sense

def cellTimingSense (cell) -> TimingSense:
    # Cannot determine cell sense of a non-single-output cell
    output_pin = cellSingleOutput(cell)
    if output_pin == False:
        return TimingSense.NONE
    # Cell is unate if timing sense of all pins is equal
    # The cell's output pin contains timing sense info w.r.t. to all inputs
    return outputpinTimingSense(output_pin)

multi_output = []
# Loop through all cells in library
for cell in lib.get_groups('cell'):
    cell_name = cell.args[0]
    output_count = 0

    # Check if cell is in alwaysuse list
    isAlwaysuseMatch = False
    if (args.always_use != None):   # Don't bother if alwaysuse list is empty
        flatten = lambda t: [item for sublist in t for item in sublist]
        rs = map(re.compile, flatten(args.always_use))
        for r in rs:
            if r.match(cell_name) != None:
                isAlwaysuseMatch = True
    
    # Skip cell marked 'dont use' which are not in the alwaysUseList
    if cellIsDontuse(cell) and not isAlwaysuseMatch:
        debug('Skipping cell marked as "dont use" in input library: {}'.format(cell_name))
        continue
    
    # If exists cells with multiple outputs, try to modify different data paths
    if cellSingleOutput(cell) == False:
        multi_output.append(cell_name)
        debug('Skipping cell without exactly one output pin: {}'.format(cell_name))
        continue
    
    # Skip non-unate gates
    if not cellIsUnate(cell):
        debug('Skipping cell with non-unate output: {}'.format(cell_name))
        continue
    
    # Skip sequential gates
    if cellIsSeq(cell):
        debug('Skipping sequential cell: {}'.format(cell_name))
        continue
        
    # Get output pin's function and translate to Genlib format
    output_pin = cellSingleOutput(cell)
    func = output_pin['function'].value
    
    area = cell['area'] 
    # Print Genlib format -- gate information
    print('GATE\t{}\t\t{}\t{}={};'.format(cell_name, area, output_pin.args[0], func))

    # Print Genlib format -- pin information
    replacements = {
            '&': '*',
            '|': '+',
            '"': '',
            ' ': '',
    }
    func = ''.join([replacements.get(c, c) for c in func])
    # Replace 0 and 1 functions with CONST0/CONST1
    if func == '0':
        func = 'CONST0'
    elif func == '1':
        func = 'CONST1'
    
    # Translate timing_sense to Genlib format
    switcher = {
            TimingSense.POSITIVE_UNATE: 'NONINV',
            TimingSense.NEGATIVE_UNATE: 'INV',
            TimingSense.UNKNOWN: 'UNKNOWN',
    }
            
    for pin in cell.get_groups('pin'):
        phase = switcher.get(cellTimingSense(cell), TimingSense.UNKNOWN)
        pin_name = pin.args[0]
        if not cellIsTie(cell):
            if pin['direction'] == 'input':
                input_load = pin['capacitance']
                
                for pin in cell.get_groups('pin'):
                    if pin['direction'] == 'output':
                        max_load = pin['max_capacitance']
                        for timing in pin.get_groups('timing'):
                            if timing['related_pin'] == pin_name:
                                
                                cell_fall = timing.get_groups('cell_fall')[0]['values']
                                cell_fall_str = str(cell_fall)[1:-1].replace('\\', '').strip(',"')
                                fall_fanout_delay = float(cell_fall_str.split(",")[-1])
                                
                                cell_rise = timing.get_groups('cell_rise')[0]['values']
                                cell_rise_str = str(cell_rise)[1:-1].replace('\\', '').strip(',"')
                                rise_fanout_delay = float(cell_rise_str.split(",")[-1])

                                fall_trans = timing.get_groups('fall_transition')[0]['values']
                                fall_trans_str = str(fall_trans)[1:-1].replace('\\', '').strip(',"')
                                fall_block_delay = float(fall_trans_str.split(",")[-1])
                                
                                rise_trans = timing.get_groups('rise_transition')[0]['values']
                                rise_trans_str = str(rise_trans)[1:-1].replace('\\', '').strip(',"')
                                rise_block_delay = float(rise_trans_str.split(",")[-1])
                
                
                print('PIN \t{}\t\t{}\t\t{}\t\t{}\t\t{}\t\t{}\t\t{}\t\t{}'.format(
                    pin_name, 
                    phase,
                    input_load,
                    max_load,
                    rise_block_delay,
                    rise_fanout_delay,
                    fall_block_delay,
                    fall_fanout_delay
                ))

    print()
    
# Deal with the multi-output cells
for cell in lib.get_groups('cell'):
    cell_name = cell.args[0]
    if cell_name in multi_output:
        output_count = 0
        
        # Skip sequential gates
        if cellIsSeq(cell):
            debug('Skipping sequential cell: {}'.format(cell_name))
            continue
            
        # Get output pin's function and translate to Genlib format
        for pin in cell.get_groups('pin'):
            if pinIsOutput(pin):
                output_pin = pin
                
                func = output_pin['function'].value
                
                area = cell['area'] 
                # Print Genlib format -- gate information
                print('GATE\t{}\t\t{}\t{}={};'.format(cell_name, area, output_pin.args[0], func))

                # Print Genlib format -- pin information
                replacements = {
                        '&': '*',
                        '|': '+',
                        '"': '',
                        ' ': '',
                }
                func = ''.join([replacements.get(c, c) for c in func])
                # Replace 0 and 1 functions with CONST0/CONST1
                if func == '0':
                    func = 'CONST0'
                elif func == '1':
                    func = 'CONST1'
                
                # Translate timing_sense to Genlib format
                switcher = {
                        TimingSense.POSITIVE_UNATE: 'NONINV',
                        TimingSense.NEGATIVE_UNATE: 'INV',
                        TimingSense.UNKNOWN: 'UNKNOWN',
                }
                        
                for pin in cell.get_groups('pin'):
                    phase = switcher.get(cellTimingSense(cell), TimingSense.UNKNOWN)
                    pin_name = pin.args[0]
                    if not cellIsTie(cell):
                        if pin['direction'] == 'input':
                            input_load = pin['capacitance']
                            
                            for pin in cell.get_groups('pin'):
                                if pin['direction'] == 'output':
                                    max_load = pin['max_capacitance']
                                    for timing in pin.get_groups('timing'):
                                        if timing['related_pin'] == pin_name:
                                            
                                            cell_fall = timing.get_groups('cell_fall')[0]['values']
                                            cell_fall_str = str(cell_fall)[1:-1].replace('\\', '').strip(',"')
                                            fall_fanout_delay = float(cell_fall_str.split(",")[-1])
                                            
                                            cell_rise = timing.get_groups('cell_rise')[0]['values']
                                            cell_rise_str = str(cell_rise)[1:-1].replace('\\', '').strip(',"')
                                            rise_fanout_delay = float(cell_rise_str.split(",")[-1])

                                            fall_trans = timing.get_groups('fall_transition')[0]['values']
                                            fall_trans_str = str(fall_trans)[1:-1].replace('\\', '').strip(',"')
                                            fall_block_delay = float(fall_trans_str.split(",")[-1])
                                            
                                            rise_trans = timing.get_groups('rise_transition')[0]['values']
                                            rise_trans_str = str(rise_trans)[1:-1].replace('\\', '').strip(',"')
                                            rise_block_delay = float(rise_trans_str.split(",")[-1])
                            
                            
                            print('PIN \t{}\t\t{}\t\t{}\t\t{}\t\t{}\t\t{}\t\t{}\t\t{}'.format(
                                pin_name, 
                                phase,
                                input_load,
                                max_load,
                                rise_block_delay,
                                rise_fanout_delay,
                                fall_block_delay,
                                fall_fanout_delay
                            ))

                print()
                
