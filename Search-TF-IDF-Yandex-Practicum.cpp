#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>

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
	cin.ignore();
	return result;
}

vector<int> ReadLineWithRatings() {
	int times;
	cin >> times;
	vector<int> ratings;
	int result = 0;

	for (int i = 0; i < times; ++i) {
		std::cin >> result;
		ratings.push_back(result);

	}
	cin.ignore();
	return ratings;
}

int ComputeAverageRating(const vector<int>& ratings) {

	if (ratings.empty())
	{
		return 0;
	}

	int sum = std::accumulate(ratings.begin(), ratings.end(), 0);

	if (sum == 0)
	{
		return 0;
	}

	return sum / static_cast<int>(ratings.size());
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
	int rating;
};

class SearchServer {

public:

	void SetStopWords(const string& text) {
		for (const string& word : SplitIntoWords(text)) {
			stop_words_.insert(word);
		}
	}

	void AddDocument(int document_id, const string& document, const vector<int>& ratings) {

		++documents_count_;

		int rating = ComputeAverageRating(ratings);

		document_ratings_[document_id] = rating;

		const vector<string> words = SplitIntoWordsNoStop(document);

		std::map<std::string, int> countMap;

		for (const auto& word : words) {
			countMap[word]++;
		}

		for (const auto& pair : countMap) {

			documents_[pair.first][document_id] = pair.second / static_cast<double>(words.size());
		}
	}

	vector<Document> FindTopDocuments(const string& raw_query) const {

		auto matched_documents = FindAllDocuments(ParseQuery(raw_query));

		vector<Document> matched_documents2;
		matched_documents2.reserve(matched_documents.size());

		for (auto& [document_id, relev] : matched_documents)
		{
			matched_documents2.push_back({ document_id, relev, document_ratings_.at(document_id) });
		}

		sort(matched_documents2.begin(), matched_documents2.end(),
			[](const Document& first_doc, const Document& second_doc) {
				return first_doc.relevance > second_doc.relevance;
			});

		if (static_cast<int>(matched_documents2.size()) > MAX_RESULT_DOCUMENT_COUNT) {
			matched_documents2.resize(MAX_RESULT_DOCUMENT_COUNT);
		}

		return matched_documents2;
	}

private:

	int documents_count_ = 0;

	map<string, map<int, double>> documents_;
	set<string> stop_words_;
	map<int, int> document_ratings_;

	struct Query {
		set<string> query_words_;
		set<int> minus_words_;
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
		set<int> minus_words;

		for (string& word : SplitIntoWordsNoStop(text)) {
			if (ParseQueryWord(word))
				query_words.insert(word);
			else
			{
				auto it = documents_.find(word.erase(0, 1));
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

		auto doc = ReadLine();
		auto doc_ratings = ReadLineWithRatings();

		search_server.AddDocument(document_id, doc, doc_ratings);
	}

	return search_server;
}

int main() {

	const SearchServer search_server = CreateSearchServer();

	const string query = ReadLine();

	const auto search_result = search_server.FindTopDocuments(query);

	for (const auto& [document_id, relevance, rating] : search_result) {
		cout << "{ document_id = "s << document_id << ", "
			<< "relevance = "s << relevance << ", " << "rating = "s << rating << " }"s << endl;
	}

	return 0;
}