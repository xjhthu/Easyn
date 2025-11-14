#include "new_dpsolver.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fmt/core.h>
#include <iostream>
#include <mockturtle/utils/tech_library.hpp>
#include "ot/headerdef.hpp"
#include "timing.hpp"
#include "util.hpp"
#include "omp.h"
#include "PlaceInfo.hpp"
#include<set>

using namespace NewDPSolver;
using namespace std;
using namespace Util;
using namespace PlaceInfo;

class treapnode
{
public:

    treapnode* ls;
    treapnode* rs;
    treapnode* father;
    int original_y,final_y;
    int rand_val,size,id;
    double weight_e,original_x,width;
    double sum_e,sum_ex,sum_w,time_e_w;
    double sum_ex2,sum_ex_w,sum_e_w2;
    void init()
    {
        ls=rs=father=nullptr;
        final_y=0;
        rand_val=rand()*rand()+rand(),size=1;
    }
    void pushup()
    {
        father=nullptr;
        size=1;
        sum_e=weight_e;
        sum_ex=weight_e*original_x;
        sum_w=width;
        time_e_w=0;
        sum_ex2=weight_e*original_x*original_x,sum_ex_w=0,sum_e_w2=0;
        if(ls!=nullptr)
        {
            ls->father=this;
            size+=ls->size;
            sum_e+=ls->sum_e;
            sum_ex+=ls->sum_ex;
            sum_w+=ls->sum_w;
            time_e_w=ls->time_e_w+weight_e*ls->sum_w;
            sum_ex2+=ls->sum_ex2;
            sum_ex_w=ls->sum_ex_w + weight_e*original_x*ls->sum_w;
            sum_e_w2=ls->sum_e_w2 + weight_e*ls->sum_w*ls->sum_w;
        }
        if(rs!=nullptr)
        {
            rs->father=this;
            size+=rs->size;
            time_e_w+=rs->time_e_w+rs->sum_e*sum_w;
            sum_ex2+=rs->sum_ex2;
            sum_ex_w+=rs->sum_ex_w + rs->sum_ex * sum_w;
            sum_e_w2+=rs->sum_e_w2 + rs->sum_e * sum_w*sum_w + 2 * sum_w * rs->time_e_w;
            sum_e+=rs->sum_e;
            sum_ex+=rs->sum_ex;
            sum_w+=rs->sum_w;
        }
    }
    double calculate_x1()
    {
        // cout<<"calc "<<sum_ex<<' '<<time_e_w<<' '<<sum_e<<endl;
        return (sum_ex-time_e_w)/sum_e;
    }
    double calculate_displace()
    {
        int x1=calculate_x1();
        return sum_ex2-2*sum_ex*x1-2*sum_ex_w+2*x1*time_e_w+x1*x1*sum_e+sum_e_w2;
    }
    bool operator<(const treapnode &b)const
    {
        if(original_x==b.original_x&&width==b.width)return id<b.id;
        return original_x==b.original_x?(width<b.width):(original_x<b.original_x);
    }
    bool operator<=(const treapnode &b)const
    {
        if(original_x==b.original_x&&width==b.width)return id<=b.id;
        return original_x==b.original_x?(width<=b.width):(original_x<=b.original_x);
    }
    treapnode* getroot()
    {
        treapnode* x=this;
        while(x->father!=nullptr)
        {
            // cout<<x<<' '<<x->father<<endl;
            x=x->father;
        }
        return x;
    }
void debug(vector<treapnode*> &node_list)
    {
        if(ls!=nullptr)ls->debug(node_list);
        node_list.push_back(this);
        if(rs!=nullptr)rs->debug(node_list);
    }
};

class treaps
{
public:
    struct cell_id{
        int id;
        treapnode* node;
        cell_id(int _id,treapnode* _node)
        {
            id=_id,node=_node;
        }
        bool operator<(const cell_id &b)const
        {
            if(node->original_x==b.node->original_x&&node->width==b.node->width)return id<b.id;
            return node->original_x==b.node->original_x?(node->width<b.node->width):(node->original_x<b.node->original_x);
        }
        bool operator<=(const cell_id &b)const
        {
            if(node->original_x==b.node->original_x&&node->width==b.node->width)return id<=b.id;
            return node->original_x==b.node->original_x?(node->width<=b.node->width):(node->original_x<=b.node->original_x);
        }
    };
    std::set<cell_id> nodes;
    void init(int n)
    {
        
    }
    
    static int getrank(treapnode* x,treapnode* target)
    {
        if(x==nullptr)
        {
            return 0;
        }
        // cout<<"getrank "<<x->id<<endl;
        int ans=0;
        if(*x<=*target)
        {
            if(x->ls!=nullptr)ans+=x->ls->size;
            ans+=1;
            ans+=getrank(x->rs,target);
        }
        else
        {
            ans+=getrank(x->ls,target);
        }
        return ans;
    }
    static treapnode* merge(treapnode* x,treapnode* y)
    {
        if(x==nullptr||y==nullptr)
        {
            if(x!=nullptr)
            {
                x->pushup();
                return x;
            }
            else if(y!=nullptr)
            {
                y->pushup();
                return y;
            }
            return nullptr;
        }
        // cout<<"merge"<<x->id<<' '<<y->id<<endl;
        if(x->rand_val<y->rand_val)
        {
            x->rs=merge(x->rs,y);
            x->pushup();
            return x;
        }
        else
        {
            // cout<<(x->original_y)<<' '<<(y->original_y)<<endl;
            y->ls=merge(x,y->ls);
            y->pushup();
            return y;
        }
    }
    static void split(treapnode* x,int k,treapnode* &rt1,treapnode* &rt2)
    {
        if(x==nullptr)
        {
            rt1=nullptr;rt2=nullptr;
            return;
        }
        // cout<<x<<' '<<(x->ls != nullptr)<<' '<<(x->rs != nullptr)<<endl;
        if(x->ls==nullptr)
        {
            if(k==0)
            {
                rt1=nullptr,rt2=x;
            }
            else
            {
                rt1=x;
                split(x->rs,k-1,x->rs,rt2);
            }
        }
        else if(k<=x->ls->size)
        {
            rt2=x;
            split(x->ls,k,rt1,x->ls);
        }
        else
        {
            rt1=x;
            split(x->rs,k-(x->ls->size)-1,x->rs,rt2);
        }
        x->pushup();
    }
    double most_lx(treapnode* x)
    {
        while(x->ls != nullptr)
        {
            x=x->ls;
        }
        return x->original_x;
    }
    double most_rx(treapnode* x)
    {
        while(x->rs != nullptr)x=x->rs;
        return x->original_x+x->width;
    }
    treapnode* most_lnode(treapnode* x)
    {
        while(x->ls != nullptr)x=x->ls;
        return x;
    }
    treapnode* most_rnode(treapnode* x)
    {
        while(x->rs != nullptr)x=x->rs;
        return x;
    }
    void do_placeat(treapnode* current_node, int id)
    {
        // int debug=0;
        // if(current_node->final_y==16896)debug=1;
        treaps::cell_id current_cell_id(id,current_node);
        treapnode* root=current_node;
        nodes.insert(current_cell_id);
        int flag=0;
        // if(debug)
        // {
        //     cout<<"insert node "<<current_node->original_x<<' '<<current_node->width<<endl;
        //     for(auto itor:nodes)
        //     {
        //         cout<<itor.id<<' '<<itor.node->original_x<<' '<<itor.node->width<<endl;
        //     }
        // }
        while(1)
        {
            root=current_node->getroot();
            treapnode* tmp_r_node=most_rnode(root);
            treaps::cell_id rnode_id(tmp_r_node->id,tmp_r_node);
            auto itor = nodes.upper_bound(rnode_id);
            if(itor!=nodes.end())
            {
                int left_cut_rank=-1,right_cut_rank=-1;
                treapnode* rightroot= itor->node->getroot();
                int rightx1=rightroot->calculate_x1();
                if(rightx1+rightroot->sum_w>NewDPSolver::max_x_length)rightx1=NewDPSolver::max_x_length-rightroot->sum_w;
                int rootx1=root->calculate_x1();
                if(rootx1<0)rootx1=0;
                if(rightx1<=rootx1+root->sum_w+1)
                {
                    // if(debug)
                    // {
                    //     cout<<"try right merge with "<<itor->id<<' '<<itor->node->original_x<<' '<<itor->node->width<<endl;
                    // }
                    treapnode* rt1=nullptr;
                    treapnode* rt2=nullptr;
                    right_cut_rank=getrank(rightroot,current_node);
                    split(rightroot,right_cut_rank,rt1,rt2);
                    root=merge(rt1,root);
                    root=merge(root,rt2);
                    flag=1;
                    // if(debug)
                    // {
                    //     vector<treapnode*> &node_list=*(new vector<treapnode*>());
                    //     root->debug(node_list);
                    //     for(auto node:node_list)
                    //     {
                    //         cout<<"after right merge "<<node->id<<' '<<node->original_x<<' '<<node->width<<endl;
                    //     }
                    // }
                }
            }
            treapnode* tmp_l_node=most_lnode(root);
            treaps::cell_id lnode_id(0,tmp_l_node);
            itor = nodes.lower_bound(lnode_id);
            if(itor!=nodes.begin())
            {
                itor--;
                int left_cut_rank=-1,right_cut_rank=-1;
                treapnode* leftroot= itor->node->getroot();
                int leftx1=leftroot->calculate_x1();
                if(leftx1<0)leftx1=0;
                int rootx1=root->calculate_x1();
                if(rootx1+root->sum_w>NewDPSolver::max_x_length)rootx1=NewDPSolver::max_x_length-root->sum_w;
                if(leftx1+leftroot->sum_w+1>=rootx1)
                {
                    // if(debug)
                    // {
                    //     cout<<"try left merge with "<<itor->id<<' '<<itor->node->original_x<<' '<<itor->node->width<<endl;
                    // }
                    treapnode* rt1=nullptr;
                    treapnode* rt2=nullptr;
                    left_cut_rank=getrank(leftroot,current_node);
                    split(leftroot,left_cut_rank,rt1,rt2);
                    root=merge(rt1,root);
                    root=merge(root,rt2);
                    flag=1;
                    // if(debug)
                    // {
                    //     vector<treapnode*> &node_list=*(new vector<treapnode*>());
                    //     root->debug(node_list);
                    //     for(auto node:node_list)
                    //     {
                    //         cout<<"after left merge "<<node->id<<' '<<node->original_x<<' '<<node->width<<endl;
                    //     }
                    // }
                }
            }
            if(!flag)break;
            flag=0;
        }
        // if(debug)
        // {
        //     int nowx1=root->calculate_x1();
        //     if(nowx1<0)nowx1=0;
        //     if(nowx1+root->sum_w>NewDPSolver::max_x_length)nowx1=NewDPSolver::max_x_length-root->sum_w;
        //     cout<<"now x1 "<<nowx1<<' '<<(root->sum_w)<<' '<<NewDPSolver::max_x_length<<endl;
        // }
    }
    double try_placeat(treapnode* current_node, int id)
    {
        treaps::cell_id current_cell_id(id,current_node);
        double displacement_increase1=0,displacement_increase2=0;
        treapnode* root=current_node;
        // cout<<"enter place "<<current_node->original_x<<' '<<current_node->original_y<<' '<<current_node->width<<' '<<current_node->weight_e<<endl;
        // for(auto itor:nodes)
        // {
        //     cout<<itor.node->original_x<<' '<<itor.node->original_y<<' '<<itor.node->width<<endl;
        // }
        auto itor = nodes.lower_bound(current_cell_id);
        if(itor!=nodes.end())
        {
            int left_cut_rank=-1,right_cut_rank=-1;
            treapnode* rightroot= itor->node->getroot();
            // cout<<(rightroot->original_x)<<' '<<(rightroot->original_y)<<' '<<(rightroot->size)<<' '<<(rightroot->sum_e)<<endl;
            if(rightroot->calculate_x1()<=current_node->original_x+current_node->width)
            {
                treapnode* rt1=nullptr;
                treapnode* rt2=nullptr;
                displacement_increase1-=rightroot->calculate_displace();
                right_cut_rank=getrank(rightroot,current_node);
                split(rightroot,right_cut_rank,rt1,rt2);

                // cout<<(rt1->ls)<<' '<<(rt1->rs)<<' '<<(current_node->ls)<<' '<<(current_node->rs)<<endl;
                root=merge(rt1,current_node);

                // if(root->ls)cout<<"p1"<<' '<<root->size<<' '<<root->id<<' '<<root->ls->id<<endl;
                // cout<<(root->size)<<' '<<(root->original_y)<<' '<<(root->sum_e)<<endl;
                // if(root->ls)cout<<root->ls->original_y<<endl;
                root=merge(root,rt2);

                // if(root->ls)cout<<"p1"<<' '<<root->size<<' '<<root->id<<' '<<root->ls->id<<endl;
                // cout<<(root->size)<<' '<<(root->original_y)<<' '<<(root->sum_e)<<endl;
                displacement_increase1+=root->calculate_displace();
            }
            // cout<<"p1"<<endl;
            if(itor!=nodes.begin())
            {
                itor--;
                treapnode* leftroot= itor->node->getroot();
                // cout<<leftroot->original_x<<endl;
                // cout<<most_rx(leftroot)<<endl;
                // cout<<most_lx(root)<<endl;
                if(leftroot!=root&&(leftroot->calculate_x1() + leftroot->sum_w)>=root->calculate_x1())
                {
                    left_cut_rank=leftroot->size;
                    displacement_increase1-=leftroot->calculate_displace()+root->calculate_displace();
                    root=merge(leftroot,root);
                    displacement_increase1+=root->calculate_displace();
                    split(root,left_cut_rank,leftroot,root);
                }
            }
            displacement_increase1/=(root->size);
            // cout<<"p2"<<' '<<root->size<<' '<<right_cut_rank<<endl;
            if(right_cut_rank!=-1)
            {
                treapnode* rt1=nullptr;
                treapnode* rt2=nullptr;
                // cout<<"p4"<<endl;
                split(root,right_cut_rank,rt1,root);
                // cout<<"p5"<<endl;
                split(root,1,root,rt2);
                // cout<<"p6"<<endl;
                merge(rt1,rt2);
            }
        }
        // cout<<"p3"<<endl;
        itor = nodes.lower_bound(current_cell_id);
        if(itor!=nodes.begin())
        {
            int left_cut_rank=-1,right_cut_rank=-1;
            itor--;
            treapnode* leftroot= itor->node->getroot();
            if(leftroot->calculate_x1()+leftroot->sum_w>=current_node->original_x)
            {
                treapnode* rt1=nullptr;
                treapnode* rt2=nullptr;
                displacement_increase2-=leftroot->calculate_displace();
                left_cut_rank=getrank(leftroot,current_node);
                // cout<<leftroot->size<<' '<<left_cut_rank<<endl;
                split(leftroot,left_cut_rank,rt1,rt2);
                // cout<<"rt1 "<<rt1<<' '<<rt2<<' '<<current_node<<endl;
                root=merge(rt1,current_node);
                root=merge(root,rt2);
                displacement_increase1+=root->calculate_displace();
            }
            // cout<<"p4"<<endl;
            itor++;
            if(itor!=nodes.end())
            {
                treapnode* rightroot= itor->node->getroot();
                if(rightroot!=root&&rightroot->calculate_x1()<=root->calculate_x1()+root->sum_w)
                {
                    right_cut_rank=root->size;
                    displacement_increase2-=rightroot->calculate_displace()+root->calculate_displace();
                    root=merge(root,rightroot);
                    displacement_increase2+=root->calculate_displace();
                    split(root,right_cut_rank,root,rightroot);
                }
            }
            displacement_increase2/=(root->size);
            // cout<<"p5"<<endl;
            if(left_cut_rank!=-1)
            {
                treapnode* rt1=nullptr;
                treapnode* rt2=nullptr;
                split(root,left_cut_rank,rt1,root);
                split(root,1,root,rt2);
                merge(rt1,rt2);
            }
        }
        return displacement_increase1>displacement_increase2?displacement_increase1:displacement_increase2;
    }
};

class New_Legalizer
{
public:
    vector<treapnode>treapnode_vector;
    vector<string>row_height;
};

namespace NewDPSolver
{
    DPSolver::HashTable<DPSolver::HashTable<New_DP_Gate_Sol>> dp_table;
    HashTable<int> tend_to_12T;
    HashTable<string> tend_to_12T_name;
    double area_slack;
    double max_delay_iteration;
    HashTable<mockturtle::gate> name_to_gate;
    HashTable<treapnode*>treapnode_by_n;
    std::unordered_map<int , treaps*> Placer;
    std::unordered_map<int , double> row_density_map;
    double max_x_length;
    HashTable<HashTable<double>> dp_back_rtat;
    HashTable<int> dp_iter_map;
    HashTable<std::vector<int>> dp_fanouts;
    PlaceInfo::HashTable<PlaceInfo::Gate> initialPlaceInfoMap;
    PlaceInfo::HashTable<PlaceInfo::Macro> initialPlaceInfo_macro;
    HashTable<vector<uint32_t>> node_cut_map;
    HashTable<gate> node_to_gate_map;
    HashTable<string> repower_gate_name;
    HashTable<float> node_to_slack_map;
    HashTable<vector<uint32_t>> node_parent_map;
    std::vector<std::pair<string, float>> sorted_node_to_slack_map;
    HashTable<float> output_pin_delay_map;
    float positive_slack_threshold = 0.0f;
    float negative_slack_threshold = 0.0f;
    double displacement_threshold=10000000;
    bool check_input_gate(binding_view<klut_network> *res, int32_t n)
    {
        bool input_gate_flag = 1;
        res->foreach_fanin(n, [&](auto fanin)
                           {
            // std::cout<<fanin<<" is ci "<<res->is_ci(fanin)<<" is constant "<<res->is_constant(fanin)<<" result "<<(!res->is_ci(fanin) && !res->is_constant(fanin))<<"\n";
            if (!res->is_ci(fanin) && !res->is_constant(fanin)){
                // std::cout<<"am I here "<<n<<"\n";
                input_gate_flag = 0;
            } });
        return input_gate_flag;
    }

    void calculate_prune_threshold_in_dp_table_init()
    {
        float positive_slack_ratio = 0.5; // the ratio of maintaining the original gate
        int positive_slack_cnt = 0;
        for (auto pair : sorted_node_to_slack_map)
        {
            // std::cout<<pair.first<<" "<<pair.second<<"\n";
            if (pair.second > 0)
                positive_slack_cnt += 1;
        }
        // std::cout<<"positive slack ratio: "<<(positive_slack_cnt*1.0f/sorted_node_to_slack_map.size())<<"\n";
        if (positive_slack_cnt * 1.0f / sorted_node_to_slack_map.size() > positive_slack_ratio)
        {
            int index = (sorted_node_to_slack_map.size() * positive_slack_ratio);
            positive_slack_threshold = sorted_node_to_slack_map[index].second;
        }
        else
        {
            positive_slack_threshold = 0.0f;
        }

        negative_slack_threshold = sorted_node_to_slack_map[floor(0.9 * sorted_node_to_slack_map.size())].second;
        // std::cout<<(int)0.8*sorted_node_to_slack_map.size()<<" "<<floor(sorted_node_to_slack_map.size()*0.8)<<"\n";
        std::cout << "positive_slack_threshold " << positive_slack_threshold << "\n";
        std::cout << "negative_slack_threshold " << negative_slack_threshold << "\n";
    }

    bool check_gate_height_match(std::string assigned_height, std::string gate_name)
    {
        if (assigned_height == "8T")
        {
            if (Util::find_substring_from_string("8T", gate_name))
            {
                return true;
            }
        }
        else if (assigned_height == "12T")
        {
            if (!Util::find_substring_from_string("8T", gate_name))
            {
                return true;
            }
        }
        return false;
    }
    bool check_prune_in_dp_table_init(string n, string assigned_gate_height, string candidate_gate_name)
    {

        if (check_gate_height_match(assigned_gate_height, node_to_gate_map[n].name))
        {
            if (node_to_gate_map[n].name == candidate_gate_name||(!check_gate_height_match(assigned_gate_height, candidate_gate_name)))
            {
                return false;
            }
            return true;
        }
        return false;
    }

    

    void New_init_dp_table(binding_view<klut_network> *res, const std::vector<gate> &lib_gates, std::vector<gate> &gates, binding_cut_type &cuts, HashTable<std::string> &gateHeightAssignmentMap, ot::Timer &timer, bool remap_flag, PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap, HashTable<PlaceInfo::Macro>& PlaceInfo_macro)
    {
        tech_library tech_lib(gates);
        // construct node_to_slack_map
        // timer.update_all();
        topo_view<binding_view<klut_network>>(*res).foreach_node([&](auto n)
                                                                 {
            if (!res->is_ci(n) && !res->is_constant(n)){
                auto original_gate = lib_gates[res->get_binding_index( n )];
                float slack = *timer.my_report_slack("g"+to_string(n)+":"+original_gate.output_name, ot::MAX,ot::FALL);
                // std::cout<<"slack of "<<n<<"="<<slack<<"\n";
                node_to_slack_map[std::to_string(n)] = slack;
            } });

        for (auto &i : node_to_slack_map)
            sorted_node_to_slack_map.push_back(i);

        std::sort(sorted_node_to_slack_map.begin(), sorted_node_to_slack_map.end(),
                  [=](std::pair<string, float> &a, std::pair<string, float> &b)
                  { return a.second > b.second; });
        int positive_slack_cnt = 0;
        for (auto pair : sorted_node_to_slack_map)
        {
            if (pair.second > 0)
                positive_slack_cnt += 1;
        }

        calculate_prune_threshold_in_dp_table_init();

        topo_view<binding_view<klut_network>>(*res).foreach_node([&](auto n)
                                                                 {
            DPSolver::HashTable<New_DP_Gate_Sol> gate_sol_map;
            dp_table[std::to_string(n)] = gate_sol_map;
            
            if (res->is_ci(n) || res->is_constant(n)){
                // std::cout<<"ci or constant "<<n<<"\n";
                New_DP_Gate_Sol gate_sol;
                gate_sol.name = std::to_string(n);
                gate_sol.cell_type = "pi";
                gate_sol.area = 0;
                gate_sol.delay_list = 0;
                gate_sol.vote = 0;
                gate_sol.is_calculated = true;
                gate_sol.is_pruned = false;
                
                dp_table[std::to_string(n)][std::to_string(n)] = gate_sol;
            }
            else{
               
            
                // auto tt = res->node_function(n);
                // kitty::static_truth_table<4> fe;
                // if (tt.num_vars() < 4)
                //     fe = kitty::extend_to<4>( tt );
                // else
                //     fe = kitty::shrink_to<4>( tt );  // control on four inputs
                // auto supergates_pos = tech_lib.get_supergates( fe );
                int cnt8T=0,cnt12T=0;
                auto index = res->node_to_index( n );
                
                auto original_gate = lib_gates[res->get_binding_index( n )];
                
                vector<uint32_t> original_gate_cut_list;
               
                res->foreach_fanin(n,[&](auto fanin){
                    original_gate_cut_list.push_back(fanin);
                });
                sort(original_gate_cut_list.begin(),original_gate_cut_list.end());
                node_cut_map[to_string(n)] = original_gate_cut_list;

                node_to_gate_map[to_string(n)] = original_gate;
                // repower_gate_name[to_string(n)] = original_gate.name;

                float current_cut_distance = 0;
                float current_node_x = 0;
                float current_node_y = 0;
                    current_node_x = PlaceInfoMap["g"+to_string(n)].x;
                    current_node_y = PlaceInfoMap["g"+to_string(n)].y;
                    for (auto l : original_gate_cut_list){
                        int x,y;
                        if (res->is_ci(l)){
                            x = 0;
                            y = 0;
                        }
                        else{
                            x = PlaceInfoMap["g"+to_string(l)].x;
                            y = PlaceInfoMap["g"+to_string(l)].y;
                        }
                        current_cut_distance += Util::return_absolute_value(x-current_node_x);
                        current_cut_distance += Util::return_absolute_value(y-current_node_y);
                    }
                    current_cut_distance /= original_gate_cut_list.size();
              
                auto & node_cuts = (cuts).cuts( index );
                char current_gate_name[100]; //tmp variable for permutation repeat
                auto gate_height = gateHeightAssignmentMap[to_string(n)];
               
                for ( auto& cut : node_cuts ){
                    {
                        vector<uint32_t> now_gate_cut_list;
                        for (int l : *cut)
                        {
                            now_gate_cut_list.push_back(l);
                        }
                        if(now_gate_cut_list.size()!=original_gate_cut_list.size())continue;
                        sort(now_gate_cut_list.begin(),now_gate_cut_list.end());
                        int flag=0;
                        for(int i=0;i<now_gate_cut_list.size();i++)
                        {
                            if(now_gate_cut_list[i]!=original_gate_cut_list[i])
                            {  
                                flag=1;
                                break;
                            }
                        }
                        if(flag)continue;
                    }
                    

                    strcpy(current_gate_name, "none");
                    
                    if ((cut->size()&&*cut->begin() == index) ==1 || cut->size()>4){
                        continue;
                    }

                    if (current_cut_distance >0){
                            float new_cut_distance = 0;
                            for (int l : *cut){
                                int x,y;
                                if (res->is_ci(l)){
                                    x = 0;
                                    y = 0;
                                }
                                else{
                                    x = PlaceInfoMap["g"+to_string(l)].x;
                                    y = PlaceInfoMap["g"+to_string(l)].y;
                                }
                                // auto x = PlaceInfoMap["g"+to_string(l)].x;
                                // auto y = PlaceInfoMap["g"+to_string(l)].y;
                                
                                new_cut_distance += Util::return_absolute_value(x-current_node_x);
                                new_cut_distance += Util::return_absolute_value(y-current_node_y);
                            }
                         
                            new_cut_distance /= (*cut).size();
                            if (new_cut_distance*2 <= current_cut_distance || new_cut_distance == current_cut_distance){
                                if (new_cut_distance*2 < current_cut_distance){
                                    std::cout<<*cut<<"\n";
                                 std::cout<<"cut size "<<(*cut).size()<<"current_cut_distance "<<current_cut_distance<<" new_cust distance "<<new_cut_distance<<"\n";
                                }
                                
                            }
                            else{
                                continue;
                            }
                           
                        }
                    auto tt = (cuts).truth_table( *cut );
                    auto fe = kitty::shrink_to<4>( tt );
                    auto supergates_pos = tech_lib.get_supergates( fe );
                    
                    if (supergates_pos !=nullptr){
                        for ( auto &gate : *supergates_pos ){

                            if (unsigned(gate.polarity)!=0){ // skip nodes which require negation
                                continue;
                            }  
                            // if(PlaceInfoMap["g"+std::to_string(n)].cell_type=="AND3_X2_12T"||lib_gates[gate.root->id].name=="AND3_X2_12T")
                            // {
                            //     cout<<"cao "<<n<<' '<<PlaceInfoMap["g"+std::to_string(n)].cell_type<<' '<<lib_gates[gate.root->id].name<<' '<<PlaceInfo_macro[PlaceInfoMap["g"+std::to_string(n)].cell_type].area<<' '<<PlaceInfo_macro[lib_gates[gate.root->id].name].area<<endl;
                            // }

                            if (strcmp(lib_gates[gate.root->id].name.c_str(),current_gate_name)==0){//skip same gate with different pin permutation
                                continue;
                            }

                            //filter gate with minor height
                            // if((!check_gate_height_match(gate_height,lib_gates[gate.root->id].name ))){
                            //     continue;
                            // }

                            // Pass the CLK gate
                            if(Util::find_substring_from_string("CLK",lib_gates[gate.root->id].name)){
                                continue;
                            }

                            // Pass the TBUF gate
                            if(Util::find_substring_from_string("TBUF",lib_gates[gate.root->id].name)){
                                continue;
                            }

                            // Pass 16X gate
                            if(Util::find_substring_from_string("_X16_",lib_gates[gate.root->id].name)){
                                continue;
                            }

                            // Pass 12X gate
                            if(Util::find_substring_from_string("_X12_",lib_gates[gate.root->id].name)){
                                continue;
                            }

                            // Pass 8X gate
                            if(Util::find_substring_from_string("_X8_",lib_gates[gate.root->id].name)){
                                continue;
                            }

                            // Pass 4X gate
                            if(Util::find_substring_from_string("_X4_",lib_gates[gate.root->id].name)){
                                continue;
                            }

                            // if(check_prune_in_dp_table_init(to_string(n),gate_height,lib_gates[gate.root->id].name)){
                            //     continue;
                            // }

                            // if((Util::find_substring_from_string("8T", lib_gates[gate.root->id].name) && cnt8T)||(Util::find_substring_from_string("12T", lib_gates[gate.root->id].name) && cnt12T)){
                            //     continue;
                            // }

                            New_DP_Gate_Sol gate_sol;
                            // if (check_input_gate(res, n)){ //for input gates
                            //     auto ctr = 0;
                            //     timer.repower_gate("g"+to_string(n),lib_gates[gate.root->id].name);
                            //     timer.run_all_tasks();
                            //     timer.update_at_pin("g"+to_string(n)+":"+original_gate.output_name, ot::MAX,ot::FALL);
                            //     float tmp_delay = *timer.direct_report_at("g"+to_string(n)+":"+original_gate.output_name, ot::MAX,ot::FALL);
                            //     // std::cout<<"node "<<n<<" gate name "<<lib_gates[gate.root->id].name<<" output name "<<lib_gates[gate.root->id].output_name<<" tmp delay "<<tmp_delay<<"\n";
                            //     gate_sol.name = lib_gates[gate.root->id].name;
                            //     gate_sol.cell_type = "input_gate";
                            //     gate_sol.area = lib_gates[gate.root->id].area; 
                            //     gate_sol.delay_list = tmp_delay;
                            //     gate_sol.cut = cut;
                            //     gate_sol.vote = 0;
                            //     gate_sol.permutation = gate.permutation;
                            //     gate_sol.gate_function = gate.root->function;
                            //     gate_sol.gate_id = gate.root->id;
                            //     gate_sol.pins = lib_gates[gate.root->id].pins;
                            //     gate_sol.output_name = lib_gates[gate.root->id].output_name;
                            //     gate_sol.is_calculated = false;
                            //     gate_sol.is_pruned = false;
                            // }
                            // else
                            {// for normal gates not calculated by dp
                                gate_sol.name = lib_gates[gate.root->id].name;
                                gate_sol.cell_type = "normal_gate";
                                gate_sol.area = PlaceInfo_macro[gate_sol.name].area*10000/2000/2000;
                                gate_sol.delay_list = MAXFLOAT; 
                                gate_sol.cut = cut;
                                gate_sol.vote = 0;
                                gate_sol.permutation = gate.permutation;
                                gate_sol.gate_function = gate.root->function;
                                gate_sol.gate_id = gate.root->id;
                                gate_sol.pins = lib_gates[gate.root->id].pins;
                                gate_sol.output_name = lib_gates[gate.root->id].output_name;
                                gate_sol.is_calculated = false;
                                gate_sol.is_pruned = false;
                            }
                            if(Util::find_substring_from_string("_X1_",lib_gates[gate.root->id].name)&&((Util::find_substring_from_string("8T", lib_gates[gate.root->id].name) && Util::find_substring_from_string("8T", gate_height) && Util::find_substring_from_string("12T", original_gate.name)) ))
                            {
                                PlaceInfoMap["g"+std::to_string(n)].cell_type =lib_gates[gate.root->id].name;
                            }    
                            else if((Util::find_substring_from_string("12T", lib_gates[gate.root->id].name) && Util::find_substring_from_string("12T", gate_height) && Util::find_substring_from_string("8T", original_gate.name)))
                            {
                                if(Util::find_substring_from_string("_X2_",lib_gates[gate.root->id].name))
                                {
                                    PlaceInfoMap["g"+std::to_string(n)].cell_type =lib_gates[gate.root->id].name;
                                }
                                else if(Util::find_substring_from_string("8T", PlaceInfoMap["g"+std::to_string(n)].cell_type))
                                {
                                    PlaceInfoMap["g"+std::to_string(n)].cell_type =lib_gates[gate.root->id].name;
                                }
                            }
                            dp_table[std::to_string(n)][lib_gates[gate.root->id].name] = gate_sol;
                            strcpy(current_gate_name, lib_gates[gate.root->id].name.c_str());
                            if(Util::find_substring_from_string("8T", lib_gates[gate.root->id].name))cnt8T++;
                            else cnt12T++;
                        }
                    }
                }
                    timer.repower_gate("g"+to_string(n),PlaceInfoMap["g"+std::to_string(n)].cell_type);
                    timer.run_all_tasks();
                    // timer.update_at_pin("g"+to_string(n)+":"+original_gate.output_name, ot::MAX,ot::FALL);
            } });
        int total_gate = 0;
        int single_gate = 0;
        topo_view<binding_view<klut_network>>(*res).foreach_node([&](auto n)
                                                                 {
            total_gate+=1;
            if(dp_table[std::to_string(n)].size()==1)
                single_gate +=1;
            if(dp_table[std::to_string(n)].size()>=2){
                // timer.repower_gate("g"+to_string(n), dp_table[std::to_string(n)].begin()->second.name);
                // repower_gate_name[to_string(n)] = dp_table[std::to_string(n)].begin()->second.name;
                // std::cout<<"multiple candidate "<<dp_table[std::to_string(n)].size()<<endl;
            } });
        timer.update_all();
        std::cout << "single " << single_gate << " total " << total_gate << "\n";
    }

    void convert_node_on_timer(int n, string gate_name, const std::vector<gate> &lib_gates, ot::Timer &timer,PlaceInfo::HashTable<PlaceInfo::Net> &PlaceInfo_net, PlaceInfo::HashTable<std::string> &PlaceInfo_gateToNet,
        PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap, PlaceInfo::HashTable<std::string> &PlaceInfo_verilog)
    {
        if(node_to_gate_map[to_string(n)].id==dp_table[to_string(n)][gate_name].gate_id)return;
        vector<uint32_t> cut_list;
        int cut_value = 0;

        for (uint32_t l : *(dp_table[std::to_string(n)][gate_name].cut))
        {
            cut_list.push_back(l);
            cut_value += l;
        }
        if (node_cut_map[to_string(n)] != cut_list)
        {
            // TODO: reconstruct rc tree in opentimer
            std::cout << "n " << n << " look at the pin of gate " << gate_name << " output name " << dp_table[to_string(n)][gate_name].output_name << ":\n";
            for (auto pin : dp_table[to_string(n)][gate_name].pins)
            {
                std::cout << pin.name << " ";
            }

            createRCTree *tree = new createRCTree(PlaceInfo_net, PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_verilog, timer);

            FluteResult result;
            tree->buildTopo_remap(node_cut_map[to_string(n)], cut_list, n, gate_name, dp_table[to_string(n)][gate_name].permutation, dp_table[to_string(n)][gate_name].pins, dp_table[to_string(n)][gate_name].output_name);
            result = tree->runFlute();
            delete tree;

            // update the global hashmap
            node_cut_map[to_string(n)] = cut_list;
            node_to_gate_map[to_string(n)] = lib_gates[dp_table[to_string(n)][gate_name].gate_id];

            // std::cout << "opentimer cut for " << n << ":";
            // for (auto l : node_cut_map[to_string(n)])
            // {
            //     std::cout << l << " ";
            // }
            // std::cout << "\n";
            // std::cout << "dp cut for " << n << ":";
            // for (auto l : cut_list)
            // {
            //     std::cout << l << " ";
            // }
            // std::cout << "\n";
        }
        timer.repower_gate("g" + to_string(n), gate_name);
        // timer.run_all_tasks();
        node_to_gate_map[to_string(n)] = lib_gates[dp_table[to_string(n)][gate_name].gate_id];
        timer.run_all_tasks();
        timer.update_at_pin("g" + to_string(n) + ":" + node_to_gate_map[to_string(n)].output_name, ot::MAX, ot::FALL);
    }
    New_DP_Gate_Sol New_dp_solve_iteration_rec(binding_view<klut_network> *res, uint32_t n, 
        const std::vector<gate> &lib_gates, ot::Timer &timer,
        PlaceInfo::HashTable<PlaceInfo::Net> &PlaceInfo_net, PlaceInfo::HashTable<std::string> &PlaceInfo_gateToNet,
        PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap, PlaceInfo::HashTable<std::string> &PlaceInfo_verilog, std::vector<pair<pair<int,string>,double>>&change_ratios, string stage, double period)
    {
        if(res->is_ci(n) || res->is_constant(n)) return dp_table[std::to_string(n)][std::to_string(n)];
        if(dp_table[to_string(n)][PlaceInfoMap["g"+to_string(n)].cell_type].is_calculated) return dp_table[std::to_string(n)][PlaceInfoMap["g"+to_string(n)].cell_type];
        for(auto gate_sol:dp_table[to_string(n)])
        {
            if(gate_sol.second.is_calculated)continue;
            string gate_name=gate_sol.first;
            for (auto fanin : *(dp_table[std::to_string(n)][gate_name].cut))
            {
                New_dp_solve_iteration_rec(res, fanin, lib_gates, timer, PlaceInfo_net, PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_verilog, change_ratios,stage,period);
            }
        }
        for(auto gate_sol:dp_table[to_string(n)])
        {
            if(gate_sol.second.is_calculated)continue;
            string gate_name=gate_sol.first;
            if(gate_name==PlaceInfoMap["g"+to_string(n)].cell_type)continue;
            convert_node_on_timer(n, gate_name, lib_gates, timer, PlaceInfo_net, PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_verilog);
            dp_table[std::to_string(n)][gate_name].delay_list = *timer.report_at("g"+to_string(n)+":"+node_to_gate_map[to_string(n)].output_name ,ot::MAX,ot::FALL);
            dp_table[std::to_string(n)][gate_name].is_calculated = true;
            std::cout << "gate " << n << " " << gate_name << " is calculated "<< dp_table[std::to_string(n)][gate_name].delay_list << endl;
        }
        string gate_name=PlaceInfoMap["g"+to_string(n)].cell_type;
        convert_node_on_timer(n, gate_name, lib_gates, timer, PlaceInfo_net, PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_verilog);
        dp_table[std::to_string(n)][gate_name].delay_list = *timer.report_at("g"+to_string(n)+":"+node_to_gate_map[to_string(n)].output_name ,ot::MAX,ot::FALL);
        dp_table[std::to_string(n)][gate_name].is_calculated = true;
        double original_area=dp_table[std::to_string(n)][gate_name].area,original_delay=dp_table[std::to_string(n)][gate_name].delay_list;
        float require_at=*timer.report_rat("g"+to_string(n)+":"+node_to_gate_map[to_string(n)].output_name ,ot::MAX,ot::FALL);
        if((stage=="slack"&&original_delay+period<require_at)||(stage=="tight"&&original_delay+period>require_at))
        {
            for(auto gate_sol:dp_table[to_string(n)])
            {
                if(stage=="slack")
                {
                    if(gate_sol.second.area<original_area)
                        change_ratios.push_back(make_pair(make_pair(n,gate_sol.first),(gate_sol.second.delay_list-original_delay)/(original_area-gate_sol.second.area)));
                }
                else
                {
                    if(gate_sol.second.area>original_area)
                        change_ratios.push_back(make_pair(make_pair(n,gate_sol.first),(gate_sol.second.delay_list-original_delay)/(gate_sol.second.area-original_area)));
                }
            }
        }
        std::cout << "gate " << n << " " << gate_name << " is calculated "<< dp_table[std::to_string(n)][gate_name].delay_list<<' '<<original_delay+period<<' '<<require_at << endl;

        return dp_table[std::to_string(n)][gate_name];
    }

    New_DP_Gate_Sol New_dp_solve_iteration_rec_old(binding_view<klut_network> *res, uint32_t n, std::string gate_name, 
                                     const std::vector<gate> &lib_gates, ot::Timer &timer,
                                     PlaceInfo::HashTable<PlaceInfo::Net> &PlaceInfo_net, PlaceInfo::HashTable<std::string> &PlaceInfo_gateToNet,
                                     PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap, PlaceInfo::HashTable<std::string> &PlaceInfo_verilog)
    {
        // If the dp entry is already calculated, return the result
        if (dp_table[std::to_string(n)][gate_name].is_calculated)
        {
            return dp_table[std::to_string(n)][gate_name];
        }

        auto index = res->node_to_index(n);

        char current_gate_name[100];    // tmp variable for permutation repeat
        vector<vector<string>> allVecs; // 2-d vectors, store all permutations of the gates slected for all fanins
        
        DPSolver::HashTable<float> pin2pin_delay_map; // pin2pin_delay_map[l][i]: the delay from gate i in fanin l to the fanout of current node n
        vector<float> area_list;
        vector<float> delay_list;
        vector<uint32_t> max_fanin_list;
        vector<int> max_index_list;
        vector<string> max_fanin_gate_list;
        vector<vector<string>> fanin_perm_index_list;

        convert_node_on_timer(n,gate_name, lib_gates, timer, PlaceInfo_net, PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_verilog);

        
        // node_to_gate_map[to_string(n)] = lib_gates[dp_table[to_string(n)][gate_name].gate_id];
        // if(dp_table[std::to_string(n)].size()>1){
        //     std::cout<<repower_gate_name[std::to_string(n)]<<" "<<gate_name<<"\n";
        //     if(repower_gate_name[std::to_string(n)]!=gate_name){
        //     }
        //     else{
        //     }

        // }

        HashTable<float> faninArrivalTimeMap;
        // auto start = std::chrono::high_resolution_clock::now();
        // float current_gate_arrival_time = *timer.report_at("g"+to_string(n)+":"+node_to_gate_map[to_string(n)].output_name ,ot::MAX,ot::FALL);
        // std::cout<<"at g"+to_string(n)<<' '<<current_gate_arrival_time<<"\n";
        // timer.debug("g"+to_string(n)+":"+node_to_gate_map[to_string(n)].output_name ,ot::MAX,ot::FALL);
        // auto end = std::chrono::high_resolution_clock::now();
        // auto time_span = static_cast<std::chrono::duration<double>>(end - start);   // measure time span between start & end
        // for(auto fanin : *(dp_table[std::to_string(n)][gate_name].cut)){
        //     if (res->is_ci(fanin)||res->is_constant(fanin)){
        //         faninArrivalTimeMap[to_string(fanin)]=0.0;

        //     }
        //     else{
        //         std::string output_pin_name = "g"+to_string(fanin)+":"+node_to_gate_map[to_string(fanin)].output_name;
        //         auto start = std::chrono::high_resolution_clock::now();
        //         faninArrivalTimeMap[to_string(fanin)] = *timer.report_at(output_pin_name,ot::MAX,ot::FALL);
        //         cout<<"fanin g"+to_string(fanin)<<' '<<faninArrivalTimeMap[to_string(fanin)]<<'\n';
        //         auto end = std::chrono::high_resolution_clock::now();
        //         auto time_span = static_cast<std::chrono::duration<double>>(end - start);   // measure time span between start & end
        //     }

        // }
        float optimal_delay = -MAXFLOAT;
        for (auto fanin : *(dp_table[std::to_string(n)][gate_name].cut))
        {
            vector<string> one_node_permutation;
            // pin2pin_delay_map[to_string(fanin)] = current_gate_arrival_time - faninArrivalTimeMap[to_string(fanin)];
            New_DP_Gate_Sol dp_entry;
            float current_min_delay = MAXFLOAT;
            for (auto gate_sol : dp_table[to_string(fanin)])
            {
                if (gate_sol.second.is_pruned)
                    continue;
                dp_entry=New_dp_solve_iteration_rec_old(res, fanin, gate_sol.first, lib_gates, timer, PlaceInfo_net, PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_verilog);
                // timer.run_all_tasks();
                // timer.update_at_pin("g" + to_string(n) + ":" + node_to_gate_map[to_string(n)].output_name, ot::MAX, ot::FALL);
                
                // {
                //     float current_delay=MAXFLOAT;
                //     current_delay=*timer.report_at("g"+to_string(n)+":"+node_to_gate_map[to_string(n)].output_name ,ot::MAX,ot::FALL);
                //     // if(res->is_ci(fanin) || res->is_constant(fanin))
                //     //     current_delay = dp_entry.delay_list + timer.report_delay_between("g" + to_string(n) + ":" + node_to_gate_map[to_string(n)].output_name, "x" + to_string(fanin) , ot::MAX, ot::FALL);
                //     // else if(gate_sol.first==PlaceInfoMap["g"+std::to_string(fanin)].cell_type)
                //     //     current_delay = dp_entry.delay_list + timer.report_delay_between("g" + to_string(n) + ":" + node_to_gate_map[to_string(n)].output_name, "g" + to_string(fanin) + ":" + node_to_gate_map[to_string(fanin)].output_name, ot::MAX, ot::FALL);
                //     current_min_delay=current_min_delay>current_delay?current_delay:current_min_delay;
                //     // cout<<n<<' '<<fanin<<' '<<dp_entry.delay_list<<' '<<current_delay<<endl;
                // }
            }
            // optimal_delay=optimal_delay>current_min_delay?optimal_delay:current_min_delay;
        }

        for (auto fanin : *(dp_table[std::to_string(n)][gate_name].cut))
        {
            if(!(res->is_ci(fanin) || res->is_constant(fanin)))
            convert_node_on_timer(fanin,PlaceInfoMap["g"+std::to_string(fanin)].cell_type, lib_gates, timer, PlaceInfo_net, PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_verilog);
        }

        // convert_node_on_timer(n,gate_name, lib_gates, timer, PlaceInfo_net, PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_verilog);
        dp_table[std::to_string(n)][gate_name].delay_list = *timer.report_at("g"+to_string(n)+":"+node_to_gate_map[to_string(n)].output_name ,ot::MAX,ot::FALL);
        dp_table[std::to_string(n)][gate_name].is_calculated = true;

        std::cout << "gate " << n << " " << gate_name << " is calculated "<< dp_table[std::to_string(n)][gate_name].delay_list << endl;

        return dp_table[std::to_string(n)][gate_name];
    }

    New_DP_Gate_Sol New_dp_solve_rec(binding_view<klut_network> *res, uint32_t n, std::string gate_name, mockturtle::tech_library<4, mockturtle::classification_type::np_configurations> tech_lib,
                                     binding_cut_type &cuts, const std::vector<gate> &lib_gates, ot::Timer &timer,
                                     PlaceInfo::HashTable<PlaceInfo::Net> &PlaceInfo_net, PlaceInfo::HashTable<std::string> &PlaceInfo_gateToNet,
                                     PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap, PlaceInfo::HashTable<std::string> &PlaceInfo_verilog)
    {
        // If the dp entry is already calculated, return the result
        if (dp_table[std::to_string(n)][gate_name].is_calculated)
        {
            return dp_table[std::to_string(n)][gate_name];
        }

        auto index = res->node_to_index(n);

        auto &node_cuts = (cuts).cuts(index);
        char current_gate_name[100];    // tmp variable for permutation repeat
        vector<vector<string>> allVecs; // 2-d vectors, store all permutations of the gates slected for all fanins
        vector<uint32_t> cut_list;
        DPSolver::HashTable<float> pin2pin_delay_map; // pin2pin_delay_map[l][i]: the delay from gate i in fanin l to the fanout of current node n
        vector<float> area_list;
        vector<float> delay_list;
        vector<uint32_t> max_fanin_list;
        vector<int> max_index_list;
        vector<string> max_fanin_gate_list;
        vector<vector<string>> fanin_perm_index_list;
        int cut_value = 0;

        for (uint32_t l : *(dp_table[std::to_string(n)][gate_name].cut))
        {
            cut_list.push_back(l);
            cut_value += l;
        }

        if (node_cut_map[to_string(n)] != cut_list)
        {
            // TODO: reconstruct rc tree in opentimer
            std::cout << "n " << n << " look at the pin of gate " << gate_name << " output name " << dp_table[to_string(n)][gate_name].output_name << ":\n";
            for (auto pin : dp_table[to_string(n)][gate_name].pins)
            {
                std::cout << pin.name << " ";
            }

            createRCTree *tree = new createRCTree(PlaceInfo_net, PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_verilog, timer);

            FluteResult result;
            tree->buildTopo_remap(node_cut_map[to_string(n)], cut_list, n, gate_name, dp_table[to_string(n)][gate_name].permutation, dp_table[to_string(n)][gate_name].pins, dp_table[to_string(n)][gate_name].output_name);
            result = tree->runFlute();
            delete tree;

            // update the global hashmap
            node_cut_map[to_string(n)] = cut_list;
            node_to_gate_map[to_string(n)] = lib_gates[dp_table[to_string(n)][gate_name].gate_id];

            // std::cout << "opentimer cut for " << n << ":";
            // for (auto l : node_cut_map[to_string(n)])
            // {
            //     std::cout << l << " ";
            // }
            // std::cout << "\n";
            // std::cout << "dp cut for " << n << ":";
            // for (auto l : cut_list)
            // {
            //     std::cout << l << " ";
            // }
            // std::cout << "\n";
        }
        timer.repower_gate("g" + to_string(n), gate_name);
        timer.run_all_tasks();
        // node_to_gate_map[to_string(n)] = lib_gates[dp_table[to_string(n)][gate_name].gate_id];
        // if(dp_table[std::to_string(n)].size()>1){
        //     std::cout<<repower_gate_name[std::to_string(n)]<<" "<<gate_name<<"\n";
        //     if(repower_gate_name[std::to_string(n)]!=gate_name){
        //     }
        //     else{
        //     }

        // }

        HashTable<float> faninArrivalTimeMap;
        // auto start = std::chrono::high_resolution_clock::now();
        // float current_gate_arrival_time = *timer.report_at("g"+to_string(n)+":"+node_to_gate_map[to_string(n)].output_name ,ot::MAX,ot::FALL);
        // std::cout<<"at g"+to_string(n)<<' '<<current_gate_arrival_time<<"\n";
        // timer.debug("g"+to_string(n)+":"+node_to_gate_map[to_string(n)].output_name ,ot::MAX,ot::FALL);
        // auto end = std::chrono::high_resolution_clock::now();
        // auto time_span = static_cast<std::chrono::duration<double>>(end - start);   // measure time span between start & end
        // for(auto fanin : *(dp_table[std::to_string(n)][gate_name].cut)){
        //     if (res->is_ci(fanin)||res->is_constant(fanin)){
        //         faninArrivalTimeMap[to_string(fanin)]=0.0;

        //     }
        //     else{
        //         std::string output_pin_name = "g"+to_string(fanin)+":"+node_to_gate_map[to_string(fanin)].output_name;
        //         auto start = std::chrono::high_resolution_clock::now();
        //         faninArrivalTimeMap[to_string(fanin)] = *timer.report_at(output_pin_name,ot::MAX,ot::FALL);
        //         cout<<"fanin g"+to_string(fanin)<<' '<<faninArrivalTimeMap[to_string(fanin)]<<'\n';
        //         auto end = std::chrono::high_resolution_clock::now();
        //         auto time_span = static_cast<std::chrono::duration<double>>(end - start);   // measure time span between start & end
        //     }

        // }
        float optimal_delay = -MAXFLOAT;
        for (auto fanin : *(dp_table[std::to_string(n)][gate_name].cut))
        {
            vector<string> one_node_permutation;
            // pin2pin_delay_map[to_string(fanin)] = current_gate_arrival_time - faninArrivalTimeMap[to_string(fanin)];
            New_DP_Gate_Sol dp_entry;
            float current_min_delay = MAXFLOAT;
            for (auto gate_sol : dp_table[to_string(fanin)])
            {
                if (gate_sol.second.is_pruned)
                    continue;
                dp_entry=New_dp_solve_rec(res, fanin, gate_sol.first, tech_lib, cuts, lib_gates, timer, PlaceInfo_net, PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_verilog);
                timer.run_all_tasks();
                timer.update_at_pin("g" + to_string(n) + ":" + node_to_gate_map[to_string(n)].output_name, ot::MAX, ot::FALL);
                float current_delay;
                if(res->is_ci(fanin) || res->is_constant(fanin))
                    current_delay = dp_entry.delay_list + timer.report_delay_between("g" + to_string(n) + ":" + node_to_gate_map[to_string(n)].output_name, "x" + to_string(fanin) , ot::MAX, ot::FALL);
                else
                    current_delay = dp_entry.delay_list + timer.report_delay_between("g" + to_string(n) + ":" + node_to_gate_map[to_string(n)].output_name, "g" + to_string(fanin) + ":" + node_to_gate_map[to_string(fanin)].output_name, ot::MAX, ot::FALL);
                current_min_delay=current_min_delay>current_delay?current_delay:current_min_delay;
                // cout<<n<<' '<<fanin<<' '<<dp_entry.delay_list<<' '<<current_delay<<endl;
            }
            optimal_delay=optimal_delay>current_min_delay?optimal_delay:current_min_delay;
        }

        dp_table[std::to_string(n)][gate_name].delay_list = optimal_delay;
        dp_table[std::to_string(n)][gate_name].is_calculated = true;

        std::cout << "gate " << n << " " << gate_name << " is calculated "<< optimal_delay << "\n";

        return dp_table[std::to_string(n)][gate_name];
    }

    pair<int,int> calculate_dy(int n,int row,string gate)
    {
        int dy_fanin=0,dy_fanout=0;
        for(int fanout:dp_fanouts[std::to_string(n)])
        {
            if(treapnode_by_n[std::to_string(fanout)])
            {
                dy_fanout+=abs(treapnode_by_n[std::to_string(fanout)]->final_y - row);
                // cout<<"fanout "<<fanout<<' '<<treapnode_by_n[std::to_string(fanout)]->final_y<<endl;
            }
            else 
            {
                dy_fanout+=row;
                // cout<<"fanout "<<fanout<<' '<<0<<endl;
            }
        }
        for (uint32_t l : *(dp_table[to_string(n)][gate].cut))
        {
            if(treapnode_by_n[std::to_string(l)])
            {
                dy_fanin+=abs(treapnode_by_n[std::to_string(l)]->original_y - row);
                // cout<<"fanin "<<l<<' '<<treapnode_by_n[std::to_string(l)]->final_y<<endl;
            }
            else 
            {
                dy_fanin+=row;
                // cout<<"fanin "<<l<<' '<<0<<endl;
            }
        }
        return make_pair(dy_fanin,dy_fanout);
    }

    void New_dp_back_propagation_iteration_rec(binding_view<klut_network> *res, uint32_t n, queue<int> &topo_q, ot::Timer &timer, PlaceInfo::HashTable<PlaceInfo::Net> &PlaceInfo_net, PlaceInfo::HashTable<std::string> &PlaceInfo_gateToNet,
        PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap, PlaceInfo::HashTable<Macro> PlaceInfo_macro, PlaceInfo::HashTable<std::string> &PlaceInfo_verilog, const std::vector<gate> &lib_gates, HashTable<std::string> &gateHeightAssignmentMap, 
        std::vector<int>& reassign_minor_rows,std::vector<int>& reassign_major_rows, json &def_data, string phase, double slack_threshold, double period)
    {
        if(res->is_ci(n) || res->is_constant(n))
        {
            return;
        }
        string gate_name;
        string height;
        string original_gate, orginal_height;
        original_gate=PlaceInfoMap["g"+std::to_string(n)].cell_type;
        if(Util::find_substring_from_string("8T",original_gate))orginal_height="8T";
        else orginal_height="12T";
        pair<string,double>result_8T("",0),result_12T("",0);
        float require_at=*timer.report_rat("g"+to_string(n)+":"+node_to_gate_map[to_string(n)].output_name ,ot::MAX,ot::FALL);
        max_delay_iteration=min(max_delay_iteration,(double)require_at);
        for (auto dp_gate : dp_table[std::to_string(n)])
        {
            gate_name=dp_gate.second.name;
            // float require_at=dp_back_rtat[to_string(n)][gate_name];
            if(phase=="slack")
            {
                if(dp_table[to_string(n)][gate_name].area<dp_table[to_string(n)][original_gate].area&&area_slack<0)
                {
                    double ratio=(dp_table[to_string(n)][gate_name].delay_list-dp_table[to_string(n)][original_gate].delay_list)/(dp_table[to_string(n)][original_gate].area-dp_table[to_string(n)][gate_name].area);
                    if(ratio<=slack_threshold&&require_at>dp_table[to_string(n)][gate_name].delay_list+period)
                    {
                        if(Util::find_substring_from_string("8T",gate_name))
                        {
                            if(result_8T.first==""||result_8T.second<ratio)result_8T=make_pair(gate_name,ratio);
                        }
                        else
                        {
                            if(result_12T.first==""||result_12T.second<ratio)result_12T=make_pair(gate_name,ratio);
                        }
                    }
                }
            }
            else if(dp_table[to_string(n)][original_gate].delay_list+period>require_at)
            {
                if(dp_table[to_string(n)][gate_name].area>dp_table[to_string(n)][original_gate].area&&dp_table[to_string(n)][gate_name].area-dp_table[to_string(n)][original_gate].area<=area_slack)
                {
                    double ratio=(dp_table[to_string(n)][gate_name].delay_list-dp_table[to_string(n)][original_gate].delay_list)/(dp_table[to_string(n)][gate_name].area-dp_table[to_string(n)][original_gate].area);
                    if(ratio<=slack_threshold)
                    {
                        if(Util::find_substring_from_string("8T",gate_name))
                        {
                            if(result_8T.first==""||result_8T.second>ratio)result_8T=make_pair(gate_name,ratio);
                        }
                        else
                        {
                            if(result_12T.first==""||result_12T.second>ratio)result_12T=make_pair(gate_name,ratio);
                        }
                    }
                }
            }
            cout<<n<<' '<<gate_name<<' '<<require_at<<' '<<dp_gate.second.delay_list<<' '<<dp_table[std::to_string(n)][gate_name].area<<endl;
        }
        // cout<<n<<' '<<result_8T[0].second<<' '<<result_12T[0].second<<endl;
        vector<string>test_gate;
        if(phase=="slack")
        {
            if(result_12T.first!=""&&result_8T.first!="")
            {
                if(result_12T.second<result_8T.second)
                {
                    test_gate.push_back(result_12T.first);
                    test_gate.push_back(result_8T.first);
                }
                else
                {
                    test_gate.push_back(result_8T.first);
                    test_gate.push_back(result_12T.first);
                }
            }
            else if(result_8T.first!="")
            {
                test_gate.push_back(result_8T.first);
            }
            else if(result_12T.first!="")
            {
                test_gate.push_back(result_12T.first);
            }
            
        }
        else
        {
            if(result_12T.first!=""&&result_8T.first!="")
            {
                if(result_12T.second<result_8T.second)
                {
                    test_gate.push_back(result_12T.first);
                    test_gate.push_back(result_8T.first);
                }
                else
                {
                    test_gate.push_back(result_8T.first);
                    test_gate.push_back(result_12T.first);
                }
            }
            else if(result_8T.first!="")
            {
                test_gate.push_back(result_8T.first);
            }
            else if(result_12T.first!="")
            {
                test_gate.push_back(result_12T.first);
            }
        }
        gate_name=original_gate;
        for(int i=0;i<test_gate.size();i++)
        {
            treapnode *current_node=treapnode_by_n[std::to_string(n)];
            pair<int,int> original_dy= calculate_dy(n,current_node->original_y,original_gate);
            int flag=-1;
            if(Util::find_substring_from_string("8T",test_gate[i]))
            {
                for(int row:reassign_major_rows)
                {
                    float width=PlaceInfo_macro[test_gate[i]].width*100;
                    if(row_density_map[row]+width > max_x_length)continue;
                    pair<int,int> row_dy= calculate_dy(n,row,test_gate[i]);
                    if(original_dy.first+10000<row_dy.first&&original_dy.second+10000<row_dy.second)
                    {
                        continue;
                    }
                    flag=row;
                    break;
                }
            }
            else
            {
                for(int row:reassign_minor_rows)
                {
                    float width=PlaceInfo_macro[test_gate[i]].width*100;
                    if(row_density_map[row]+width > max_x_length)continue;
                    pair<int,int> row_dy= calculate_dy(n,row,test_gate[i]);
                    if(original_dy.first+4096<row_dy.first&&original_dy.second+4096<row_dy.second)continue;
                    flag=row;
                    break;
                }
            }
            if(flag!=-1)
            {
                gate_name=test_gate[i];
                break;
            }
        }
        area_slack+=dp_table[std::to_string(n)][original_gate].area-dp_table[std::to_string(n)][gate_name].area;
        PlaceInfoMap["g"+std::to_string(n)].cell_type=gate_name;
        def_data["g"+to_string(n)]["macroName"]=gate_name;
        cout<<n<<' '<<PlaceInfoMap["g"+std::to_string(n)].cell_type<<endl;
        if(gate_name!=original_gate)
        {
            cout<<"changed "<<(dp_table[to_string(n)][gate_name].delay_list-dp_table[to_string(n)][original_gate].delay_list)/(dp_table[to_string(n)][original_gate].area-dp_table[to_string(n)][gate_name].area)<<endl;
        }
        treapnode_by_n[std::to_string(n)]->width=PlaceInfo_macro[gate_name].width*100;
        treapnode_by_n[std::to_string(n)]->pushup();
        
        {
            double min_cost=MAXFLOAT;
            int min_row=-1;
            treapnode *current_node=treapnode_by_n[std::to_string(n)];
            pair<int,int> original_dy= calculate_dy(n,current_node->original_y,gate_name);
            vector<int> *reassign_rows;
            if(Util::find_substring_from_string("8T",gate_name))reassign_rows=&reassign_major_rows;
            else reassign_rows=&reassign_minor_rows;
            // cout<<"1"<<endl;
            int original_index=lower_bound(reassign_rows->begin(),reassign_rows->end(),current_node->original_y)-reassign_rows->begin();
            if(original_index==reassign_rows->size())original_index--;
            for(int i=0;;)
            {
                int row=(*reassign_rows)[i+original_index];
                pair<int,int> row_dy= calculate_dy(n,row,gate_name);
                // if(original_dy.first+2048<row_dy.first&&original_dy.second+2048<row_dy.second)continue;
                double cost=((-original_dy.first+row_dy.first)+(-original_dy.second+row_dy.second))*1e3;
                // std::cout<<"cost "<<cost<<endl;
                if(cost>min_cost)break;
                // std::cout<<"width "<<row<<' '<<row_density_map[row]<<' '<<current_node->width<<' '<<max_x_length<<endl;
                if(row_density_map[row]+current_node->width <= max_x_length)
                {
                    // std::cout<<"try "<<i<<' '<<row<<endl;
                    double cost2=Placer[row]->try_placeat(current_node,n);
                    // std::cout<<cost2<<endl;
                    // cost+=cost2;
                    if(cost<=min_cost)
                    {
                        min_cost=cost;
                        min_row=row;
                    }
                }
                int next1=0,next2=0;
                if(i>=0)next1=-i-1;
                else next1=-i;
                if(original_index+next1<0||original_index+next1>=reassign_rows->size())
                {
                    if(next1>=0)next2=-next1-1;
                    else next2=-next1;
                    if(original_index+next2<0||original_index+next2>=reassign_rows->size())break;
                    i=next2;
                }
                else
                {
                    i=next1;
                }
                // cout<<"cost "<<n<<" "<<gate_name<<" "<<row<<" "<<current_node->original_y<<" "<<cost<<' '<<cost2<<endl;
            }
            current_node->final_y=min_row;
            // std::cout<<"min_row "<<min_row<<endl;
            if(min_row==-1)cout<<"error not enough space "<<n<<' '<<PlaceInfoMap["g"+std::to_string(n)].cell_type<<endl;
            Placer[min_row]->do_placeat(current_node,n);
            row_density_map[min_row]+=current_node->width;
        }
        // if(current_node->final_y==512)cout<<"fuck "<<n<<' '<<gate_name
        // if (gate_name!=original_gate)
        {
            auto ctr = 0u;
            auto num_vars = dp_table[std::to_string(n)][gate_name].cut->size();
            // std::cout<<"num_vars "<<num_vars<<" cut "<<*(dp_final_solution[to_string(n)][0].cut)<<"\n";
            std::vector<signal_type> children( num_vars );
            for ( auto l : *(dp_table[std::to_string(n)][gate_name].cut) ){
                if ( ctr >= num_vars )
                    break;
                children[dp_table[std::to_string(n)][gate_name].permutation[ctr]] = l;
                ctr++;   
            }
            auto f = res->create_node_with_index(children, dp_table[std::to_string(n)][gate_name].gate_function, n);
            res->add_binding( res->get_node( f ), dp_table[std::to_string(n)][gate_name].gate_id );
        }

        
        convert_node_on_timer(n,gate_name, lib_gates, timer, PlaceInfo_net, PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_verilog);

        // for (uint32_t l : *(dp_table[to_string(n)][gate_name].cut))
        // {
        //     for (auto cut_gate : dp_back_rtat[to_string(l)])
        //     {
        //         if(res->is_ci(l) || res->is_constant(l))
        //         {
        //             continue;
        //         }
        //         convert_node_on_timer(l,cut_gate.first, lib_gates, timer, PlaceInfo_net, PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_verilog);
                
        //         timer.update_at_pin("g" + to_string(n) + ":" + node_to_gate_map[to_string(n)].output_name, ot::MAX, ot::FALL);
        //         dp_back_rtat[to_string(l)][cut_gate.first]=min(dp_back_rtat[to_string(l)][cut_gate.first],dp_back_rtat[to_string(n)][gate_name]-
        //             timer.report_delay_between("g" + to_string(n) + ":" + node_to_gate_map[to_string(n)].output_name, "g" + to_string(l) + ":" + node_to_gate_map[to_string(l)].output_name, ot::MAX, ot::FALL));
                
        //     }
        // }

        for (auto dp_gate : dp_table[to_string(n)]){
            for (uint32_t l : *(dp_gate.second.cut))
            {
                dp_fanouts[std::to_string(l)].push_back(n);
                dp_iter_map[to_string(l)]--;
                if(dp_iter_map[to_string(l)]==0) 
                {
                    dp_iter_map[to_string(l)]--;
                    topo_q.push(l);
                }
            }
        }
    }

    // void New_dp_back_propagation_rec(binding_view<klut_network> *res, uint32_t n, queue<int> &topo_q, ot::Timer &timer, PlaceInfo::HashTable<PlaceInfo::Net> &PlaceInfo_net, PlaceInfo::HashTable<std::string> &PlaceInfo_gateToNet,
    //                                  PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap, PlaceInfo::HashTable<std::string> &PlaceInfo_verilog, const std::vector<gate> &lib_gates, HashTable<std::string> &gateHeightAssignmentMap, 
    //                                  std::vector<int>& reassign_minor_rows,std::vector<int>& reassign_major_rows, json &def_data)
    // {
    //     if(res->is_ci(n) || res->is_constant(n))
    //     {
    //         return;
    //     }
    //     string gate_name;
    //     double min_area=MAXFLOAT;
    //     cout<<n<<endl;
    //     int good_12T=0;
    //     string good_12T_name;
    //     for (auto dp_gate : dp_table[std::to_string(n)])
    //     {
    //         double original_area=lib_gates[dp_table[std::to_string(n)][PlaceInfoMap["g"+std::to_string(n)].cell_type].gate_id].area;
    //         cout<<n<<' '<<PlaceInfoMap["g"+std::to_string(n)].cell_type<<' '<<dp_gate.first<<' '<<dp_gate.second.area-original_area<<' '<<dp_gate.second.delay_list<<' '<<dp_back_rtat[std::to_string(n)][dp_gate.first]<<endl;
    //         if(dp_gate.second.area>original_area+1e-6 &&dp_gate.second.area-original_area>area_slack)
    //             continue;
    //         if(dp_gate.second.delay_list+80<=dp_back_rtat[std::to_string(n)][dp_gate.first] && min_area>dp_gate.second.area)
    //         {
    //             int flag=0;
    //             if(Util::find_substring_from_string("12T", dp_gate.first))
    //             {
    //                 treapnode *current_node=treapnode_by_n[std::to_string(n)];
    //                 for(auto minor_row:reassign_minor_rows)
    //                 {
    //                     if(row_density_map[minor_row]+current_node->width > max_x_length)continue;
    //                     if((double)(minor_row- current_node->original_y)*(minor_row- current_node->original_y)+Placer[minor_row]->try_placeat(current_node,n) <= displacement_threshold)
    //                     {
    //                         flag=1;
    //                         break;
    //                     }
    //                 }
    //                 if(!flag)
    //                 {
    //                     good_12T=1;
    //                     good_12T_name=dp_gate.first;
    //                 }
    //             }
    //             if(Util::find_substring_from_string("8T", dp_gate.first) || flag)
    //             {
    //                 gate_name=dp_gate.first;
    //                 min_area=dp_gate.second.area;
    //             }
    //         }
    //     }
    //     cout<<gate_name<<' '<<min_area<<' '<<"good_12T"<<good_12T<<endl;
    //     if(min_area==MAXFLOAT)
    //     {
    //         double original_area=lib_gates[dp_table[std::to_string(n)][PlaceInfoMap["g"+std::to_string(n)].cell_type].gate_id].area;
    //         for (auto dp_gate : dp_table[std::to_string(n)])
    //         {
    //             cout<<n<<' '<<dp_gate.first<<' '<<dp_gate.second.area-original_area<<' '<<dp_gate.second.delay_list<<' '<<dp_back_rtat[std::to_string(n)][dp_gate.first]<<endl;
    //             if(dp_gate.second.area>original_area+1e-6 &&dp_gate.second.area-original_area>area_slack)
    //                 continue;
    //             if(min_area==MAXFLOAT || dp_gate.second.delay_list<dp_table[std::to_string(n)][gate_name].delay_list)
    //             {
    //                 int flag=0;
    //                 if(Util::find_substring_from_string("12T", dp_gate.first))
    //                 {
    //                     treapnode *current_node=treapnode_by_n[std::to_string(n)];
    //                     for(auto minor_row:reassign_minor_rows)
    //                     {
    //                         if(row_density_map[minor_row]+current_node->width > max_x_length)continue;
    //                         if((double)(minor_row- current_node->original_y)*(minor_row- current_node->original_y)+Placer[minor_row]->try_placeat(current_node,n) <= displacement_threshold)
    //                         {
    //                             flag=1;
    //                             break;
    //                         }
    //                     }
    //                     if(!flag)
    //                     {
    //                         good_12T=1;
    //                         good_12T_name=dp_gate.first;
    //                         continue;
    //                     }
    //                 }
    //                 gate_name=dp_gate.first;
    //                 min_area=dp_gate.second.area;
    //             }
    //         }
    //     }
    //     if(good_12T)
    //     {
    //         tend_to_12T["g"+to_string(n)]=1;
    //         tend_to_12T_name["g"+to_string(n)]=good_12T_name;
    //     }
    //     cout<<gate_name<<' '<<min_area<<' '<<"good_12T"<<good_12T<<endl;
    //     area_slack-=dp_table[std::to_string(n)][gate_name].area-lib_gates[dp_table[std::to_string(n)][PlaceInfoMap["g"+std::to_string(n)].cell_type].gate_id].area;
    //     PlaceInfoMap["g"+std::to_string(n)].cell_type=gate_name;

    //     if(Util::find_substring_from_string("12T", gate_name))
    //     {
    //         double min_cost=MAXFLOAT;
    //         int min_row;
    //         treapnode *current_node=treapnode_by_n[std::to_string(n)];
    //         pair<int,int> original_dy= calculate_dy(n,current_node->original_y,gate_name);
    //         for(int row:reassign_minor_rows)
    //         {
    //             if(row_density_map[row]+current_node->width > max_x_length)continue;
    //             pair<int,int> row_dy= calculate_dy(n,row,gate_name);
    //             if(original_dy.first<row_dy.first&&original_dy.second<row_dy.second)continue;
    //             double cost=(double)(row- current_node->original_y)*(row- current_node->original_y)+Placer[row]->try_placeat(current_node,n);
    //             if(cost<min_cost)
    //             {
    //                 min_cost=cost;
    //                 min_row=row;
    //             }
    //         }
    //         current_node->final_y=min_row;
    //         Placer[min_row]->do_placeat(current_node,n);
    //         row_density_map[min_row]+=current_node->width;
    //     }
    //     else
    //     {
    //         double min_cost=MAXFLOAT;
    //         int min_row;
    //         treapnode *current_node=treapnode_by_n[std::to_string(n)];
    //         for(int row:reassign_major_rows)
    //         {
    //             // cout<<"try row "<<row<<' '<<current_node->original_y<<endl;
    //             if(row_density_map[row]+current_node->width > max_x_length)continue;
    //             double cost=(double)(row- current_node->original_y)*(row- current_node->original_y)+Placer[row]->try_placeat(current_node,n);
    //             // cout<<"cost "<<cost<<endl;
    //             if(cost<min_cost)
    //             {
    //                 min_cost=cost;
    //                 min_row=row;
    //             }
    //         }
    //         current_node->final_y=min_row;
    //         Placer[min_row]->do_placeat(current_node,n);
    //         row_density_map[min_row]+=current_node->width;
    //     }
    //     if (!res->is_ci(n) && !res->is_constant(n))
    //     {
    //         auto ctr = 0u;
    //         auto num_vars = dp_table[std::to_string(n)][gate_name].cut->size();
    //         // std::cout<<"num_vars "<<num_vars<<" cut "<<*(dp_final_solution[to_string(n)][0].cut)<<"\n";
    //         std::vector<signal_type> children( num_vars );
            

    //         for ( auto l : *(dp_table[std::to_string(n)][gate_name].cut) ){
    //             if ( ctr >= num_vars )
    //                 break;
    //             children[dp_table[std::to_string(n)][gate_name].permutation[ctr]] = l;
    //             ctr++;   
    //         }
    //         def_data["g"+to_string(n)]["macroName"]=dp_table[std::to_string(n)][gate_name].name;
            
    //         auto f = res->create_node_with_index(children, dp_table[std::to_string(n)][gate_name].gate_function, n);
    //         res->add_binding( res->get_node( f ), dp_table[std::to_string(n)][gate_name].gate_id );
            
    //     }
    //     // cout<<"here3"<<endl;
    //     // for (auto dp_gate : dp_table[to_string(n)]){
    //     //     cout<<dp_gate.first<<endl;
    //     // }

    //     vector<uint32_t> cut_list;
    //     uint32_t cut_value;
    //     for (uint32_t l : *(dp_table[std::to_string(n)][gate_name].cut))
    //     {
    //         cut_list.push_back(l);
    //         cut_value += l;
    //     }

    //     if (node_cut_map[std::to_string(n)] != cut_list)
    //     {
    //         // TODO: reconstruct rc tree in opentimer
    //         std::cout << "n " << n << " look at the pin of gate " << gate_name << " output name " << dp_table[to_string(n)][gate_name].output_name << ":\n";
    //         for (auto pin : dp_table[std::to_string(n)][gate_name].pins)
    //         {
    //             std::cout << pin.name << " ";
    //         }

    //         createRCTree *tree = new createRCTree(PlaceInfo_net, PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_verilog, timer);

    //         FluteResult result;
    //         tree->buildTopo_remap(node_cut_map[to_string(n)], cut_list, n, gate_name, dp_table[to_string(n)][gate_name].permutation, dp_table[to_string(n)][gate_name].pins, dp_table[to_string(n)][gate_name].output_name);
    //         result = tree->runFlute();
    //         delete tree;

    //         // update the global hashmap
    //         node_cut_map[to_string(n)] = cut_list;
    //         node_to_gate_map[to_string(n)] = lib_gates[dp_table[to_string(n)][gate_name].gate_id];

    //         // std::cout << "opentimer cut for " << n << ":";
    //         // for (auto l : node_cut_map[to_string(n)])
    //         // {
    //         //     std::cout << l << " ";
    //         // }
    //         // std::cout << "\n";
    //         // std::cout << "dp cut for " << n << ":";
    //         // for (auto l : cut_list)
    //         // {
    //         //     std::cout << l << " ";
    //         // }
    //         // std::cout << "\n";
    //     }
    //     timer.repower_gate("g" + to_string(n), gate_name);
    //     node_to_gate_map[to_string(n)] = lib_gates[dp_table[to_string(n)][gate_name].gate_id];

    //     for (uint32_t l : *(dp_table[to_string(n)][gate_name].cut))
    //     {
    //         for (auto cut_gate : dp_back_rtat[to_string(l)])
    //         {
    //             if(res->is_ci(l) || res->is_constant(l))
    //             {
    //                 continue;
    //             }

    //             vector<uint32_t> cut_list;
    //             uint32_t cut_value;
    //             for (uint32_t k : *(dp_table[std::to_string(l)][cut_gate.first].cut))
    //             {
    //                 cut_list.push_back(k);
    //                 cut_value += k;
    //             }

    //             if (node_cut_map[std::to_string(l)] != cut_list)
    //             {
    //                 // TODO: reconstruct rc tree in opentimer
    //                 std::cout << "l " << l << " look at the pin of gate " << cut_gate.first << " output name " << dp_table[to_string(l)][cut_gate.first].output_name << ":\n";
    //                 for (auto pin : dp_table[std::to_string(l)][cut_gate.first].pins)
    //                 {
    //                     std::cout << pin.name << " ";
    //                 }

    //                 createRCTree *tree = new createRCTree(PlaceInfo_net, PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_verilog, timer);

    //                 FluteResult result;
    //                 tree->buildTopo_remap(node_cut_map[to_string(l)], cut_list, l, cut_gate.first, dp_table[to_string(l)][cut_gate.first].permutation, dp_table[to_string(l)][cut_gate.first].pins, dp_table[to_string(l)][cut_gate.first].output_name);
    //                 result = tree->runFlute();
    //                 delete tree;

    //                 // update the global hashmap
    //                 node_cut_map[to_string(l)] = cut_list;
    //                 node_to_gate_map[to_string(l)] = lib_gates[dp_table[to_string(l)][cut_gate.first].gate_id];

    //             }                                                           

    //             timer.repower_gate("g" + to_string(l), cut_gate.first);
    //             timer.run_all_tasks();
    //             timer.update_at_pin("g" + to_string(n) + ":" + node_to_gate_map[to_string(n)].output_name, ot::MAX, ot::FALL);
    //             dp_back_rtat[to_string(l)][cut_gate.first]=min(dp_back_rtat[to_string(l)][cut_gate.first],dp_back_rtat[to_string(n)][gate_name]-
    //                 timer.report_delay_between("g" + to_string(n) + ":" + node_to_gate_map[to_string(n)].output_name, "g" + to_string(l) + ":" + node_to_gate_map[to_string(l)].output_name, ot::MAX, ot::FALL));
                
    //         }
    //     }
        
    //     for (auto dp_gate : dp_table[to_string(n)]){
    //         for (uint32_t l : *(dp_gate.second.cut))
    //         {
    //             dp_fanouts[std::to_string(l)].push_back(n);
    //             dp_iter_map[to_string(l)]--;
    //             if(dp_iter_map[to_string(l)]==0) 
    //             {
    //                 dp_iter_map[to_string(l)]--;
    //                 topo_q.push(l);
    //             }
    //         }
    //     }
    // }


    void New_dp_back_propagation(binding_view<klut_network> *res, ot::Timer &timer, PlaceInfo::HashTable<PlaceInfo::Net> &PlaceInfo_net, PlaceInfo::HashTable<std::string> &PlaceInfo_gateToNet,
                                 PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap, PlaceInfo::HashTable<Macro> PlaceInfo_macro, PlaceInfo::HashTable<std::string> &PlaceInfo_verilog, 
                                 const std::vector<gate> &lib_gates, HashTable<std::string> &gateHeightAssignmentMap, std::vector<int>& reassign_minor_rows,std::vector<int>& reassign_major_rows,
                                 std::string design_name, json &def_data, double period, string phase, double slack_threshold)
    {
        std::cout << "enter propagation\n";
        // initialize dp_final_solution
        
        for(auto placeinfo:PlaceInfoMap)
        {
            if(Placer.find(placeinfo.second.y)==Placer.end())
            {
                treaps *t=new treaps;
                Placer.insert(make_pair(placeinfo.second.y,t));
            }
        }

        for(auto itor:Placer)
        {
            itor.second->nodes.clear();
        }
        for(auto itor:row_density_map)
        {
            row_density_map[itor.first]=0;
        }

        
        topo_view<binding_view<klut_network>>(*res).foreach_node([&](auto n){
            if (!res->is_ci(n) && !res->is_constant(n)){
                dp_iter_map[std::to_string(n)] = 0; 
                dp_fanouts[std::to_string(n)].clear();
                treapnode* n_node;
                if(!treapnode_by_n[std::to_string(n)])
                {   
                    n_node=new treapnode();
                    treapnode_by_n[std::to_string(n)]=n_node;
                }
                else
                {
                    n_node=treapnode_by_n[std::to_string(n)];
                }
                n_node->init();
                n_node->original_x=PlaceInfoMap["g"+std::to_string(n)].x;
                n_node->original_y=PlaceInfoMap["g"+std::to_string(n)].y;
                n_node->weight_e=1;
                n_node->width=PlaceInfo_macro[PlaceInfoMap["g"+std::to_string(n)].cell_type].width*100;
                n_node->id=n;
                n_node->pushup();
            }
        });
        
        
        topo_view<binding_view<klut_network>>(*res).foreach_node([&](auto n){
            if (res->is_ci(n) || res->is_constant(n))
            {
            }
            else{
                for (std::pair<string,NewDPSolver::New_DP_Gate_Sol> dp_gate : dp_table[std::to_string(n)]){
                    dp_back_rtat[to_string(n)][dp_gate.first]=timer.clocks().at("ext_clk_virt").period();
                    for (uint32_t l : *(dp_gate.second.cut))
                    {
                        dp_iter_map[std::to_string(l)]++; 
                    }
                }
            } 
        });
        // topo_view<binding_view<klut_network>>(*res).foreach_node([&](auto n){
        //     if (res->is_ci(n) || res->is_constant(n))
        //     {
        //         dp_back_rtat[std::to_string(n)][std::to_string(n)]=period;
        //     }
        //     else{
        //         for (std::pair<string,NewDPSolver::New_DP_Gate_Sol> dp_gate : dp_table[std::to_string(n)]){
        //             dp_back_rtat[std::to_string(n)][dp_gate.first]=period;
        //             for (uint32_t l : *(dp_gate.second.cut))
        //             {
        //                 dp_iter_map[std::to_string(l)]++; 
        //             }
        //         }
        //     } 
        // });
        std::queue<int >res_nodes;


        res->foreach_po([&](auto const &o, uint32_t index){
            // std::cout<<"back propagation, po "<<o<<endl;;
            if (o!=0&&dp_iter_map[std::to_string(o)]==0){
                res_nodes.push(o);
                } });
        while(!res_nodes.empty())
        {
            int o=res_nodes.front();
            res_nodes.pop();
            cout<<"start "<<o<<endl;
            New_dp_back_propagation_iteration_rec(res,o,res_nodes,timer,PlaceInfo_net,PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_macro, PlaceInfo_verilog,lib_gates,gateHeightAssignmentMap, reassign_minor_rows, reassign_major_rows,def_data,phase, slack_threshold, period);
            cout<<"end "<<o<<endl;
        }

        double area_tot=0;
        topo_view<binding_view<klut_network>>(*res).foreach_node([&](auto n){
            if (!res->is_ci(n) && !res->is_constant(n)){
                area_tot+=PlaceInfo_macro[PlaceInfoMap["g"+std::to_string(n)].cell_type].area*10000/2000/2000;
            }
        });
        cout<<"area slack "<<area_slack<<' '<<area_tot<<endl;
        
    }

    // void reassign_height_new(HashTable<PlaceInfo::Gate>& PlaceInfoMap, HashTable<PlaceInfo::Macro>& PlaceInfo_macro,int chosen_top_K, 
    //     std::vector<int>& rows_12T,std::vector<int>& rows_8T, int density_12T=3)
    // {
    //     std::unordered_map<int, double> count12TByY;
    //     std::unordered_map<int, double> count8TByY;
    //     rows_12T.clear();
    //     rows_8T.clear();
    //     vector<int>ylist;
    //     for (const auto& gate : PlaceInfoMap) {
    //         int y = gate.second.y;
    //         std::string cell_type = gate.second.cell_type;
    //         ylist.push_back(y);
    //         if (cell_type.find("8T") != std::string::npos) {
    //             if(count8TByY.find(y) != count8TByY.end())
    //             {
    //                 count8TByY[y]+=PlaceInfo_macro[cell_type].area;
    //             }
    //             else
    //             {
    //                 count8TByY[y]=PlaceInfo_macro[cell_type].area;
    //             }
    //         }
    //         else
    //         {
    //             if(count12TByY.find(y) != count12TByY.end())
    //             {
    //                 count12TByY[y]+=PlaceInfo_macro[cell_type].area;
    //             }
    //             else
    //             {
    //                 count12TByY[y]=PlaceInfo_macro[cell_type].area;
    //             }
    //         }
    //     }
    //     std::sort(ylist.begin(),ylist.end());
    //     std::vector<int>::iterator s=std::unique(ylist.begin(),ylist.end());
    //     ylist.erase(s,ylist.end());
    //     std::vector<std::pair<int, float>> rowScorePairs;
    //     std::unordered_map<int, string>row_height;
    //     for(int y:ylist)
    //     {
    //         rowScorePairs.emplace_back(y,0);
    //         row_height[y]="8T";
    //         if(count12TByY.find(y)!=count12TByY.end())
    //         {
    //             rowScorePairs.back().second+=count12TByY[y];
    //         }
    //         if(count8TByY.find(y)!=count8TByY.end())
    //         {
    //             rowScorePairs.back().second-=count8TByY[y];
    //         }
    //     }
    //     std::sort(rowScorePairs.begin(), rowScorePairs.end(),
    //             [](const auto& lhs, const auto& rhs) {
    //                 return lhs.second > rhs.second;
    //             });
        
    //     for(int i=0;i<chosen_top_K;i++)
    //     {
    //         row_height[rowScorePairs[i].first]="12T";
    //     }
    //     for(int i=0;i<ylist.size();i++)
    //     {
    //         // cout<<"fuck "<<i<<' '<<ylist[i]<<' '<<row_height[ylist[i]]<<endl;
    //         if(row_height[ylist[i]]=="12T")continue;
    //         int cnt0=0;
    //         for(int j=0;j+1<2*density_12T;j++)
    //         {
    //             if(i+j<ylist.size()&&row_height[ylist[i+j]]=="8T")
    //             {
    //                 cnt0++;
    //             }
    //             else
    //             {
    //                 break;
    //             }
    //         }
    //         // cout<<ylist[i]<<' '<<i<<' '<<cnt0<<' '<<density_12T<<endl;
    //         if(cnt0>=density_12T)
    //         {
    //             // cout<<"12T "<<ylist[i+cnt0/2]<<endl;
    //             row_height[ylist[i+cnt0/2]]="12T";
    //             i=i+cnt0/2;
    //         }
    //     }
    //     // int sum=0;
    //     // for(int i=0;i+1<ylist.size();i++)
    //     // {
    //     //     sum++;
    //     //     if(row_height[ylist[i]]!=row_height[ylist[i+1]])
    //     //     {
    //     //         if(sum%2==0)
    //     //         {
    //     //             sum=0;
    //     //             continue;
    //     //         }
    //     //         else
    //     //         {
    //     //             if(row_height[ylist[i]]=="8T")
    //     //             {
    //     //                 row_height[ylist[i]]="12T";
    //     //                 sum=1;
    //     //             }
    //     //             else
    //     //             {
    //     //                 row_height[ylist[i]]="8T";
    //     //             }
    //     //         }
    //     //     }
    //     // }
    //     for(int i=0;i<ylist.size();i++)
    //     {
    //         if(row_height[ylist[i]]=="12T")rows_12T.push_back(ylist[i]);
    //         else rows_8T.push_back(ylist[i]);
    //     }
    // }

    void reassign_height_new(HashTable<PlaceInfo::Gate>& PlaceInfoMap, HashTable<PlaceInfo::Macro>& PlaceInfo_macro,int chosen_top_K, 
        std::vector<int>& rows_12T,std::vector<int>& rows_8T, int density_12T=3, int density_8T=-1)
    {
        std::unordered_map<int, double> count12TByY;
        std::unordered_map<int, double> count8TByY;
        rows_12T.clear();
        rows_8T.clear();
        vector<int>ylist;
        for (const auto& gate : PlaceInfoMap) {
            int y = gate.second.y;
            std::string cell_type = gate.second.cell_type;
            ylist.push_back(y);
            if (cell_type.find("8T") != std::string::npos) {
                if(count8TByY.find(y) != count8TByY.end())
                {
                    count8TByY[y]+=PlaceInfo_macro[cell_type].area;
                }
                else
                {
                    count8TByY[y]=PlaceInfo_macro[cell_type].area;
                }
            }
            else
            {
                if(count12TByY.find(y) != count12TByY.end())
                {
                    count12TByY[y]+=PlaceInfo_macro[cell_type].area;
                }
                else
                {
                    count12TByY[y]=PlaceInfo_macro[cell_type].area;
                }
            }
        }
        std::sort(ylist.begin(),ylist.end());
        std::vector<int>::iterator s=std::unique(ylist.begin(),ylist.end());
        ylist.erase(s,ylist.end());
        std::vector<std::pair<int, float>> rowScorePairs;
        std::unordered_map<int, string>row_height;
        for(int i=0;i+1<ylist.size();i+=2)
        {
            rowScorePairs.emplace_back(i,0);
            for(int j=0;j<2;j++)
            {
                int y=ylist[i+j];
                row_height[y]="8T";
                if(count12TByY.find(y)!=count12TByY.end())
                {
                    rowScorePairs.back().second+=count12TByY[y];
                }
                if(count8TByY.find(y)!=count8TByY.end())
                {
                    rowScorePairs.back().second-=count8TByY[y];
                }
            }
        }
        std::sort(rowScorePairs.begin(), rowScorePairs.end(),
                [](const auto& lhs, const auto& rhs) {
                    return lhs.second > rhs.second;
                });
        
        for(int i=0;i<chosen_top_K;i++)
        {
            row_height[ylist[rowScorePairs[i].first]]="12T";
            row_height[ylist[rowScorePairs[i].first+1]]="12T";
        }
        for(int i=0;i<ylist.size();i+=2)
        {
            // cout<<"fuck "<<i<<' '<<ylist[i]<<' '<<row_height[ylist[i]]<<endl;
            if(row_height[ylist[i]]=="12T")continue;
            int cnt0=0;
            for(int j=0;j+1<2*density_12T;j++)
            {
                if(i+j*2<ylist.size()&&row_height[ylist[i+j*2]]=="8T")
                {
                    cnt0++;
                }
                else
                {
                    break;
                }
            }
            // cout<<ylist[i]<<' '<<i<<' '<<cnt0<<' '<<density_12T<<endl;
            if(cnt0>=density_12T)
            {
                // cout<<"12T "<<ylist[i+cnt0/2]<<endl;
                row_height[ylist[i+cnt0/2*2]]="12T";
                row_height[ylist[i+cnt0/2*2+1]]="12T";
                i=i+cnt0/2*2;
            }
        }
        if(density_8T!=-1)
        {
            for(int i=0;i<ylist.size();i+=2)
            {
                // cout<<"fuck "<<i<<' '<<ylist[i]<<' '<<row_height[ylist[i]]<<endl;
                if(row_height[ylist[i]]=="8T")continue;
                int cnt0=0;
                for(int j=0;j+1<2*density_8T;j++)
                {
                    if(i+j*2<ylist.size()&&row_height[ylist[i+j*2]]=="12T")
                    {
                        cnt0++;
                    }
                    else
                    {
                        break;
                    }
                }
                // cout<<ylist[i]<<' '<<i<<' '<<cnt0<<' '<<density_12T<<endl;
                if(cnt0>=density_8T)
                {
                    // cout<<"12T "<<ylist[i+cnt0/2]<<endl;
                    row_height[ylist[i+cnt0/2*2]]="8T";
                    row_height[ylist[i+cnt0/2*2+1]]="8T";
                    i=i+cnt0/2*2;
                }
            }
        }
        // int sum=0;
        // for(int i=0;i+1<ylist.size();i++)
        // {
        //     sum++;
        //     if(row_height[ylist[i]]!=row_height[ylist[i+1]])
        //     {
        //         if(sum%2==0)
        //         {
        //             sum=0;
        //             continue;
        //         }
        //         else
        //         {
        //             if(row_height[ylist[i]]=="8T")
        //             {
        //                 row_height[ylist[i]]="12T";
        //                 sum=1;
        //             }
        //             else
        //             {
        //                 row_height[ylist[i]]="8T";
        //             }
        //         }
        //     }
        // }
        for(int i=0;i<ylist.size();i++)
        {
            if(row_height[ylist[i]]=="12T")rows_12T.push_back(ylist[i]);
            else rows_8T.push_back(ylist[i]);
        }
    }

    std::pair<std::string, std::string> score_row_height_baseline(HashTable<PlaceInfo::Gate>& PlaceInfoMap, HashTable<PlaceInfo::Macro>& PlaceInfo_macro,
                                                    std::vector<int>& minor_coordinates, std::map<std::string, float>& minor_width,
                                                    std::vector<int>& chosen_centers ,int chosen_top_K, std::vector<int>& row_coordinates,
                                                    std::map<std::string, int>& minor_map,std::map<std::string, int>& major_map,
                                                    std::map<std::string, float>& major_width, string phase)
    {
        std::unordered_map<int, std::vector<std::string>> gateNamesByY;
        std::unordered_map<int, std::string> rowHeightByY;
        std::unordered_map<int, float> rowScoreByY;
        std::unordered_map<std::string, std::string> gateInfo;
        float totalArea8T = 0;
        float totalArea12T = 0;
        int number_of_8T=0,number_of_12T=0;

        // Count the gate name and cell_type for each y value and 
        // calculate the height of each row based on the number of cell_types
        for (const auto& gate : PlaceInfoMap) {
            int y = gate.second.y;
            gateNamesByY[y].push_back(gate.second.name);
            std::string cell_type = gate.second.cell_type;
            if (cell_type.find("8T") != std::string::npos) {
                number_of_8T++;
                gateInfo[gate.second.name] = "8T";
                if (rowHeightByY.find(y) == rowHeightByY.end()) {
                    rowHeightByY[y] = "8T";
                } else {
                    int count8T = 0;
                    int count12T = 0;
                    for (const auto& name : gateNamesByY[y]) {
                        std::string cell_type = PlaceInfoMap[name].cell_type;
                        if (cell_type.find("8T") != std::string::npos) {
                            count8T++;
                        } else if (cell_type.find("12T") != std::string::npos) {
                            count12T++;
                        }
                    }
                    if (count8T >= count12T) {
                        rowHeightByY[y] = "8T";
                    } else {
                        rowHeightByY[y] = "12T";
                    }
                }
                totalArea8T += PlaceInfo_macro[cell_type].area;
            }
            else if (cell_type.find("12T") != std::string::npos) {
                number_of_12T++;
                gateInfo[gate.second.name] = "12T";
                if (rowHeightByY.find(y) == rowHeightByY.end()) {
                    rowHeightByY[y] = "12T";
                } else {
                    int count8T = 0;
                    int count12T = 0;
                    for (const auto& name : gateNamesByY[y]) { 
                        std::string cell_type = PlaceInfoMap[name].cell_type;
                        if (cell_type.find("8T") != std::string::npos) {
                            count8T++;
                        } else if (cell_type.find("12T") != std::string::npos) {
                            count12T++;
                        }
                    }
                    if (count12T >= count8T) {
                        rowHeightByY[y] = "12T";
                    } else {
                        rowHeightByY[y] = "8T";
                    }
                }
                totalArea12T += PlaceInfo_macro[cell_type].area;
                // std::cout<<cell_type<<' '<<PlaceInfo_macro[cell_type].area<<std::endl;
            }
        }
        std::cout<<"8T 12T "<<number_of_8T<<' '<<number_of_12T<<std::endl;

        std::string target_minor = totalArea12T > totalArea8T? "8T" : "12T";
        std::string target_major = totalArea12T < totalArea8T? "8T" : "12T";
        for (const auto& gate : PlaceInfoMap) {
            int y = gate.second.y;
            std::string cell_type = gate.second.cell_type;

            if (cell_type.find(target_minor) != std::string::npos) {
                minor_coordinates.push_back(y);
                minor_map[gate.second.name] = y;
                minor_width[gate.second.name] = PlaceInfo_macro[cell_type].width;
            } 

            if (cell_type.find(target_major) != std::string::npos) {
                major_map[gate.second.name] = y;
                major_width[gate.second.name] = PlaceInfo_macro[cell_type].width;
            } 
        }

        // Calculate the score for each row
        for (const auto& [y, gateNames] : gateNamesByY) {
            float areaSameHeight = 0;
            float areaDiffHeight = 0;
            std::string rowHeight = totalArea8T > totalArea12T ? "12T" : "8T";
            for (const auto& name : gateNames) {
                std::string cell_type = PlaceInfoMap[name].cell_type;
                if (cell_type.find(rowHeight) != std::string::npos) {
                    areaSameHeight += PlaceInfo_macro[cell_type].area;
                }
                else {
                    areaDiffHeight += PlaceInfo_macro[cell_type].area;
                }
            }
            rowScoreByY[y] = areaSameHeight - areaDiffHeight;
        }

        // Sort the rows according to the row score
        // Create a vector of pairs for sorting
        std::vector<std::pair<int, float>> rowScorePairs;
        for (const auto& [y, gateNames] : gateNamesByY) {
            rowScorePairs.emplace_back(y, rowScoreByY[y]);
            row_coordinates.emplace_back(y);
        }

        // Sort the row scores in descending order
        std::sort(rowScorePairs.begin(), rowScorePairs.end(),
                [](const auto& lhs, const auto& rhs) {
                    return lhs.second > rhs.second;
                });
        for (int i = 0; i < chosen_top_K; i++){
            if (i >= rowScorePairs.size()){
                break;
            }
            chosen_centers.push_back(rowScorePairs[i].first);
            std::cout<<"center "<<rowScorePairs[i].first<<std::endl;
        }

        std::cout << "Total Area (8T cell): " << totalArea8T << std::endl;
        std::cout << "Total Area (12T cell): " << totalArea12T << std::endl;
        std::cout << "The cell type with larger total area is: "
                    << (totalArea8T > totalArea12T ? "8Tcell" : "12Tcell") <<std::endl;
        
        return std::make_pair(target_minor,target_major);
        
    }

    std::vector<float> kmeans(std::vector<int> minor_coordinate, std::vector<int> chosen_centers, 
                        int num_clusters, int iter) 
    {   
        std::vector<int> cluster_assignment(minor_coordinate.size());  // Record which cluster each point belongs to
        std::vector<int> cluster_size(num_clusters);          // Record the number of points contained in each cluster
        std::vector<int> cluster_sum(num_clusters);  // Record the sum of the coordinates of the points in each cluster
        std::vector<float> cluster_center(num_clusters);

        // Information on initializing clusters
        for (int i = 0; i < minor_coordinate.size(); i++) {
            float best_cluster = 0;
            float best_distance = abs(minor_coordinate[i] - chosen_centers[0]);
            for (int j = 1; j < chosen_centers.size(); j++) {
                float d = abs(minor_coordinate[i] - chosen_centers[j]);
                if (d < best_distance) {
                    best_cluster = j;
                    best_distance = d;
                }
            }
            cluster_assignment[i] = best_cluster;
            cluster_size[best_cluster]++;
            cluster_sum[best_cluster] += minor_coordinate[i];
        }
    
        // Perform an iterative process until the allocation of clusters no longer changes
        for (int k = 0; k < iter; k++){

            // Update the centre position of each cluster
            std::vector<int> new_centers(num_clusters);
            for (int i = 0; i < num_clusters; i++) {
                // std::cout<<"cluster size "<<cluster_size[i]<<"\n";
                if (cluster_size[i] > 0) {
                    cluster_center[i] = (float)cluster_sum[i] / cluster_size[i];
                    new_centers[i] = cluster_center[i];
                    
                }
                else{
                    // If there are more num_clusters, then add a random cluster centre
                    int random_index = rand() % minor_coordinate.size();
                    new_centers[i] = minor_coordinate[random_index];
                    cluster_center[i] = (float)new_centers[i];
                }
            }
            chosen_centers = new_centers;

            // Reallocation of clusters per point
            for (int i = 0; i < minor_coordinate.size(); i++) {
                float best_cluster = 0;
                float best_distance = abs(minor_coordinate[i] - chosen_centers[0]);
                for (int j = 1; j < num_clusters; j++) {
                    float d = abs(minor_coordinate[j] - chosen_centers[j]);
                    if (d < best_distance) {
                        best_cluster = j;
                        best_distance = d;
                    }
                }
                if (best_cluster != cluster_assignment[i]) {
                    cluster_size[cluster_assignment[i]]--;
                    cluster_sum[cluster_assignment[i]] -= minor_coordinate[i];
                    cluster_assignment[i] = best_cluster;
                    cluster_sum[best_cluster] += minor_coordinate[i];
                    cluster_size[best_cluster]++;
                }

            }
        }
        std::sort(cluster_center.begin(), cluster_center.end());
        auto last = std::unique(cluster_center.begin(), cluster_center.end());
        cluster_center.erase(last, cluster_center.end());
        return cluster_center;
    }

    std::vector<int> reassign_height(std::vector<int>& chosen_centers, std::vector<int> minor_coordinates, 
                                std::map<std::string, float>& minor_width, float threshold_minor, float threshold_major,int iter,
                                std::vector<int>& row_coordinates, 
                                std::map<std::string, int>& minor_map,
                                 std::vector<int>& reassign_major_centers)
    {
        float W_sum = 0.0;
        for (const auto& width : minor_width){
            W_sum += width.second;
        }
        float W_average = W_sum / chosen_centers.size();

        std::vector<int> reassign_centers;
        std::vector<int> current_centers(chosen_centers.size());

        std::transform(chosen_centers.begin(), chosen_centers.end(), current_centers.begin(),
            [](int val) { return static_cast<float>(val); });

        int num_clusters = chosen_centers.size();
            swap(current_centers[1],current_centers[2]);
        while (true){
            bool flag = 1;
            std::vector<int> reassign_centers_tmp;
            std::map<std::string, int> minor_map_copy = minor_map;
            // std::cout<<"before kmeans"<<num_clusters<<' '<<iter<<endl;
            std::vector<float> kmeans_centers = kmeans(minor_coordinates, current_centers, num_clusters, iter);
            // std::cout<<"before get center\n";
            // Find the row that is most similar to kmeans_centers
            for (auto center : kmeans_centers) {
                float min_dist = std::numeric_limits<float>::max();
                int closest_row = 0;
                for (auto row : row_coordinates) {
                    float dist = std::abs(center - row);
                    if (dist < min_dist) {
                        min_dist = dist;
                        closest_row = row;
                    }
                }
                reassign_centers_tmp.push_back(closest_row);
            }
        
            // Update y-coordinates in minor_map and get gate names
            std::map<int, float> row_width_sum;
            for (auto& entry : minor_map_copy){
                std::string gate_name = entry.first;
                int gate_y = entry.second;
                float min_dist = std::numeric_limits<float>::max();
                int closest_row = 0;
                for (auto row : reassign_centers_tmp){
                    float dist = std::abs(gate_y - row);
                    if (dist < min_dist){
                        min_dist = dist;
                        closest_row = row;
                    }
                }
                entry.second = closest_row;
                row_width_sum[closest_row] += minor_width[gate_name];
            }
            

            // Reallocate the rows that have a total width larger than threshold * W_average
            for(auto& center : reassign_centers_tmp){
                if (row_width_sum[center] > threshold_minor * W_average){
                    // Find the nearest row to the current centre in row_coordinates 
                    float new_center_up = center; 
                    float new_center_below = center; 
                    float min_dist_up = std::numeric_limits<float>::max();
                    float min_dist_below = std::numeric_limits<float>::max();
                    for (auto row : row_coordinates){
                        // Check if the row is already assigned as a center
                        if (std::find(reassign_centers_tmp.begin(), reassign_centers_tmp.end(), row) == reassign_centers_tmp.end()){
                            if (row < center){
                                float dist = std::abs(center - row);
                                if (dist < min_dist_up){
                                    min_dist_up = dist;
                                    new_center_up = row;
                                }
                            } else if (row > center) {
                                float dist = std::abs(center - row);
                                if (dist < min_dist_below){
                                    min_dist_below = dist;
                                    new_center_below = row;
                                }
                            }
                        }
                    }
                
                    if (new_center_up != center && new_center_below != center) {
                        reassign_centers_tmp.erase(std::remove(reassign_centers_tmp.begin(), reassign_centers_tmp.end(), center), reassign_centers_tmp.end());
                        reassign_centers_tmp.push_back(new_center_up);
                        reassign_centers_tmp.push_back(new_center_below);
                    } else if (new_center_up != center ) {
                        reassign_centers_tmp.push_back(new_center_up);   
                    } else if (new_center_below != center) {
                        reassign_centers_tmp.push_back(new_center_below);
                    } else {
                        // No new center found, do nothing
                        std::cout << "No new center found." << std::endl;
                    }
                    num_clusters += 1;
                    current_centers = reassign_centers_tmp;
                    flag = false;
                }
            }

            if (flag){
                reassign_centers = reassign_centers_tmp;
                std::sort(reassign_centers.begin(), reassign_centers.end());
                auto last = std::unique(reassign_centers.begin(), reassign_centers.end());
                reassign_centers.erase(last, reassign_centers.end());
                break;
            }
        }
        
        // Find the major cell on reassign_centers
        std::vector<int> major_rows;
        std::map<int, float> major_row_width_sum;
        for (auto row : major_rows) {
            major_row_width_sum[row] = 0.0;
        }
        for (auto row : row_coordinates) {
            if (std::find(reassign_centers.begin(), reassign_centers.end(), static_cast<float>(row)) == reassign_centers.end()) {
                major_rows.push_back(static_cast<float>(row));
            }
        }

        reassign_major_centers = major_rows;
        return reassign_centers;
    }

    void row_height_assign(std::string design_name, HashTable<PlaceInfo::Gate>& PlaceInfoMap, HashTable<PlaceInfo::Macro>& PlaceInfo_macro,
                    std::vector<int>& reassign_minor_rows,std::vector<int>& reassign_major_rows, std::pair<std::string, 
                    std::string>& target, HashTable<std::string>& gateHeightAssignmentMap, std::string phase,
                    int chosen_top_K, int iter, int density_12T, int density_8T)
    {
        std::vector<int> minor_coordinates;
        std::vector<int> row_coordinates;
        std::vector<int> chosen_centers;
        std::map<std::string, int> minor_map;
        std::map<std::string, float> minor_width;
        std::map<std::string, int> major_map;
        std::map<std::string, float> major_width;
        // int chosen_top_K = 12; //5 //3
        // target = score_row_height_baseline(PlaceInfoMap,PlaceInfo_macro, minor_coordinates, minor_width, chosen_centers,chosen_top_K, 
        //                                                     row_coordinates, minor_map, major_map,major_width, phase);
        // float threshold_minor = 1.0f; //0.65  //0.95
        // float threshold_major = 1.0f; //0.95 //1.0
        // // int iter = 15; //6 //10

        // HashTable<PlaceInfo::Gate> PlaceInfoMap_modify = PlaceInfoMap;
        // reassign_minor_rows = reassign_height(chosen_centers, minor_coordinates, minor_width, threshold_minor, 
        //                                         threshold_major, iter, row_coordinates, 
        //                                         minor_map,   reassign_major_rows);

        reassign_height_new(PlaceInfoMap,PlaceInfo_macro, chosen_top_K, reassign_minor_rows, reassign_major_rows, density_12T, density_8T);
        std::cout << "Minor_rows: ";
        for (const auto& center : reassign_minor_rows) {
            std::cout << center << " ";
        }
        std::cout << std::endl;
        std::cout << "Major_rows: ";
        for (const auto& center : reassign_major_rows) {
            std::cout << center << " ";
        }
        std::cout << std::endl;

        // std::string file_path = "../examples/benchmarks/" + design_name + "/" + design_name + "_def_modified_"+phase+".json";
        // PlaceInfo::rewrite_def_json(file_path, PlaceInfoMap_modify);
    }

    
    void iteration_rowheight(binding_view<klut_network> *res, ot::Timer &timer, PlaceInfo::HashTable<PlaceInfo::Net> &PlaceInfo_net, PlaceInfo::HashTable<std::string> &PlaceInfo_gateToNet,
                                 PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap, PlaceInfo::HashTable<Macro> PlaceInfo_macro, PlaceInfo::HashTable<std::string> &PlaceInfo_verilog, 
                                 const std::vector<gate> &lib_gates, HashTable<std::string> &gateHeightAssignmentMap, std::vector<int>& reassign_minor_rows,
                                 std::vector<int>& reassign_major_rows,std::string design_name, json &def_data, int iter_rowheight
                                 , int chosen_top_K, int iter,double period, double slack_rate_init=0.01)
    {
        std::cout << "enter iteration"<<endl;
        string phase="tight";
        double slack_rate=slack_rate_init,tight_rate=0.005;
        while(iter_rowheight>0)
        {
            iter_rowheight--;
            if(phase=="tight")
            {
                phase="slack";
                slack_rate*=2;
            }
            else 
            {
                phase="tight";
                if(area_slack>=0.3)tight_rate*=2;
            }
            if(phase=="slack"&&area_slack>=0)
            {
                slack_rate/=2;
                continue;
            }
            if(phase=="tight"&&area_slack<=0&&iter_rowheight!=0)
            {
                continue;
            }
            double area_tot=0;
            topo_view<binding_view<klut_network>>(*res).foreach_node([&](auto n){
                if (!res->is_ci(n) && !res->is_constant(n)){
                    area_tot+=PlaceInfo_macro[PlaceInfoMap["g"+std::to_string(n)].cell_type].area*10000/2000/2000;
                }
            });
            cout<<"area tot "<<area_tot<<endl;
            topo_view<binding_view<klut_network>>(*res).foreach_node([&](auto n)
                                                                 {
            if (res->is_ci(n) || res->is_constant(n)){
            }
            else{
                for(auto gate:dp_table[to_string(n)])
                {
                    dp_table[to_string(n)][gate.first].delay_list=MAXFLOAT;
                    dp_table[to_string(n)][gate.first].is_calculated=0;
                }
            }});
            // res->foreach_po([&](auto const &o, uint32_t index)
            //             {
            //                 // #pragma omp parallel for default(shared)
            //                 vector<string> candidate_gate_names;
            //                 for (auto gate_selection : dp_table[to_string(o)])
            //                 {
            //                     candidate_gate_names.push_back(gate_selection.first);
            //                 }
            //                 // #pragma omp parallel for default(shared) 
            //                 for (auto candidate_gate : candidate_gate_names)
            //                 {
            //                     // #pragma omp critical
            //                     New_dp_solve_iteration_rec(res, o, candidate_gate, lib_gates, timer, PlaceInfo_net, 
            //                         PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_verilog);
            //                 } });
            std::vector<pair<pair<int,string>,double>> change_ratios;
            res->foreach_po([&](auto const &o, uint32_t index)
                        {
                                New_dp_solve_iteration_rec(res, o, lib_gates, timer, PlaceInfo_net, 
                                    PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_verilog, change_ratios, phase, period);
                            });
            double slack_threshold;
            if(phase=="slack")
            {
                sort(change_ratios.begin(),change_ratios.end(),[](const auto& lhs, const auto& rhs) {
                    return lhs.second < rhs.second;
                });
                double area_sum=0;
                for(int i=0;i<change_ratios.size();i++)
                {
                    pair<pair<int,string>,double> it=change_ratios[i];
                    area_sum+=dp_table[to_string(it.first.first)][PlaceInfoMap["g"+to_string(it.first.first)].cell_type].area-dp_table[to_string(it.first.first)][it.first.second].area;
                    cout<<"cao "<<it.first.first<<' '<<it.first.second<<' '<<it.second<<' '<<area_sum<<endl;
                    if(area_sum>=area_tot*slack_rate||i+1==change_ratios.size())
                    {
                        slack_threshold=it.second;
                        break;
                    }
                }
            }
            else
            {
                sort(change_ratios.begin(),change_ratios.end(),[](const auto& lhs, const auto& rhs) {
                    return lhs.second < rhs.second;
                });
                double area_sum=0;
                for(int i=0;i<change_ratios.size();i++)
                {
                    pair<pair<int,string>,double> it=change_ratios[i];
                    area_sum+=dp_table[to_string(it.first.first)][it.first.second].area-dp_table[to_string(it.first.first)][PlaceInfoMap["g"+to_string(it.first.first)].cell_type].area;
                    cout<<"cao "<<it.first.first<<' '<<it.first.second<<' '<<it.second<<' '<<area_sum<<endl;
                    if(area_sum>=area_tot*tight_rate||i+1==change_ratios.size())
                    {
                        slack_threshold=it.second;
                        break;
                    }
                }
            }
            cout<<"slack_threshold "<<slack_threshold<<' '<<change_ratios.size() <<endl;
            
            // std::vector<int> reassign_minor_rows;
            // std::vector<int> reassign_major_rows;
            // std::pair<std::string, std::string> target;
            // PlaceInfo::HashTable<PlaceInfo::Gate> PlaceInfoMap_modify= PlaceInfoMap;
            // if(iter_rowheight!=0)
            // {
            //     topo_view<binding_view<klut_network>>(*res).foreach_node([&](auto n){
            //     if (!res->is_ci(n) && !res->is_constant(n)){
            //         if(tend_to_12T["g"+to_string(n)]!=0)
            //         {
            //             PlaceInfoMap_modify["g"+to_string(n)].cell_type=tend_to_12T_name["g"+to_string(n)];
            //             tend_to_12T["g"+to_string(n)]=0;
            //         }
            //     } });
            // }
            
            // row_height_assign(design_name, PlaceInfoMap_modify,PlaceInfo_macro,reassign_minor_rows,
            //                 reassign_major_rows, target, gateHeightAssignmentMap,"iteration",
            //                 chosen_top_K, iter);
            max_delay_iteration=MAXFLOAT;
            New_dp_back_propagation(res, timer, PlaceInfo_net, PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_macro, PlaceInfo_verilog, 
                        lib_gates, gateHeightAssignmentMap, reassign_minor_rows, reassign_major_rows, design_name,def_data,period, phase, slack_threshold);
                        cout<<"iter "<<iter_rowheight<<" end"<<endl;
                        write_verilog_with_binding(*res, "../examples/benchmarks/" + design_name + "/" + design_name +"_new_remap_iter"+to_string(iter_rowheight)+".v");

                        double totaldisplace=0,maxdisplace=0;
                        topo_view<binding_view<klut_network>>(*res).foreach_node([&](auto n){
                            if(res->is_ci(n) || res->is_constant(n))
                            {
                                return;
                            }
                            // cout<<"n "<<n<<endl;
                            treapnode *n_node=treapnode_by_n[std::to_string(n)];
                            treapnode *root=n_node->getroot();
                            int index=treaps::getrank(root,n_node);
                            // cout<<index<<' '<<root->size<<endl;
                            int cluster_x=root->calculate_x1();
                            if(cluster_x<0)cluster_x=0;
                            if(cluster_x+root->sum_w>max_x_length)cluster_x=max_x_length-root->sum_w;
                            // cout<<cluster_x<<endl;
                            treapnode *rt1=nullptr,*rt2=nullptr;
                            treaps::split(root,index,rt1,rt2);
                            // cout<<rt1<<' '<<rt2<<endl;
                            def_data["g"+to_string(n)]["x"]=cluster_x+rt1->sum_w-n_node->width;
                            def_data["g"+to_string(n)]["y"]=n_node->final_y;
                            double nodex=def_data["g"+to_string(n)]["x"];
                            double displacement=(nodex-(n_node->original_x))*(nodex-(n_node->original_x))+(double)((n_node->final_y)-(n_node->original_y))*((n_node->final_y)-(n_node->original_y));
                            if(displacement<0)cout<<"fuck "<<nodex<<' '<<n_node->original_x<<' '<<n_node->final_y<<' '<<n_node->original_y<<' '<<(nodex-n_node->original_x)<<' '<<(nodex-n_node->original_x)*(nodex-n_node->original_x)<<' '<<(n_node->final_y-n_node->original_y)<<' '<<(n_node->final_y-n_node->original_y)*(n_node->final_y-n_node->original_y)<<endl;
                            else
                            {
                                displacement=sqrt(displacement);
                                totaldisplace+=displacement;
                                maxdisplace=max(maxdisplace,displacement);
                            }
                
                            treaps::merge(rt1,rt2);
                        });
                        cout<<"maxdisplacement "<<maxdisplace<<" totaldisplacement "<<totaldisplace<<endl;
                        std:: string output_f_name = "../examples/benchmarks/" + design_name + "/" + design_name + "_def_" + "new" +"_remap_iter"+to_string(iter_rowheight)+".json";
                        std::ofstream modified_file(output_f_name);
                        modified_file << std::setw(4) << def_data << std::endl;
                        modified_file.close();
            // cout<<"maxdelayiter "<<iter_rowheight<<' '<<1750-max_delay_iteration<<endl;
            // if(phase=="tight")
            // {
            //     std::optional<float> tns = timer.report_tns();
            //     std::optional<float> wns = timer.report_wns();
            //     std::optional<float> area = timer.report_area();
            //     std::cout << "tns " << *tns << " wns " << *wns << " area " << *area << "\n";
            // }
        }

    }

    void New_dp_solve(binding_view<klut_network> *res, const std::vector<gate> &lib_gates, std::vector<gate> &gates,
                  binding_cut_type &cuts, HashTable<std::string> &gateHeightAssignmentMap, PlaceInfo::HashTable<PlaceInfo::Macro> PlaceInfo_macro, std::string design_name,
                  PlaceInfo::HashTable<PlaceInfo::Net> &PlaceInfo_net, PlaceInfo::HashTable<std::string> &PlaceInfo_gateToNet,
                  PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap, PlaceInfo::HashTable<std::string> &PlaceInfo_verilog, bool remap_flag, int chosen_top_K, int iter, double _area_slack, int _iteration_rowheight, int density_12T, int density_8T, double period_extra, double slack_rate_init)
    {

        tech_library tech_lib(gates);

        // initialize opentimer object
        ot::Timer timer;
        std::string lib_file = "../examples/benchmarks/NanGate_15nm_OCL_typical_conditional_nldm_8T_12T.lib";
        std::string verilog_file = "../examples/benchmarks/" + design_name + "/" + design_name + ".placed.v";
        std::string spef_file = "../examples/benchmarks/" + design_name + "/" + design_name + ".placed.spef";
        std::string sdc_file = "../examples/benchmarks/" + design_name + "/" + design_name + ".placed.sdc";
        timer.read_celllib(lib_file)
            .read_verilog(verilog_file)
            .read_spef(spef_file)
            .read_sdc(sdc_file)
            .update_timing();
        

        New_init_dp_table(res, lib_gates, gates, cuts, gateHeightAssignmentMap, timer, remap_flag, PlaceInfoMap, PlaceInfo_macro);
        

        std::vector<int> reassign_minor_rows;
        std::vector<int> reassign_major_rows;
        std::pair<std::string, std::string> target;
        
        row_height_assign(design_name, PlaceInfoMap,PlaceInfo_macro,reassign_minor_rows,
                        reassign_major_rows, target, gateHeightAssignmentMap,"baseline",
                        chosen_top_K, iter, density_12T, density_8T);
        for(auto gate:gates)
        {
            name_to_gate[gate.name]=gate;
        }       
        
        

             
        std::ifstream f_def("../examples/benchmarks/" + design_name + "/" + design_name + "_def_cell.json");
        json def_data=json::parse(f_def);
        std::ifstream f_core("../examples/benchmarks/" + design_name + "/" + design_name + "_core.json");
        json core_data=json::parse(f_core);
        max_x_length=core_data["die"]["xh"];
        area_slack=_area_slack;
        // double period = timer.clocks().at("ext_clk_virt").period();
        // New_dp_back_propagation(res, timer, PlaceInfo_net, PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_macro, PlaceInfo_verilog, 
        //                 lib_gates, gateHeightAssignmentMap, reassign_minor_rows, reassign_major_rows, design_name,def_data,period);
        int iter_rowheight=_iteration_rowheight;
        iteration_rowheight(res, timer, PlaceInfo_net, PlaceInfo_gateToNet, PlaceInfoMap, PlaceInfo_macro, PlaceInfo_verilog, 
                        lib_gates, gateHeightAssignmentMap, reassign_minor_rows, reassign_major_rows, design_name,def_data,iter_rowheight,
                        chosen_top_K, iter, period_extra, slack_rate_init);    


        write_verilog_with_binding(*res, "../examples/benchmarks/" + design_name + "/" + design_name +"_new_remap_"+to_string(2)+".v");

        double totaldisplace=0,maxdisplace=0,totalarea=0;
        topo_view<binding_view<klut_network>>(*res).foreach_node([&](auto n){
            if(res->is_ci(n) || res->is_constant(n))
            {
                return;
            }
            // cout<<"n "<<n<<endl;
            treapnode *n_node=treapnode_by_n[std::to_string(n)];
            treapnode *root=n_node->getroot();
            int index=treaps::getrank(root,n_node);
            // cout<<index<<' '<<root->size<<endl;
            int cluster_x=root->calculate_x1();
            if(cluster_x<0)cluster_x=0;
            if(cluster_x+root->sum_w>max_x_length)cluster_x=max_x_length-root->sum_w;
            // cout<<cluster_x<<endl;
            treapnode *rt1=nullptr,*rt2=nullptr;
            treaps::split(root,index,rt1,rt2);
            // cout<<rt1<<' '<<rt2<<endl;
            def_data["g"+to_string(n)]["x"]=cluster_x+rt1->sum_w-n_node->width;
            // if(n_node->final_y==16896)
            // {
            //     cout<<n<<' '<<index<<' '<<cluster_x<<' '<<rt1->sum_w<<' '<<n_node->width<<endl;
            // }
            def_data["g"+to_string(n)]["y"]=n_node->final_y;
            string type=def_data["g"+to_string(n)]["macroName"];
            totalarea+=PlaceInfo_macro[type].area*10000/2000/2000;
            // if(n_node->final_y==512)cout<<"fuck "<<n<<' '<<def_data["g"+to_string(n)]["macroName"]<<endl;
            double nodex=def_data["g"+to_string(n)]["x"];
            double displacement=(nodex-n_node->original_x)*(nodex-n_node->original_x)+(double)(n_node->final_y-n_node->original_y)*(n_node->final_y-n_node->original_y);
            if(displacement<0)cout<<"fuck "<<nodex<<' '<<n_node->original_x<<' '<<n_node->final_y<<' '<<n_node->original_y<<endl;
            else
            {
                displacement=sqrt(displacement);
                totaldisplace+=displacement;
                maxdisplace=max(maxdisplace,displacement);
            }

            treaps::merge(rt1,rt2);
        });
        cout<<"maxdisplacement "<<maxdisplace<<" totaldisplacement "<<totaldisplace<<endl;
        cout<<"total area "<<totalarea<<endl;
        std:: string output_f_name = "../examples/benchmarks/" + design_name + "/" + design_name + "_def_" + "new" +"_remap"+"2"+".json";
        std::ofstream modified_file(output_f_name);
        modified_file << std::setw(4) << def_data << std::endl;
        modified_file.close();
        // std::optional<float> tns = timer.report_tns();
        // std::optional<float> wns = timer.report_wns();
        // std::optional<float> area = timer.report_area();
        // std::cout << "tns " << *tns << " wns " << *wns << " area " << *area << "\n";

    }
}
