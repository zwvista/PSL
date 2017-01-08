#pragma once

#include "stdafx.h"

template<class puz_state, bool directed = true, bool shortest_paths_only = true, bool goal_states_only = true>
class puz_solver_dijkstra
{
    typedef boost::property<boost::vertex_color_t, boost::default_color_type,
        boost::property<boost::vertex_rank_t, unsigned int,
        boost::property<boost::vertex_distance_t, unsigned int,
        boost::property<boost::vertex_predecessor_t, unsigned int> > > > vert_prop;
    typedef boost::property<boost::edge_weight_t, unsigned int> edge_prop;
    typedef typename boost::mpl::if_c<directed, boost::directedS, boost::undirectedS>::type directedSOrUndirectedS;
    typedef boost::adjacency_list<boost::listS, boost::vecS, directedSOrUndirectedS, vert_prop, edge_prop> mygraph_t;
    typedef typename mygraph_t::vertex_descriptor vertex_t;
    typedef typename unordered_multimap<vertex_t, unsigned int> MultiPredMap;
    typedef typename boost::bimap<vertex_t, puz_state> StateMap;
    typedef typename boost::property_map<mygraph_t, boost::vertex_predecessor_t>::type PredMap;
    typedef typename boost::property_map<mygraph_t, boost::vertex_distance_t>::type DistMap;

    static const bool first_solution_only = shortest_paths_only && goal_states_only;

    struct found_goal {};

    struct puz_context
    {
        size_t m_vertices_examined = 0;
        vector<vertex_t> m_goal_vertices;
        unsigned int m_goal_distance = numeric_limits<unsigned int>::max();
        MultiPredMap m_mpmap;
        StateMap m_smap;
    };

    struct puz_visitor : boost::default_astar_visitor
    {
        puz_visitor(puz_context &context) : m_context(context) {}
        template <class Vertex, class Graph>
        void examine_vertex(Vertex u, const Graph& cg) {
            Graph& g = const_cast<Graph&>(cg);
            ++m_context.m_vertices_examined;
            boost::default_astar_visitor::examine_vertex(u, g);
            DistMap dmap = get(boost::vertex_distance_t(), g);
            PredMap pmap = get(boost::vertex_predecessor_t(), g);
            MultiPredMap &mpmap = m_context.m_mpmap;
            StateMap &smap = m_context.m_smap;
            // check for goal
            const puz_state& cur = smap.left.at(u);
            if(cur.is_goal_state()){
                m_context.m_goal_vertices.push_back(u);
                m_context.m_goal_distance = std::min(m_context.m_goal_distance, dmap[u]);
                if(first_solution_only)
                    throw found_goal();
                return;
            }
            // add successors of this state
            list<puz_state> children;
            cur.gen_children(children);
            for(puz_state& child : children) {
                unsigned int dist = cur.get_distance(child);
                unsigned int new_dist = dmap[u] + dist;
                if(shortest_paths_only && new_dist > m_context.m_goal_distance) continue;
                try{
                    vertex_t v = smap.right.at(child);
                    if(new_dist < dmap[v]){
                        remove_edge(pmap[v], v, g);
                        add_edge(u, v, edge_prop(dist), g);
                        smap.left.replace_data(smap.left.find(v), child);
                        pmap[v] = u;
                        if(!first_solution_only){
                            mpmap.erase(v);
                            mpmap.emplace(v, u);
                        }
                    }
                    else if(!first_solution_only && new_dist == dmap[v])
                        mpmap.emplace(v, u);
                } catch(out_of_range&) {
                    vertex_t v = add_vertex(vert_prop(boost::white_color), g);
                    smap.insert(typename StateMap::relation(v, child));
                    dmap[v] = numeric_limits<unsigned int>::max();
                    add_edge(u, v, edge_prop(dist), g);
                    if(!first_solution_only)
                        mpmap.emplace(v, u);
                }
            }
        }
    private:
        puz_context &m_context;
    };

    struct puz_heuristic : boost::astar_heuristic<mygraph_t, unsigned int>
    {
        puz_heuristic(StateMap& smap)
            : m_smap(smap) {}
        unsigned int operator()(vertex_t u) {
            return 0;
        }
    private:
        StateMap& m_smap;
    };

public:
    static pair<bool, size_t> find_solution(const puz_state& sstart, list<list<puz_state>>& state_paths)
    {
        mygraph_t g;
        puz_context context;
        vertex_t start = add_vertex(vert_prop(boost::white_color), g);
        context.m_mpmap.emplace(start, start);
        context.m_smap.insert(typename StateMap::relation(start, sstart));
        try{
            boost::astar_search(g, start, puz_heuristic(context.m_smap),
                visitor(puz_visitor(context)).
                color_map(get(boost::vertex_color, g)).
                rank_map(get(boost::vertex_rank, g)).
                distance_map(get(boost::vertex_distance, g)).
                predecessor_map(get(boost::vertex_predecessor, g)));
        } catch(found_goal&) {}
        bool found = !context.m_goal_vertices.empty();
        if(found){
            list<vertex_t> vertex_path;
            if(goal_states_only){
                PredMap p = get(boost::vertex_predecessor, g);
                for(vertex_t v : context.m_goal_vertices){
                    vertex_path.clear();
                    for(;;){
                        vertex_path.push_front(v);
                        if(p[v] == v) break;
                        v = p[v];
                    }
                    list<puz_state> state_path;
                    for(vertex_t v : vertex_path)
                        state_path.push_back(context.m_smap.left.at(v));
                    state_paths.push_back(state_path);
                }
            }
            else{
                vector<vector<vertex_t>> stack;
                stack.push_back(context.m_goal_vertices);
                while(!stack.empty()){
                    auto& vs = stack.back();
                    if(vs.empty()){
                        stack.pop_back();
                        if(!vertex_path.empty())
                            vertex_path.pop_front();
                    }
                    else{
                        auto v = vs.front();
                        vs.erase(vs.begin());
                        vertex_path.push_front(v);
                        auto ret = context.m_mpmap.equal_range(v);
                        if(ret.first->second == v){
                            list<puz_state> state_path;
                            for(vertex_t v2 : vertex_path)
                                state_path.push_back(context.m_smap.left.at(v2));
                            state_paths.push_back(state_path);
                            vertex_path.pop_front();
                        }
                        else{
                            vector<vertex_t> vs2;
                            for(auto it = ret.first; it != ret.second; ++it)
                                vs2.push_back(it->second);
                            stack.push_back(vs2);
                        }
                    }
                }
            }
        }
        return make_pair(found, context.m_vertices_examined);
    }
};
