#include <mockturtle/mockturtle.hpp>
#include "PlaceInfo.hpp"

namespace PlaceInfo{
    void construct_gate_height_assignment_map(json gate_height_assignment_data,HashTable<std::string>* gateHeightAssignmentMap){
        for (auto& item : gate_height_assignment_data.items()) {
            (*gateHeightAssignmentMap)[item.key()] = item.value();
        }
    }
    void construct_placeInfo_map(nlohmann::json def_data,HashTable<Gate>* PlaceInfoMap){
        for (auto& item : def_data.items()) {
            // std::cout << item.key() << " : " << item.value() << "\n";
            Gate gate;
            gate.cell_type = item.value()["macroName"]; 
            gate.name = item.key();
            gate.isFixed = item.value()["isFixed"];
            gate.isPlaced = item.value()["isPlaced"];
            gate.orient = item.value()["orient"];
            gate.x = item.value()["x"];
            gate.y = item.value()["y"];
            (*PlaceInfoMap)[gate.name] = gate;
            // std::cout <<" inside map "<<PlaceInfoMap[gate.name].name <<"\n";
        }
    }

    void process_net(nlohmann::json net_data, HashTable<Net>* PlaceInfo_net, 
                    HashTable<Pin>* PlaceInfo_pin, HashTable<std::string>* PlaceInfo_gateToNet, HashTable<Gate>& PlaceInfoMap)
    {
        for (auto& net : net_data.items()){
            Net net_info;
            net_info.name = net.value()["name"];
            auto pins = net.value()["pins"];
            for (auto& pin : pins)
            {   
                Pin pin_info;
                pin_info.name = pin["name"].get<std::string>();
                pin_info.type = pin["type"].get<std::string>();
                if (pin_info.type == "IOpin"){
                    pin_info.x = pin["x"].get<int>();
                    pin_info.y = pin["y"].get<int>();
                } else if (pin_info.type == "standardcell"){
                    std::string cellname = pin["cellname"].get<std::string>();
                    auto it = PlaceInfoMap.find(cellname);
                    if (it != PlaceInfoMap.end()){
                        // Read x and y coordinates
                        pin_info.x = it->second.x;
                        pin_info.y = it->second.y;
                        pin_info.macroname = it->second.cell_type;
                        pin_info.cellname = it->first;
                        //std::cout << "x = " << pin_info.x << ", y = " << pin_info.y << std::endl;
                    }else{
                        std::cerr << "Cannot find gate " << cellname << std::endl;
                    }

                    // if the pin of the gate is "Z" or "ZN", store the gate-to-net map to PlaceInfo_gateToNet
                    if (pin_info.name=="Z" or pin_info.name=="ZN"){
                        auto it = PlaceInfo_gateToNet->find(cellname);
                        if (it == PlaceInfo_gateToNet->end()){
                            (*PlaceInfo_gateToNet)[cellname] = net_info.name;
                        }
                        else{
                            std::cerr << "Gate " <<cellname<<" already exists in the gateToNet map"<< std::endl;
                        }
                    }
                } 

                // Check if the pin already exists in the same Net object
                auto net_it = PlaceInfo_net->find(net_info.name);
                if (net_it != PlaceInfo_net->end()) {
                    // This net already exists in PlaceInfo_net
                    auto& pins_vec = net_it->second.pins;
                    auto it = std::find_if(pins_vec.begin(), pins_vec.end(),
                                        [&](const Pin& p){ return p.name == pin_info.name; });
                    if (it != pins_vec.end()){
                        std::cerr << "Pin " << pin_info.name << " already exists in Net " << net_info.name << std::endl;
                        it->x = pin_info.x;
                        it->y = pin_info.y;
                        it->type = pin_info.type;
                        it->macroname = pin_info.macroname;
                        it->cellname = pin_info.cellname;
                    } else {
                        pins_vec.push_back(pin_info);
                    }
                } else {
                    net_info.pins.push_back(pin_info);
                }

                (*PlaceInfo_pin)[pin_info.name] = pin_info;        
            }
            (*PlaceInfo_net)[net_info.name] = net_info;
        }
        std::cout<<"Finish processing net data. "<<std::endl;

    }

    void parseVerilogFile(const std::string& verilog_data, HashTable<std::string>& PlaceInfo_verilog)
    {
        std::istringstream iss(verilog_data);
        std::string line;
        bool in_module = false;
        std::vector<std::string> input_signals;
        std::vector<std::string> output_signals;

        while (std::getline(iss, line)) {
            // Remove whitespace characters at the beginning and end of lines
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);

            // Check if the module definition is entered
            if (line.find("module") == 0) {
                in_module = true;
                continue;
            }

            // Check if you have left the module definition
            if (line.find("endmodule") == 0) {
                in_module = false;
                continue;
            }

            if (in_module) {
                // Check if it is an input signal
                if (line.find("input") == 0) {
                    line.erase(0, 6); 
                    std::istringstream iss(line);
                    std::string signal_name;
                    while (std::getline(iss, signal_name, ',')) {
                        signal_name.erase(0, signal_name.find_first_not_of(' ')); // Remove leading spaces
                        signal_name.erase(signal_name.find_last_not_of(' ') + 1); // Remove trailing spaces
                        input_signals.push_back(signal_name);
                    }
                }

                // Check if it is an output signal
                if (line.find("output") == 0) {
                    line.erase(0, 7); // Remove the "output" keyword
                    std::istringstream iss(line);
                    std::string signal_name;
                    while (std::getline(iss, signal_name, ',')) {
                        signal_name.erase(0, signal_name.find_first_not_of(' ')); // Remove leading spaces
                        signal_name.erase(signal_name.find_last_not_of(' ') + 1); // Remove trailing spaces
                        output_signals.push_back(signal_name);
                    }
                }
            }
        }

        // Store in a hash table
        for (const auto& signal_name : input_signals) {
            PlaceInfo_verilog.emplace(signal_name, "input");
        }
        for (const auto& signal_name : output_signals) {
            PlaceInfo_verilog.emplace(signal_name, "output");
        }
    }

    int a(){
        return 1;
    }

    void rewrite_net_json(std::string design_name, const HashTable<Net>& PlaceInfo_net, const json& original_json) 
    {
        json net_json = original_json;
        for (json::iterator it = net_json.begin(); it != net_json.end(); ++it) {
            json& net_obj = *it;
            std::string net_name = net_obj["name"];
            const Net& net = PlaceInfo_net.find(net_name)->second;
            json pins_array;
            for (const auto& pin : net.pins) {
                json pin_obj;
                pin_obj["name"] = pin.name;
                pin_obj["type"] = pin.type;
                if (pin.type == "IOpin") {
                    pin_obj["x"] = pin.x;
                    pin_obj["y"] = pin.y;
                } else if (pin.type == "standardcell") {
                    pin_obj["cellname"] = pin.cellname;
                }
                pins_array.push_back(pin_obj);
            }
            net_obj["pins"] = pins_array;
        }

        std::ofstream modified_file("../examples/benchmarks/"+design_name+"/"+design_name + "_net_modified.json");
        modified_file << std::setw(4) << net_json << std::endl;
        modified_file.close();
    }

    void rewrite_def_json(std::string file_path, const HashTable<Gate>& PlaceInfoMap) 
    {
        json def_json;
        
        // Iterate through all gates in the PlaceInfoMap
        for (auto& gate_pair : PlaceInfoMap) {
            json gate_json;
            gate_json["isFixed"] = gate_pair.second.isFixed; 
            gate_json["isPlaced"] = gate_pair.second.isPlaced; 
            gate_json["macroName"] = gate_pair.second.cell_type; 
            gate_json["orient"] = gate_pair.second.orient; 
            gate_json["x"] = gate_pair.second.x; 
            gate_json["y"] = gate_pair.second.y; 

            // Add the JSON object to the def JSON with the gate's name as the key
            def_json[gate_pair.second.name] = gate_json;
        }
 
        std::ofstream modified_file(file_path);
        modified_file << std::setw(4) << def_json << std::endl;
        modified_file.close();
    }

    void process_lef(nlohmann::json lef_data, HashTable<Macro>* PlaceInfo_macro) 
    {
        for (auto& [key, value] : lef_data.items()) {
            PlaceInfo::Macro m;
            m.macroname = key;
            m.height = value["height"].get<float>()/100;
            m.width = value["width"].get<float>()/100;
            m.area = m.height * m.width;
            PlaceInfo_macro->insert(std::make_pair(key, m));
        }
    }

}
    



