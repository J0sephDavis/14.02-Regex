#include <cctype>
#include <stdexcept>
#include <string.h>
#include <iostream>
#include <vector>
#include <string>
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
#define PRINT_MESSAGES 1
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
			std::cout << "defaulting in rule_to_string";
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
		int getLiteral();

		regex* getNext();
		virtual void setNext(regex*);
		virtual bool match_here(char* text);
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
		bool match_here(char* text) override;
		rules getRule() override { return R_STAR; };
};
class regex_plus : public regex {
	public:
		regex_plus(int _literal, substitution_type _sub_rule = S_LITERAL) :
			regex(_literal, _sub_rule) {};
		bool match_here(char* text) override;
		rules getRule() override { return R_PLUS; };
};
class regex_opt : public regex {
	public:
		regex_opt(int _literal, substitution_type _sub_rule = S_LITERAL) :
			regex(_literal, _sub_rule) {};
		bool match_here(char* text) override;
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
			std::cout << "::accepts() - sub rule defaulting to S_LITERAL\n\n";
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
bool match(regex* expression, char* text) {
	do {
		if(PRINT_MESSAGES)
			std::cout << "\n<-match-loop->\t" << text << "\n";
		if (expression->match_here(text))
			return true;
	} while(*text++ != '\0');
	return false; 
}
bool regex::match_here(char *text) {
	if (PRINT_MESSAGES) {
		std::cout << "match:\t";
		if (sub_rule == S_LITERAL)
			std::cout << (char)getLiteral() << " == " << *text;
		else
			std::cout << sub_as_string() << " == " << *text;
		std::cout << "\t|" << "alt:" << std::string((alternate)?"T":"F") << "\t";
		std::cout << "\t|" << "next:" << std::string((next)?"T":"F") << "\n";
	}
/* Match
 * process current rule.
 * If returns true: process next.
 * If returns false && has alternate: process alternate.
 * Else: return false
 * */
	if (accepts(*text)) {
		if (next == NULL) {
			return true;
		}
		return (next->match_here(text+1));
	}
	else if (alternate != NULL) {
		return alternate->match_here(text);
	}
	//no valid expressions, returns false by default
	return false;
}
bool regex_star::match_here(char *text) {
	if (PRINT_MESSAGES) {
		std::cout << "match*:\t";
		if (sub_rule == S_LITERAL)
			std::cout << (char)getLiteral() << " == " << *text;
		else
			std::cout << sub_as_string() << " == " << *text;
		std::cout << "\t|" << "next:" << std::string((next)?"T":"F") << "\n";
	}
	//TODO create a flag for shortest or longest match. This is currently a shortest match implementation
	//perform the absolute shortest match if we have no options.
	if (next == NULL) {
		//the shortest possible match in R_STAR is NO match
		return true;
	}
	//continue attempting the current rule
	else {
		char* tmp_text = text; //used so that if/when we call the alternate case, we have the original starting point
		do {
			if(next->match_here(tmp_text)) return true;
		} while (*tmp_text != '\0' && accepts(*tmp_text++));
	}
	//Failed to match expressions. Attempt alternate
	if (alternate != NULL) {
		if (alternate->match_here(text)) return true;
	}
	//no valid expressions, returns false by default
	return false;
}

bool regex_plus::match_here(char* text) {
	if (PRINT_MESSAGES) {
		std::cout << "match+:\t";
		if (sub_rule == S_LITERAL)
			std::cout << (char)getLiteral() << " == " << *text;
		else
			std::cout << sub_as_string() << " == " << *text;
		std::cout << "\t|" << "next:" << std::string((next)?"T":"F") << "\n";
	}
	//TODO Allow for a longest match (currently doing shortest)
	if (next == NULL && accepts(*text)) return true;
	if (next != NULL) {
		for (char* tmp_text = text;*text != '\0' && accepts(*text);text++) {
			//we use the tmp array here so that if it fails we can attempt an alternate match.
			//Otherwise, we could just store the offset instead of duplicating text
			if (next->match_here(tmp_text)) return true;
		}
	}
	if (alternate != NULL) {
		if(alternate->match_here(text)) return true;
	}
	return false;
}
bool regex_opt::match_here(char *text) {
	if (PRINT_MESSAGES) {
		std::cout << "match?:\t";
		if (sub_rule == S_LITERAL)
			std::cout << (char)getLiteral() << " == " << *text;
		else
			std::cout << sub_as_string() << " == " << *text;
		std::cout << "\t|" << "next:" << std::string((next)?"T":"F") << "\n";
	}
	if (accepts(*text)) {
		if (next != NULL ) return next->match_here(++text);
		else return true;
	}
	else {
		if (next->match_here(text)) return true;
		else if (alternate != NULL) {
			if (alternate->match_here(text)) return true;
		}
	}
	return false;
}
//
///*** 				regex compilation 				    ***/
///* The following code is for compiling the    *//** 	      prototypes 	        **/
/*regex from an input string into the linked  */rules symbol_to_rrule(regex*, char);
/*regex nodes.                                */substitution_type symbol_to_srule(char);
///*                                            */regex* re_create_f_str(char*);
//if the character is a repetition rule, the RE's rule will be changed. otherwise it is not touched & false is returned
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
//return either the matching substitution rule or R_CHAR
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
		std::cout << "cfs: i=" << i << "\n";
	//begin compilation
		regex* current_node;
		rules current_rule = R_DEFAULT;
		if (i+1 < regex_tape.size())
			current_rule = symbol_to_rrule(regex_tape.at(i+1));
		auto subrule = symbol_to_srule(regex_tape.at(i));
		int literal = regex_tape.at(i);
		if (literal == '(') {
			std::cout << "set do_alternative. Skip...\n";
			do_alternative = true;
			continue;
		}
		else if (literal == ')') {
			std::cout << "unset do_alternative. Skip...\n";
			do_alternative = false;
			continue;
		}
		else if (literal == '\\' and escaped_symbol == false) {
			std::cout << "set escaped_symbol. Skip...\n";
			escaped_symbol = true;
			continue;
		}
		std::cout << "CR:\t" << rule_to_string(current_rule) << "\n";
		std::cout << "SR:\t" << sub_to_string(subrule) << "\n";
		std::cout << "L:\t" << (char)literal << "\n";
		std::cout << "Alt?\t" << std::string((do_alternative)?"true":"false") << "\n";
		std::cout << "Esc?\t" << std::string((escaped_symbol)?"true":"false") << "\n";
		if (escaped_symbol) {
			subrule = S_LITERAL;
			escaped_symbol = false;
			std::cout << "subRule = S_LITERAL, unset Esc\n";
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
		std::cout << "------\n";
	}//end compilation
	if (alternate_node != NULL) {
		std::cout << "ending with alt node != NULL. last.setNext(alt)\n";
		//This occurs when the regex string ends with a ')'
		last_node->setNext(alternate_node);
	}
	return root_node;	
}
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
	//regex_expression = malloc(strlen(argv[2]));
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
	if (PRINT_MESSAGES)
		printf("REGEX:%s\nTEXT:%s\n", regex_expression, input_text);
//	regex* regexpr = re_create_f_str(regex_expression);
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
	while(instance) {
		regex* next_inst;
		if (instance->getAlternate())
			next_inst = instance->getAlternate();
		else
			next_inst = instance->getNext();
	//	re_destroy(instance);
		instance = next_inst;
	}
	free(input_text);
}
