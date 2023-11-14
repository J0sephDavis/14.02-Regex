#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

/***                           	  the regex class                                   ***/
struct regex_t {
	int rule;
	int literal;
	struct regex_t* next;
	int c_count; //count of children
	struct regex_t** children;
	//char* list of symbols to find a match in? possibly the SEQUENCE of characters(i.e., word) to find a match for first..
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
	//R_WILD -> might be handled by like 27 regex children to an element? Or a char* of all the valid characters
};
char* rules_names[] = {
	"char",
	"star",
	"plus",
	"opt"
};
//the symbols for every rule (besides a character...)
char* rule_symbols = "*+?\\";
/** 	      prototypes 	        **/
regex re_create(int, int);
void re_destroy(regex);   
int re_gRule(regex instance);
void re_setRule(regex, int);
int re_gLiteral(regex);
regex re_getNext(regex);
void re_setNext(regex, regex);
int re_getChildren(regex);
void re_addChild(regex, regex);
regex re_getChild(regex, int);
void re_print(regex);
//
regex re_create(int _rule, int _literal) {
	regex instance = calloc(1, sizeof(struct regex_t));
	instance->rule = _rule;
	instance->literal = _literal;
	instance->next = NULL;
	instance->c_count = 0;
	return instance;
}
void re_destroy(regex instance) {
	//printf("FREE:"); re_print(instance); printf("\n");
	free(instance);
}
int re_gRule(regex instance) {
	return instance->rule;
}
void re_setRule(regex instance, int _rule) {
	instance->rule = _rule;
}
int re_gLiteral(regex instance) {
	return instance->literal;
}
regex re_getNext(regex instance) {
	return instance->next;
}
void re_setNext(regex instance, regex next_instance) {
	instance->next = next_instance;
}
int re_getChildren(regex instance) {
	return instance->c_count;
}
void re_addChild(regex instance, regex child) {
	if (!instance->children) { 		//if there are no children, allocate
		instance->children = calloc(1, sizeof(struct regex_t**));
	}
	else { 					//if there are some children, reallocate
		instance->children = realloc(instance->children, sizeof(struct regex_t**) * (instance->c_count+1));
	}
	instance->children[instance->c_count++] = child;
}
regex re_getChild(regex instance, int index) {
	if (index > re_getChildren(instance) || index < 0) {
		return NULL;
	}
	else
		return instance->children[index];
}
void re_print(regex instance) {
	if (instance)
		printf("[Rule: %s | Literal: %c | Next? : %s | Children: %d]",
				rules_names[instance->rule], instance->literal,
				(instance->next) ? "Yes" : "No",
				re_getChildren(instance));
	else printf("[-]");
}
/*** 					matching 				    ***/
/* match is called from the root node & text. *//** 	      prototypes 	        **/
/* match-here is called when the text is to be*/bool match(regex instance, char* text);
/* moved.                                     */bool m_here(regex instance, char* text);

/*                                            */bool m_char(regex instance, char* text);
/*                                            */bool m_star(regex instance, char* text);
/*                                            */bool m_plus(regex instance, char* text);
/*                                            */bool m_optional(regex instance, char* text);

/*                                            */bool m_parent_char(regex instance, char* text);
/*                                            */bool m_parent_star(regex instance, char* text);
/*                                            */bool m_parent_plus(regex instance, char* text);
/*                                            */bool m_parent_optional(regex instance, char* text);
/** 	      implementation         **/
//returns true if the expression found a match in the text, otherwise it returns false.
bool match(regex expression, char* text) {
	do {
		printf("<-match-loop->\n");
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
//	if (re_getChildren(instance) != 0) {
//		for (int a = 0; a < re_getChildren(instance); a++) {
//			if (m_here(re_getChild(instance,a), text))
//				return true;
//		}
//		return false;
//	}
	switch(re_gRule(instance)) {
		case (R_CHAR):
			if (children == 0)
				return m_char(instance,text);
			else
				return m_parent_char(instance,text);
		case (R_STAR):
			if (children == 0)
			return m_star(instance, text);
			else return m_parent_star(instance,text);
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
		default:
			break;
	}
	//rules over
	if (*text == '\0') 			//rules left, but no text left. FAIL
		return false;
	return false;
}
//match a single literal
bool m_char(regex instance, char* text) {
	printf("m_char: "); re_print(instance); printf("%s\n", text);
	if(re_gLiteral(instance) == *text)
		return m_here(re_getNext(instance), text+1);
	else return false;
}
//match the instance 0 or more times
bool m_star(regex instance, char* text) {
	printf("m_star: "); re_print(instance); printf("%s\n", text);
	do {
		if (m_here(re_getNext(instance), text+1))
			return 1;
	} while (*text != '\0' && re_gLiteral(instance) == *text++);
	return 0;
}
//match ZERO or ONE times
bool m_optional(regex instance, char* text) {
	if (m_char(instance, text)) 		//if we find the character
		return m_here(re_getNext(instance), text+1);
	else 	//if we did not find the literal. Don't move text, but bring the regex forward
		return m_here(re_getNext(instance), text);
}
//match one or more times
bool m_plus(regex instance, char* text) {
	printf("m_plus: "); re_print(instance); printf("%s\n", text);
	while (*text != '\0' && re_gLiteral(instance) == *text++) {
		if (m_here(re_getNext(instance), text+1))
			return 1;
	}
	return 0;
}
//match a children zero or many times
bool m_parent_star(regex parent, char* text) {
	printf("mp_star\n");
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
	printf("mp_char\n");
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
	printf("mp_plus\n");
	int total_children = re_getChildren(parent);
	for (int child_index = 0; child_index < total_children; child_index++) {
		regex child = re_getChild(parent, child_index);
		while (*text != '\0') {
			if (m_here(child,text))
				return true;
		}
		return false;
	}
	return (parent != NULL && text != NULL);
}
//returns true if the match at the next element is valid or if any of the children are valid
bool m_parent_optional(regex parent, char* text) {
	printf("mp_opt\n");
	int total_children = re_getChildren(parent);
	for (int a = 0; a < total_children; a++ ) {
		if (m_here(re_getChild(parent, a),text))
			return true;
	}
	return m_here(re_getNext(parent), text);
}
/*** 				regex compilation 				    ***/
regex re_create_f_str(char* regexp) {
	if (regexp == NULL) 			//if we are given an empty string
		return NULL; 			//return nothing
	regex first_node = NULL;	 	//pointer to the first node (also returned at the end)
	regex last_node = NULL; 		//point to the last node made.
	regex parent_node = NULL; 		//pointer to the parent_node.
	int literal; 				//stores the current character in the regexp
	int rule; 				//stores the current rule
	int a; 					//iterator
	bool in_parent = false; 		//is the character string still inside the parentheses?
	bool last_backslash = false; 		//isntead of using a regexp[a] == '\' && regexp[a+1] == symbol... we just set a flag. probably inefficient
//iterate over all the passed characters
	const bool print_info = false;
	//we could handle this without a for loop, a while loop(or do-while) that increments the *regexp will work just as well
	for (a = 0; regexp[a] != '\0'; a++) {
		//print information
		if (print_info) {
			printf("-------------------------------\n");
			if (parent_node)
				{printf("PARENT:\t"); re_print(parent_node); printf("\n"); }
			if (last_node)
				{printf("LAST:\t"); re_print(last_node); printf("\n"); }
			printf("EXPR:\t[%s]-\n", regexp+a);
		}
		rule = R_CHAR;
		literal = regexp[a];
//if the current regex char, modifies the LAST node.
//we 'continue' after each call because we aren't making a new regex
		if (!in_parent && parent_node) { 	//meaning we found a ')' last iteration,
			if (print_info)
				printf(">ready to apply rule to parent\n");
			//STAR
			if (regexp[a] == '*') {
				re_setRule(parent_node, R_STAR);
				continue;
			}
			//PLUS
			else if (regexp[a] == '+') {
				re_setRule(parent_node, R_PLUS);
				continue;
			}
			//OPTIONAL
			else if (regexp[a] == '?') {
				re_setRule(parent_node, R_OPT);
				continue;
			}
			
		}
		else if (!last_backslash) {
			if (print_info)
				printf(">read to apply rule to LAST\n");
			//STAR
			if (regexp[a] == '*') {
				re_setRule(last_node, R_STAR);
				continue;
			}
			//PLUS
			else if (regexp[a] == '+') {
				re_setRule(last_node, R_PLUS);
				continue;
			}
			//OPTIONAL
			else if (regexp[a] == '?') {
				re_setRule(last_node, R_OPT);
				continue;
			}
		}
		if (last_backslash) last_backslash = false;
		else if (regexp[a] == '\\') {
			last_backslash = true;
			continue;
		}
		if (print_info)
			printf(">NO RULES APPLIED\n");
//else if BRACE,
		//if we aren't in a parent & find the opening
		//set parent to true & continue
		if (!in_parent && regexp[a] == '(') {
			if (print_info)
				printf(">in_parent = TRUE\n");
			in_parent = true;
		}
		else if (in_parent && regexp[a] == ')') {
			if (print_info)
				printf(">in_parent = FALSE\n");
			in_parent = false;
			continue; //skip to the next entry so we can link the children to the new
		}
//create character regex
		regex tmp_re = re_create(rule,literal);
		if(print_info) {
			printf(">SPAWNED: "); re_print(tmp_re); printf("\n");
		}
//if FIRST, set the first node
		if (!first_node) {
			if(print_info) {
				printf(">A:!first_node");
				if (in_parent && !parent_node) {
					printf(" && in_parent && !parent_node: set parent & first & last\n");
				}
				else
					printf(": set first & last\n");
			}
			first_node = tmp_re;
			last_node = tmp_re;
			if (in_parent && !parent_node){
				parent_node = tmp_re;
			}
			continue;
		}
//if in_parent, but an orphan. set parent
		if (in_parent && !parent_node) {
			if (print_info)
				printf(">B:in_parent && !parent_node: set parent\n");
			parent_node = tmp_re;
			re_setNext(last_node, parent_node);
		}
//if in_parent, add new regex to parent
		else if (in_parent && parent_node) {
			if(print_info) {
				printf(">C:in_parent && parent_node: add child\n");
			}
			re_addChild(parent_node, tmp_re);
		}
//if !in_parent, with parent_node set. Link the children to the new-node
		else if (!in_parent && parent_node) {
			if (print_info)
				printf(">D:!in_parent && parent_node: link children\n");
			for (int b = 0; b < re_getChildren(parent_node); b++) {
				re_setNext(re_getChild(parent_node, b), tmp_re);
				if (rule != R_CHAR) re_setRule(re_getChild(parent_node, b), rule);
			}
			re_setNext(parent_node, tmp_re);
			parent_node = NULL;
		}
//else, add new regex to last->next.
		else { 
			if(print_info)
				printf(">E: linking last_node to new node\n");
			re_setNext(last_node, tmp_re);
		}
		last_node = tmp_re;
/* Consider labelling the parent-node with a special rule or flag to indicate that its contents are ignored?
 * */
		//when in_parent is ended on previous iteration. thus leaving the kids as NULL if the stream ends
	}
	return first_node;
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
	printf("REGEX:%s\nTEXT:%s\n", regex_expression, input_text);
	regex regexpr = re_create_f_str(regex_expression);
	regex instance = regexpr;
	while(instance) {
		re_print(instance);
		printf("\n");
		if (re_getChildren(instance) != 0) {
			for (int b = 0; b < re_getChildren(instance); b++) {
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
