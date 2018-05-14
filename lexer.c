#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "datastructs.h"
#include "lexer.h"

static char strTemp[100];

static void throwExeption(char* msg, ...){
	va_list ptr;	
	va_start(ptr, msg);
	printf("Error on line %i. ", line);
	vprintf(msg, ptr);
	va_end(ptr);
	getchar();
	exit(-1);
}

char* tokenToString(Token* t){
	
	if(t->tag < 256){
		sprintf(strTemp, "%c", (char)t->tag);
		return strTemp;
	}
	if(t->tag == NUM){
		sprintf(strTemp, "%n", t->numValue);
		return strTemp;		
	}
	if(t->tag == REAL){
		sprintf(strTemp, "%f", t->realValue);
		return strTemp;		
	}
	
	return t->lexeme;	
}

char* tokenTagToString(int tag){

	if(tag < 256){
		sprintf(strTemp, "%c", (char)tag);
		return strTemp;
	}
	
	switch(tag){
		case AND: 	return "&&";
		case BASIC: return "BASIC";
		case BREAK: return "BREAK";
		case DO:	return "DO";
		case ELSE:	return "ELSE";
		case EQ:	return "==";
		case FALSE:	return "FALSE";
		case GE:	return ">=";
		case ID:	return "ID";
		case IF:	return "IF";
		case INDEX:	return "INDEX";
		case LE:	return "<=";				
		case MINUS:	return "-";
		case NE:	return "!";
		case NUM:	return "NUM";
		case OR:	return "OR";
		case REAL:	return "REAL";
		case TEMP:	return "TEMP";
		case TRUE:	return "TRUE";
		case WHILE:	return "WHILE";					
	}	
	
	sprintf(strTemp, "%c", (char)tag);
	return strTemp;
}


Token* token(int t){
	Token* token = malloc(sizeof(Token));
	token->tag = t;
	return token; 
}

Token* tokenNum(int v){
	Token* token = malloc(sizeof(Token));
	token->tag = NUM;
	token->numValue = v;
	return token; 
}

Token* tokenReal(float v){
	Token* token = malloc(sizeof(Token));
	token->tag = REAL;
	token->realValue = v;
	return token; 
}

Token* tokenWord(int t, char* s){
	Token* token = malloc(sizeof(Token));
	token->tag = t;
	token->lexeme = malloc(strlen(s) + 1);
	strcpy(token->lexeme,s);
	return token;
}

Token* tokenType(char* s){
	Token* token = malloc(sizeof(Token));
	token->tag = BASIC;
	token->lexeme = malloc(strlen(s) + 1);
	strcpy(token->lexeme,s);	
	return token;
}


void reserve(HashtableElement** hashtable, Token* t){
	hashtableAdd(hashtable, t->lexeme, t);
}

HashtableElement** idHashtableInitialize(int hashBasis){
	
	HashtableElement** hashtabFstElementPtPt = hashtableCreate(hashBasis);

	reserve(hashtabFstElementPtPt, tokenWord(IF, "if"));
	reserve(hashtabFstElementPtPt, tokenWord(ELSE, "else"));
	reserve(hashtabFstElementPtPt, tokenWord(WHILE, "while"));
	reserve(hashtabFstElementPtPt, tokenWord(DO, "do"));	
	reserve(hashtabFstElementPtPt, tokenWord(BREAK, "break"));
	reserve(hashtabFstElementPtPt, tokenWord(TRUE, "true"));
	reserve(hashtabFstElementPtPt, tokenWord(FALSE, "false"));
	
	reserve(hashtabFstElementPtPt, tokenWord(AND,"&&"));
	reserve(hashtabFstElementPtPt, tokenWord(OR,"||"));
	reserve(hashtabFstElementPtPt, tokenWord(EQ,"=="));	
	reserve(hashtabFstElementPtPt, tokenWord(NE,"!="));
	reserve(hashtabFstElementPtPt, tokenWord(LE,"<="));
	reserve(hashtabFstElementPtPt, tokenWord(GE,">="));	
	
	reserve(hashtabFstElementPtPt, tokenType("bool"));
	reserve(hashtabFstElementPtPt, tokenType("char"));
	reserve(hashtabFstElementPtPt, tokenType("int"));
	reserve(hashtabFstElementPtPt, tokenType("float"));

	return hashtabFstElementPtPt;
}

char readch(FILE* fp){
	return (char)fgetc(fp);	
}

int readAndEqualch(FILE* fp, char e){
	char peek = readch(fp);
	if(peek == e)
		return 1;
	ungetc(peek, fp);
	return 0;	
}

Token* getNextToken(FILE* fp, HashtableElement** hashtable){	

	char peek = ' ';
	
	while(1){
		if(peek == ' ' || peek == '\t' || peek == '\n'){
			if(peek == '\n') {line++;}
			peek = readch(fp);
			continue;
		}
		else {
			break;
		}
	}
		
	switch(peek){	
		case '&':
			if(readAndEqualch(fp, '&')) return hashtableGetValue(hashtable, "&&");
			return token(peek);
		case '|':
			if(readAndEqualch(fp, '|')) return hashtableGetValue(hashtable, "||");
			return token(peek);
		case '=':
			if(readAndEqualch(fp, '=')) return hashtableGetValue(hashtable, "==");
			return token(peek);
		case '!':
			if(readAndEqualch(fp, '=')) return hashtableGetValue(hashtable, "!=");
			return token(peek);			
		case '<':
			if(readAndEqualch(fp, '=')) return hashtableGetValue(hashtable, "<=");
			return token(peek);			
		case '>':
			if(readAndEqualch(fp, '=')) return hashtableGetValue(hashtable, ">=");
			return token(peek);						
	}
	
	if(peek >= '0' && peek <= '9'){
		int v = 0;
		
		do {		
			int val = peek - '0';
			v = 10 * v + val;
			peek = readch(fp);		
		} while(peek >= '0' && peek <= '9');
		
		if(peek != '.'){
			ungetc(peek, fp);	
			return tokenNum(v);
		}
		
		float x = (float)v; float d = 10;
		peek = readch(fp);
		
		while(peek >= '0' && peek <= '9'){
			x += (peek - '0')/d;
			d *= 10;
			peek = readch(fp);
		}
		
		ungetc(peek, fp);	
		return tokenReal(x);
	}
	
	if((peek >= 'A' && peek <= 'Z') || (peek >= 'a' && peek <= 'z') || (peek == '_')){		
		int i = 0;
		
		do {	
			strTemp[i++] = peek;
			if(i == 36){
				strTemp[i] = '\0';	
				throwExeption("Too long id %s. Max length of id is 35", strTemp);
			}
			peek = readch(fp);		
		} while((peek >= '0' && peek <= '9') || (peek >= 'A' && peek <= 'Z') || (peek >= 'a' && peek <= 'z') || (peek == '_'));

		strTemp[i] = '\0';		
		
		Token* id = hashtableGetValue(hashtable, strTemp);
		
		if (id == NULL){
			id = tokenWord(ID, strTemp);
			hashtableAdd(hashtable, strTemp, id);
		}
		
		ungetc(peek, fp);
		return id;
	}
	
	Token* t = token(peek);
	return t;
}