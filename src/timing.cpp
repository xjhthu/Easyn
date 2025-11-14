#include <flute.h>
#include "timing.hpp"
#include <iomanip> 
// #include <omp.h>

using json = nlohmann::json;
template<typename T>
using HashTable = std::unordered_map<std::string , T>;

FluteResult createRCTree::runFlute(){

    std::cout << "Start rctree generation! " << std::endl;

    auto res_unit = timer->resistance_unit()->value();
    auto cap_unit = timer->capacitance_unit()->value();
    float rf = WIRE_RESISTANCE_PER_MICRON / res_unit;      
    float cf = WIRE_CAPACITANCE_PER_MICRON / cap_unit;

    Flute::readLUT();
    auto start = std::chrono::high_resolution_clock::now();
    
    // Delete duplicate net_name
    std::sort(net_names.begin(), net_names.end());
    auto net_end = std::unique(net_names.begin(), net_names.end());
    net_names.erase(net_end, net_names.end());

    // Create an RC tree for each net 
    // Attention: Currently not considering multiple pins occupying the same point position 
//     omp_lock_t lock;
//     omp_init_lock(&lock);
// #pragma omp parallel for
    for (const auto& net_name : net_names){
        // std::cout<<"Re-building rctree for net:"<<net_name<<std::endl;

        // Add RC nodes and RC edges into the RC tree stored
        auto& net = timer->nets().find(net_name)->second;       
        auto& tree = net.emplace_rct();
        int degree = 0;

        // Find the original information for this net  
        std::vector<PlaceInfo::Pin> pins;
        auto it = PlaceInfo_net.find(net_name);
        if (it != PlaceInfo_net.end()) {
            degree = it->second.pins.size(); 
            // std::cout << "Net " << net_name << " has " << degree << " pins." << std::endl;
            pins = it->second.pins;
        }

        // Find the leftmost and bottommost position
        // Use this as a relative coordinate system to mark positions
        int leftmost_x = INT_MAX;
        int bottommost_y = INT_MAX;
        for (auto& pin : pins){
            leftmost_x = std::min(leftmost_x, pin.x);
            bottommost_y = std::min(bottommost_y, pin.y);
        }
        
        // Caculate the offset of each pin relative to the left-bottom point
        std::vector<std::tuple<int, int>> pinCenters;
        std::vector<std::pair<std::string, std::string>> pin_info;
        for (auto& pin : pins){
            int offset_x = pin.x - leftmost_x;
            int offset_y = pin.y - bottommost_y;
            pinCenters.emplace_back(std::make_tuple(offset_x, offset_y));
            if (pin.type == "standardcell"){
                pin_info.emplace_back(std::make_pair(pin.name, pin.cellname));
            }else{
                pin_info.emplace_back(std::make_pair(pin.name, ""));
            }
            
        }

        // Location to pins
        // Loc2Pins: relative coordinates of the pin and the index number
        std::unordered_map<std::tuple<int, int>, int, hash_tuple> loc2Pins; 
        for (int i = 0; i < pinCenters.size(); i++){
            loc2Pins[pinCenters[i]] = i;
        }

        // This is a lambda expression takes a local pin index in this net as input
        // and insert node into the rc tree if the node has not been inserted yet.
        // The name of rc node will be returned.
        auto emplace_rc_node = 
        [&](int index, int pinIdx, std::tuple<int,int> pin) -> std::string {
            std::string name;
            if (pinIdx != -1) {       
                int pinIndex = loc2Pins[pin];       
                auto [pin_name,cell_name] = pin_info[pinIndex]; 
                if (!cell_name.empty()){
                    name = cell_name + ":" + pin_name;
                }else{
                    name = pin_name;
                }
               
                if (!tree.node(name)) {
                    tree.insert_node(name, 0); 

                    // Set the inner pin pointer of this rc node.
                    tree.node(name)->pin(timer->pins().at(name)); 
                }   

            } else { // This rc node is a Steiner point.
                name = "steiner:" + std::to_string(index); 
                if (!tree.node(name)) {
                    tree.insert_node(name, 0);
                }
            }
            return name;
        };

        int xs[100 * degree];
        int ys[100 * degree];
        int pt_cnt = 0;          
        int node_cnt = 0;        // steiner nodes count = steiner points + pins

        for (auto& loc2pin : loc2Pins){
            const std::tuple<int, int>& loc = loc2pin.first;
            xs[pt_cnt] = std::get<0>(loc);
            ys[pt_cnt] = std::get<1>(loc);
            pt_cnt++;
        }

        // Call FLUTE to construct the Steiner Tree of the net
        std::map<int, SNode> snodes; //Store all nodes in the Steiner tree
        if (degree >= 2 ){
            Flute::Tree flutetree = Flute::flute(degree, xs, ys, FLUTE_ACCURACY);
            // loc2Node: containing loc2pins + extra steiner points
            std::unordered_map<std::tuple<int, int>, int, hash_tuple> loc2Node;  
            // Iterate through each branch of the tree
            for (int i = 0; i < degree * 2 - 2; i++){
                Flute::Branch& branch1 = flutetree.branch[i];
                Flute::Branch& branch2 = flutetree.branch[branch1.n];
                // Take out the endpoints for each branch
                std::tuple<int, int> fluteEdge[2];
                fluteEdge[0] = std::make_tuple(branch1.x, branch1.y);
                fluteEdge[1] = std::make_tuple(branch2.x, branch2.y);
                
                // create steiner nodes
                for (int j = 0; j < 2; j++){
                    std::tuple<int, int>& nodeLoc = fluteEdge[j];
                    // No saved nodes exist
                    if (loc2Node.find(nodeLoc) == loc2Node.end()){
                        // Create a new point in the steiner tree
                        SNode node(fluteEdge[j]);
                        // If node is a pin
                        if (loc2Pins.find(nodeLoc) != loc2Pins.end()){
                            node.pinIdxs = loc2Pins[nodeLoc];
                        }
                        node.idx = node_cnt;
                        snodes[node_cnt++] = node;
                        loc2Node[nodeLoc] = snodes.size() - 1; //Add index number
                    }
                }
     
                // add connectivity info
                auto distance = manhattanDistance(fluteEdge[0], fluteEdge[1]);
                double wl = (distance * 1.0) /UNIT_TO_MICRON; 
                
                // Add an edge between the two pins
                if (fluteEdge[0] != fluteEdge[1]){
                    int nodeIdx1 = loc2Node[fluteEdge[0]];
                    int nodeIdx2 = loc2Node[fluteEdge[1]];

                    snodes[nodeIdx1].toConnect.insert(nodeIdx2);
                    snodes[nodeIdx2].toConnect.insert(nodeIdx1);

                    // insert the node and capacitance (*CAP section) 
                    auto from = emplace_rc_node(nodeIdx1, snodes[nodeIdx1].pinIdxs, 
                        std::make_tuple(snodes[nodeIdx1].x,snodes[nodeIdx1].y));
                    
                    auto to = emplace_rc_node(nodeIdx2, snodes[nodeIdx2].pinIdxs, 
                        std::make_tuple(snodes[nodeIdx2].x,snodes[nodeIdx2].y));
                    
                    // insert the segment (*RES section)
                    tree.insert_segment(from, to, rf * wl);
                    tree.increase_cap(from, cf * wl * 0.5);
                    tree.increase_cap(to,   cf * wl * 0.5);

                }else{
                    continue;
                }
            }
            free(flutetree.branch);
        } else if (degree == 1){  // The RCTree only contains a single root node
            // std::cout << "Pay attention: the net only contains one pin!"<<std::endl;
            std::string name;      
            name = pin_info[0].second + ":"+pin_info[0].first;
            if (!tree.node(name)) {
                tree.insert_node(name, 0); 

                // Set the inner pin pointer of this rc node.
                tree.node(name)->pin(timer->pins().at(name)); 
            }   
            // auto nodeLoc = std::make_tuple(xs[0], ys[0]);
            // SNode node(nodeLoc);
            // if (loc2Pins.find(nodeLoc) != loc2Pins.end()){
            //     node.pinIdxs = loc2Pins[nodeLoc];
            //     node.idx = node_cnt;
            //     snodes[node_cnt].toConnect.insert(0);
            //     snodes[node_cnt++] = node;
                
            //     // insert rcnode as root node
            // } else{
            //     std::cerr << "Error: loc2Pins is empty." << std::endl;
            // }
        } else {
            std::cerr << "Error: degree = 0." << std::endl;
        }      
        // tree.print_node_names();
        // print_snodes(snodes);

        // Specify the root node: it is the one with index 0 in the pin
        std::string root_pin_name;
        std::string root_pin_cellname;

        bool found_root_pin = false;

        for (const auto& pin : pins) {
            if (pin.name == "ZN" || pin.name == "Z") {
                root_pin_name = pin.name;
                root_pin_cellname = pin.cellname;
                found_root_pin = true;
                break;
            }
            else if (pin.type == "IOpin") { //优先判断IOpin
                root_pin_name = pin.name;
                root_pin_cellname = "";
                found_root_pin = true;
                break;
            }
        }

        if (!found_root_pin) {
            root_pin_name = pins[pins.size()-1].name;
            root_pin_cellname = pins[pins.size()-1].cellname;
            tree.assign_root(root_pin_cellname + ":" + root_pin_name);
        }else{
            if (!root_pin_cellname.empty()){
                tree.assign_root(root_pin_cellname + ":" + root_pin_name);
            }else{
                tree.assign_root(root_pin_name);
            }
        }

        tree.update_rc_timing();

        // // --------------------------------------------
        // // The frontier insertion is very IMPORTANT!
        // // Otherwise you will not get updated timing report each time.
 
        timer->insert_frontier(*net.root()); //insert_frontier should point to the root node of the rctree

        // Debugging......

        // auto rct = net.rct();
        // for (const auto& [name, node] : rct->nodes()) {
        //     std::cout<<name<<std::endl;
        //     const auto& pin_ptr = node.pin();
        //     if (pin_ptr==nullptr) {
        //         std::cout << "Node: " << name << std::endl;
        //         continue;
        //     }
        //     const auto& pin = *pin_ptr;
        //     std::cout << "Node: " << name << ", Pin: " << pin.name()
        //             << ", is_rct_root: " << pin.is_rct_root() << std::endl;
        // }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    std::cout << "finish rctree generation: " << std::fixed << std::setprecision(4) << duration.count() << " s" << std::endl;
    
    // Remember to call update_states!
    timer->update_states();

    // Result report
    std::cout << "Timing reports after rctree building: "<<std::endl;
    std::optional<float> tns  = timer->report_tns();
    std::optional<float> wns  = timer->report_wns();
    std::optional<float> area = timer->report_area();
    std::optional<float> lkp =  timer->report_leakage_power();
    std::cout << std::fixed << std::setprecision(8); 
    std::cout <<"tns "<<*tns <<" wns "<<*wns<<" area "<<*area<< " leakage_power "<<*lkp<< "\n";
    // std::ofstream of_regen("../build/regen_slack.txt");
    // timer->dump_slack(of_regen);
    // of_regen.close();

    FluteResult result;
    result.tns = *tns;
    result.wns = *wns;

    return result;

}

float createRCTree::manhattanDistance(const std::tuple<int, int>& p1, const std::tuple<int, int>& p2) {
    return abs(std::get<0>(p1) - std::get<0>(p2)) + abs(std::get<1>(p1) - std::get<1>(p2));
}

void createRCTree::print_snodes(const std::map<int, SNode>& snodes) {
  for (const auto& snode : snodes) {
    std::cout << "SNode " << snode.second.idx << ": (" << snode.second.x << ", " << snode.second.y << ")\n";
    std::cout << "  Connections:";
    for (const auto& conn : snode.second.toConnect) {
        std::cout << " " << conn;
    }
    std::cout << std::endl;
    }
}


/* 
    Several types of net skipped in functions:
    1. There is only one pin in the net.
    2. There is no such output pin for Z and ZN in net.
*/
void createRCTree::buildTopo_global(std::string lib_file, std::string verilog_file, std::string sdc_file)
{

    timer->read_celllib(lib_file)
        .read_verilog(verilog_file)
        .read_sdc(sdc_file)
        .update_timing();

    for (auto& net_pair : PlaceInfo_net){
        const std::string& net_name = net_pair.first;
        const auto& pins = net_pair.second.pins;

        bool has_iopin = false;
        for (const auto& pin : pins){
            if (pin.type == "iopin"){
                has_iopin = true;
                break;
            } else{
                if (pin.name == "Z" || pin.name == "ZN"){
                    has_iopin = true;
                    break;
                }
            }
        }
        if (!has_iopin){
            continue;
        }

        if (pins.size() > 1) {
            net_names.push_back(net_name);
        } else {
            continue;
        }
    }
    
    for (const auto& net_name: net_names){
        auto& net = PlaceInfo_net[net_name];
        // for (auto&pin :net.pins){
        //     if (pin.type == "standardcell"){
        //         timer->disconnect_pin(pin.cellname + ":" + pin.name);
        //     } else{  // pin.type = "IOpin"
        //         timer->disconnect_pin(pin.name);
        //     }
            
        // }
        timer->remove_net(net_name)
             .update_timing();

        // disconnect all pins except gate_chosen
        timer->insert_net(net_name);
        for (auto& pin : net.pins) {
            if (pin.type != "standardcell"){
                timer->connect_pin(pin.name, net_name);
                continue;
            }else
                timer->connect_pin(pin.cellname+":"+pin.name, net_name);
        }        
        timer->update_timing();
    }
    timer->update_timing();
    std::cout << "hello!"<<std::endl;
}  


void createRCTree::report_timing()
{    
    std::optional<float> tns  = timer->report_tns();
    std::optional<float> wns  = timer->report_wns();
    std::optional<float> area = timer->report_area();
    std::cout << std::fixed << std::setprecision(4); 
    std::cout <<"tns_r "<<*tns <<" wns_r "<<*wns<<" area_r "<<*area<<"\n";
    std::cout<<"timer->num_nets()"<<timer->num_nets()<<std::endl;

    // std::ofstream of_verilog("../build/tmp_greedy.v");
    // timer->dump_verilog(of_verilog);
    // of_verilog.close();

    // std::ofstream of_spef("../build/tmp_greedy.spef");
    // timer->dump_spef(of_spef);
    // of_spef.close();

    return;

}   

void createRCTree::buildTopo_test(std::string lib_file, std::string verilog_file, 
                        std::string spef_file, std::string sdc_file)
{
    // Incremental timing analysis

    // read a library
    timer->read_celllib(lib_file)
         .read_verilog(verilog_file)
         .read_spef(spef_file)
         .read_sdc(sdc_file)
         .update_timing();

    std::optional<float> tns  = timer->report_tns();
    std::optional<float> wns  = timer->report_wns();
    std::optional<float> area = timer->report_area();
    std::cout << std::fixed << std::setprecision(8); 
    std::cout <<"tns "<<*tns <<" wns "<<*wns<<" area "<<*area<<"\n";
    std::cout<<"timer->num_nets()"<<timer->num_nets()<<std::endl;
    std::cout<<"timer->num_pins()"<<timer->num_pins()<<std::endl;
    std::cout<<"timer->num_gates()"<<timer->num_gates()<<std::endl;
    std::ofstream of_regen("../build/origin_slack.txt");
    timer->dump_slack(of_regen);
    of_regen.close();


    // Change the circuit structure
    // Remove the connection to the pin on the current net, but if the pin is 
    // still connected to another net, the connection must be replenished.
    // ! The corresponding spef information will also be deleted after the move.
    std::cout<<"Start changing the circuit structure!"<<std::endl;
    timer->remove_gate("g416");
    timer->update_timing();

    std::optional<float> tns_s  = timer->report_tns();
    std::optional<float> wns_s = timer->report_wns();
    std::optional<float> area_s = timer->report_area();
    std::cout << std::fixed << std::setprecision(8); 
    std::cout <<"tns_s "<<*tns_s <<" wns_s "<<*wns_s<<" area_s "<<*area_s<<"\n";
    std::cout<<"timer->num_nets()"<<timer->num_nets()<<std::endl;
    std::cout<<"timer->num_pins()"<<timer->num_pins()<<std::endl;
    std::cout<<"timer->num_gates()"<<timer->num_gates()<<std::endl;

    
    // Re-import the modified net and try to build the RCtree
    // std::cout<<"Re-import the modified net!"<<std::endl;

    timer->insert_gate("g416","NAND3_X1_12T");

    timer->remove_net("n414");
    timer->remove_net("n415");
    timer->remove_net("n241");
    timer->remove_net("n412");;

    timer->connect_pin("g416:ZN","n416");
   

    timer->insert_net("n415");
    timer->connect_pin("g474:A1","n415");
    timer->connect_pin("g442:A2","n415");
    timer->connect_pin("g415:Z","n415");

    timer->insert_net("n414");
    timer->connect_pin("g442:A3","n414");
    timer->connect_pin("g416:A1","n414");
    timer->connect_pin("g414:ZN","n414");

    timer->insert_net("n241");
    timer->connect_pin("g685:I1","n241");
    timer->connect_pin("g415:A1","n241");
    timer->connect_pin("g414:A1","n241");
    timer->connect_pin("g413:A2","n241");
    timer->connect_pin("g241:ZN","n241");
    timer->connect_pin("g416:A2","n241");

    timer->insert_net("n412");
    timer->connect_pin("g832:A1","n412");
    timer->connect_pin("g415:A2","n412");
    timer->connect_pin("g413:A1","n412");
    timer->connect_pin("g412:ZN","n412");
    timer->connect_pin("g416:A3","n412");


    timer->update_timing();
    
    std::optional<float> tns_r  = timer->report_tns();
    std::optional<float> wns_r  = timer->report_wns();
    std::optional<float> area_r = timer->report_area();
    std::cout << std::fixed << std::setprecision(8); 
    std::cout <<"tns_r "<<*tns_r <<" wns_r "<<*wns_r<<" area_r "<<*area_r<<"\n";
    std::cout<<"timer->num_nets()"<<timer->num_nets()<<std::endl;
    std::cout<<"timer->num_pins()"<<timer->num_pins()<<std::endl;
    std::cout<<"timer->num_gates()"<<timer->num_gates()<<std::endl;
    std::ofstream of_rebuild("../build/rebuild_slack.txt");
    timer->dump_slack(of_rebuild);
    of_rebuild.close();

    net_names.emplace_back("n414");
    net_names.emplace_back("n415");
    net_names.emplace_back("n241");
    net_names.emplace_back("n412");
    net_names.emplace_back("n416");
    
}   

void createRCTree::buildTopo_remap(std::vector<uint32_t>& original_cut, std::vector<uint32_t>& new_cut,
                             int node_index, std::string gate_name, std::vector<uint8_t> permutation, 
                             std::vector<mockturtle::pin> lib_pins, std::string output_name) 
{
    // std::cout << "\nTiming reports before buildTopo: "<<std::endl;
    // std::optional<float> tns_r  = timer->report_tns();
    // std::optional<float> wns_r  = timer->report_wns();
    // std::optional<float> area_r = timer->report_area();
    // std::cout <<"tns_r "<<*tns_r <<" wns_r "<<*wns_r<<" area_r "<<*area_r<<"\n";


    // Find the net that needs to be processed
    std::vector<std::string> net_to_be_update; //Only the net corresponding to the standardcell output is counted.
    std::vector<std::string> original_element, new_element;
    std::string gate_chosen = "g" + std::to_string(node_index);
    // gate.cellname -> pin_permutaion in the lib
    std::unordered_map<std::string, std::string> cut_to_pin_map;
    for (size_t i = 0; i < permutation.size(); ++i){
        std::string search_key = "x" + std::to_string(new_cut[permutation[i]]);
        auto iter = PlaceInfo_verilog.find(search_key);
        if (iter != PlaceInfo_verilog.end()) {
            cut_to_pin_map["x" + std::to_string(new_cut[permutation[i]])] = lib_pins[i].name;
        } else {
            cut_to_pin_map["g" + std::to_string(new_cut[permutation[i]])] = lib_pins[i].name;
        }
    }

    // Concatenate gate strings and add corresponding nets to net_to_be_update
    // Note: Check the gate name is to enter the pin "x." and gate "g."
    sort(original_cut.begin(), original_cut.end());
    sort(new_cut.begin(), new_cut.end());
    for (const auto& n : original_cut) {
        std::string search_key = "x" + std::to_string(n);
        auto iter = PlaceInfo_verilog.find(search_key);
        if (iter != PlaceInfo_verilog.end()) {
            original_element.emplace_back(search_key);
            net_to_be_update.emplace_back(search_key);
        } else {
            original_element.emplace_back("g" + std::to_string(n));
            net_to_be_update.emplace_back(PlaceInfo_gateToNet["g" + std::to_string(n)]);
        }
       
    }

    for (const auto& n : new_cut) {
        std::string search_key = "x" + std::to_string(n);
        auto iter = PlaceInfo_verilog.find(search_key);
        if (iter != PlaceInfo_verilog.end()) {
            new_element.emplace_back(search_key);
            net_to_be_update.emplace_back(search_key);
        } else {
            new_element.emplace_back("g" + std::to_string(n));
            net_to_be_update.emplace_back(PlaceInfo_gateToNet["g" + std::to_string(n)]);
        }
    }

    std::sort(net_to_be_update.begin(), net_to_be_update.end());
    auto net_end = std::unique(net_to_be_update.begin(), net_to_be_update.end());
    net_to_be_update.erase(net_end, net_to_be_update.end());
    net_to_be_update.erase(std::remove(net_to_be_update.begin(), net_to_be_update.end(), ""), net_to_be_update.end());

    // Remove and reinsert the chosen gate
    timer->remove_gate(gate_chosen);
    timer->insert_gate(gate_chosen, gate_name); 
    {
        std::string gate_chosen_net = PlaceInfo_gateToNet[gate_chosen];
        timer->remove_net(gate_chosen_net);
        net_names.emplace_back(gate_chosen_net);
        timer->insert_net(gate_chosen_net);
        auto& net = PlaceInfo_net[gate_chosen_net];
        for (auto& pin : net.pins) {
            if (pin.type != "standardcell"){
                timer->connect_pin(pin.name, gate_chosen_net);
            }else{
                if (pin.cellname != gate_chosen){
                    // Assuming that the output pin name of gate remains the same after the replacement
                    timer->connect_pin(pin.cellname + ":" + pin.name, gate_chosen_net); 
                } else {
                    timer->connect_pin(gate_chosen + ":" + output_name, gate_chosen_net);
                }
                
            }
        }

        auto netIter = PlaceInfo_net.find(gate_chosen_net);
        if (netIter != PlaceInfo_net.end()) {
            for (auto& pin : netIter->second.pins) {
                if (pin.cellname == gate_chosen) {
                    pin.name = output_name;
                    break; 
                }
            }
        }
    }
    
    std::cout << std::endl;
    for (const auto& net_name: net_to_be_update){
        std::cout << "net_name:"<<net_name <<std::endl;
        auto net = PlaceInfo_net[net_name];
        std::string net_to_gate = ""; //n244->g244

        // Adding flags to determine the net attribute.
        bool contains_orig_only = false;
        bool contains_new_only = false;
        bool has_common_gates = false;

        bool found_ZN_Z_pin = false;
        for (const auto& pin : net.pins) {
            // std::cout<<"pin.name:"<<pin.name<<std::endl;
            // std::cout << "pin.type:"<<pin.type<<std::endl;
            if (pin.name != "ZN" && pin.name != "Z") {
                continue;
            }
               
            found_ZN_Z_pin = true;
            auto it_orig = original_element.end();
            auto it_new = new_element.end(); 
            
            // if (pin.type == "standardcell"){
            net_to_gate = pin.cellname;
            it_orig = std::find(original_element.begin(), original_element.end(), pin.cellname);
            it_new = std::find(new_element.begin(), new_element.end(), pin.cellname);

            if (it_orig != original_element.end() && it_new != new_element.end()) {
                has_common_gates = true;
            } else if (it_orig != original_element.end() && it_new == new_element.end()) {
                contains_orig_only = true;
            } else if (it_orig == original_element.end() && it_new != new_element.end()) {
                contains_new_only = true;
            } else {
                std::cout << "Error occurs!" << std::endl;
                break;
            }
            
        }

        if (!found_ZN_Z_pin){
            for (const auto& pin : net.pins) {
                if (pin.type != "IOpin") {
                    continue;
                }

                auto it_orig = original_element.end();
                auto it_new = new_element.end(); 

                net_to_gate = pin.name;
                it_orig = std::find(original_element.begin(), original_element.end(), pin.name);
                it_new = std::find(new_element.begin(), new_element.end(), pin.name);

                if (it_orig != original_element.end() && it_new != new_element.end()) {
                    has_common_gates = true;
                } else if (it_orig != original_element.end() && it_new == new_element.end()) {
                    contains_orig_only = true;
                } else if (it_orig == original_element.end() && it_new != new_element.end()) {
                    contains_new_only = true;
                } else {
                    std::cout << "Error occurs!" << std::endl;
                    break;
                }

            }

        }

        if (!has_common_gates && !contains_orig_only && !contains_new_only){
            continue;
        }

        auto modify_PinInfo_net = [&](const std::string& net_name, const std::string& gate_chosen, const std::string& pin_permu) {
            auto netIter = PlaceInfo_net.find(net_name);
            if (netIter != PlaceInfo_net.end()) {
                for (auto pinIter = netIter->second.pins.begin(); pinIter != netIter->second.pins.end(); ++pinIter) {
                    if (pinIter->cellname == gate_chosen) {
                        if (!pin_permu.empty()) {
                            pinIter->name = pin_permu;
                        } else {
                            netIter->second.pins.erase(pinIter);
                        }
                        break; 
                    }
                }
            }
        };


        timer->remove_net(net_name);
        net_names.emplace_back(net_name);
        timer->insert_net(net_name);

        for (auto& pin : net.pins) {
            if (pin.type != "standardcell"){
                timer->connect_pin(pin.name, net_name);
            } else {
                if (pin.cellname == gate_chosen){
                    if (contains_orig_only){
                        modify_PinInfo_net(net_name, gate_chosen, "");
                        std::cout << "Remove pin!"<<" net_name:"<<net_name<<" gate_chosen:"<<gate_chosen<<std::endl;
                        continue;
                    }else {  //has_common_gates 
                        std::string pin_permu = cut_to_pin_map[net_to_gate];
                        timer->connect_pin(gate_chosen +":" + pin_permu, net_name);

                        modify_PinInfo_net(net_name, gate_chosen, pin_permu);
                        std::cout << "Change pin!"<<" net_name:"<<net_name<<" gate_chosen:"<<gate_chosen<<std::endl;
                        continue;
                    }
                    
                } else{
                    timer->connect_pin(pin.cellname + ":" + pin.name, net_name);
                }
            }
        }

        if (contains_new_only){
            std::string pin_permu = cut_to_pin_map[net_to_gate];
            timer->connect_pin(gate_chosen +":" + pin_permu, net_name);

            PlaceInfo::Pin newPin;
            newPin.name = pin_permu;
            newPin.type = "standardcell";
            newPin.macroname = gate_name;
            newPin.cellname = gate_chosen;
            newPin.x = PlaceInfoMap[gate_chosen].x;
            newPin.y = PlaceInfoMap[gate_chosen].y;

            auto netIter = PlaceInfo_net.find(net_name);
            if (netIter != PlaceInfo_net.end()) {
                netIter->second.pins.push_back(newPin);
                std::cout << "Add new pin!"<<" net_name:"<<net_name<<" gate_chosen:"<<gate_chosen<<std::endl;
            }
            
        }

    }

    timer->update_timing();
    // std::cout << "Finish buildTopo! "<<std::endl;
    
}   


