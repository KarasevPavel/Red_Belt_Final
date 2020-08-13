#include "search_server.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>
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

SearchServer::SearchServer(istream& document_input){   ///Optimized
    InvertedIndex new_index;
    for (string current_document; getline(document_input, current_document); ) {
        new_index.Add(move(current_document));
    }
    index = move(new_index);
}

void SearchServer::UpdateDocumentBase(istream& document_input) {  ///Optimized
  future_update_document_base = async([&document_input,this]{UpdateDocumentBaseSingleThread(document_input);});
}

void SearchServer::UpdateDocumentBaseSingleThread(istream &document_input) {
    InvertedIndex new_index;

    for (string current_document; getline(document_input, current_document); ) {
        new_index.Add(move(current_document));
    }
    m.lock();
    index = move(new_index);
    m.unlock();
}

void SearchServer::AddQueriesStream(
  istream& query_input, ostream& search_results_output
) {
    vector<string>queries;
  for (string current_query; getline(query_input, current_query); ) {
      queries.push_back(current_query);
  }

  vector<future<vector<pair<string,vector<pair<size_t,size_t>>>>>> futures;

  auto it = queries.begin();
  for(auto new_it = min(next(queries.begin(),2000),queries.end()); it != queries.end();){
      futures.push_back(move(async([this,it,new_it]{return AddQueriesStreamSingleThread({it,new_it});})));
      it = new_it;
      new_it = min(queries.end(),next(new_it,2000));
  }

      ///////// 5-th block /////////////
      {
          //TimeChecker ti("5th block");
          for(auto& f : futures) {
              for (auto&[query_name, hit_counts] : f.get()) {
                  search_results_output << query_name << ':';
                  for (auto& [docid, hitcount] : hit_counts) {
                      search_results_output << " {"
                                            << "docid: " << docid << ", "
                                            << "hitcount: " << hitcount << '}';
                  }
                  search_results_output << '\n';
              }
          }
      }
  }

vector<pair<string,vector<pair<size_t, size_t>>>> SearchServer::AddQueriesStreamSingleThread(vector<string>&& queries) {

    vector<pair<size_t,size_t>> docid_count;
    vector<pair<string,vector<pair<size_t,size_t>>>>tmp;
    tmp.reserve(queries.size());

    for(const auto& item : queries){
        vector<pair<size_t,size_t>>tmp1;
        tmp1.reserve(5);

        docid_count.clear();
    ////// 2nd block ////////
        {
        //TimeChecker ti("2nd block");

            for (const auto &word : SplitIntoWords(item)) {
                m.lock_shared();
                for (const auto& item1 : index.Lookup(word)) {
                    if(item1.first >= docid_count.size()) {
                        docid_count.resize((item1.first + 1));
                    }
                    docid_count[item1.first].second += item1.second;
                    docid_count[item1.first].first = item1.first;
                }
                m.unlock_shared();
            }
        }
    ///////////// 3-rd block //////////
        {
        //TimeChecker ti("3rd block");
            partial_sort(
                begin(docid_count), min(next(begin(docid_count), 5), end(docid_count)),
                end(docid_count),
                [](const pair<size_t, size_t>& lhs, const pair<size_t, size_t>& rhs) {
                    return (lhs.second > rhs.second) || (lhs.second == rhs.second) && (lhs.first < rhs.first);
                }
            );
        }
        ////////////// 4-th block /////////////
        {
            //TimeChecker ti("4th block");

            for (auto[docid, hitcount] : Head(docid_count, 5)) {
                if (hitcount == 0) {
                    break;
                }
                tmp1.emplace_back(docid, hitcount);
            }
            tmp.emplace_back(item, tmp1);
        }
    }
    return tmp;
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
