#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
/* --RULES--
 * c matches any literal character c
 * . matches any single character
 * ^ matches beginning of the input string
 * $ matches the end of the input string
 * * matches zero or more occurrences of the previous character
 *  + matches one or more occurrences of previous character
 *  ? zero or ONE matches
 *  \ escapes a rule-character(meta-char) to stand for literal
 *  # matches a digit 	| sets a modifier variable & acts like '.', unless super-rule
 *  & matches a letter 	| sets a modifier variable & acts like '.', unless super-rule
 *  NEW RULE
 *  [abc] matches a, b or c.
 *  [a-c] matches a, b or c.
 *  [0ab9] matches 0, a, b or 9
 *  [a-zA-Z] matches all lower&upper case characters, equivalent to &
 */
//data structures
enum rules { //each one matches the rules, declared above, in order
	R_CHAR = 0,
//	R_ANY_CHAR, 	//can use M_digit, M_letter, or M_none for default any-match
	R_BEGINNING,
	R_END,
	R_STAR, //think about how to separate this into shortes/longest match
//	R_PLUS, //think about separating shortest & longest matches
//	R_BINARY,
};
const char* rule_names[8] = {
	"CHAR",
//	"ANY_CHAR",
	"BEGIN",
	"END",
	"STAR",
//	"PLUS",
//	"BINARY",
};
struct regex_t {
	int rule; 	//the type of function this represents?
	char character; 	//the character itself
	struct regex_t* next; //the next rule to execute. (not ensemble sub-rule)
	int child_count; //length of children array
	struct regex_t** children; //if this is an ensemble, it will have children
};
typedef struct regex_t* regex;

/*** opaque data-handling ***/
//creates a new regex, with rule, char, & pointer to next element. Might remove the next_instance and just use regex_setNext
regex 	regex_create(int _rule, char _char, regex next_instance) {
	regex re = calloc(1, sizeof(regex));
	re->rule = _rule;
	re->character = _char;
	re->next = next_instance;
	re->child_count = 0;
	//init children to nullptr???
	return re;
}
//destroys the specified instance, its children must be handled first.
void 	regex_destroy(regex instance) {
	if (instance)
		free(instance);
}
//sets the next instance from the current instance
void 	regex_setNext(regex instance, regex next_instance) {
	instance->next = next_instance;
}
//adds a child to a regex instance
void 	regex_addChild(regex instance, regex child) {
	if (instance->child_count == 0){
		instance->children = calloc(1, sizeof(instance->children));
		if (!instance->children[0])
	}
}
//returns the pointer to the next instance
regex* 	regex_next(regex* instance); 
//returns the amount of children a regex has
int 	regex_childCount(regex* instance);
//returns a pointer to the child
regex* 	regex_getChild(regex* instance, int index);
//returns the character of the regex
char 	regex_getChar(regex* instance);
//returns the rule of the regex
int 	regex_getRule(regex* instance);

/*** prototypes ***/
//functions
int isSymbol(int c);
int isPostSymbol(int c);
int isPreSymbol(int c);
int isAnchor(int c);
int isSubstitute(int c);
regex* regex_to_code(char* regexp);
//general match
int match(regex* regexp, char *text);
int matchhere(regex* regexp, char *text);
//match rule
int matchstar(regex* regexp, char *text);

/*** rule symbols ***/
//all symbols
const int all_symbols_len = 9;
const char *all_symbols = ".&#^$*+?\\";
//postfix functions
const char *postfix_symbols = "*+?";
const int postfix_len = 3;
//prefix functions
const int prefix_len = 1;
const char *prefix_symbols = "\\";
//anchors
const int anchor_len = 2;
const char *anchor_symbols = "^$";
//substitutes
const int substitute_len = 3;
const char *substitute_symbols = ".&#";

/*** general match ***/
//search for regexp anywhere in text
/* tests if there is an occurence of the regex anywhere
 * in the text. If so, it returns 1, else 0. When there
 * is more than one match, it returns the leftmost &
 * shortest.
 */
int match(regex *regexp, char *text) {
	//^ is the anchor to the lines start
	if (regexp[0].rule == R_BEGINNING) { 		
		return matchhere(regex_next(regexp), text);
	}
	//search the string, even if it is empty
	//if it empty, it can be matched by a single '*'
	//thus, the do-while loop instead of a while loop
	do {
		printf("---------------------\n");
		//matches against text shift along each char(text++ moves us down)
		if (matchhere(regexp,text))
			return 1;
	} while (*text++ != '\0'); //if this is the end of the string, don't try another loop
	return 0;
}

//search for regexp at beginning of text
//this function acts like a train junction, directing each rule to its proper handler 
int matchhere(regex* regexp, char *text) {
	printf("here: [%c] [%s]\t[%s]\n", regexp[0].character, rule_names[regexp[0].rule], text);
	//if we have reached the end of the string,
	//all previous test must've succeeded.
	//Thus, the regex matches on the text (return 1)
	if (!regexp) return 1; //end of regexp;i.e, the end-case
	//run the next rule
	int childCount = regex_childCount(regexp);
	if (childCount != 0) {
		//if any of the children determine the RE exists in the language we return 1
		for (int i = 0; i < childCount; i++) {
			regex* subexp = regex_getChild(regexp,i);
			if (matchhere(regexp,text)) return 1;
		}
		//if none of the children make-up the word, we return 0.
		return 0;
	}
	switch (regexp[0].rule) {
		case R_STAR:
			return matchstar(regexp, text);
		case R_CHAR:
			if (regexp[0].character == *text)
				return matchhere(regex_next(regexp), text+1);
			else break;
		//if we want to run a rule, but are out of text. FAILED.
		default:
			if (*text == '\0') return 0;
	}
	//if all the previous matches failed, there can be no matchs
	return 0;
}

/*** match rules ***/
//search for c*regexp at beginning of text
//this is the shortest left-most wildcard match
int matchstar(regex* regexp, char *text) {
	printf("*(%c)\t[%s]\n", regexp[0].character, text);
	do { //a * matches zero or more instances
		if (matchhere(regex_next(regexp),text)) {
			return 1;
		}
	} while(*text != '\0' && ((regexp[0].character == *text++) || (regexp[0].modifier != M_literal)));
	return 0;
}

/*** functions for compilation ***/
//checks if the character is a symbol used for rules
int isSymbol(int c) {
	for (int a = 0; a < all_symbols_len; a++)
		if (c == all_symbols[a])
			return 1;
	return 0;
}
int isPostSymbol(int c) {
	for (int a = 0; a < postfix_len; a++)
		if (c == postfix_symbols[a])
			return 1;
	return 0;
}
int isPreSymbol(int c) {
	for (int a = 0; a < prefix_len; a++)
		if (c == prefix_symbols[a])
			return 1;
	return 0;
}
int isAnchor(int c) {
	for (int a = 0; a < anchor_len; a++)
		if (c == anchor_symbols[a])
			return 1;
	return 0;
}
int isSubstitute(int c) {
	for (int a = 0; a < substitute_len; a++)
		if (c == substitute_symbols[a])
			return 1;
	return 0;
}
int char_to_rule(int c) {
	switch(c) {
		case('*'):
			return R_STAR;
		case('+'):
			return R_PLUS;
		case('?'):
			return R_BINARY;
		case('^'):
			return R_BEGINNING;
		case('$'):
			return R_END;
		default:
			return R_CHAR;
	}
}
regex* regex_to_code(char* regexp) {
	if (regexp == NULL) //if we receive nothing, give noting
		return NULL;
	int regex_count = 0;
	int a;
	regex* rvalue = malloc(sizeof(regex) * strlen(regexp));
	for (a = 0; regexp[a] != '\0'; a++) {
		int c = regexp[a];
		rvalue[regex_count].modifier = M_none;
		//printf("--%d|%c| [%s] |%ld| --\n",a,c,&regexp[a],strlen(regexp)-a);
		//if we dealing with a literal + function
		if (strlen(regexp)-a >= 3 && isPreSymbol(regexp[a]) && isPostSymbol(regexp[a+2])) {
			//	printf("LITERAL+FUNC SYMBOL\n");
				rvalue[regex_count].modifier = M_literal;
				if (regexp[a+1] =='\0'){
					printf("ERROR !!!!\n");
					free(rvalue);
					return NULL;	
				}
				rvalue[regex_count].character = regexp[a+1];
				rvalue[regex_count++].rule = char_to_rule(regexp[a+2]);
				a+=2;
		}
		else if (isPreSymbol(c)) {
		//	printf("LITERAL SYMBOL\n");
			if (regexp[a+1] =='\0'){
				printf("ERROR !!!!\n");
				free(rvalue);
				return NULL;
			}
			rvalue[regex_count].rule = R_CHAR;
			rvalue[regex_count].modifier = M_literal;
			rvalue[regex_count++].character = regexp[a+1];
			a++;
		}
		else if (isPostSymbol(regexp[a+1])) {
		//	printf("SYMBOL: %c\n", regexp[a+1]);
			rvalue[regex_count].rule = char_to_rule(regexp[a+1]);
			rvalue[regex_count++].character = c;
			a++;
		}
		else if (isAnchor(c)) {
		//	printf("ANCHOR\n");
			rvalue[regex_count].rule = (c == '^') ? R_BEGINNING : R_END;
			rvalue[regex_count++].character = c;
		}
		else if (isSubstitute(c)) {
		//	printf("ANY_CHAR\n");
			if (c == '#')
				rvalue[regex_count].modifier = M_digit;
			else if (c == '&')
				rvalue[regex_count].modifier = M_letter;
			rvalue[regex_count].rule = R_ANY_CHAR;
			rvalue[regex_count++].character = c;
		}
		else {
		//	printf("CHAR\n");
			rvalue[regex_count].rule = R_CHAR;
			rvalue[regex_count++].character = c;
		}
	}
	return rvalue; //default
}

int main(int argc, char* argv[]) {
	if (argc < 3) return -1;
	char* regexpr = NULL;
	char* input_text = NULL;
	char* insertion_point;
	int len = 0;
	int a;
	regex* r_code = NULL;
	//get regexpr
	regexpr = malloc(strlen(argv[2]));
	if (regexpr == NULL) return -1;
	strncpy(regexpr, argv[1], strlen(argv[1]));
	//get input_text

	for (a = 2; a < argc; a++)
		len += strlen(argv[a]);
	len += (argc-2);
	input_text = malloc(len);
	if (input_text == NULL) return -1;
	//
	insertion_point = input_text;
	for (a = 2; a < argc; a++) {
		int arg_len = strlen(argv[a]);
		strncpy(insertion_point, argv[a], arg_len);
		insertion_point += arg_len;
		if ((a+1) < argc)
			*insertion_point++ = ' ';
	}
	//
	printf("expr[%s]\ntext[%s]\n", regexpr,input_text);
	//
	r_code = regex_to_code(regexpr);
	if (r_code == NULL) return -1;
	//prints out each regex value
	//for (a = 0; a < r_len; a++)
	//	printf("%d:[%c][%s][%s]\n",
	//			a,
	//			r_code[a].character,
	//			rule_names[r_code[a].rule],
	//			modifier_names[r_code[a].modifier]
	//	);
	//call for the matching
	printf(">%s\n", (match(r_code, input_text) == 1) ? "MATCH" : "NO MATCH");
	//free all mallocs
	free (r_code);
	free(input_text);
	free(regexpr);
	return 0;
}
