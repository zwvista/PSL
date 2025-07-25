#pragma once

template<typename T>
concept puz_state_move_generator = copyable<T> && requires(const T& t, list<T>& children)
{
    { t.gen_children(children) } -> same_as<void>;
};

template<puz_state_move_generator puz_state, bool directed = true>
class puz_move_generator
{
    using vert_prop = boost::property<boost::vertex_color_t, boost::default_color_type>;
    using directedSOrUndirectedS = boost::mpl::if_c<directed, boost::directedS, boost::undirectedS>::type;
    using mygraph_t = boost::adjacency_list<boost::listS, boost::vecS, directedSOrUndirectedS, vert_prop>;
    using vertex_t = mygraph_t::vertex_descriptor;
    using StateMap = boost::bimap<vertex_t, puz_state>;

    struct puz_visitor : boost::default_bfs_visitor
    {
        puz_visitor(list<vertex_t> &seq, StateMap &smap)
            : m_seq(seq), m_smap(smap) {}
        template <class Vertex, class Graph>
        void examine_vertex(Vertex u, Graph& g) {
            m_seq.push_back(u);
            boost::default_bfs_visitor::examine_vertex(u, g);
            const puz_state& cur = m_smap.left.at(u);
            // add successors of this state
            list<puz_state> children;
            cur.gen_children(children);
            for (puz_state& child : children) {
                try{
                    m_smap.right.at(child);
                } catch (out_of_range&) {
                    vertex_t v = add_vertex(vert_prop(boost::white_color), g);
                    m_smap.insert(typename StateMap::relation(v, child));
                    add_edge(u, v, g);
                }
            }
        }
    private:
        list<vertex_t> &m_seq;
        StateMap &m_smap;
    };

public:
    static list<puz_state> gen_moves(const puz_state& sstart)
    {
        mygraph_t g;
        list<vertex_t> examine_seq;
        StateMap smap;
        vertex_t start = add_vertex(vert_prop(boost::white_color), g);
        smap.insert(typename StateMap::relation(start, sstart));

        boost::breadth_first_search(g, start,
            visitor(puz_visitor(examine_seq, smap)).
            color_map(get(boost::vertex_color, g)));

        list<puz_state> smoves;
        for (vertex_t v : examine_seq)
            smoves.push_back(smap.left.at(v));
        return smoves;
    }
};
