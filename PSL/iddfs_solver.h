#pragma once

#include "stdafx.h"

template<class puz_state, bool directed = true>
class puz_solver_iddfs
{
    using vert_prop = boost::property<boost::vertex_color_t, boost::default_color_type,
        boost::property<boost::vertex_rank_t, unsigned int,
        boost::property<boost::vertex_distance_t, unsigned int,
        boost::property<boost::vertex_predecessor_t, unsigned int> > > >;
    using edge_prop = boost::property<boost::edge_weight_t, unsigned int>;
    using directedSOrUndirectedS = boost::mpl::if_c<directed, boost::directedS, boost::undirectedS>::type;
    using mygraph_t = boost::adjacency_list<boost::listS, boost::vecS, directedSOrUndirectedS, vert_prop, edge_prop>;
    using vertex_t = mygraph_t::vertex_descriptor;
    using StateMap = boost::bimap<vertex_t, puz_state>;
    using PredMap = boost::property_map<mygraph_t, boost::vertex_predecessor_t>::type;
    using DistMap = boost::property_map<mygraph_t, boost::vertex_distance_t>::type;

    struct found_goal {};

    struct puz_visitor : boost::default_dfs_visitor
    {
        puz_visitor(list<vertex_t> &seq, StateMap &smap)
            : m_seq(seq), m_smap(smap) {}
        template <class Vertex, class Graph>
        void examine_vertex(Vertex u, Graph& g) {
            m_seq.push_back(u);
            boost::default_astar_visitor::examine_vertex(u, g);
            DistMap dmap = get(boost::vertex_distance_t(), g);
            PredMap pmap = get(boost::vertex_predecessor_t(), g);
            // check for goal
            const puz_state& cur = m_smap.left.at(u);
            if (cur.is_goal_state())
                throw found_goal();
            // add successors of this state
            list<puz_state> children;
            cur.gen_children(children);
            for (puz_state& child : children) {
                try{
                    vertex_t v = m_smap.right.at(child);
                    unsigned int dist = cur.get_distance(child);
                    if (dmap[u] + dist < dmap[v]) {
                        remove_edge(pmap[v], v, g);
                        add_edge(u, v, edge_prop(dist), g);
                        m_smap.left.replace_data(m_smap.left.find(v), child);
                    }
                } catch (out_of_range&) {
                    vertex_t v = add_vertex(vert_prop(boost::white_color), g);
                    m_smap.insert(StateMap::relation(v, child));
                    dmap[v] = numeric_limits<unsigned int>::max();
                    add_edge(u, v, edge_prop(cur.get_distance(child)), g);
                }
            }
        }
    private:
        list<vertex_t> &m_seq;
        StateMap &m_smap;
    };

    struct puz_heuristic : boost::astar_heuristic<mygraph_t, unsigned int>
    {
        puz_heuristic(StateMap& smap)
            : m_smap(smap) {}
        unsigned int operator()(vertex_t u) {
            return m_smap.left.at(u).get_heuristic();
        }
    private:
        StateMap& m_smap;
    };

public:
    static pair<bool, size_t> find_solution(const puz_state& sstart, list<puz_state>& spath)
    {
        bool found = false;
        mygraph_t g;
        list<vertex_t> examine_seq;
        StateMap smap;
        vertex_t start = add_vertex(vert_prop(boost::white_color), g);
        smap.insert(StateMap::relation(start, sstart));
        try {
            boost::astar_search(g, start, puz_heuristic(smap),
                visitor(puz_visitor(examine_seq, smap)).
                color_map(get(boost::vertex_color, g)).
                rank_map(get(boost::vertex_rank, g)).
                distance_map(get(boost::vertex_distance, g)).
                predecessor_map(get(boost::vertex_predecessor, g)));
        } catch (found_goal&) {
            found = true;
            PredMap p = get(boost::vertex_predecessor, g);
            list<vertex_t> shortest_path;
            for (vertex_t v = examine_seq.back();; v = p[v]) {
                shortest_path.push_front(v);
                if (p[v] == v)
                    break;
            }
            for (vertex_t v : shortest_path)
                spath.push_back(smap.left.at(v));
        }
        return {found, examine_seq.size()};
    }
};
