#pragma once

#include "stdafx.h"

template<class puz_state, bool directed = true, bool complete = false>
class puz_solver_bfs
{
	typedef boost::property<boost::vertex_color_t, boost::default_color_type,
		boost::property<boost::vertex_distance_t, unsigned int,
		boost::property<boost::vertex_predecessor_t, unsigned int> > > vert_prop;
	typedef boost::property<boost::edge_weight_t, unsigned int> edge_prop;
	typedef typename boost::mpl::if_c<directed, boost::directedS, boost::undirectedS>::type directedSOrUndirectedS;
	typedef boost::adjacency_list<boost::listS, boost::vecS, directedSOrUndirectedS, vert_prop, edge_prop> mygraph_t;
	typedef typename mygraph_t::vertex_descriptor vertex_t;
	typedef typename unordered_multimap<vertex_t, unsigned int> MultiPredMap;
	typedef typename boost::bimap<vertex_t, puz_state> StateMap;
	typedef typename boost::property_map<mygraph_t, boost::vertex_predecessor_t>::type PredMap;
	typedef typename boost::property_map<mygraph_t, boost::vertex_distance_t>::type DistMap;

	struct found_goal {};

	struct puz_context
	{
		size_t m_vertices_examined = 0;
		vector<vertex_t> m_goal_vertices;
		unsigned int m_goal_distance = numeric_limits<unsigned int>::max();
		MultiPredMap m_mpmap;
		StateMap m_smap;
	};

	struct puz_visitor : boost::default_bfs_visitor
	{
		puz_visitor(puz_context &context) : m_context(context) {}
		template <class Vertex, class Graph>
		void examine_vertex(Vertex u, Graph& g) {
			++m_context.m_vertices_examined;
			boost::default_bfs_visitor::examine_vertex(u, g);
			DistMap dmap = get(boost::vertex_distance_t(), g);
			PredMap pmap = get(boost::vertex_predecessor_t(), g);
			MultiPredMap &mpmap = m_context.m_mpmap;
			StateMap &smap = m_context.m_smap;
			// check for goal
			const puz_state& cur = smap.left.at(u);
			if(cur.is_goal_state()){
				m_context.m_goal_vertices.push_back(u);
				m_context.m_goal_distance = dmap[u];
				if(!complete)
					throw found_goal();
				return;
			}
			// add successors of this state
			list<puz_state> children;
			cur.gen_children(children);
			for(puz_state& child : children) {
				unsigned int dist = cur.get_distance(child);
				unsigned int new_dist = dmap[u] + dist;
				if(complete && new_dist > m_context.m_goal_distance) continue;
				try{
					vertex_t v = smap.right.at(child);
					if(new_dist < dmap[v]){
						remove_edge(pmap[v], v, g);
						add_edge(u, v, edge_prop(dist), g);
						dmap[v] = new_dist;
						pmap[v] = u;
						smap.left.replace_data(smap.left.find(v), child);
						if(complete){
							mpmap.erase(v);
							mpmap.emplace(v, u);
						}
					}
					else if(complete && new_dist == dmap[v])
						mpmap.emplace(v, u);
				} catch(out_of_range&) {
					vertex_t v = add_vertex(vert_prop(boost::white_color), g);
					add_edge(u, v, edge_prop(dist), g);
					dmap[v] = new_dist;
					pmap[v] = u;
					smap.insert(StateMap::relation(v, child));
					if(complete)
						mpmap.emplace(v, u);
				}
			}
		}
	private:
		puz_context &m_context;
	};

public:
	static pair<bool, size_t> find_solution(const puz_state& sstart, list<list<puz_state>>& spaths)
	{
		mygraph_t g;
		puz_context context;
		vertex_t start = add_vertex(vert_prop(boost::white_color), g);
		DistMap dmap = get(boost::vertex_distance_t(), g);
		PredMap pmap = get(boost::vertex_predecessor_t(), g);
		dmap[start] = 0;
		pmap[start] = start;
		context.m_mpmap.emplace(start, start);
		context.m_smap.insert(typename StateMap::relation(start, sstart));
		try {
			boost::breadth_first_search(g, start,
				visitor(puz_visitor(context)).
				color_map(get(boost::vertex_color, g)).
				distance_map(get(boost::vertex_distance, g)).
				predecessor_map(get(boost::vertex_predecessor, g)));
		} catch(found_goal&) {}
		bool found = !context.m_goal_vertices.empty();
		if(found){
			list<vertex_t> shortest_path;
			if(!complete){
				PredMap p = get(boost::vertex_predecessor, g);
				for(vertex_t v = context.m_goal_vertices.back();; v = p[v]) {
					shortest_path.push_front(v);
					if(p[v] == v)
						break;
				}
				list<puz_state> spath;
				for(vertex_t v : shortest_path)
					spath.push_back(context.m_smap.left.at(v));
				spaths.push_back(spath);
			}
			else{
				vector<vector<vertex_t>> stack;
				stack.push_back(context.m_goal_vertices);
				while(!stack.empty()){
					auto& vs = stack.back();
					if(vs.empty()){
						stack.pop_back();
						if(!shortest_path.empty())
							shortest_path.pop_back();
					}
					else{
						auto v = vs.back();
						vs.pop_back();
						shortest_path.push_back(v);
						auto ret = context.m_mpmap.equal_range(v);
						if(ret.first->second == v){
							list<puz_state> spath;
							BOOST_REVERSE_FOREACH(vertex_t v, shortest_path)
								spath.push_back(context.m_smap.left.at(v));
							spaths.push_back(spath);
							shortest_path.pop_back();
						}
						else{
							vector<vertex_t> vs;
							for(auto it = ret.first; it != ret.second; ++it)
								vs.push_back(it->second);
							stack.push_back(vs);
						}
					}
				}
			}
		}
		return make_pair(found, context.m_vertices_examined);
	}
};
