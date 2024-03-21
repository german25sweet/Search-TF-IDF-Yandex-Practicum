#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <stdexcept>

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
	explicit SearchServer() {}

	template <typename StringContainer>
	explicit SearchServer(const StringContainer& stop_words) {
		SetStopWords(stop_words);
	}

	explicit SearchServer(const string& stop_words) {
		SetStopWords(SplitIntoWords(stop_words));
	}

	int GetDocumentCount() const
	{
		return static_cast<int>(documents_id.size());
	}

	void AddDocument(int document_id, const string& document, DocumentStatus status,
		const vector<int>& ratings) {
		if (document_id < 0) {
			throw invalid_argument("id("s + to_string(document_id) + ") меньше 0 "s);
		}
		if (document_ratings_and_status.count(document_id)) {
			throw invalid_argument("Документ с id - "s + to_string(document_id) + "уже был добавлен");
		}
		if (!IsValidWord(document)) {
			throw invalid_argument(" Наличие недопустимых символов в тексте добавляемого документа с id = " + to_string(document_id));
		}

		documents_id.push_back(document_id);

		int rating = ComputeAverageRating(ratings);

		document_ratings_and_status[document_id] = { rating, status };

		const vector<string> documentWords = SplitIntoWordsNoStop(document);
		const double words_count = 1 / static_cast<double>(documentWords.size());

		map<std::string, int> countMap;

		for (const auto& word : documentWords) {
			documents_[word][document_id] += words_count;
		}
	}

	static int ComputeAverageRating(const vector<int>& ratings) {
		if (ratings.empty()) {
			return 0;
		}
		int sum = std::accumulate(ratings.begin(), ratings.end(), 0);
		return sum / static_cast<int>(ratings.size());
	}

	int GetDocumentId(int index) const {
		return documents_id.at(index);
	}

	template<typename Predicate>
	vector<Document> FindTopDocuments(const string& raw_query, Predicate filter_func) const {
		auto query = ParseQuery(raw_query);
		auto top_documents = FindAllDocuments(query, filter_func);

		sort(top_documents.begin(), top_documents.end(),
			[](const Document& lhs, const Document& rhs) {
				return (abs(lhs.relevance - rhs.relevance) < EPSILON) ? lhs.rating > rhs.rating : lhs.relevance > rhs.relevance;
			});

		if (static_cast<int>(top_documents.size()) > MAX_RESULT_DOCUMENT_COUNT) {
			top_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
		}

		return top_documents;
	}

	vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus document_status = DocumentStatus::ACTUAL) const {
		return FindTopDocuments(raw_query, [document_status](int document_id, DocumentStatus status, int rating) { return status == document_status; });
	}

	tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
		auto query = ParseQuery(raw_query);

		set<string> matched_strings;

		if (!query.minus_words_.count(document_id)) {
			for (const auto& query_word : query.query_words_) {
				if (documents_.count(query_word) > 0 && documents_.at(query_word).count(document_id)) {
					matched_strings.insert(query_word);
				}
			}
		}
		vector <string> matched_words_vector{ matched_strings.begin(), matched_strings.end() };

		return { { matched_words_vector}, {document_ratings_and_status.at(document_id).status  } };
	}

private:

	struct DocumentAttributes {
		int rating;
		DocumentStatus status;
	};

	set<string> stop_words_;
	vector<int> documents_id;
	map<string, map<int, double>> documents_;
	map<int, DocumentAttributes> document_ratings_and_status;

	struct Query {
		set<string> query_words_;
		set<int> minus_words_;
	};

	template <typename StringContainer>
	void SetStopWords(const StringContainer& stop_words) {
		if (!IsValidWords(stop_words)) {
			throw invalid_argument("Стоп слово содержит недопустимые символы");
		}
		stop_words_ = set<string>{ stop_words.begin(),stop_words.end() };
	}

	static bool IsValidWord(const string& word) {
		return none_of(word.begin(), word.end(), [](char c) {
			return c >= '\0' && c < ' ';
			});
	}

	template<typename WordContainer>
	static bool IsValidWords(const WordContainer& words) {
		return all_of(words.begin(), words.end(), IsValidWord);
	}

	vector<string> SplitIntoWordsNoStop(const string& text) const {
		vector<string> words;
		for (const string& word : SplitIntoWords(text)) {
			if (!stop_words_.count(word)) {
				words.push_back(word);
			}
		}
		return words;
	}

	struct QueryWord {
		string word;
		bool is_stop = false;
		bool is_minus = false;
	};

	Query ParseQuery(const string& text) const {
		if (!IsValidWord(text)) {
			throw invalid_argument("В словах поискового запроса есть недопустимые символы");
		}
		set<string> query_words;
		set<int> minus_words;

		for (const auto& raw_query_word : SplitIntoWords(text)) {
			auto query = ParseQueryWord(raw_query_word);

			if (query.is_stop) {
				if (!stop_words_.count(query.word)) {
					query_words.insert(query.word);
				}
			}
			else if (query.is_minus) {
				if (auto it = documents_.find(query.word); it != documents_.end() && !stop_words_.count(query.word)) {
					for (const auto& [document_id, value] : it->second) {
						minus_words.insert(document_id);
					}
				}
			}
		}
		return { query_words,minus_words };
	}


	QueryWord ParseQueryWord(const string& raw_query_word) const {
		if (raw_query_word == "-") {
			throw invalid_argument("Отсутствие текста после символа «минус» в поисковом запросе");
		}
		if (raw_query_word[0] != '-') {
			return { raw_query_word, true, false };
		}
		else {
			if (raw_query_word[1] == '-') {
				throw invalid_argument("Наличие более чем одного минуса у минус слов поискового запроса");
			}
			return { raw_query_word.substr(1), false, true };
		}
	}

	template<typename Predicate>
	vector<Document> FindAllDocuments(const Query& query, const Predicate& filter_func) const {
		map<int, double> matched_documents;
		map<string, map<int, double>> relevant_documents;

		for (const auto& query_word : query.query_words_) {
			if (documents_.count(query_word)) {
				relevant_documents[query_word] = documents_.at(query_word);
			}
		}

		for (const auto& [query_word, relev_pairs] : relevant_documents) {
			double idf = CalculateIdf(query_word);
			for (const auto& [document_id, tf_value] : relev_pairs) {
				const auto& [rating, status] = document_ratings_and_status.at(document_id);
				if (!query.minus_words_.count(document_id) && filter_func(document_id, status, rating))
					matched_documents[document_id] += tf_value * idf;
			}
		}

		vector<Document> top_documents;
		top_documents.reserve(matched_documents.size());

		for (const auto& [document_id, document_relevance] : matched_documents) {
			top_documents.push_back({ document_id, document_relevance, document_ratings_and_status.at(document_id).rating });
		}

		return top_documents;
	}

	double CalculateIdf(const string& word) const {
		return log(static_cast<double>(GetDocumentCount()) / static_cast<int>(documents_.at(word).size()));
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
	setlocale(LC_ALL, "Russian");

	try {
		//SearchServer search_server("и \x0A в на"s);
		SearchServer search_server;
		search_server.GetDocumentId(20);
		search_server.FindTopDocuments("--прив");
	}

	catch (invalid_argument& ex) {
		cout << "Ошибка - " << ex.what() << endl;
	}
	catch (out_of_range& ex) {
		cout << "Ошибка - " << ex.what() << endl;
	}

	try {
		SearchServer search_server("и в на"s);
		search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
		search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
		search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
		cout << "ACTUAL by default:"s << endl;
		for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
			PrintDocument(document);
		}
		cout << "BANNED:"s << endl;
		for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
			PrintDocument(document);
		}
		cout << "Even ids:"s << endl;
		for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
			PrintDocument(document);
		}
	}

	catch (invalid_argument& ex) {
		cout << "Ошибка - " << ex.what() << endl;
	}
	catch (out_of_range& ex) {
		cout << "Ошибка - " << ex.what() << endl;
	}
}
