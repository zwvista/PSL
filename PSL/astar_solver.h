#pragma once

#include "stdafx.h"

template<class puz_state, bool directed = true>
class puz_solver_astar
{
	typedef boost::property<boost::vertex_color_t, boost::default_color_type,
		boost::property<boost::vertex_rank_t, unsigned int,
		boost::property<boost::vertex_distance_t, unsigned int,
		boost::property<boost::vertex_predecessor_t, unsigned int> > > > vert_prop;
	typedef boost::property<boost::edge_weight_t, unsigned int> edge_prop;
	typedef typename boost::mpl::if_c<directed, boost::directedS, boost::undirectedS>::type directedSOrUndirectedS;
	typedef boost::adjacency_list<boost::listS, boost::vecS, directedSOrUndirectedS, vert_prop, edge_prop> mygraph_t;
	typedef typename mygraph_t::vertex_descriptor vertex_t;
	typedef typename boost::bimap<vertex_t, puz_state> StateMap;
	typedef typename boost::property_map<mygraph_t, boost::vertex_predecessor_t>::type PredMap;
	typedef typename boost::property_map<mygraph_t, boost::vertex_distance_t>::type DistMap;

	struct found_goal {};

	struct puz_visitor : boost::default_astar_visitor
	{
		puz_visitor(pair<size_t, vertex_t> &seq, StateMap &smap)
			: m_seq(seq), m_smap(smap) {}
		template <class Vertex, class Graph>
		void examine_vertex(Vertex u, const Graph& cg) {
			Graph& g = const_cast<Graph&>(cg);
			++m_seq.first, m_seq.second = u;
			boost::default_astar_visitor::examine_vertex(u, g);
			DistMap dmap = get(boost::vertex_distance_t(), g);
			PredMap pmap = get(boost::vertex_predecessor_t(), g);
			// check for goal
			const puz_state& cur = m_smap.left.at(u);
			if(cur.is_goal_state())
				throw found_goal();
			// add successors of this state
			list<puz_state> children;
			cur.gen_children(children);
			for(puz_state& child : children) {
				unsigned int dist = cur.get_distance(child);
				try{
					vertex_t v = m_smap.right.at(child);
					if(dmap[u] + dist < dmap[v]){
						remove_edge(pmap[v], v, g);
						add_edge(u, v, edge_prop(dist), g);
						m_smap.left.replace_data(m_smap.left.find(v), child);
						put(pmap, v, u);
					}
				} catch(out_of_range&) {
					vertex_t v = add_vertex(vert_prop(boost::white_color), g);
					m_smap.insert(StateMap::relation(v, child));
					dmap[v] = numeric_limits<unsigned int>::max();
					add_edge(u, v, edge_prop(dist), g);
				}
			}
		}
	private:
		pair<size_t, vertex_t> &m_seq;
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
		StateMap smap;
		vertex_t start = add_vertex(vert_prop(boost::white_color), g);
		pair<size_t, vertex_t> examine_seq = make_pair(0, start);
		smap.insert(StateMap::relation(start, sstart));
		try {
			boost::astar_search(g, start, puz_heuristic(smap),
				visitor(puz_visitor(examine_seq, smap)).
				color_map(get(boost::vertex_color, g)).
				rank_map(get(boost::vertex_rank, g)).
				distance_map(get(boost::vertex_distance, g)).
				predecessor_map(get(boost::vertex_predecessor, g)));
		} catch(found_goal&) {
			found = true;
			PredMap p = get(boost::vertex_predecessor, g);
			list<vertex_t> shortest_path;
			for(vertex_t v = examine_seq.second;; v = p[v]) {
				shortest_path.push_front(v);
				if(p[v] == v)
					break;
			}
			for(vertex_t v : shortest_path)
				spath.push_back(smap.left.at(v));
		}
		return make_pair(found, examine_seq.first);
	}
};
