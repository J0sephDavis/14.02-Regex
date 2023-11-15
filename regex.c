#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#define PRINT_MESSAGES 1
/***                           	  the regex class                                   ***/
struct regex_t {
	int rule;
	int literal;
	struct regex_t* next;
	int c_count; //count of children
	struct regex_t** children;
};
typedef struct regex_t* regex;
//the rules that change functionality
//some rules such as using a "\" to indicate a symbol is literal,
//are handled during compilation & thus don't need a named rule.
enum rules {
	R_CHAR = 0,
	R_STAR,
	R_PLUS,
	R_OPT,
	R_DIGIT, 	//DIGIT - numbers 0-9
	R_ALPHA, 	//ALPHA - letters a-zA-Z
	R_ALPHANUMERIC, //ALPHANUMERIC - any letter or number
};
char* rules_names[] = {
	"char",
	"star",
	"plus",
	"opt",
	"digit",
	"letter",
	"alphanumeric"
};
/** 	      prototypes 	        **/
regex re_create(int, int);
void re_destroy(regex);   
int re_getRule(regex instance);
void re_setRule(regex, int);
int re_getLiteral(regex);
regex re_getNext(regex);
void re_setNext(regex, regex);
int re_getChildren(regex);
void re_addChild(regex, regex);
regex re_getChild(regex, int);
void re_print(regex);
//creates a regex object with specified rule & literal
regex re_create(int _rule, int _literal) {
	regex instance = calloc(1, sizeof(struct regex_t));
	instance->rule = _rule;
	instance->literal = _literal;
	instance->next = NULL;
	instance->c_count = 0;
	return instance;
}
//destroys the SPECIFIED node, not its children or related nodes
void re_destroy(regex instance) {
	//printf("FREE:"); re_print(instance); printf("\n");
	free(instance);
}
//returns the rule of the node
int re_getRule(regex instance) {
	return instance->rule;
}
//sets the rule of the node
void re_setRule(regex instance, int _rule) {
	instance->rule = _rule;
}
//gets the literal of the node
int re_getLiteral(regex instance) {
	return instance->literal;
}
//returns the next node from the node
regex re_getNext(regex instance) {
	return instance->next;
}
//sets the next node of the current node
void re_setNext(regex instance, regex next_instance) {
	instance->next = next_instance;
}
//returns the COUNT of a node's children
int re_getChildren(regex instance) {
	return instance->c_count;
}
//adds a child to a node
void re_addChild(regex instance, regex child) {
	if (!instance->children) { 		//if there are no children, allocate
		instance->children = calloc(1, sizeof(struct regex_t**));
	}
	else { 					//if there are some children, reallocate
		instance->children = realloc(instance->children, sizeof(struct regex_t**) * (instance->c_count+1));
	}
	instance->children[instance->c_count++] = child;
}
//returns a child of a node, given by index
regex re_getChild(regex instance, int index) {
	if (index > re_getChildren(instance) || index < 0) {
		return NULL;
	}
	else
		return instance->children[index];
}
//prints the nodes information
void re_print(regex instance) {
	if (instance)
		printf("[Rule: %s | Literal: %c | Next? : %s | Children: %d]",
				rules_names[instance->rule], instance->literal,
				(instance->next) ? "Yes" : "No",
				re_getChildren(instance));
	else printf("[-]");
}
/*** 				regex compilation 				    ***/
/* The following code is for compiling the    *//** 	      prototypes 	        **/
/*regex from an input string into the linked  */bool re_char_to_reptition_rule(regex, char);
/*regex nodes.                                */int char_to_substitution_rule(char);
/*                                            */regex re_create_f_str(char*);
//if the character is a repetition rule, the RE's rule will be changed. otherwise it is not touched & false is returned
bool re_char_to_reptition_rule(regex instance, char c) {
	switch(c) {
		case('*'):
			re_setRule(instance,R_STAR);
			return true;
		case('?'):
			re_setRule(instance,R_OPT);
			return true;
		case('+'):
			re_setRule(instance,R_PLUS);
			return true;
		default:
			return false;
	}
}
//return either the matching substitution rule or R_CHAR
int char_to_substitution_rule(char c) {
	switch(c) {
		case('.'):
			return R_ALPHANUMERIC;
		case('#'):
			return R_DIGIT;
		case('&'):
			return R_ALPHA;
	}
	return R_CHAR;
}
//create a node-tree of REs from an input string
regex re_create_f_str(char* regexp) {
	if (regexp == NULL) 			//if we are given an empty string
		return NULL; 			//return nothing
	//
	regex first_node = NULL;	 	//pointer to the first node (also returned at the end)
	regex last_node = NULL; 		//point to the last node made.
	regex parent_node = NULL; 		//pointer to the parent_node.
	//
	int literal; 				//stores the current character in the regexp
	int rule; 				//stores the current rule
	//
	bool in_parent = false; 		//is the character string still inside the parentheses?
	bool last_backslash = false; 		//isntead of using a regexp[a] == '\' && regexp[a+1] == symbol... we just set a flag. probably inefficient
	//
	int a; 					//iterator
//iterate over all the passed characters
	for (a = 0; regexp[a] != '\0'; a++) {
		rule = R_CHAR;
		literal = regexp[a];
//RULE APPLICATION
		if (!last_backslash) {
			if (regexp[a] == '\\') {
				last_backslash = true;
				continue;
			}
			else if (regexp[a] == ')') {
				in_parent = false;
				last_node = parent_node; //if the next character is a repetition symbol, it will apply to the parent!
				continue;
			}
			else if (!in_parent && regexp[a] == '(') {
				in_parent = true;
			}
			//because we are not dealing with a literal, apply the rule to the last node.
			if (re_char_to_reptition_rule(last_node, regexp[a]))
				continue;
			else rule = char_to_substitution_rule(regexp[a]);

		}
		else { 
			last_backslash = false;
		}
//CREATE NODE
		regex tmp_re = re_create(rule,literal);
//LINK NODE(S)
		if (!first_node) {
			first_node = tmp_re;
			last_node = tmp_re;
			if (in_parent && !parent_node)
				parent_node = tmp_re;
			continue;
		}
		if (in_parent) {
			if (parent_node)
				re_addChild(parent_node, tmp_re);
			else {
				parent_node = tmp_re;
				re_setNext(last_node, parent_node);
			}
		}
		else {
			if (parent_node) {
				for (int b = 0; b < re_getChildren(parent_node); b++) {
					re_setNext(re_getChild(parent_node, b), tmp_re);
				}
				re_setNext(parent_node, tmp_re);
				parent_node = NULL;
			}
			else 
				re_setNext(last_node, tmp_re);
		}
		last_node = tmp_re;
	}
	return first_node;
}
/*** 					matching 				    ***/
/* match is called from the root node & text. *//** 	      prototypes 	        **/
/* match-here is called when the text is to be*/bool match(regex instance, char* text);
/* moved.                                     */bool m_here(regex instance, char* text);

/*                                            */bool m_char(regex instance, char* text);
/*                                            */bool m_star(regex instance, char* text);
/*                                            */bool m_plus(regex instance, char* text);
/*                                            */bool m_optional(regex instance, char* text);

/*                                            */bool m_alpha(regex instance, char* text);
/*                                            */bool m_digit(regex instance, char* text);
/*                                            */bool m_alphanum(regex instance, char* text);

/*                                            */bool m_parent_char(regex instance, char* text);
/*                                            */bool m_parent_star(regex instance, char* text);
/*                                            */bool m_parent_plus(regex instance, char* text);
/*                                            */bool m_parent_optional(regex instance, char* text);
/** 	      implementation         **/
//returns true if the expression found a match in the text, otherwise it returns false.
bool match(regex expression, char* text) {
	do {
		if(PRINT_MESSAGES)printf("<-match-loop->\n");
		if (m_here(expression, text))
			return true;
	} while(*text++ != '\0');
	return false; 
}
bool m_here(regex instance, char* text) {
	if (!instance) 			//out of regex to execute, meaning all rules
		return true; 		//were passed correctly
	//process rules
	int children = re_getChildren(instance);
	switch(re_getRule(instance)) {
		case (R_CHAR):
			if (children == 0)
				return m_char(instance,text);
			else
				return m_parent_char(instance,text);
		case (R_STAR):
			if (children == 0)
				return m_star(instance, text);
			else
				return m_parent_star(instance,text);
		case (R_PLUS):
			if (children == 0)
				return m_plus(instance,text);
			else
				return m_parent_plus(instance,text);
		case (R_OPT):
			if (children == 0)
				return m_optional(instance, text);
			else
				return m_parent_optional(instance,text);
		case (R_ALPHA):
			return m_alpha(instance, text);
		case (R_DIGIT):
			return m_digit(instance, text);
		default:
			break;
	}
	//
	if (*text == '\0') 			//rules left, but no text left. FAIL
		return false;
	return false;
}
//match a single literal
bool m_char(regex instance, char* text) {
	if(PRINT_MESSAGES){printf("m_char: "); re_print(instance); printf("%s\n", text);}
	if(re_getLiteral(instance) == *text)
		return m_here(re_getNext(instance), text+1);
	else return false;
}
//match the instance 0 or more times
bool m_star(regex instance, char* text) {
	if(PRINT_MESSAGES){printf("m_star: "); re_print(instance); printf("%s\n", text);}
	do {
		if (m_here(re_getNext(instance), text))
			return true;
	} while (*text != '\0' && re_getLiteral(instance) == *text++);
	return false;
}
//match one or more times
bool m_plus(regex instance, char* text) {
	if(PRINT_MESSAGES){printf("m_plus: "); re_print(instance); printf("%s\n", text);}
	while (*text != '\0' && re_getLiteral(instance) == *text++) {
		if (m_here(re_getNext(instance), text+1))
			return 1;
	}
	return 0;
}
//match ZERO or ONE times
bool m_optional(regex instance, char* text) {
	if(PRINT_MESSAGES){printf("m_opt: "); re_print(instance); printf("%s\n", text);}
	if (m_char(instance, text)) 		//if we find the character
		return m_here(re_getNext(instance), text+1);
	else 	//if we did not find the literal. Don't move text, but bring the regex forward
		return m_here(re_getNext(instance), text);
}
//mathes an alphabetical character
bool m_alpha(regex instance, char* text) {
	if(PRINT_MESSAGES){printf("m_alpha: "); re_print(instance); printf("%s\n", text);}
	if (isalpha(*text))
		return m_here(re_getNext(instance), text+1);
	return false;
}
bool m_digit(regex instance, char* text) {
	if(PRINT_MESSAGES){printf("m_digit: "); re_print(instance); printf("%s\n", text);}
	if (isdigit(*text))
		return m_here(re_getNext(instance), text+1);
	return false;
}
bool m_alphanum(regex instance, char* text) {
	if(PRINT_MESSAGES){printf("m_alphanum: "); re_print(instance); printf("%s\n", text);}
	if (isalnum(*text))
		return m_here(re_getNext(instance), text+1);
	return false;
}
//match a children zero or many times
bool m_parent_star(regex parent, char* text) {
	if(PRINT_MESSAGES){printf("mp_star\n");}
	int child_index = 0;
	int total_children = re_getChildren(parent);
	for (child_index = 0; child_index < total_children; child_index++) {
		char* tmp_text = text;
		regex child = re_getChild(parent, child_index);
		do {
			if(m_here(child, tmp_text))
				return true;
		}  while(*tmp_text != '\0' && m_char(child,tmp_text++));
	}
	return false;
}
//returns true if the match for each 
bool m_parent_char(regex parent, char* text) {
	if(PRINT_MESSAGES){printf("mp_char\n");}
	int total_children = re_getChildren(parent);
	for (int child_index = 0; child_index < total_children; child_index++){
		regex child = re_getChild(parent, child_index);
		if (m_here(child,text))
			return true;
	};
	return false;
}
//match a child one or more times. if none of the children match once or more, return false
bool m_parent_plus(regex parent, char* text) {
	if(PRINT_MESSAGES)printf("mp_plus\n");
	int total_children = re_getChildren(parent);
	for (int child_index = 0; child_index < total_children; child_index++) {
		char* tmp_text = text;
		regex child = re_getChild(parent, child_index);
		while (*tmp_text++ != '\0') {
			if (m_here(child,tmp_text))
				return true;
		}
	}
	return false;
}
//returns true if the match at the next element is valid or if any of the children are valid
bool m_parent_optional(regex parent, char* text) {
	if(PRINT_MESSAGES)printf("mp_opt\n");
	int total_children = re_getChildren(parent);
	for (int a = 0; a < total_children; a++ ) {
		if (m_here(re_getChild(parent, a),text))
			return true;
	}
	return m_here(re_getNext(parent), text);
}
//
//
//
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
	input_text = calloc(1,input_len); //calloc to initialize memory to 0. unsure of the importance, but valgrind wasn't happy without it
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
	regex regexpr = re_create_f_str(regex_expression);
	regex instance = regexpr;
	while(instance) {
		if(PRINT_MESSAGES) {
			re_print(instance);
			printf("\n");
		}
		if (re_getChildren(instance) != 0) {
			if(PRINT_MESSAGES)for (int b = 0; b < re_getChildren(instance); b++) {
				printf("*\t");
				re_print(re_getChild(instance, b));
				printf("\n");
			}
			instance = re_getNext(re_getChild(instance,0));
		}
		else instance = re_getNext(instance);
		
	}
	printf("%s\n", (match(regexpr, input_text))? "match" : "no match");
	instance = regexpr;
	while(instance) {
		regex next_inst= re_getNext(instance);
		int children = re_getChildren(instance);
		for (int b = 0 ; b < children; b++) {
			if (re_getChildren(re_getChild(instance,b)) != 0) printf("WARNING PARENT->CHILD->CHILD, NOT FREED\n");
			re_destroy(re_getChild(instance,b));
		}
		re_destroy(instance);
		instance = next_inst;
	}
	free(input_text);
}
