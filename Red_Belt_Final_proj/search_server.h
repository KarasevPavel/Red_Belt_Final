#pragma once

#include <istream>
#include <ostream>
#include <set>
#include <list>
#include <vector>
#include <map>
#include <string>
#include <deque>
using namespace std;

class InvertedIndex {
public:
  void Add(string&& document);
  const deque<pair<size_t,size_t>>& Lookup(const string& word) const;
  size_t GetDocId() const;

private:
  map<string, deque<pair<size_t,size_t>>> index;
  deque<pair<size_t,size_t>> empty;
  size_t docid = 0;
};

class SearchServer {
public:
  SearchServer() = default;
  explicit SearchServer(istream& document_input);
  void UpdateDocumentBase(istream& document_input);
  void AddQueriesStream(istream& query_input, ostream& search_results_output);

private:
  InvertedIndex index;
};
