#include "C1402_regex.h"
#include <catch2/catch_test_macros.hpp>
#define CATCH_CONFIG_MAIN
using namespace C1402_regex;
bool match(regex* expression, std::string input_text) {
	char* text = (char*)calloc(1,input_text.size());
	char* start_of_text = text;
	strncpy(text, input_text.c_str(), input_text.size());
	char* end_of_match = NULL;
	do {
		end_of_match = expression->match_here(text);
		if (end_of_match != NULL)
			break;
	} while(*text++ != '\0');
	if (end_of_match != NULL) {
		std::cout << "found match of length:" << (end_of_match - text) << "\n";
		std::cout << "match begins after " << (text - start_of_text) << " chars\n";
		char* matched_text = (char*)calloc(1, end_of_match - text);
		strncpy(matched_text, text, end_of_match-text);
		std::cout << "matched text: [" << std::string(matched_text) << "]\n";
		return true;
	}
	return false; 
	free(start_of_text);
}
TEST_CASE( "test match on a single literal" ) {
	regex* expression = utils::create_from_string("a");
	//
	REQUIRE(match(expression, "qwerty abcd"));
	REQUIRE(match(expression, "a"));
	REQUIRE(match(expression, "b") == false);
	delete expression;
}
TEST_CASE( "test match on a S_ALNUM" ) {
	regex* expression = utils::create_from_string(".");
	//
	REQUIRE(match(expression, "qwerty abcd"));
	REQUIRE(match(expression, "a"));
	REQUIRE(match(expression, "1"));
	REQUIRE(match(expression, "") == false);
	delete expression;
}
TEST_CASE( "test match on a S_ALPHA" ) {
	regex* expression = utils::create_from_string("&");
	//
	REQUIRE(match(expression, "qwerty bcd"));
	REQUIRE(match(expression, "b"));
	REQUIRE(match(expression, "b2"));
	REQUIRE(match(expression, "1") == false);
	REQUIRE(match(expression, "") == false);
	delete expression;
}
TEST_CASE( "test match on a S_DIGIT" ) {
	regex* expression = utils::create_from_string("#");
	//
	REQUIRE(match(expression, "a") == false);
	REQUIRE(match(expression, "qwerty abcd") == false);
	REQUIRE(match(expression, "") == false);
	REQUIRE(match(expression, "qwe2ty bcd"));
	REQUIRE(match(expression, "0"));
	delete expression;
}
TEST_CASE( "test match on a single escaped symbol" ) {
	regex* expression = utils::create_from_string("\\#");
	//
	REQUIRE(match(expression, "a") == false);
	REQUIRE(match(expression, "qwerty abcd") == false);
	REQUIRE(match(expression, "") == false);
	REQUIRE(match(expression, "qwe2ty bcd") == false);
	REQUIRE(match(expression, "0") == false);
	REQUIRE(match(expression, "#"));
	delete expression;
}
//
TEST_CASE( "test match with optional" ) {
	regex* expression = utils::create_from_string("ab?d");
	//
	REQUIRE(match(expression, "abd"));
	REQUIRE(match(expression, "ad"));
	REQUIRE(match(expression, "ab") == false);
	REQUIRE(match(expression, "") == false);
	delete expression;
}
TEST_CASE( "test match with star" ) {
	regex* expression = utils::create_from_string("ab*d");
	//
	REQUIRE(match(expression, "abd"));
	REQUIRE(match(expression, "ad"));
	REQUIRE(match(expression, "ab") == false);
	REQUIRE(match(expression, "") == false);
	REQUIRE(match(expression, "abbbbbbbbbbbbbbd"));
	delete expression;
}
TEST_CASE( "test match with plus" ) {
	regex* expression = utils::create_from_string("ab+d");
	//
	REQUIRE(match(expression, "abd"));
	REQUIRE(match(expression, "ad") == false);
	REQUIRE(match(expression, "ab") == false);
	REQUIRE(match(expression, "") == false);
	REQUIRE(match(expression, "abbbbbbbbbbbbbbd"));
	delete expression;
}
TEST_CASE( "test match with alternate literals in expression" ) {
	regex* expression = utils::create_from_string("a(bc)d");
	//
	REQUIRE(match(expression, "acd"));
	REQUIRE(match(expression, "abd"));
	REQUIRE(match(expression, "acbd") == false);
	REQUIRE(match(expression, "") == false);
	delete expression;
}
TEST_CASE( "test match with alternate star repetitions in expression" ) {
	regex* expression = utils::create_from_string("a(b*c*)d");
	//
	REQUIRE(match(expression, "acd"));
	REQUIRE(match(expression, "abd"));
	REQUIRE(match(expression, "abbbbbbbbbbbbbbbbbbbbbd"));
	REQUIRE(match(expression, "accccccccccccccccccccd"));
	REQUIRE(match(expression, "abcd") == false);
	REQUIRE(match(expression, "ad"));
	REQUIRE(match(expression, "") == false);
	delete expression;
}
TEST_CASE( "test match with alternate plus repetitions in expression" ) {
	regex* expression = utils::create_from_string("a(b+c+)d");
	//
	REQUIRE(match(expression, "acd"));
	REQUIRE(match(expression, "abd"));
	REQUIRE(match(expression, "abbbbbbbbbbbbbbbbbbbbbd"));
	REQUIRE(match(expression, "accccccccccccccccccccd"));
	REQUIRE(match(expression, "abcd") == false);
	REQUIRE(match(expression, "ad") == false);
	REQUIRE(match(expression, "") == false);
	delete expression;
}
