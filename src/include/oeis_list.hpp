#pragma once

#include <number.hpp>

#include <string>
#include <unordered_map>
#include <unordered_set>

class OeisList
{
public:

  static const std::string& getListsHome();

  static void loadList( const std::string& path, std::unordered_set<size_t>& list );

  static void loadMap( const std::string& path, std::unordered_map<size_t, size_t>& map );

private:

  static std::string LISTS_HOME;

};
