#include <cctype>
#include <stdexcept>
#include <string.h>
#include <iostream>
#include <string>
#include <catch2/catch_test_macros.hpp>
/* 1."abc" -> 	acceptsthe literal string "abc"
 * 2."a?bc" ->	accepts "abc" or "bc".
 * 3."ab*c" -> 	accepts "ac", "abc", "abbbbbbbbbbbbbbbbc"
 * 4."ab+c" -> 	accepts "abc", "abbbbbbbbbbbbbbc"
 * 5."ab(CD)" -> 	accepts "abC", "abD"
 * 6."ab(C*D*)"->	accepts "ab", "abC", "abD", "abCCCCCCCC", "abDDDDDD"
 * 7."ab(CD)*" -> acceptance to be determined when compilation/parsing code is rewritten. (Possibly make equivalent to input string 6? Pre-processing or somesuch method)
 * 8. "ab." -> 	accepts "abZ", "abf", "ab9"
 * 9. "ab&" -> accepts "aba", "abB", "abz"
 * 10."ab#" -> accepts "ab1", "ab2", "ab9"
 * 11. "ab.*" -> accepts "ab", "ab1", "ab22222222222", "abf", "abZZZZZZZZz"
 * */
#ifndef PRINT_MESSAGES
#define PRINT_MESSAGES 1
#endif
//the rules that change functionality
//some rules such as using a "\" to indicate a symbol is literal,
//are handled during compilation & thus don't need a named rule.
//
//using an enum is restricting, unless I guess we just make every combination of values.. handling (a*bc)? means a parent of R_OPT with child a* who has alternates b & c.
//but if we want to use alternates,
enum rules {
	R_DEFAULT,
	//these rules are reserved fo class printouts
	R_STAR,
	R_PLUS,
	R_OPT,
};
enum substitution_type {
	S_LITERAL = 0,
	S_DIGIT, 	//DIGIT - numbers 0-9
	S_ALPHA, 	//ALPHA - letters a-zA-Z
	S_ALNUM, //ALPHANUMERIC - any letter or number
};
static std::string rule_to_string(rules rule) {
	switch(rule) {
		default:
#if PRINT_MESSAGES==1
			std::cout << "defaulting in rule_to_string";
#endif
			[[fallthrough]];
		case(R_DEFAULT):
			return "DEFAULT";
		case(R_OPT):
			return "OPTION";
		case(R_PLUS): //visit notes and recall what the name of this is. kleene closure or something
			return "PLUS";
		case(R_STAR):
			return "STAR";
	}
}
std::string sub_to_string(substitution_type sub_rule) {
	switch (sub_rule) {
		case(S_ALNUM):
			return "ALNUM";
		case(S_ALPHA):
			return "ALPHA";
		case(S_DIGIT):
			return "DIGIT";
		default:
			return "ERROR";
		case(S_LITERAL):
			return "LITERAL";		
	}
};
/***                           	  the regex class                                   ***/
class regex {
	public:
		regex(int, substitution_type);
		virtual ~regex() {
			if (alternate != NULL) {
				delete alternate;
			}
			//because the alternate will also point to next
			else if (next != NULL) {
				delete next;
			}
		}
		int getLiteral();

		regex* getNext();
		virtual void setNext(regex*);
		//return a char* to the pointer where the match ends. This can be used to determine where the match occurred, and its length
		virtual char* match_here(char* text);
		virtual rules getRule() {
			return R_DEFAULT;
		}
		bool accepts(int character);
		void addAlternate(regex*);
		regex* getAlternate();
		//
		std::string sub_as_string() {
			return sub_to_string(sub_rule);
		}
	protected:
		class regex* next;
		class regex* alternate;
		substitution_type sub_rule;
	private:
		//literal's value will not matter when substitute != R_CHAR
		int literal;
};
class regex_star : public regex {
	public:
		regex_star(int _literal, substitution_type _sub_rule = S_LITERAL) :
			regex(_literal,_sub_rule) {};
		char* match_here(char* text) override;
		rules getRule() override { return R_STAR; };
};
class regex_plus : public regex {
	public:
		regex_plus(int _literal, substitution_type _sub_rule = S_LITERAL) :
			regex(_literal, _sub_rule) {};
		char* match_here(char* text) override;
		rules getRule() override { return R_PLUS; };
};
class regex_opt : public regex {
	public:
		regex_opt(int _literal, substitution_type _sub_rule = S_LITERAL) :
			regex(_literal, _sub_rule) {};
		char* match_here(char* text) override;
		rules getRule() override { return R_OPT; };
};
/** 	      prototypes 	        **/
void re_print(regex*);
//creates a regex object with specified rule & literal
regex::regex(int _literal, substitution_type _sub_rule = S_LITERAL) {
	literal = _literal;
	next = NULL;
	alternate = NULL; 	//should we only find these in a regex child? I think so, imagine the tree, it has to split only at child nodes.
	sub_rule = _sub_rule;
}
bool regex::accepts(int character) {
	switch (sub_rule) {
		case(S_ALNUM):
			return std::isalnum(character);
		case(S_ALPHA):
			return std::isalpha(character);
		case(S_DIGIT):
			return std::isdigit(character);
		default:
#if PRINT_MESSAGES==1
			std::cout << "::accepts() - sub rule defaulting to S_LITERAL\n\n";
#endif
			[[fallthrough]];
		case(S_LITERAL):
			return character == literal;
	}
}
//gets the literal of the node
int regex::getLiteral() {
	return literal;
}
//returns the next node from the node
regex* regex::getNext() {
	return next;
}
//sets the next node of the current node
void regex::setNext(regex* next_instance) {
	next = next_instance;
	//if there are alternates, set their next value to the nodes next value
	for (regex* alt = alternate; alt != NULL; alt = alt->getAlternate())
		alt->setNext(next_instance);
}
regex* regex::getAlternate() {
	return alternate;
}
void regex::addAlternate(regex* _alternate) {
	if (alternate != NULL)
		alternate->addAlternate(_alternate);
	else
		alternate = _alternate;
}
//prints the nodes information
void re_print(regex* instance) {
	if (instance)
		printf("[Rule: %s | Literal: %c | Next? : %s | Alt?: %s]",
				rule_to_string(instance->getRule()).c_str(),
				instance->getLiteral(),
				(instance->getNext()) ? "Yes" : "No",
				(instance->getAlternate() ? "yes" : "no"));
	else printf("[-]");
}
/*** 					matching 				    ***/
//returns true if the expression found a match in the text, otherwise it returns false.
bool match(regex* expression, std::string input_text) {
	char* text = (char*)calloc(1,input_text.size());
	char* start_of_text = text;
	strncpy(text, input_text.c_str(), input_text.size());
	char* end_of_match = NULL;
	do {
#if PRINT_MESSAGES==1
			std::cout << "\n<-match-loop->\t" << text << "\n";
#endif
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
char* regex::match_here(char *text) {
#if PRINT_MESSAGES==1
		std::cout << "match:\t";
		if (sub_rule == S_LITERAL)
			std::cout << (char)getLiteral() << " == " << *text;
		else
			std::cout << sub_as_string() << " == " << *text;
		std::cout << "\t|" << "alt:" << std::string((alternate)?"T":"F") << "\t";
		std::cout << "\t|" << "next:" << std::string((next)?"T":"F") << "\n";
#endif
/* Match
 * process current rule.
 * If returns true: process next.
 * If returns false && has alternate: process alternate.
 * Else: return false
 * */
	if (accepts(*text)) {
		if (next == NULL) {
			return text+1;
		}
		return (next->match_here(text+1));
	}
	else if (alternate != NULL) {
		return alternate->match_here(text);
	}
	//no valid expressions, returns false by default
	return NULL;
}
char* regex_star::match_here(char *text) {
#if PRINT_MESSAGES==1
		std::cout << "match*:\t";
		if (sub_rule == S_LITERAL)
			std::cout << (char)getLiteral() << " == " << *text;
		else
			std::cout << sub_as_string() << " == " << *text;
		std::cout << "\t|" << "next:" << std::string((next)?"T":"F") << "\n";
#endif
	//TODO create a flag for shortest or longest match. This is currently a shortest match implementation
	//perform the absolute shortest match if we have no options.
	if (next == NULL) {
		//the shortest possible match in R_STAR is NO match
		return text;
	}
	//continue attempting the current rule
	else {
		char* tmp_text = text; //used so that if/when we call the alternate case, we have the original starting point
		do {
			auto retVal = next->match_here(tmp_text);
			if(retVal != NULL) return retVal;
		} while (*tmp_text != '\0' && accepts(*tmp_text++));
	}
	//Failed to match expressions. Attempt alternate
	if (alternate != NULL) {
		auto retVal = alternate->match_here(text);
		if (retVal != NULL) return retVal;
	}
	//no valid expressions, returns false by default
	return NULL;
}

char* regex_plus::match_here(char* text) {
#if PRINT_MESSAGES==1
		std::cout << "match+:\t";
		if (sub_rule == S_LITERAL)
			std::cout << (char)getLiteral() << " == " << *text;
		else
			std::cout << sub_as_string() << " == " << *text;
		std::cout << "\t|" << "next:" << std::string((next)?"T":"F") << "\n";
#endif
	//TODO Allow for a longest match (currently doing shortest)
	if (next == NULL && accepts(*text)) return text+1;
	if (next != NULL) {
		for (char* tmp_text = text;*tmp_text != '\0' && accepts(*tmp_text);tmp_text++) {
			//we use the tmp array here so that if it fails we can attempt an alternate match.
			//Otherwise, we could just store the offset instead of duplicating text
			auto retVal = next->match_here(tmp_text+1);
			if (retVal != NULL) return retVal;
		}
	}
	if (alternate != NULL) {
		auto retVal = alternate->match_here(text);
		if (retVal != NULL) return retVal;
	}
	return NULL;
}
char* regex_opt::match_here(char *text) {
#if PRINT_MESSAGES==1
		std::cout << "match?:\t";
		if (sub_rule == S_LITERAL)
			std::cout << (char)getLiteral() << " == " << *text;
		else
			std::cout << sub_as_string() << " == " << *text;
		std::cout << "\t|" << "next:" << std::string((next)?"T":"F") << "\n";
#endif
	if (accepts(*text)) {
		if (next != NULL ) return next->match_here(++text);
		else return text+1;
	}
	else {
		auto retVal = next->match_here(text);
		if (retVal) return retVal;
		else if (alternate != NULL) {
			retVal = alternate->match_here(text);
			if (retVal != NULL) return retVal;
		}
	}
	return NULL;
}

/*** 				regex compilation 				    ***/
/* The following code is for compiling the    *//** 	      prototypes 	        **/
/*regex from an input string into the linked  */rules symbol_to_rrule(regex*, char);
/*regex nodes.                                */substitution_type symbol_to_srule(char);
//Return the rule associated with a symbol. R_DEFAULT if there is no matching rule
rules symbol_to_rrule(char c) {
	switch(c) {
		case('*'):
			return R_STAR;
		case('?'):
			return R_OPT;
		case('+'):
			return R_PLUS;
		default:
			return R_DEFAULT;
	}
}
//Return the substitution rule associated with the symbol. S_LITERAL for non-matching symbol
substitution_type symbol_to_srule(char c) {
	switch(c) {
		case('.'):
			return S_ALNUM;
		case('#'):
			return S_DIGIT;
		case('&'):
			return S_ALPHA;
		default:
			return S_LITERAL;
	}
}
//TODO lint input, e.g., (ab)* is invalid, but (a*b*) is valid
//TODO implement this as a constructor of regex
//create a node-tree of REs from an input string
regex* create_from_string(std::string regex_tape) {
	regex* root_node = NULL;
	regex* last_node = NULL;
	regex* alternate_node = NULL; //holds the node which has alternates
	bool do_alternative = false;
	bool escaped_symbol = false;
	for (size_t i = 0; i < regex_tape.size(); i++) {
#if PRINT_MESSAGES==1
		std::cout << "cfs: i=" << i << "\n";
#endif
	//begin compilation
		regex* current_node;
		rules current_rule = R_DEFAULT;
		if (i+1 < regex_tape.size())
			current_rule = symbol_to_rrule(regex_tape.at(i+1));
		auto subrule = symbol_to_srule(regex_tape.at(i));
		int literal = regex_tape.at(i);
		if (literal == '(') {
#if PRINT_MESSAGES==1
			std::cout << "set do_alternative. Skip...\n";
#endif
			do_alternative = true;
			continue;
		}
		else if (literal == ')') {
#if PRINT_MESSAGES==1
			std::cout << "unset do_alternative. Skip...\n";
#endif
			do_alternative = false;
			continue;
		}
		else if (literal == '\\' and escaped_symbol == false) {
#if PRINT_MESSAGES==1
			std::cout << "set escaped_symbol. Skip...\n";
#endif
			escaped_symbol = true;
			continue;
		}
#if PRINT_MESSAGES==1
		std::cout << "CR:\t" << rule_to_string(current_rule) << "\n";
		std::cout << "SR:\t" << sub_to_string(subrule) << "\n";
		std::cout << "L:\t" << (char)literal << "\n";
		std::cout << "Alt?\t" << std::string((do_alternative)?"true":"false") << "\n";
		std::cout << "Esc?\t" << std::string((escaped_symbol)?"true":"false") << "\n";
#endif
		if (escaped_symbol) {
			subrule = S_LITERAL;
			escaped_symbol = false;
#if PRINT_MESSAGES==1
			std::cout << "subRule = S_LITERAL, unset Esc\n";
#endif
		}
		switch (current_rule) {
			case R_DEFAULT:
				current_node = new regex(literal, subrule);
				break;
			case R_OPT:
				current_node = new regex_opt(literal, subrule);
				break;
			case R_PLUS:
				current_node = new regex_plus(literal, subrule);
				break;
			case R_STAR:
				current_node = new regex_star(literal, subrule);
				break;
		}
		if (root_node == NULL) {
			root_node = current_node;
			last_node = current_node;
			if (do_alternative && alternate_node == NULL) {
				//the alternate node would likely be anything BUT null at this time
				alternate_node = current_node;
			}
		}
		else if (do_alternative) {
			if (alternate_node == NULL)
				alternate_node = current_node;
			else
				alternate_node->addAlternate(current_node);
		}
		else {
			if (alternate_node != NULL) {
				last_node->setNext(alternate_node);
				last_node = alternate_node;
				alternate_node = NULL;
			}
			last_node->setNext(current_node);
			last_node = current_node;
		}
		if (current_rule != R_DEFAULT) i+=1; //consume an extra character if we set a rule
#if PRINT_MESSAGES==1
		std::cout << "------\n";
#endif
	}//end compilation
	if (alternate_node != NULL) {
#if PRINT_MESSAGES==1
		std::cout << "ending with alt node != NULL. last.setNext(alt)\n";
#endif
		//This occurs when the regex string ends with a ')'
		last_node->setNext(alternate_node);
	}
	return root_node;	
}
#ifndef UNIT_TESTING
/*** 				    main 					    ***/
int main(int argc, char** argv) {
	int a; 				//iterator
	int input_len = 0; 			//the length of the input string, for malloc
	char* regex_expression = NULL; 	//the regex given by input
	char* input_text = NULL; 	//the text to find a match in
	char* input_anchor = NULL; 	//pointer to somewhere in input_text
	//
	if (argc < 3) return -1;
//set regex_expression
	regex_expression = argv[1];
//allocate input_text
	for (a = 2; a < argc; a++)
		input_len += strlen(argv[a]);
	input_len += (argc-2); 		//because spaces are considered separators
					//we have to add them back in.
	input_text = (char*)calloc(1,input_len); //calloc to initialize memory to 0. unsure of the importance, but valgrind wasn't happy without it
//set input text
	input_anchor = input_text; //set the anchor to the beginning of the input text
	for (a = 2; a < argc; a++) { 			//for-each arg after 1
		int arg_len = strlen(argv[a]); 		//length of the sub-string
		strncpy(input_anchor, argv[a], arg_len); 	//copy the sub-string in
		input_anchor+=arg_len; 		//move the pointer down
		if ((a+1) < argc) 			//if not last word
			*input_anchor++ = ' '; //add a space BETWIXT words
	}
//output
	printf("REGEX:%s\nTEXT:%s\n", regex_expression, input_text);
	regex* regexpr = create_from_string(regex_expression);

	regex* instance = regexpr;
	while(instance && PRINT_MESSAGES) {
		re_print(instance);
		printf("\n");
		if (instance->getAlternate()) {
			instance = instance->getAlternate();
			printf("*\t");
		}
		//no kids or alternate
		else {
			instance = instance->getNext();
		}
	}
	printf("%s\n", (match(regexpr, input_text))? "match" : "no match");
	free(input_text);
	delete (regexpr);
}
#else
#define CATCH_CONFIG_MAIN
TEST_CASE( "test match on a single literal" ) {
	regex* expression = create_from_string("a");
	//
	REQUIRE(match(expression, "qwerty abcd"));
	REQUIRE(match(expression, "a"));
	REQUIRE(match(expression, "b") == false);
	delete expression;
}
TEST_CASE( "test match on a S_ALNUM" ) {
	regex* expression = create_from_string(".");
	//
	REQUIRE(match(expression, "qwerty abcd"));
	REQUIRE(match(expression, "a"));
	REQUIRE(match(expression, "1"));
	REQUIRE(match(expression, "") == false);
	delete expression;
}
TEST_CASE( "test match on a S_ALPHA" ) {
	regex* expression = create_from_string("&");
	//
	REQUIRE(match(expression, "qwerty bcd"));
	REQUIRE(match(expression, "b"));
	REQUIRE(match(expression, "b2"));
	REQUIRE(match(expression, "1") == false);
	REQUIRE(match(expression, "") == false);
	delete expression;
}
TEST_CASE( "test match on a S_DIGIT" ) {
	regex* expression = create_from_string("#");
	//
	REQUIRE(match(expression, "a") == false);
	REQUIRE(match(expression, "qwerty abcd") == false);
	REQUIRE(match(expression, "") == false);
	REQUIRE(match(expression, "qwe2ty bcd"));
	REQUIRE(match(expression, "0"));
	delete expression;
}
TEST_CASE( "test match on a single escaped symbol" ) {
	regex* expression = create_from_string("\\#");
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
	regex* expression = create_from_string("ab?d");
	//
	REQUIRE(match(expression, "abd"));
	REQUIRE(match(expression, "ad"));
	REQUIRE(match(expression, "ab") == false);
	REQUIRE(match(expression, "") == false);
	delete expression;
}
TEST_CASE( "test match with star" ) {
	regex* expression = create_from_string("ab*d");
	//
	REQUIRE(match(expression, "abd"));
	REQUIRE(match(expression, "ad"));
	REQUIRE(match(expression, "ab") == false);
	REQUIRE(match(expression, "") == false);
	REQUIRE(match(expression, "abbbbbbbbbbbbbbd"));
	delete expression;
}
TEST_CASE( "test match with plus" ) {
	regex* expression = create_from_string("ab+d");
	//
	REQUIRE(match(expression, "abd"));
	REQUIRE(match(expression, "ad") == false);
	REQUIRE(match(expression, "ab") == false);
	REQUIRE(match(expression, "") == false);
	REQUIRE(match(expression, "abbbbbbbbbbbbbbd"));
	delete expression;
}
TEST_CASE( "test match with alternate literals in expression" ) {
	regex* expression = create_from_string("a(bc)d");
	//
	REQUIRE(match(expression, "acd"));
	REQUIRE(match(expression, "abd"));
	REQUIRE(match(expression, "acbd") == false);
	REQUIRE(match(expression, "") == false);
	delete expression;
}
TEST_CASE( "test match with alternate star repetitions in expression" ) {
	regex* expression = create_from_string("a(b*c*)d");
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
	regex* expression = create_from_string("a(b+c+)d");
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
#endif
