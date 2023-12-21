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
//create a node-tree of REs from an input string
regex* create_from_string(std::string regex_tape) {
	regex* root_node = NULL;
	regex* last_node = NULL;
	bool do_alternative = false;
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
		std::cout << "CR:\t" << rule_to_string(current_rule) << "\n";
		std::cout << "SR:\t" << sub_to_string(subrule) << "\n";
		std::cout << "L:\t" << literal << "\n";
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
		if (root_node == NULL)
			root_node = current_node;
		else {
			if (do_alternative)
				last_node->addAlternate(current_node);
			else
				last_node->setNext(current_node);
		}
		last_node = current_node;
		if (current_rule != R_DEFAULT) i+=1; //consume an extra character if we set a rule
	}//end compilation
	return root_node;	
//	REGEX STRING: "ab(qz)d"
//	regex_plus regexpr('a');
//	
//	regex* next_regex = new regex_plus('b');
//	regexpr.setNext(next_regex);
//
//	regex* tmp_regex = new regex('q');
//	tmp_regex->addAlternate(new regex('z'));
//	next_regex->setNext(tmp_regex);
//	next_regex = next_regex->getNext();
//
//	next_regex->setNext(new regex('d'));
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
//regex* re_create_f_str(char* regexp) {
//	if (regexp == NULL) 			//if we are given an empty string
//		return NULL; 			//return nothing
//	//
//	regex* first_node = NULL;	 	//pointer to the first node (also returned at the end)
//	regex* last_node = NULL; 		//point to the last node made.
//	regex* parent_node = NULL; 		//pointer to the parent_node that needs alternates.
//	//
//	int literal; 				//stores the current character in the regexp
//	int rule; 				//stores the current rule
//	//
//	bool in_parent = false; 		//is the character string still inside the parentheses?
//	bool last_backslash = false; 		//isntead of using a regexp[a] == '\' && regexp[a+1] == symbol... we just set a flag. probably inefficient
//	bool child_next = true; 		//if the node should be the NEXT (true) pointer for a child, or if it should be an alternate (FALSE)
//	//
//	int a; 					//iterator
////iterate over all the passed characters
//	for (a = 0; regexp[a] != '\0'; a++) {
//		rule = R_CHAR;
//		literal = regexp[a];
//		if (PRINT_MESSAGES) printf("-PROCESSING:%c\n", literal);
////RULE APPLICATION
//		if (!last_backslash) {
//			if (PRINT_MESSAGES) printf("I:!backslash\n");
//			if (regexp[a] == '\\') {
//				if (PRINT_MESSAGES) printf("\ta:\\-skip\n");
//				last_backslash = true;
//				continue;
//			}
//			else if (regexp[a] == ')') {
//				if (PRINT_MESSAGES) printf("\tb:')'-skip\n");
//				in_parent = false;
//				last_node = parent_node; //if the next character is a repetition symbol, it will apply to the parent!
//				continue;
//			}
//			else if (!in_parent && regexp[a] == '(') {
//				if (PRINT_MESSAGES) printf("\tc:'('\n");
//				in_parent = true;
//			}
//			//because we are not dealing with a literal, apply the rule to the last node.
//			else if (re_char_to_reptition_rule(last_node, regexp[a])) {
//				if (PRINT_MESSAGES) printf("\td:rep apply-skip\n");
//				continue;
//			}
//			else if (in_parent && regexp[a] == '|') {
//				if (PRINT_MESSAGES) printf("\te:'|': child-next->false-skip\n");
//				child_next = false;
//				continue;
//			}
//			else {
//				if (PRINT_MESSAGES) printf("\te:sub rule\n");
//				rule = char_to_substitution_rule(regexp[a]);
//			}
//
//		}
//		else { 
//			if (PRINT_MESSAGES) printf("II:backslash\n");
//			last_backslash = false;
//		}
////CREATE NODE
//		regex* tmp_re = new regex(rule,literal);
////LINK NODE(S)
//		if (!first_node) {
//			if (PRINT_MESSAGES) printf("III:!first_node - skip\n");
//			first_node = tmp_re;
//			last_node = tmp_re;
//			if (in_parent && !parent_node){
//				if (PRINT_MESSAGES) printf("\ta:in_parent && !parent_node: parent = new\n");
//				parent_node = tmp_re;
//			}
//			continue;
//		}
//		if (in_parent) {
//			if (PRINT_MESSAGES) printf("IV:in_parent\n");
//			if (parent_node) {
//					if (PRINT_MESSAGES) printf("\ta:parent_node: ADD CHILD(parent, new)\n");
//					//if there is a child & we want to append a child to the last
//					if (parent_node->getChild() && child_next) {
//						if (PRINT_MESSAGES) printf("\t\ti:already has a child && child_next, append to child\n");
//						last_node->setNext(tmp_re);
//					}
//					//either no previous child, or we don't want to append a child to the last
//					else {
//						if (PRINT_MESSAGES) printf("\t\tii:add a child\n");
//						parent_node->addChild(tmp_re);
//					}
//			}
//			else {
//				if (PRINT_MESSAGES) printf("\tb:!parent_node: SET NEXT & parent = new. last.next=parent\n");
//				parent_node = tmp_re;
//				last_node->setNext(parent_node);
//			}
//		}
//		else {
//			if (PRINT_MESSAGES) printf("V:!in_parent\n");
//			if (parent_node) {
//				if (PRINT_MESSAGES) printf("\ta:parent_node: EACH:parent.child.alternate.next = new\n");
//				regex* tmp_child = parent_node->getChild();
//				while (tmp_child != NULL) {
//					if (tmp_child->getNext()) {
//						if (PRINT_MESSAGES) printf("\t\ti:child has next, find end & link\n");
//						regex* tmp_subChild = tmp_child->getNext();
//						while (tmp_subChild->getNext())
//							tmp_subChild = tmp_subChild->getNext();
//						tmp_subChild->setNext(tmp_re);
//					}
//					else {
//						if (PRINT_MESSAGES) printf("\t\tii:child has no descendants,link to next\n");
//						tmp_child->setNext(tmp_re);
//					}
//					tmp_child = tmp_child->getAlternate();
//				}
//				parent_node->setNext(tmp_re);
//				parent_node = NULL;
//			}
//			else {
//				if (PRINT_MESSAGES) printf("\tb:!parent_node: set next(last->new)\n");
//				last_node->setNext(tmp_re);
//			}
//		}
//		if (PRINT_MESSAGES) printf("VII:last->new\n");
//		//reset the var
//		if (!child_next) child_next = true;
//		if (PRINT_MESSAGES&&!child_next) printf("VIII:child_next = true\n");
//		last_node = tmp_re;
//	}
//	return first_node;
//}
