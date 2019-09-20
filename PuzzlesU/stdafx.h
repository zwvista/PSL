#pragma once

#define NOMINMAX
#define _SCL_SECURE_NO_WARNINGS

//#include "targetver.h"

#include <stdio.h>
//#include <tchar.h>

#include <algorithm>
#include <numeric>
#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <array>

#include <boost/config/warning_disable.hpp>
#include "nonconst_bfs.hpp" // so we can modify the graph
#include "nonconst_dfs.hpp" // so we can modify the graph
#include <boost/graph/astar_search.hpp>
#include "idastar_search.hpp"
#include <boost/tuple/tuple.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bimap.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/cxx11/none_of.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/algorithm/cxx11/all_of.hpp>
#include "pugixml.hpp"
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/range/numeric.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <libs/spirit/workbench/high_resolution_timer.hpp>
using namespace std;
namespace qi = boost::spirit::qi;
namespace karma = boost::spirit::karma;
namespace ascii = boost::spirit::ascii;
namespace phx = boost::phoenix;
using namespace phx::arg_names;
using namespace pugi;
using boost::format;
using boost::lexical_cast;