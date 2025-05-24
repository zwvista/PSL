#include <algorithm>
#include <iostream>
#include <list>

#include "nonconst_bfs.hpp" // so we can modify the graph
#include <boost/graph/astar_search.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/bimap.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
using namespace boost;
using namespace std;

class pstate_t : public vector<int>
{
public:
    int m_r, m_c;
    optional<char> m_mi;
    pstate_t() {}
    pstate_t(int rows, int cols)
        : vector<int>(m_r * m_c), m_r(rows), m_c(cols) {}
    template <class I>
    pstate_t(int rows, int cols, I beg, I end)
        : vector<int>(beg, end), m_r(rows), m_c(cols) {}
    inline int get(int r, int c) const {
        return operator[](cells(r, c));
    }
    inline void move(int i, int j, char dir) {
        int tmp = operator[](i);
        operator[](i) = operator[](j);
        operator[](j) = tmp;
        m_mi = dir;
    }
    // find offset of coordinates
    inline int cells(int r, int c) const {
        return r * m_c + c;
    }
    // find coordinates of an offset
    inline void coords(int i, int &r, int &c) const {
        r = i / m_c;
        c = i % m_c;
    }
};
ostream & operator<<(ostream &out, const pstate_t &p)
{
    if (p.m_mi)
        out << "move: " << *p.m_mi << endl;
    for (int i = 0; i < p.m_r; ++i) {
        for (int j = 0; j < p.m_c; ++j) {
            if (p.get(i, j) > 0)
                out << p.get(i, j) << " ";
            else
                out << "  ";
        }
        out << endl;
    }
    return out;
}

void gen_children(const pstate_t &p, list<pstate_t>& children)
{
    pstate_t::const_iterator i = find(p.begin(), p.end(), 0);
    int sr, sc, soff = i - p.begin();
    p.coords(soff, sr, sc);
    if (sc > 0) { // move tile to left of space
        children.push_back(p);
        children.back().move(soff, p.cells(sr, sc - 1), 'w');
    }
    if (sc < p.m_c - 1) { // move tile to right of space
        children.push_back(p);
        children.back().move(soff, p.cells(sr, sc + 1), 'e');
    }
    if (sr > 0) { // move tile above space
        children.push_back(p);
        children.back().move(soff, p.cells(sr - 1, sc), 'n');
    }
    if (sr < p.m_r - 1) { // move tile below space
        children.push_back(p);
        children.back().move(soff, p.cells(sr + 1, sc), 's');
    }
}

typedef property<vertex_color_t, default_color_type,
    property<vertex_rank_t, unsigned int,
    property<vertex_distance_t, unsigned int,
    property<vertex_predecessor_t, unsigned int> > > > vert_prop;
typedef property<edge_weight_t, unsigned int> edge_prop;
typedef adjacency_list<listS, vecS, undirectedS, vert_prop, edge_prop> mygraph_t;
typedef mygraph_t::vertex_descriptor vertex_t;
typedef mygraph_t::vertex_iterator vertex_iterator_t;
typedef bimap<vertex_t, pstate_t> StateMap;
typedef property_map<mygraph_t, edge_weight_t>::type WeightMap;
typedef property_map<mygraph_t, vertex_predecessor_t>::type PredMap;
typedef property_map<mygraph_t, vertex_distance_t>::type DistMap;

struct found_goal {};
template <class VisitorType>
class puz_visitor : public VisitorType
{
public:
    puz_visitor(pstate_t &goal, list<vertex_t> &seq, StateMap &smap)
        : m_goal(goal), m_seq(seq), m_smap(smap) {}
    template <class Vertex, class Graph>
    void examine_vertex(Vertex u, Graph& g) {
        m_seq.push_back(u);
        VisitorType::examine_vertex(u, g);
        DistMap dmap = get(vertex_distance_t(), g);
        // check for goal
        const pstate_t& cur = m_smap.left.at(u);
        if (cur == m_goal)
            throw found_goal();
        // add successors of this state
        list<pstate_t> children;
        gen_children(cur, children);
        for (pstate_t& child : children) {
            // make sure this state is new
            try{
                vertex_t v = m_smap.right.at(child);
                //add_edge(u, v, edge_prop(1), g);
            } catch(out_of_range&) {
                vertex_t v = add_vertex(vert_prop(white_color), g);
                m_smap.insert(StateMap::relation(v, child));
                dmap[v] = numeric_limits<unsigned int>::max();
                add_edge(u, v, edge_prop(1), g);
            }
        }
    }
private:
    pstate_t &m_goal;
    list<vertex_t> &m_seq;
    StateMap &m_smap;
};

// manhattan distance heuristic
class manhattan_dist
    : public astar_heuristic<mygraph_t, unsigned int>
{
public:
    manhattan_dist(pstate_t &goal, StateMap &smap)
        : m_goal(goal), m_smap(smap) {}
    unsigned int operator()(vertex_t u) {
        unsigned int md = 0;
        pstate_t::const_iterator i, j;
        int ir, ic, jr, jc;
        const pstate_t& cur = m_smap.left.at(u);
        for (i = cur.begin(); i != cur.end(); ++i) {
            j = find(m_goal.begin(), m_goal.end(), *i);
            cur.coords(i - cur.begin(), ir, ic);
            m_goal.coords(j - m_goal.begin(), jr, jc);
            md += myabs(jr - ir) + myabs(jc - ic);
        }
        return md;
    }
private:
    pstate_t &m_goal;
    StateMap &m_smap;
    inline unsigned int myabs(int i) {
        return static_cast<unsigned int>(i < 0 ? -i : i);
    }
};

int main(int argc, char **argv)
{
    mygraph_t g;
    list<vertex_t> examine_seq;
    StateMap smap;
    vertex_t start = add_vertex(vert_prop(white_color), g);
    //int sstart[] = {2, 8, 3, 1, 6, 4, 7, 0, 5}; // 5 steps
    //int sstart[] = {2, 1, 6, 4, 0, 8, 7, 5, 3}; // 18 steps
    //int sgoal[] = {1, 2, 3, 8, 0, 4, 7, 6, 5};
    int sstart[] = {7, 5, 6, 8, 3, 2, 4, 0, 1}; // 23 steps
    int sgoal[] = {1, 2, 3, 4, 5, 6, 7, 8, 0};
    smap.insert(StateMap::relation(start, pstate_t(3, 3, &sstart[0], &sstart[9])));
    pstate_t psgoal(3, 3, &sgoal[0], &sgoal[9]);
    cout << "Start state:" << endl << smap.left.at(start) << endl;
    cout << "Goal state:" << endl << psgoal << endl;
    try {
        puz_visitor<default_astar_visitor> vis(psgoal, examine_seq, smap);
        astar_search(g, start, manhattan_dist(psgoal, smap),
            visitor(vis).color_map(get(vertex_color, g)).
            rank_map(get(vertex_rank, g)).
            distance_map(get(vertex_distance, g)).
            predecessor_map(get(vertex_predecessor, g)));
    } catch(found_goal&) {
        PredMap p = get(vertex_predecessor, g);
        list<vertex_t> shortest_path;
        for (vertex_t v = examine_seq.back();; v = p[v]) {
            shortest_path.push_front(v);
            if (p[v] == v)
                break;
        }
        println("Sequence of moves:");
        for (vertex_t v : shortest_path)
            cout << smap.left.at(v) << endl;
        cout << "Number of moves: "
            << shortest_path.size() - 1 << endl;
        cout << "Number of vertices examined: "
            << examine_seq.size() << endl;
    }
    return 0;
}
