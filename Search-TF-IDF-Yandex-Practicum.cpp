// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь: погуглил, ответ 271

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
};

class SearchServer {

public:

    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {

        ++documents_count_;

        const vector<string> words = SplitIntoWordsNoStop(document);

        std::map<std::string, int> word_count_map;

        for (const auto& word : words) {
            word_count_map[word]++;
        }

        for (const auto& [word, count] : word_count_map) {

            documents_[word][document_id] = count / static_cast<double>(words.size());
        }
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {

        auto matched_documents_map = FindAllDocuments(ParseQuery(raw_query));

        vector<Document> matched_documents_vector;
        matched_documents_vector.reserve(matched_documents_map.size());

        for (auto& [document_id, relevance] : matched_documents_map)
        {
            matched_documents_vector.push_back({ document_id, relevance });
        }

        sort(matched_documents_vector.begin(), matched_documents_vector.end(),
            [](const Document& first_doc, const Document& second_doc) {
                return first_doc.relevance > second_doc.relevance;
            });

        if (static_cast<int>(matched_documents_vector.size()) > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents_vector.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents_vector;
    }

private:

    int documents_count_ = 0;

    map<string, map<int, double>> documents_;
    set<string> stop_words_;

    struct Query {
        set<string> query_words_;
        set<int> minus_words_;
    };

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (![&](const string& word) { return stop_words_.count(word) > 0; } (word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string& text) const {
        set<string> query_words;
        set<int> minus_words;

        for (string& word : SplitIntoWordsNoStop(text)) {
            if (ParseQueryWord(word))
                query_words.insert(word);
            else
            {
                auto it = documents_.find(word.erase(0,1));
                if (it != documents_.end()) {
                    for (auto& [document_id, value] : it->second) {
                        minus_words.insert(document_id);
                    }
                }
            }         
        }

        return { query_words, minus_words };
    }

    bool ParseQueryWord(const string& query_word) const
    {
        return query_word[0] != '-';
    }

    map<int, double> FindAllDocuments(const Query& query) const {
        map<int, double> matched_documents;
        map<string, map<int, double>> relevant_documents;

        for (auto& query_word : query.query_words_)
        {
            if (documents_.count(query_word) > 0)
            {
                relevant_documents[query_word] = documents_.at(query_word);
            }
        }

        for (auto& [query_word, relev_pairs] : relevant_documents)
        {
            for (auto& [document_id, tf_value] : relev_pairs)
            {
                if (!query.minus_words_.count(document_id))
                    matched_documents[document_id] += tf_value * CalculateIdf(static_cast<int>(relev_pairs.size()));
            }
        }
        return matched_documents;
    }

    double CalculateIdf(int documents_with_word_count) const
    {
        return log(static_cast<double>(documents_count_) / documents_with_word_count);
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();

    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {

    const SearchServer search_server = CreateSearchServer();
    const string query = ReadLine();
    const auto search_result = search_server.FindTopDocuments(query);

    for (const auto& [document_id, relevance] : search_result) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }

    return 0;
}
