#include "search_server.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>
#include "profile.h"


////Documents count = 10000
////Words in a document = 100
////Queries count = 20000
////Words in a query = 10

vector<string> SplitIntoWords(const string& line) {   /// optimized
  istringstream words_input(line);
  return {istream_iterator<string>(words_input), istream_iterator<string>()};
}

SearchServer::SearchServer(istream& document_input) {   ///Optimized
  UpdateDocumentBase(document_input);
}

void SearchServer::UpdateDocumentBase(istream& document_input) {  ///Optimized
  InvertedIndex new_index;

  for (string current_document; getline(document_input, current_document); ) {
    new_index.Add(move(current_document));
  }
  index = move(new_index);
}

void SearchServer::AddQueriesStream(
  istream& query_input, ostream& search_results_output
) {
  for (string current_query; getline(query_input, current_query); ) {
    const auto words = SplitIntoWords(current_query);

    map<size_t, size_t> docid_count;
    for (const auto& word : words) {
      for (const size_t& docid : index.Lookup(word)) {
        docid_count[docid]++;
      }
    }

    map<size_t,deque<size_t>> search_results;
    for(auto& item : docid_count){
        search_results[item.second].push_back(item.first);
    }

    search_results_output << current_query << ':';
    int i = 0;
    for (auto it = search_results.rbegin(); it != search_results.rend(); it++) {
        for(const auto& item1 : it->second){
      search_results_output << " {"
        << "docid: " << item1 << ", "
        << "hitcount: " << it->first << '}';
        i++;
        if(i >= 5){
            break;
        }
        }
        if(i >= 5){
            break;
        }
    }
    search_results_output << endl;
  }
}

void InvertedIndex::Add(string&& document) {
  for (const auto& word : SplitIntoWords(document)) {  ///M /// M - number of words in document
    index[word].push_back(docid);  ///logN + 0 /// N - number of keys
  }
  docid++;
}

list<size_t> InvertedIndex::Lookup(const string& word) const {
  if (auto it = index.find(word); it != index.end()) {   ///logN /// N - number of keys
    return it->second;
  } else {
    return {};
  }
}
