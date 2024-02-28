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
const double EPSILON = 1e-6;

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

	Document() : id(0), relevance(0), rating(0) {}
	Document(int id_, double relevance_, int _rating) : id(id_), relevance(relevance_), rating(_rating) {}
};

class SearchServer {

public:
	SearchServer() {}
	SearchServer(const vector<string>& stop_words) : stop_words_(set<string>(stop_words.begin(), stop_words.end()))
	{}
	SearchServer(const set<string>& stop_words) : stop_words_(stop_words)
	{}
	SearchServer(const string& stop_words) {
		auto words = SplitIntoWords(stop_words);
		stop_words_ = set<string>(words.begin(), words.end());
	}

	int GetDocumentCount() const
	{
		return documents_count_;
	}

	void SetStopWords(const string& text) {
		for (const string& word : SplitIntoWords(text)) {
			stop_words_.insert(word);
		}
	}

	inline static constexpr int INVALID_DOCUMENT_ID = -1;

	[[nodiscard]] bool AddDocument(int document_id, const string& document, DocumentStatus status,
		const vector<int>& ratings) {

		if (document_id < 0 || document_ratings_and_status.count(document_id) || !SearchServer::IsValidWord(document))
			return false;

		++documents_count_;
		documents_id.push_back(document_id);

		int rating = ComputeAverageRating(ratings);

		document_ratings_and_status[document_id].rating = rating;
		document_ratings_and_status[document_id].status = status;

		const vector<string> documentWords = SplitIntoWordsNoStop(document);
		const double wordsCount = 1 / static_cast<double>(documentWords.size());

		map<std::string, int> countMap;

		for (const auto& word : documentWords) {
			documents_[word][document_id] += wordsCount;
		}

		return true;
	}

	static int ComputeAverageRating(const vector<int>& ratings) {
		if (ratings.empty()) {
			return 0;
		}
		int sum = std::accumulate(ratings.begin(), ratings.end(), 0);
		if (sum == 0) {
			return 0;
		}
		return sum / static_cast<int>(ratings.size());
	}

	int GetDocumentId(int index) const {

		if (index > (documents_id.size() - 1))
			return INVALID_DOCUMENT_ID;

		else return documents_id.at(index);
	}

	template<typename T>
	[[nodiscard]] bool FindTopDocuments(const string& raw_query, T filter_func, vector<Document>& result) const {
		Query query;

		if (!ParseQuery(raw_query, query))
			return false;

		auto top_documents = FindAllDocuments(query, filter_func);

		sort(top_documents.begin(), top_documents.end(),
			[](const Document& lhs, const Document& rhs) {
				return (abs(lhs.relevance - rhs.relevance) < EPSILON) ? lhs.rating > rhs.rating : lhs.relevance > rhs.relevance;
			});

		if (static_cast<int>(top_documents.size()) > MAX_RESULT_DOCUMENT_COUNT) {
			top_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
		}

		result = top_documents;
		return true;
	}

	[[nodiscard]] bool FindTopDocuments(const string& raw_query, DocumentStatus documentStatus, vector<Document>& result) const {

		return FindTopDocuments(raw_query, [documentStatus](int document_id, DocumentStatus status, int rating) { return status == documentStatus; }, result);
	}

	[[nodiscard]] bool FindTopDocuments(const string& raw_query, vector<Document>& result) const {

		return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return true; }, result);
	}

	[[nodiscard]] bool MatchDocument(const string& raw_query, int document_id,
		tuple<vector<string>, DocumentStatus>& result) const {
		Query query;

		if (!ParseQuery(raw_query, query))
			return false;

		set<string> matched_strings;

		if (!query.minus_words_.count(document_id)) {
			for (const auto& query_word : query.query_words_) {
				if (documents_.count(query_word) > 0 && documents_.at(query_word).count(document_id)) {
					matched_strings.insert(query_word);
				}
			}
		}
		vector <string> matched_words_vector{ matched_strings.begin(), matched_strings.end() };

		result = tuple(matched_words_vector, document_ratings_and_status.at(document_id).status);

		return true;
	}

private:
	int documents_count_ = 0;

	static bool IsValidWord(const string& word) {
		// A valid word must not contain special characters
		return none_of(word.begin(), word.end(), [](char c) {
			return c >= '\0' && c < ' ';
			});
	}

	map<string, map<int, double>> documents_;
	set<string> stop_words_;

	struct DocumentAttributes {
		int rating;
		DocumentStatus status;
	};

	map<int, DocumentAttributes> document_ratings_and_status;

	vector<int> documents_id;

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

	bool ParseQuery(const string& text, Query& result) const {

		if (!IsValidWord(text))
			return false;

		set<string> query_words;
		set<int> minus_words;

		for (string& word : SplitIntoWordsNoStop(text)) {
			if (word == "-")
				return false;
			if (ParseQueryWord(word)) {
				query_words.insert(word);
			}
			else {
				if (word[1] == '-')
					return false;
				auto it = documents_.find(word.erase(0, 1));
				if (it != documents_.end()) {
					for (const auto& [document_id, value] : it->second) {
						minus_words.insert(document_id);
					}
				}
			}
		}

		result = { query_words, minus_words };

		return true;
	}

	bool ParseQueryWord(const string& query_word) const
	{
		return query_word[0] != '-';
	}

	template<typename T>
	vector<Document> FindAllDocuments(const Query& query, const T& filter_func) const {
		map<int, double> matched_documents;
		map<string, map<int, double>> relevant_documents;

		for (const auto& query_word : query.query_words_) {
			if (documents_.count(query_word)) {
				relevant_documents[query_word] = documents_.at(query_word);
			}
		}

		for (const auto& [query_word, relev_pairs] : relevant_documents) {
			for (const auto& [document_id, tf_value] : relev_pairs) {
				if (!query.minus_words_.count(document_id) && filter_func(document_id, document_ratings_and_status.at(document_id).status, document_ratings_and_status.at(document_id).rating))
					matched_documents[document_id] += tf_value * CalculateIdf(static_cast<int>(relev_pairs.size()));
			}
		}

		vector<Document> top_documents;
		top_documents.reserve(matched_documents.size());

		for (const auto& [document_id, document_relevance] : matched_documents) {
			top_documents.push_back({ document_id, document_relevance, document_ratings_and_status.at(document_id).rating });
		}

		return top_documents;
	}

	double CalculateIdf(int documents_with_word_count) const {
		return log(static_cast<double>(documents_count_) / documents_with_word_count);
	}
};


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
void PrintDocument(const Document& document) {
	cout << "{ "s
		<< "document_id = "s << document.id << ", "s
		<< "relevance = "s << document.relevance << ", "s
		<< "rating = "s << document.rating
		<< " }"s << endl;
}

int main() {
	SearchServer search_server("и в на"s);
	// Явно игнорируем результат метода AddDocument, чтобы избежать предупреждения
	// о неиспользуемом результате его вызова
	(void)search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
	if (!search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 })) {
		cout << "Документ не был добавлен, так как его id совпадает с уже имеющимся"s << endl;
	}
	if (!search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 })) {
		cout << "Документ не был добавлен, так как его id отрицательный"s << endl;
	}
	if (!search_server.AddDocument(3, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, { 1, 3, 2 })) {
		cout << "Документ не был добавлен, так как содержит спецсимволы"s << endl;
	}
	vector<Document> documents;
	if (search_server.FindTopDocuments("--пушистый"s, documents)) {
		for (const Document& document : documents) {
			PrintDocument(document);
		}
	}
	else {
		cout << "Ошибка в поисковом запросе"s << endl;
	}
}
