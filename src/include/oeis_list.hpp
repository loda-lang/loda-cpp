#pragma once

#include <number.hpp>

#include <string>
#include <unordered_map>
#include <unordered_set>

class OeisList
{
public:

  static void loadList( const std::string& path, std::unordered_set<size_t>& list );

  static void loadMap( const std::string& path, std::unordered_map<size_t, size_t>& map );

};
