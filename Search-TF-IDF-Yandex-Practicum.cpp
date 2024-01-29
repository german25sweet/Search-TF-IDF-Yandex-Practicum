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


enum class DocumentStatus {
	ACTUAL,
	IRRELEVANT, 
	BANNED, 
	REMOVED
};

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

	int GetDocumentCount() const
	{
		return documents_count_;
	}

	void SetStopWords(const string& text) {
		for (const string& word : SplitIntoWords(text)) {
			stop_words_.insert(word);
		}
	}

	void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {

		++documents_count_;

		int rating = ComputeAverageRating(ratings);

		document_ratings_and_status[document_id].raiting = rating;
		document_ratings_and_status[document_id].status = status;

		const vector<string> words = SplitIntoWordsNoStop(document);

		std::map<std::string, int> countMap;

		for (const auto& word : words) {
			countMap[word]++;
		}

		for (const auto& pair : countMap) {

			documents_[pair.first][document_id] = pair.second / static_cast<double>(words.size());
		}
	}

	static int ComputeAverageRating(const vector<int>& ratings) {
		if (ratings.empty()) return 0;
		int sum = std::accumulate(ratings.begin(), ratings.end(), 0);
		if (sum == 0) return 0;
		return sum / static_cast<int>(ratings.size());
	}

	vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {

		auto matched_documents = FindAllDocuments(ParseQuery(raw_query), status);

		vector<Document> matched_documents2;
		matched_documents2.reserve(matched_documents.size());

		for (auto& [document_id, relev] : matched_documents)
		{
			matched_documents2.push_back({ document_id, relev, document_ratings_and_status.at(document_id).raiting });
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

	tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const
	{
		auto query = ParseQuery(raw_query);

		set<string> matched_strings;

		if (!query.minus_words_.count(document_id))
		{
			for (auto& query_word : query.query_words_)
			{
				if (documents_.count(query_word) > 0)
				{
					try {
						if (documents_.at(query_word).at(document_id))
							matched_strings.insert(query_word);
					}

					catch (std::out_of_range ex)
					{
						continue;
					}
				}
			}
		}

		vector <string> matched_words_vector{ matched_strings.begin(), matched_strings.end() };

		return tuple(matched_words_vector, document_ratings_and_status.at(document_id).status);
	}

private:

	struct DocumentAttributes {

		int raiting;
		DocumentStatus status;
	};

	int documents_count_ = 0;

	map<string, map<int, double>> documents_;
	set<string> stop_words_;
	map<int, DocumentAttributes> document_ratings_and_status;

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

	map<int, double> FindAllDocuments(const Query& query, DocumentStatus status) const {
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
				if (!query.minus_words_.count(document_id) && document_ratings_and_status.at(document_id).status == status)
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

		//search_server.AddDocument(document_id, doc, doc_ratings);
	}

	return search_server;
}

void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
	cout << "{ "s
		<< "document_id = "s << document_id << ", "s
		<< "status = "s << static_cast<int>(status) << ", "s
		<< "words ="s;
	for (const string& word : words) {
		cout << ' ' << word;
	}
	cout << "}"s << endl;
}
int main() {
	SearchServer search_server;
	search_server.SetStopWords("и в на"s);
	search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
	search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
	search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
	const int document_count = search_server.GetDocumentCount();
	for (int document_id = 0; document_id < document_count; ++document_id) {
		const auto [words, status] = search_server.MatchDocument("пушистый кот"s, document_id);
		PrintMatchDocumentResult(document_id, words, status);
	}
}