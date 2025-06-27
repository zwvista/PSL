#pragma once

#include "stdafx.h"

template<typename T>
concept puz_state_solver_dfs = copyable<T> && requires(const T& t, list<T>&children, const T& child, const T& t2)
{
    { t.is_goal_state() } -> same_as<bool>;
    { t.gen_children(children) } -> same_as<void>;
    { t.get_distance(child) } -> same_as<unsigned int>;
    { t < t2 } -> same_as<bool>;
};

template<puz_state_solver_dfs puz_state, bool directed = true>
class puz_solver_dfs
{
    using vert_prop = boost::property<boost::vertex_color_t, boost::default_color_type,
        boost::property<boost::vertex_distance_t, unsigned int,
        boost::property<boost::vertex_predecessor_t, unsigned int> > >;
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
        void discover_vertex(Vertex u, Graph& g) {
            m_seq.push_back(u);
            boost::default_dfs_visitor::discover_vertex(u, g);
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
                unsigned int dist = cur.get_distance(child);
                try{
                    vertex_t v = m_smap.right.at(child);
                    //if(dmap[u] + dist < dmap[v]) {
                    //    remove_edge(pmap[v], v, g);
                    //    add_edge(u, v, edge_prop(dist), g);
                    //    dmap[v] = dmap[u] + dist;
                    //    pmap[v] = u;
                    //    m_smap.left.replace_data(m_smap.left.find(v), child);
                    //}
                } catch (out_of_range&) {
                    vertex_t v = add_vertex(vert_prop(boost::white_color), g);
                    add_edge(u, v, edge_prop(dist), g);
                    dmap[v] = dmap[u] + dist;
                    pmap[v] = u;
                    m_smap.insert(typename StateMap::relation(v, child));
                }
            }
        }
    private:
        list<vertex_t> &m_seq;
        StateMap &m_smap;
    };

public:
    static pair<bool, size_t> find_solution(const puz_state& sstart, list<list<puz_state>>& spaths)
    {
        bool found = false;
        mygraph_t g;
        list<vertex_t> examine_seq;
        StateMap smap;
        vertex_t start = add_vertex(vert_prop(boost::white_color), g);
        DistMap dmap = get(boost::vertex_distance_t(), g);
        PredMap pmap = get(boost::vertex_predecessor_t(), g);
        dmap[start] = 0;
        pmap[start] = start;
        smap.insert(typename StateMap::relation(start, sstart));
        try {
            boost::depth_first_search(g,
                visitor(puz_visitor(examine_seq, smap)).
                color_map(get(boost::vertex_color, g)).
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
            list<puz_state> spath;
            for (vertex_t v : shortest_path)
                spath.push_back(smap.left.at(v));
            spaths.push_back(spath);
        }
        return {found, examine_seq.size()};
    }
};
