#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
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
	R_ANY_CHAR, 	//can use M_digit, M_letter, or M_none for default any-match
	R_BEGINNING,
	R_END,
	R_STAR, //think about how to separate this into shortes/longest match
	R_PLUS, //think about separating shortest & longest matches
	R_BINARY,
};
const char* rule_names[8] = {
	"CHAR",
	"ANY_CHAR",
	"BEGIN",
	"END",
	"STAR",
	"PLUS",
	"BINARY",
};
//these are not needed for R_CHAR, but for the functions used later
enum modifiers {
	M_none = 0, 	//meaning no special reading will be done
	M_literal, 	//only if char is a symbol, make sure it is read as a literal
	M_digit, 	//for use with # inside functions
	M_letter, 	//for use with & inside functions
};
const char* modifier_names[4]  = {
	"none",
	"literal",
	"digit",
	"letter"
};
typedef struct regex {
	int rule; 	//the type of function this represents?
	char character; 	//the character itself
	int modifier; 	//could probably save some space by using byte masks...
} regex;

/*** global symbols ***/
//all symbols
const int all_symbols_len = 7;
const char *all_symbols = ".^$*+?\\";
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
/*** prototypes ***/
//functions
int isSymbol(int c);
int isPostSymbol(int c);
int isPreSymbol(int c);
int isAnchor(int c);
int isSubstitute(int c);
regex* regex_to_code(char* regexp);
//general match
int match(int r_len, regex* regexp, char *text);
int matchhere(int r_len, regex* regexp, char *text);
//match rule
int matchplus(int r_len, regex* regexp, char *text);
int matchstar(int r_len, regex* regexp, char *text);
int long_matchstar(int r_len, regex* regexp, char *text);
int matchoptional(int r_len, regex* regexp, char *text);
int matchany(int r_len, regex* regexp, char *text);

/*** the matching code ***/
//search for regexp anywhere in text
/* tests if there is an occurence of the regex anywhere
 * in the text. If so, it returns 1, else 0. When there
 * is more than one match, it returns the leftmost &
 * shortest.
 */
int match(int r_len, regex *regexp, char *text) {
	//^ is the anchor to the lines start
	if (regexp[0].rule == R_BEGINNING) { 		
		return matchhere(r_len-1,regexp+1, text);
	}
	//search the string, even if it is empty
	//if it empty, it can be matched by a single '*'
	//thus, the do-while loop instead of a while loop
	do {
		printf("---------------------\n");
		//matches against text shift along each char(text++ moves us down)
		if (matchhere(r_len,regexp,text))
			return 1;
	} while (*text++ != '\0'); //if this is the end of the string, don't try another loop
	return 0;
}

//search for regexp at beginning of text
int matchhere(int r_len, regex* regexp, char *text) {
	printf("here: [%c] [%s]\t[%s]\n", regexp[0].character, rule_names[regexp[0].rule], text);
	//if we have reached the end of the string,
	//all previous test must've succeeded.
	//Thus, the regex matches on the text (return 1)
	if (r_len == 0)
		return 1;
	switch (regexp[0].rule) {
		case R_STAR:
			return matchstar(r_len,regexp, text);
		case R_PLUS:
			return matchplus(r_len, regexp, text);
		case R_BINARY:
			return matchoptional(r_len, regexp, text);
		case R_END:
			return (r_len == 1 && *text == '\0');
		case R_CHAR:
			if (regexp[0].character == *text)
				return matchhere(r_len-1, regexp+1, text+1);
			else break;
		case R_ANY_CHAR:
			return matchany(r_len, regexp,text);
//			return matchhere(r_len-1, regexp+1, text+1);
		default:
			if (*text == '\0') return 0;
			else return -1;
	}
	//if all the previous matches failed, there can be no matchs
	return 0;
}

int matchoptional(int r_len, regex* regexp, char *text) {
	printf("?(%c) l[%d]\t[%s]\n", regexp[0].character, r_len, text);
	//if we find the symbol we're looking for, increase the index of the row
	if (regexp[0].modifier == M_literal) {
		if (*text == regexp[0].character)
			text+=1;
	}
	else if (regexp[0].character == '.'){
		text+=1;
	}
	//if we didn't find the symbol, don't move the text
	return matchhere(r_len-1,regexp+1, text);

}

//match one or more occurences of the character
int matchplus(int r_len, regex* regexp, char *text) {
	printf("+(%c) l[%d]\t[%s]\n", regexp[0].character, r_len, text);
	while(*text != '\0'
			&&
				(*text++ == regexp[0].character
				 	|| regexp[0].modifier != M_literal))
	{
		if (matchhere(r_len-1, regexp+1, text)) {
			return 1;		
		}
	}
	return 0;
}
//search for c*regexp at beginning of text
//this is the shortest left-most wildcard match
int matchstar(int r_len, regex* regexp, char *text) {
	printf("*(%c)\t[%s]\n", regexp[0].character, text);
	do { //a * matches zero or more instances
		if (matchhere(r_len-1,regexp+1,text)) {
			return 1;
		}
	} while(*text != '\0' && ((regexp[0].character == *text++) || (regexp[0].modifier != M_literal)));
	return 0;
}
/*
//the longest left-most wildcard match
int long_matchstar(int r_len, regex* regexp, char *text) {
	char *t;
	for (t = text; *t != '\0' && (*t == c || c == '.'); t++);

	do { //matches zero or more instances
 		if (matchhere(regexp,t))
			return 1;
	} while(t-- > text);
	return 0;
}
*/
int matchany(int r_len, regex* regexp, char *text) {
	printf(".&#(%c)\t[%s]\n", regexp[0].character, text);
	if (regexp[0].modifier == M_digit) {
		if (isdigit(*text++))
			return matchhere(r_len-1, regexp+1, text);
	}
	else if (regexp[0].modifier == M_digit) {
		if (isalpha(*text++))
			return matchhere(r_len-1, regexp+1, text);
	}
	else if (regexp[0].modifier == M_none) {
		return matchhere(r_len-1, regexp+1, ++text);
	}
	//if all the above fail, ret 0
	return 0;
}

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
	//arg 0 is process name
	//arg 1 is the regexpr
	//arg 2 & beyond = input_text
	char* regexpr = NULL;
	char* input_text = NULL;
	//get regexpr
	regexpr = malloc(strlen(argv[2]));
	if (regexpr == NULL) return -1;
	strncpy(regexpr, argv[1], strlen(argv[1]));
	//get input_text
	int len = 0;
	int a;

	for (a = 2; a < argc; a++)
		len += strlen(argv[a]);
	len += (argc-2);
	input_text = malloc(len);
	if (input_text == NULL) return -1;
	//
	char* insertion_point = input_text;
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
	regex* r_code = regex_to_code(regexpr);
	if (r_code == NULL) return -1;
	int r_len = 0;
	//gets the length of the regex code array
	for (a = 0; regexpr[a] != '\0'; a++) {
		if (strlen(regexpr)-a >= 3 && isPreSymbol(regexpr[a]) && isPostSymbol(regexpr[a+2])) {
			r_len++;
			a+=2;
		}
		else if (isPreSymbol(regexpr[a])) {
			a++;
			r_len++;
		}
		else if (isPostSymbol(regexpr[a+1])) {
			a++;
			r_len++;
		}
		else if (isAnchor(regexpr[a]))
			r_len++;
		else if (isSubstitute(regexpr[a]))
			r_len++;
		else
			r_len++;
	}
	//prints out each regex value
	for (a = 0; a < r_len; a++)
		printf("%d:[%c][%s][%s]\n",
				a,
				r_code[a].character,
				rule_names[r_code[a].rule],
				modifier_names[r_code[a].modifier]
		);
	//call for the matching
	printf(">%s\n", (match(r_len, r_code, input_text) == 1) ? "MATCH" : "NO MATCH");
	//free all mallocs
	free (r_code);
	free(input_text);
	free(regexpr);
	return 0;
}
