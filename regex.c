/*** prototypes ***/
int matchstar(int c, char* regexp, char *text);
int match(char* regexp, char *text);
int matchhere(char* regexp, char *text);

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
		//matches against text shift along each char(text++ moves us down)
		if (matchhere(regexp,text))
			return 1;
	} while (*text++ != '\0'); //if this is the end of the string, don't try another loop
	return 0;
}

//search for regexp at beginning of text
int matchhere(char *regexp, char *text) {
	//if we have reached the end of the string,
	//all previous test must've succeeded.
	//Thus, the regex matches on the text (return 1)
	if (regexp[0] == '\0')
		return 1;
	//if the regex is a character followed by a *, call matchstar to see whether the closure matches
	if (regexp[1] == '*')
		return matchstar(regexp[0], regexp+2, text);
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

//search for c*regexp at beginning of text
//c -> the character we wish to see more of
int matchstar(int c, char *regexp, char *text) {
	do { //a * matches zero or more instances
 		if (matchhere(regexp,text))
			return 1;
	} while(*text != '\0' && (*text++ == c || c == '.'));
	return 0;
}
