#include <algorithm>
#include <iostream>
#include <set>
#include <map>
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
    int relevance;
};

class SearchServer {

public:

    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {

        const vector<string> words = SplitIntoWordsNoStop(document);

        for (const auto& word : words)
        {
            documents_[word].insert(document_id);
        }

        ++document_count_;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {

        auto matched_documents = FindAllDocuments(ParseQuery(raw_query));

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:

    int document_count_ = 0;

    map<string, set<int>> documents_;
    set<string> stop_words_;

    struct Query {
        set<string> query_words_;
        set<string> minus_words_;
    };

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (![&](const string& word) { return stop_words_.count(word) > 0; }(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string& text) const {
        set<string> query_words;
        set<string> minus_words;

        for (string& word : SplitIntoWordsNoStop(text)) {
            if (word[0] != '-')
                query_words.insert(word);
            else
                minus_words.insert(word.erase(0, 1));
        }
        return { query_words, minus_words };
    }

    vector<Document> FindAllDocuments(const Query& query) const {
        vector<Document> matched_documents;

        map<int, int> doc_relev;
        for (const auto& [word, documents] : documents_) {

            if (query.query_words_.count(word))
            {
                for (auto document : documents)
                {
                    doc_relev[document]++;
                }
            }
        }

        for (const auto& [word, documents] : documents_) {

            if (query.minus_words_.count(word))
            {
                for (auto document : documents)
                {
                    doc_relev[document] = 0;
                }
            }
        }

        for (auto& doc_pair : doc_relev)
        {
            if (doc_pair.second > 0) {
                matched_documents.push_back({ doc_pair.first, doc_pair.second });
            }
        }
        return matched_documents;
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

    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
}