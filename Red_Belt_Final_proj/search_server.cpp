#include "search_server.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>
//#include "profile.h"
#include "iterator_range.h"
#include <chrono>

using namespace chrono;

////Documents count = 10000
////Words in a document = 100
////Queries count = 20000
////Words in a query = 10

class TimeChecker{
public:
    explicit TimeChecker(const string& message):
    start(chrono::steady_clock::now()),message_(message){}

    ~TimeChecker(){
        value = steady_clock::now() - start;
        cerr << message_ << "  :" << duration_cast<milliseconds>(value).count() << endl;
    }

private:
    time_point<chrono::steady_clock> start;
    steady_clock::duration value;
    const string& message_;
};

vector<string> SplitIntoWords(const string& line) {   /// optimized
  istringstream words_input(line);
  vector<string>tmp = {make_move_iterator(istream_iterator<string>(words_input)), make_move_iterator(istream_iterator<string>())};
  sort(tmp.begin(),tmp.end());
  return tmp;
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
    int i = 0;
    vector<pair<size_t,size_t>> docid_count(index.GetDocId());
  for (string current_query; getline(query_input, current_query); ) {

      /////// 1st  block ////////
      {
          //TimeChecker ti("1st block" + to_string(i));
          for(size_t j = 0; j < docid_count.size();j++){
              docid_count[j] = {j,0};
          }
      }
      ////// 2nd block ////////
      {
          //TimeChecker ti("2nd block" + to_string(i));
          for (const auto &word : SplitIntoWords(current_query)) {
              for (const auto& item : index.Lookup(word)) {
                  docid_count[item.first].second += item.second;
              }
          }
      }

      ///////////// 3-rd block ///////////

      {
          //TimeChecker ti("3rd block" + to_string(i));
          partial_sort(
                  begin(docid_count), min(next(begin(docid_count), 5), end(docid_count)),
                  end(docid_count),
                  [](const pair<size_t, size_t>& lhs, const pair<size_t, size_t>& rhs) {
                      return (lhs.second > rhs.second) || (lhs.second == rhs.second) && (lhs.first < rhs.first);
                  }
          );
      }

      ///////// 4-th block /////////////
      {
          //TimeChecker ti("4th block" + to_string(i));
          search_results_output << current_query << ':';
          for (auto[docid, hitcount] : Head(docid_count, 5)) {
              if (hitcount == 0)
                  break;
              search_results_output << " {"
                                    << "docid: " << docid << ", "
                                    << "hitcount: " << hitcount << '}';
          }
          search_results_output << '\n';
          i++;
      }
  }
}

size_t InvertedIndex::GetDocId() const {
    return docid;
}

void InvertedIndex::Add(string&& document) {
    string old_word;
  for (const auto& word : SplitIntoWords(document)) {  ///M /// M - number of words in document
        if(word == old_word){
           prev(index[word].end())->second++;
        } else {
           index[word].push_back({docid,1});  ///logN + 0 /// N - number of keys
        }
        old_word = word;
  }
  docid++;
}

const deque<pair<size_t,size_t>>& InvertedIndex::Lookup(const string& word) const {
  if (auto it = index.find(word); it != index.end()) {   ///logN /// N - number of keys
    return it->second;
  } else {
    return empty;
  }
}
