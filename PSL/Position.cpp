#include "stdafx.h"
#include "Position.h"

using qi::lit;
using qi::int_;
using ascii::space;

void parse_position(const string& str, Position& p)
{
	string::const_iterator first = str.begin();
	qi::phrase_parse(first, str.end(),
		'(' >> int_[phx::ref(p.first) = qi::_1] >> ',' >>
		int_[phx::ref(p.second) = qi::_1] >> ')', space);
}

void parse_positions(const string& str, vector<Position>& vp)
{
	Position p;
	string::const_iterator first = str.begin();
	qi::phrase_parse(first, str.end(),
		*(
			lit('(') >> 
			int_[phx::ref(p.first) = qi::_1] >> ',' >>
			int_[phx::ref(p.second) = qi::_1] >> 
			lit(')')[phx::push_back(phx::ref(vp), phx::ref(p))]
		), space);
}
