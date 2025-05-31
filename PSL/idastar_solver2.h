#pragma once

#include "stdafx.h"

template<class puz_state>
class puz_solver_idastar2
{
    typedef boost::property<boost::vertex_color_t, boost::default_color_type,
        boost::property<boost::vertex_distance_t, unsigned int,
        boost::property<boost::vertex_predecessor_t, unsigned int> > > vert_prop;
    typedef boost::property<boost::edge_weight_t, unsigned int> edge_prop;
    typedef typename boost::mpl::if_c<true, boost::directedS, boost::undirectedS>::type directedSOrUndirectedS;
    typedef boost::adjacency_list<boost::listS, boost::vecS, directedSOrUndirectedS, vert_prop, edge_prop> mygraph_t;
    typedef typename mygraph_t::vertex_descriptor vertex_t;
    typedef typename mygraph_t::vertex_iterator vertex_iterator_t;
    typedef map<vertex_t, puz_state> StateMap;
    typedef set<puz_state> StateSet;
    typedef typename boost::property_map<mygraph_t, boost::vertex_predecessor_t>::type PredMap;
    typedef typename boost::property_map<mygraph_t, boost::vertex_distance_t>::type DistMap;

    struct found_goal {};

    struct puz_visitor : boost::default_idastar_visitor
    {
        puz_visitor(pair<size_t, vertex_t> &seq, StateMap &smap)
            : m_seq(seq), m_smap(smap) {}
        template <class Vertex, class Graph>
        void discover_vertex(Vertex u, Graph& g) {
            ++m_seq.first, m_seq.second = u;
            boost::default_idastar_visitor::discover_vertex(u, g);
            DistMap dmap = get(boost::vertex_distance_t(), g);
            PredMap pmap = get(boost::vertex_predecessor_t(), g);
            // check for goal
            const puz_state& cur = m_smap.at(u);
            m_sset.insert(cur);
            if (cur.is_goal_state())
                throw found_goal();
            // add successors of this state
            list<puz_state> children;
            cur.gen_children(children);
            for (puz_state& child : children) {
                // full cycle checking
                if (m_sset.count(child) != 0)
                    continue;
                unsigned int dist = cur.get_distance(child);
                vertex_t v = add_vertex(vert_prop(boost::white_color), g);
                m_smap.emplace(v, child);
                dmap[v] = numeric_limits<unsigned int>::max();
                add_edge(u, v, edge_prop(dist), g);
            }
        }
        template <class Vertex, class Graph>
        void finish_vertex(Vertex u, Graph& g) {
            typedef typename boost::graph_traits<Graph>::out_edge_iterator Iter;
            Iter ei, ei_end;
            vector<Vertex> vv;
            for (std::tie(ei, ei_end) = out_edges(u, g); ei != ei_end; ++ei) {
                Vertex v = target(*ei, g);
                vv.push_back(v);
            }
            BOOST_REVERSE_FOREACH(Vertex v, vv) {
                m_smap.erase(v);
                remove_edge(u, v, g);
                remove_vertex(v, g);
            }
            m_sset.erase(m_smap.at(u));
        }
    private:
        pair<size_t, vertex_t> &m_seq;
        StateMap &m_smap;
        StateSet m_sset;
    };

    struct puz_heuristic : boost::idastar_heuristic<mygraph_t, unsigned int>
    {
        puz_heuristic(StateMap& smap)
            : m_smap(smap) {}
        unsigned int operator()(vertex_t u) {
            return m_smap.at(u).get_heuristic();
        }
    private:
        StateMap& m_smap;
    };

public:
    static pair<bool, size_t> find_solution(const puz_state& sstart, list<list<puz_state>>& spaths)
    {
        bool found = false;
        mygraph_t g;
        StateMap smap;
        vertex_t start = add_vertex(vert_prop(boost::white_color), g);
        pair<size_t, vertex_t> examine_seq = {0, start};
        smap.emplace(start, sstart);
        try {
            boost::idastar_search(g, start, puz_heuristic(smap),
                visitor(puz_visitor(examine_seq, smap)).
                color_map(get(boost::vertex_color, g)).
                distance_map(get(boost::vertex_distance, g)).
                predecessor_map(get(boost::vertex_predecessor, g)));
        } catch(found_goal&) {
            found = true;
            PredMap p = get(boost::vertex_predecessor, g);
            list<vertex_t> shortest_path;
            for (vertex_t v = examine_seq.second;; v = p[v]) {
                shortest_path.push_front(v);
                if (p[v] == v)
                    break;
            }
            list<puz_state> spath;
            for (vertex_t v : shortest_path)
                spath.push_back(smap.at(v));
            spaths.push_back(spath);
        }
        return {found, examine_seq.first};
    }
};
