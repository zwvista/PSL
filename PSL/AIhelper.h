#pragma once

#include "Position.h"

inline unsigned int myabs(int i)
{
	return static_cast<unsigned int>(i < 0 ? -i : i);
}

inline unsigned int manhattan_distance(const Position& p1, const Position& p2)
{
	return myabs(p1.first - p2.first) + myabs(p1.second - p2.second);
}

template<class puz_game>
void load_xml(list<puz_game>& games, const string& fn_in)
{
	using namespace boost::property_tree::xml_parser;
	ptree pt;
	read_xml(fn_in, pt, no_comments | trim_whitespace);
	string str;
	vector<string> vstr;

	for(const ptree::value_type& v : pt.get_child("levels")){
		//if(v.first == "<xmlcomment>") continue;
		const ptree& child = v.second;
		str = child.data();
		boost::split(vstr, str, boost::is_any_of("\\\r\n"), boost::token_compress_on);
		vstr.pop_back();
		vstr.erase(vstr.begin());
		games.push_back(puz_game(child.get_child("<xmlattr>"), vstr, child));
	}
}
