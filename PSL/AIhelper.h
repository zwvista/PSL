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
    xml_document doc;
    doc.load_file(fn_in.c_str());
    string str;
    vector<string> vstr;

    for (xml_node v : doc.child("puzzle").child("levels").children()) {
        str = v.text().as_string();
        boost::split(vstr, str, boost::is_any_of("`\r\n"), boost::token_compress_on);
        if (!str.empty()) {
            vstr.pop_back();
            vstr.erase(vstr.begin());
        }
        games.push_back(puz_game(vstr, v));
    }
}
