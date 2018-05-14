#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "datastructs.h"
#include "lexer.h"
#include "parser.h"

#define IDTABSTACK_BUFSIZE 1024
#define SYMTABSTACK_BUFSIZE 1024
#define HASHTABLE_BASIS 17

HashtableElement** hashtable;

char* getTokenName(int);

static void throwExeption(char* msg, ...){
	va_list ptr;	
	va_start(ptr, msg);
	printf("Error on line %i. ", line);
	vprintf(msg, ptr);
	va_end(ptr);
	getchar();
	exit(-1);
}

int getTypeWidth(Type* tp){
	char* s = tp->lexeme;	
	if(strcmp(s, "char" ) == 0) return 1;
	if(strcmp(s, "bool" ) == 0) return 1;		
	if(strcmp(s, "int"  ) == 0) return 4;		
	if(strcmp(s, "float") == 0) return 8;
	return 1;
}

//Use only for type token!!! (int,float,char,bool)
Type* type(Token* tokenType){
	Type* type = malloc(sizeof(Type));
	type->lexeme = tokenType->lexeme;
	type->size = 1;
	type->width = getTypeWidth(type);
	type->arrayType = NULL;
	return type;
}

Type* typeArray(Type* tp, int size){
	Type* typePt = malloc(sizeof(Type));
	typePt->lexeme = "[]";
	typePt->size = size;
	typePt->width = (tp->width)*size;
	typePt->arrayType = tp;
	return typePt;
}

Identifier* identifier(Type* type){
	Identifier* id = malloc(sizeof(Identifier));
	id->type = type;
	return id;
}

int isNumeric(Type* type){
	if(strcmp(type->lexeme, "char") == 0) return 1;
	if(strcmp(type->lexeme, "int") == 0) return 1;
	if(strcmp(type->lexeme, "float") == 0) return 1;
	return 0;
}

Type* maxType(Type* type1, Type* type2){
	if(!isNumeric(type1) || !isNumeric(type2)) return NULL;
	if(strcmp(type1->lexeme, "float") == 0) return type1;
	if(strcmp(type2->lexeme, "float") == 0) return type2;	
	if(strcmp(type1->lexeme, "int") == 0) return type1;
	if(strcmp(type2->lexeme, "int") == 0) return type2;
	if(strcmp(type1->lexeme, "char") == 0) return type1;
	if(strcmp(type2->lexeme, "char") == 0) return type2;
	return NULL;
}

Type* checkBoolType(Type* type1, Type* type2){
	if(strcmp(type1->lexeme, "bool") == 0 && strcmp(type2->lexeme, "bool") == 0) return type1;
	return NULL;
}

Type* checkRelType(Type* type1, Type* type2){
	if(strcmp(type1->lexeme, "[]") == 0 || strcmp(type2->lexeme, "[]") == 0) return NULL;
	if(strcmp(type1->lexeme, type2->lexeme) == 0) return type(hashtableGetValue(hashtable, "bool"));
	return NULL;
}

/*********Stack enviroment operations*********/
Stack* idHashtabStack;

void idHashtabStackPut(char* key, Node* value ){
	if(idHashtabStack == NULL) throwExeption("Stack for identifier hashtable is not initialized");
	HashtableElement** hashtable = (idHashtabStack->stackFrsElement[idHashtabStack->stackPointer]);
	hashtableAdd(hashtable, key, value);
}

Node* idHashtabStackGet(char* key){
	if(idHashtabStack == NULL) throwExeption("Stack for identifier hashtable is not initialized");
	Node* identifier;
	int stackPointer = idHashtabStack->stackPointer;
	HashtableElement** hashtable;	
	while (stackPointer >= 0){
		hashtable = (idHashtabStack->stackFrsElement[stackPointer]);
		identifier = hashtableGetValue(hashtable,key);
		if(identifier != NULL)
			return identifier;
		stackPointer--;
	}
	return NULL;
}

/***********Grammar productions****************/
Token* look;
FILE* sourseCodeFile;
int used;
int blockUsed;

void move(){ look = getNextToken(sourseCodeFile, hashtable); }

void match (int tag){
	if(tag == look->tag){
		move();
		return;
	}
	throwExeption("Expected token %s, actual token is %s", tokenTagToString(tag), look->lexeme);
}

Node* exprNode(Token* token, Type* type, Node* chNode1, Node* chNode2, Node* chNode3){
	Node* node = malloc(sizeof(Node));
	node->token = token;
	node->type = type;
	node->chNode1 = chNode1;
	node->chNode2 = chNode2;
	node->chNode3 = chNode3;	
	switch(token->tag){
		case NUM: case REAL: case TRUE: case FALSE:
			node->tag = CONST;
		break;
		case AND: case OR:
			node->tag = COND;
		break;
		case EQ: case NE: case GE: case LE: case '>': case '<':
			node->tag = REL;
		break;
		case '+': case '-': case '*': case '/':
			node->tag = OP;
		break;
		case '!':
			node->tag = NOT;
		break;
		default:
			node->tag = token->tag;
		break;
	}
	return node;
}

Node* stmtNode(int tag, Node* chNode1, Node* chNode2, Node* chNode3){
	Node* node = malloc(sizeof(Node));
	node->tag = tag;
	node->token = NULL;
	node->type = NULL;
	node->chNode1 = chNode1;
	node->chNode2 = chNode2;
	node->chNode3 = chNode3;
	return node;
}

Node* block();
void decls();
Type* typeProd();
Type* dims(Type*);
Node* stmts();
Node* stmt();
Node* assign();
Node* bool();
Node* join();
Node* equality();
Node* rel();
Node* expr();
Node* term();
Node* unary();
Node* factor();
Node* offset(Node*);

Node* enclosing;

Node* program(FILE* fp){
	sourseCodeFile = (FILE*)fp;
	hashtable = idHashtableInitialize(HASHTABLE_BASIS);
	enclosing = stmtNode(STMT,NULL,NULL,NULL);
	idHashtabStack = stackCreate(SYMTABSTACK_BUFSIZE);
	used = 0;
	move();
	return block();
}

Node* block(){
	int blockUsed = used;	
	printf("block\n");
	match('{');
	HashtableElement** hashtableFstElement = hashtableCreate(HASHTABLE_BASIS);
	stackPush(idHashtabStack, hashtableFstElement);
	decls();
	Node* s = stmts();
	match('}');
	stackPop(idHashtabStack);
	used = blockUsed;
	//hashtableDelete(hashtableFstElement);
	return s;
}

void decls(){
	printf("decls\n");
	while(look->tag == BASIC){		
		Type* type = typeProd();
		Token* idToken = look;
		match(ID); match(';');		
		Node* id = exprNode(idToken,type,NULL,NULL,NULL);
		id->offset = used;
		idHashtabStackPut(idToken->lexeme, id);
		used += type->width;
	}
}

Type* typeProd(){
	printf("typeProd\n");
	Token* token = look;
	match(BASIC);
	if(look->tag == '[') return dims(type(token));
	return type(token);
}

Type* dims(Type* tp){
	printf("dims\n");
	match('[');
	Token* sizeToken = look; match(NUM);
	int size = sizeToken->numValue;
	match(']');
	if(look->tag == '[') { tp = dims(tp); }
	return typeArray(tp, size);
}

Node* stmts(){
	printf("stmts\n");
	Node *s, *s1;
	if( look->tag == '}' ) return stmtNode(STMT, NULL, NULL, NULL);
	s = stmt();
	s1 = stmts();
	if(s1->tag == STMT) return s; //?
	return stmtNode(SEQ,s,s1,NULL);
}
 
Node* stmt(){
	printf("stmt\n");
	Node *x,*s1,*s2,*saveStmt,*whileNode,*doNode,*breakNode;
	switch(look->tag){		
		case ';': 
			move();
			return NULL;
		case IF:
			move();
			match('(');
			x = bool();
			match(')');
			s1 = stmt();
			if(look->tag != ELSE) return stmtNode(IF,x,s1,NULL);
			match(ELSE);
			s2 = stmt();
			return stmtNode(ELSE,x,s1,s2);
		case WHILE:	
			whileNode = stmtNode(WHILE,NULL,NULL,NULL);
			saveStmt = enclosing;
			enclosing = whileNode;
			move(); match('(');
			x = bool(); match(')');
			s1 = stmt();
			whileNode->chNode1 = x;
			whileNode->chNode2 = s1;
			enclosing = saveStmt;
			return whileNode;
		case DO:
			doNode = stmtNode(DO,NULL,NULL,NULL);
			saveStmt = enclosing;
			enclosing = doNode;
			move(); 
			s1 = stmt();
			match(WHILE); match('(');
			x = bool(); match(')');
			match(';');
			doNode->chNode1 = s1;
			doNode->chNode2 = x;
			enclosing = saveStmt;
			return doNode;
		case BREAK:
			move(); match(';');
			if((enclosing->tag == WHILE) ||  (enclosing->tag == DO))
				breakNode = stmtNode(BREAK,enclosing,NULL,NULL);
			else throwExeption("Break is not in cycle construction");
			return breakNode;
		case BASIC:
			decls();
			return stmt();
		case '{':
			return block();
		default :
			return assign();
	}	
}

Node* assign(){
	printf("assign\n");
	Node *id, *stmt, *x;
	Token* t = look;	
	match(ID);
	id = idHashtabStackGet(t->lexeme);
	if(id == NULL) throwExeption("Id %s is undeclared", t->lexeme);
	if(look->tag == '='){
		move(); stmt = stmtNode(SET,id,bool(),NULL);
	} else {
		x = offset(id);
		match('='); stmt = stmtNode(SETELEMENT,x,bool(),NULL);
	}
	match(';');
	return stmt;
}

Node* bool(){
	printf("bool\n");
	Node *x1, *x2; Type *x3;
	x1 = join();
	while( look->tag == OR){
		Token* t = look; move();
		x2 = join();
		x3 = checkBoolType(x1->type,x2->type);
		if(x3 == NULL)throwExeption("Type is not boolean");		
		x1 = exprNode(t,x3,x1,x2,NULL);
	}
	return x1;
}

Node* join(){
	printf("join\n");
	Node *x1, *x2; Type *x3;
	x1 = equality();
	while( look->tag == AND ){
		Token* t = look; move();
		x2 = equality();
		x3 = checkBoolType(x1->type,x2->type);
		if(x3 == NULL)throwExeption("Type is not boolean");	
		x1 = exprNode(t,x3,x1,x2,NULL);
	}
	return x1;
}

Node* equality(){
	printf("equality\n");
	Node *x1, *x2; Type *x3;
	x1 = rel();
	while(look->tag == EQ || look->tag == NE){
		Token* t = look; move();
		x2 = rel();
		x3 = checkRelType(x1->type,x2->type);
		if(x3 == NULL)throwExeption("Type is not boolean");	
		x1 = exprNode(t,x3,x1,x2,NULL);
	}
	return x1;
}

Node* rel(){
	printf("rel\n");
	Token* t;
	Node *x1, *x2; Type *x3;
	x1 = expr();
	switch(look->tag){
		case '<': case LE: case GE: case '>':
			t = look; move();
			x2 = rel();			
			x3 = checkRelType(x1->type,x2->type);
			if(x3 == NULL)throwExeption("Type is not boolean");	
			return exprNode(t,x3,x1,x2,NULL);
		default:
			return x1;
	}
}

Node* expr(){
	printf("expr\n");
	Node *x1, *x2; Type *x3;
	x1 = term();
	while(look->tag == '+' || look->tag == '-'){
		Token* t = look; move();
		x2 = term();
		x3 = maxType(x1->type,x2->type);
		if(x3 == NULL)throwExeption("Implicite casting");	
		x1 = exprNode(t,x3,x1,x2,NULL);
	}
	return x1;
}

Node* term(){
	printf("term\n");
	Node *x1, *x2; Type *x3;
	x1 = unary();
	while(look->tag == '*' || look->tag == '/'){
		Token* t = look; move();
		x2 = term();
		x3 = maxType(x1->type,x2->type);
		if(x3 == NULL)throwExeption("Implicite casting");	
		x1 = exprNode(t,x3,x1,x2,NULL);
	}
	return x1;
}

Node* unary(){
	printf("unary\n");
	Token* t;
	Node* x1;
	if(look->tag == '-'){
		t = look; move();
		x1 = unary();
		if(!isNumeric(x1->type))throwExeption("Type is not numeric");	
		Node* n = exprNode(t,x1->type,x1,NULL,NULL);
		n->tag = MINUS;
		return n;
	} else 
	if(look->tag == '!'){
		t = look; move();
		x1 = unary();
		if(strcmp(x1->type->lexeme,"bool") != 0)throwExeption("Type is not numeric");	
		Node* n = exprNode(t,x1->type,x1,NULL,NULL);
		n->tag = NOT;
		return n;
	} else {
		return factor();
	}
}

Node* factor(){
	printf("factor\n");
	Node* x = NULL;
	switch(look->tag){
		case '(':
			move(); x = bool(); match(')');
			return x;
		case NUM:
			x = exprNode(look,type(hashtableGetValue(hashtable, "int")),NULL,NULL,NULL);
			move(); return x;
		case REAL:
			x = exprNode(look,type(hashtableGetValue(hashtable, "float")),NULL,NULL,NULL);
			move(); return x;
		case TRUE:
			x = exprNode(look,type(hashtableGetValue(hashtable, "bool")),NULL,NULL,NULL);
			move(); return x;
		case FALSE:
			x = exprNode(look,type(hashtableGetValue(hashtable, "bool")),NULL,NULL,NULL);
			move(); return x;
		case ID:
			x = idHashtabStackGet(look->lexeme);
			if(x == NULL) throwExeption("Id %s is undeclared", look->lexeme);
			move(); 
			if(look->tag != '[') return x;
			else return offset(x);
		default:
			throwExeption("Syntax error %s", look->lexeme);
			return x;
	}
}

Node* offset(Node* id){
	printf("offset\n");
	Node *i, *w, *t1, *t2, *loc; 	
	Type* t = id->type;
	Type* intType = type(hashtableGetValue(hashtable, "int"));
	match('['); i = bool(); match(']');
	t = t->arrayType;
	if(t == NULL)throwExeption("Id %s is not array", id->token->lexeme);
	w = exprNode(tokenNum(t->width),intType,NULL,NULL,NULL);
	t1 = exprNode(token('*'),intType,i,w,NULL);
	loc = t1;
	while( look->tag == '[' ){
		match('['); i = bool(); match(']');	
		t = t->arrayType;
		if(t == NULL) throwExeption("Id %s is not array", id->token->lexeme);	
		w = exprNode(tokenNum(t->width),intType,NULL,NULL,NULL);
		t1 = exprNode(token('*'),intType,i,w,NULL);	
		t2 = exprNode(token('+'),intType,loc,t1,NULL);	
		loc = t2;
	}
	return exprNode(tokenWord(INDEX, "[]"),t,id,loc,NULL);
}