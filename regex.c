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
 *  \ negates a rule-character(meta-char) to stand for literal
 */
//data
const int symbols_len = 7;
const char *symbols = ".^$*+?\\";
/*** prototypes ***/
//functions
int issymbol(int c);
//general match
int match(char* regexp, char *text);
int matchhere(char* regexp, char *text);
//match rule
int matchplus(int c, char* regexp, char *text);
int matchstar(int c, char* regexp, char *text);
int long_matchstar(int c, char* regexp, char *text);
int matchbinary(int c, char *regexp, char *text);
int matchliteral(int c, char *regexp, char *text);

/*** the matching code ***/
//search for regexp anywhere in text
/* tests if there is an occurence of the regex anywhere
 * in the text. If so, it returns 1, else 0. When there
 * is more than one match, it returns the leftmost &
 * shortest.
 */
int match(char *regexp, char *text) {
	//^ is the anchor to the lines start
	if (regexp[0] == '^') { 		
		return matchhere(regexp+1, text);
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
int matchhere(char *regexp, char *text) {
	printf("here: [%s]\t[%s]\n", regexp, text);
	//if we have reached the end of the string,
	//all previous test must've succeeded.
	//Thus, the regex matches on the text (return 1)
	if (regexp[0] == '\0')
		return 1;
	//if the current character is a back-slash & the next character is a symbol
	//only match the symbol
	if (regexp[0] == '\\' && issymbol(regexp[1]))
		return matchliteral(regexp[1], regexp+2, text);
	//if the regex is a character followed by a *, call matchstar to see whether the closure matches
	if (regexp[1] == '*')
		return matchstar(regexp[0], regexp+2, text);
	//if this is a char followed by a *, we call matchplus
	if (regexp[1] == '+') {
		return matchplus(regexp[0], regexp+2, text);
	}

	if (regexp[1] == '?') {
		return matchbinary(regexp[0], regexp+2, text);
	}
	//if the expr is a line-end anchor at the end of the expression.
	//then the text can only match if it is the end of the text
	if (regexp[0] == '$' && regexp[1] == '\0')
		return *text == '\0';
	//if we are not at the end of the line,
	//and the firt char of the expr matches
	//the first char of the text.
	//Begin recursion from the next char of both the text & the regex
	if (*text!='\0' && (regexp[0] == '.' || regexp[0] == *text))
		return matchhere(regexp+1, text+1);
	//if all the previous matches failed, there can be no matchs
	return 0;
}

int matchbinary(int c, char *regexp, char *text) {
	printf("?(%c): [%s]\t[%s]\n", c, regexp, text);
	//if we find the symbol we're looking for, increase the index of the row
	if ((c == '.' && isalpha(*text)) || (*text == c)) {
		text+=1;
	}
	//if we didn't find the symbol, don't move the text
	return matchhere(regexp, text);

}

//match one or more occurences of the character
int matchplus(int c, char *regexp, char *text) {
	printf("+(%c): [%s]\t[%s]\n", c, regexp, text);
	while(*text != '\0' && (*text++ == c || c == '.')) {
		if (matchhere(regexp, text)) {
			printf("<!+!>\n");
			return 1;		
		}
	}
	return 0;
}
//search for c*regexp at beginning of text
//this is the shortest left-most wildcard match
int matchstar(int c, char *regexp, char *text) {
	printf("*(%c): [%s]\t[%s]\n", c, regexp, text);
	do { //a * matches zero or more instances
		if (matchhere(regexp,text)) {
			printf("<!*!>\n");
			return 1;
		}
		printf("[SB]");
	} while(*text != '\0' && (*text++ == c || c == '.'));
	return 0;
}
//the longest left-most wildcard match
int long_matchstar(int c, char *regexp, char *text) {
	char *t;
	for (t = text; *t != '\0' && (*t == c || c == '.'); t++);

	do { //matches zero or more instances
 		if (matchhere(regexp,t))
			return 1;
	} while(t-- > text);
	return 0;
}

int matchliteral(int c, char *regexp, char *text) {
	printf("\\(%c): [%s]\t[%s]\n", c, regexp, text);
	if (regexp[0] == '*')
		return matchstar(c, regexp+1, ++text);
	if (regexp[0] == '+')
		return matchplus(c, regexp+1, ++text);
	if (regexp[0] == '?')
		return matchbinary(c, regexp+1, ++text);
	if (*text == c) return matchhere(regexp, ++text);
	return 0;
}

//checks if the character is a symbol used for rules
int issymbol(int c) {
	for (int a = 0; a < symbols_len; a++)
		if (c == symbols[a])
			return 1;
	return 0;
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
	printf("expr[%s]\ntext[%s]\n", regexpr,input_text);
	printf(">%s\n", (match(regexpr, input_text) == 1) ? "MATCH" : "NO MATCH");
	//
	free(input_text);
	free(regexpr);
	return 0;
}
