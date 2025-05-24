

//
//=======================================================================
// Copyright (c) 2010 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//

#ifndef BOOST_GRAPH_IDASTAR_SEARCH_HPP
#define BOOST_GRAPH_IDASTAR_SEARCH_HPP


#include <functional>
#include <vector>
#include <boost/limits.hpp>
#include <boost/throw_exception.hpp>
#include <boost/graph/named_function_params.hpp>
#include <boost/graph/relax.hpp>
#include <boost/graph/exception.hpp>
#include "nonconst_dfs.hpp"
#include <boost/property_map/property_map.hpp>
#include <boost/property_map/vector_property_map.hpp>


namespace boost {


    template <class Heuristic, class Graph>
    struct IDAStarHeuristicConcept {
        void constraints()
        {
            function_requires< CopyConstructibleConcept<Heuristic> >();
            h(u);
        }
        Heuristic h;
        typename graph_traits<Graph>::vertex_descriptor u;
    };


    template <class Graph, class CostType>
    class idastar_heuristic
    {
    public:
        typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
        idastar_heuristic() {}
        CostType operator()(Vertex u) { return static_cast<CostType>(0); }
    };



    template <class Visitor, class Graph>
    struct IDAStarVisitorConcept {
        void constraints()
        {
            function_requires< CopyConstructibleConcept<Visitor> >();
            vis.initialize_vertex(u, g);
            vis.start_vertex(u, g);
            vis.discover_vertex(u, g);
            vis.examine_edge(e, g);
            vis.edge_relaxed(e, g);
            vis.edge_not_relaxed(e, g);
            vis.forward_or_cross_edge(e, g);
            vis.finish_vertex(u, g);
        }
        Visitor vis;
        Graph g;
        typename graph_traits<Graph>::vertex_descriptor u;
        typename graph_traits<Graph>::edge_descriptor e;
    };


    template <class Visitors = null_visitor>
    class idastar_visitor : public dfs_visitor<Visitors> {
    public:
        idastar_visitor() {}
        idastar_visitor(Visitors vis)
            : dfs_visitor<Visitors>(vis) {}

        template <class Edge, class Graph>
        void edge_relaxed(Edge e, Graph& g) {
            invoke_visitors(this->m_vis, e, g, on_edge_relaxed());
        }
        template <class Edge, class Graph>
        void edge_not_relaxed(Edge e, Graph& g) {
            invoke_visitors(this->m_vis, e, g, on_edge_not_relaxed());
        }
    private:
        template <class Edge, class Graph>
        void tree_edge(Edge e, Graph& g) {}
        template <class Edge, class Graph>
        void back_edge(Edge e, Graph& g) {}
    };
    template <class Visitors>
    idastar_visitor<Visitors>
        make_idastar_visitor(Visitors vis) {
            return idastar_visitor<Visitors>(vis);
    }
    typedef idastar_visitor<> default_idastar_visitor;


    namespace detail {

        template <class IDAStarHeuristic, class UniformCostVisitor,
        class PredecessorMap,
        class DistanceMap, class WeightMap,
        class ColorMap, class BinaryFunction,
        class BinaryPredicate>
        struct idastar_dfs_visitor
        {

            typedef typename property_traits<ColorMap>::value_type ColorValue;
            typedef color_traits<ColorValue> Color;
            typedef typename property_traits<DistanceMap>::value_type D;

            idastar_dfs_visitor(IDAStarHeuristic h, UniformCostVisitor vis,
                PredecessorMap p,
                DistanceMap d, WeightMap w,
                ColorMap col, BinaryFunction combine,
                BinaryPredicate compare, D inf, D zero)
                : m_h(h), m_vis(vis), m_predecessor(p),
                m_distance(d), m_weight(w), m_color(col),
                m_combine(combine), m_compare(compare),
                m_inf(inf), m_zero(zero) {}


            template <class Vertex, class Graph>
            void initialize_vertex(Vertex u, Graph& g) {
                m_vis.initialize_vertex(u, g);
            }
            template <class Vertex, class Graph>
            void start_vertex(Vertex u, Graph& g) {
                m_vis.start_vertex(u, g);
            }
            template <class Edge, class Graph>
            void examine_edge(Edge e, Graph& g) {
                if (m_compare(get(m_weight, e), m_zero))
                    BOOST_THROW_EXCEPTION(negative_edge());
                m_vis.examine_edge(e, g);
            }
            template <class Vertex, class Graph>
            void discover_vertex(Vertex u, Graph& g) {
                m_vis.discover_vertex(u, g);
            }
            template <class Vertex, class Graph>
            void finish_vertex(Vertex u, Graph& g) {
                m_vis.finish_vertex(u, g);
            }

            template <class Edge, class Graph>
            void tree_edge(Edge e, Graph& g) {
                m_decreased = relax(e, g, m_weight, m_predecessor, m_distance,
                    m_combine, m_compare);

                if (m_decreased)
                    m_vis.edge_relaxed(e, g);
                else
                    m_vis.edge_not_relaxed(e, g);
            }


            template <class Edge, class Graph>
            void back_edge(Edge e, Graph& g) {
                //m_decreased = relax(e, g, m_weight, m_predecessor, m_distance,
                // m_combine, m_compare);

                //if(m_decreased)
                // m_vis.edge_relaxed(e, g);
                //else
                // m_vis.edge_not_relaxed(e, g);
            }


            template <class Edge, class Graph>
            void forward_or_cross_edge(Edge e, Graph& g) {
                m_decreased = relax(e, g, m_weight, m_predecessor, m_distance,
                    m_combine, m_compare);

                if (m_decreased) {
                    m_vis.edge_relaxed(e, g);
                    put(m_color, target(e, g), Color::gray());
                    m_vis.forward_or_cross_edge(e, g);
                } else
                    m_vis.edge_not_relaxed(e, g);
            }



            IDAStarHeuristic m_h;
            UniformCostVisitor m_vis;
            PredecessorMap m_predecessor;
            DistanceMap m_distance;
            WeightMap m_weight;
            ColorMap m_color;
            BinaryFunction m_combine;
            BinaryPredicate m_compare;
            bool m_decreased;
            D m_inf;
            D m_zero;
        };

    } // namespace detail


    template<class IDAStarHeuristic, class DistanceMap, 
    class BinaryFunction, class BinaryPredicate>
    struct idastar_dfs_term_func
    {
        typedef typename property_traits<DistanceMap>::value_type D;

        idastar_dfs_term_func(IDAStarHeuristic h, DistanceMap d, 
            BinaryFunction combine, BinaryPredicate compare,
            const D& limit, D& next_limit)
            : m_h(h), m_distance(d),
            m_combine(combine), m_compare(compare),
            m_limit(limit), m_next_limit(next_limit) {}

        template <class Vertex, class Graph>
        bool operator() (Vertex u, Graph& g) {
            D min_d = m_combine(get(m_distance, u), m_h(u));
            if (m_compare(m_limit, min_d)) {
                if (m_compare(min_d, m_next_limit))
                    m_next_limit = min_d;
                return true;
            }
            return false;
        }

        IDAStarHeuristic m_h;
        DistanceMap m_distance;
        BinaryFunction m_combine;
        BinaryPredicate m_compare;
        const D& m_limit;
        D& m_next_limit;
    };


    // Non-named parameter interface
    template <typename VertexListGraph, typename IDAStarHeuristic,
        typename IDAStarVisitor, typename PredecessorMap,
        typename DistanceMap,
        typename WeightMap,
        typename ColorMap,
        typename CompareFunction, typename CombineFunction,
        typename DistanceInf, typename DistanceZero>
        inline void
        idastar_search
        (VertexListGraph &g,
        typename graph_traits<VertexListGraph>::vertex_descriptor s,
        IDAStarHeuristic h, IDAStarVisitor vis,
        PredecessorMap predecessor,
        DistanceMap distance, WeightMap weight,
        ColorMap color,
        CompareFunction compare, CombineFunction combine,
        DistanceInf inf, DistanceZero zero)
    {
        typedef typename property_traits<ColorMap>::value_type ColorValue;
        typedef color_traits<ColorValue> Color;
        detail::idastar_dfs_visitor<IDAStarHeuristic, IDAStarVisitor,
            PredecessorMap, DistanceMap,
            WeightMap, ColorMap, CombineFunction, CompareFunction>
            dfs_vis(h, vis, predecessor, distance, weight,
            color, combine, compare, inf, zero);

        typedef typename property_traits<DistanceMap>::value_type D;
        D limit = h(s), next_limit = inf;

        idastar_dfs_term_func<IDAStarHeuristic, DistanceMap, CombineFunction, CompareFunction> 
            f(h, distance, combine, compare, limit, next_limit);

        for (;;) {
            typename graph_traits<VertexListGraph>::vertex_iterator ui, ui_end;
            for (tie(ui, ui_end) = vertices(g); ui != ui_end; ++ui) {
                put(color, *ui, Color::white());
                put(distance, *ui, inf);
                put(predecessor, *ui, *ui);
                vis.initialize_vertex(*ui, g);
            }
            put(distance, s, zero);

            std::println("{}", limit);

            depth_first_visit(g, s, dfs_vis, color, f);

            if (next_limit == inf) break;

            limit = next_limit, next_limit = inf;
        };

    }



    namespace detail {
        template <class VertexListGraph, class IDAStarHeuristic,
        class DistanceMap, class WeightMap,
        class ColorMap, class Params>
            inline void
            idastar_dispatch2
            (VertexListGraph& g,
            typename graph_traits<VertexListGraph>::vertex_descriptor s,
            IDAStarHeuristic h, DistanceMap distance,
            WeightMap weight, ColorMap color,
            const Params& params)
        {
            dummy_property_map p_map;
            typedef typename property_traits<DistanceMap>::value_type D;
            idastar_search
                (g, s, h,
                choose_param(get_param(params, graph_visitor),
                make_idastar_visitor(null_visitor())),
                choose_param(get_param(params, vertex_predecessor), p_map),
                distance, weight, color,
                choose_param(get_param(params, distance_compare_t()),
                std::less<D>()),
                choose_param(get_param(params, distance_combine_t()),
                closed_plus<D>()),
                choose_param(get_param(params, distance_inf_t()),
                std::numeric_limits<D>::max BOOST_PREVENT_MACRO_SUBSTITUTION ()),
                choose_param(get_param(params, distance_zero_t()),
                D()));
        }

        template <class VertexListGraph, class IDAStarHeuristic,
        class DistanceMap, class WeightMap,
        class IndexMap, class ColorMap, class Params>
            inline void
            idastar_dispatch1
            (VertexListGraph& g,
            typename graph_traits<VertexListGraph>::vertex_descriptor s,
            IDAStarHeuristic h, DistanceMap distance,
            WeightMap weight, IndexMap index_map, ColorMap color,
            const Params& params)
        {
            typedef typename property_traits<WeightMap>::value_type D;

            detail::idastar_dispatch2
                (g, s, h,
                choose_param(distance, vector_property_map<D, IndexMap>(index_map)),
                weight,
                choose_param(color, vector_property_map<default_color_type, IndexMap>(index_map)),
                params);
        }
    } // namespace detail


    // Named parameter interface
    template <typename VertexListGraph,
        typename IDAStarHeuristic,
        typename P, typename T, typename R>
        void
        idastar_search
        (VertexListGraph &g,
        typename graph_traits<VertexListGraph>::vertex_descriptor s,
        IDAStarHeuristic h, const bgl_named_params<P, T, R>& params)
    {

        detail::idastar_dispatch1
            (g, s, h,
            get_param(params, vertex_distance),
            choose_const_pmap(get_param(params, edge_weight), g, edge_weight),
            choose_const_pmap(get_param(params, vertex_index), g, vertex_index),
            get_param(params, vertex_color),
            params);

    }

} // namespace boost

#endif // BOOST_GRAPH_IDASTAR_SEARCH_HPP
