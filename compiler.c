#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "datastructs.h"
#include "lexer.h"
#include "parser.h"


#define HASHTABLE_BASIS 17
#define STR_MAXLENGTH 200
#define LOCSTR_MAXLENGTH 200

void throwExeption(char* msg, ...){
	va_list ptr;	
	va_start(ptr, msg);
	
	printf("%s. ","Error");
	vprintf(msg, ptr);
	
	va_end(ptr);
	
	getchar();
	exit(-1);
}

static char strTemp[STR_MAXLENGTH];

void nodeArrayIdToStringRec(Type* type, Token* token){
	if(type->arrayType == NULL){
		sprintf(strTemp, "ID(%s:%s", token->lexeme, type->lexeme);
		return;
	}
	nodeArrayIdToStringRec(type->arrayType, token);
	sprintf(&strTemp[strlen(strTemp)],"%s",type->lexeme);
}

char* nodeArrayIdToString(Node* nodePt){
	nodeArrayIdToStringRec(nodePt->type,nodePt->token);
	sprintf(&strTemp[strlen(strTemp)],")(%i)",nodePt->offset);
	return strTemp;
}

char* nodeTagToString(Node* nodePt){
	if(nodePt->tag < 256){
		sprintf(strTemp, "%c", (char)(nodePt->tag));
		return strTemp;
	}	
	switch(nodePt->tag){
		case SEQ:	return "SEQ";
		case STMT:	return "STMT";
		case WHILE:	return "WHILE";	
		case DO:	return "DO";
		case BREAK: return "BREAK";
		case IF:	return "IF";
		case ELSE:	return "ELSE";
		case SET:	return "SET";
		case SETELEMENT:	return "SETELEMENT";
		
		case CONST:	
			if(nodePt->token->tag == NUM){
				sprintf(strTemp, "CONST(%i:%s)", nodePt->token->numValue, nodePt->type->lexeme);			
			} else
			if(nodePt->token->tag == REAL){
				sprintf(strTemp, "CONST(%f:%s)", nodePt->token->realValue, nodePt->type->lexeme);			
			} else
			if(nodePt->token->tag == TRUE){
				sprintf(strTemp, "CONST(%s:%s)", "true", nodePt->type->lexeme);			
			} else
			if(nodePt->token->tag == FALSE){
				sprintf(strTemp, "CONST(%s:%s)", "false", nodePt->type->lexeme);			
			}
			return strTemp;
		case COND:	
			sprintf(strTemp, "COND(%s)", tokenTagToString(nodePt->token->tag));
			return strTemp;	
		case REL:	
			sprintf(strTemp, "REL(%s)", tokenTagToString(nodePt->token->tag));
			return strTemp;		
		case OP:
			sprintf(strTemp, "OP(%s:%s)", tokenTagToString(nodePt->token->tag), nodePt->type->lexeme);
			return strTemp;
		case NOT:	return "NOT";
		case MINUS:	return "MINUS";		
		case INDEX:	return "INDEX";						
		case ID:	
			if(nodePt->type->arrayType != NULL){
				nodeArrayIdToString(nodePt);
				return strTemp;
			}
			sprintf(strTemp, "ID(%s:%s)(%i)", nodePt->token->lexeme, nodePt->type->lexeme,  nodePt->offset);
			return strTemp;		
		case TEMP:	return "TEMP";				
	}		
	sprintf(strTemp, "UNDEF_%i", nodePt->tag);
	return strTemp;
}

void writeSyntaxTree(Node* nodePt, int indent){
	for(int i = 0; i < indent; i++){
		printf("\t");
	}
	printf("%s\n",nodeTagToString(nodePt));	
	if(nodePt->tag == BREAK) return;
	if(nodePt->chNode1 != NULL)writeSyntaxTree(nodePt->chNode1, indent + 2);
	if(nodePt->chNode2 != NULL)writeSyntaxTree(nodePt->chNode2, indent + 2);
	if(nodePt->chNode3 != NULL)writeSyntaxTree(nodePt->chNode3, indent + 2);
}


FILE* fpIn;
FILE* fpOut;
static int labels;

int newLabel(){
	return ++labels;
}

void emit(char* s){
	char strTemp[LOCSTR_MAXLENGTH];
	sprintf(strTemp,"\t%s", s);
	fputs(strTemp,fpOut);
}
void emitgoto(int i){
	char strTemp[25];
	sprintf(strTemp,"\tgoto L%i\n", i);
	fputs(strTemp,fpOut);
}
void emitlabel(int i){
	char strTemp[20];
	sprintf(strTemp,"L%i:", i);
	fputs(strTemp,fpOut);
}
void emitjumps(char* test, int t, int f){
	char strTemp[LOCSTR_MAXLENGTH];
	if((t!=0)&&(f!=0)){
		sprintf(strTemp,"if %s goto L%i\n", test, t);emit(strTemp);
		sprintf(strTemp,"goto L%i\n", f);emit(strTemp);
	} else if(t!=0){
		sprintf(strTemp,"if %s goto L%i\n", test, t);emit(strTemp);
	} else {
		sprintf(strTemp,"iffalse %s goto L%i\n", test, f);emit(strTemp);
	}
}

static char tempName[20];
static int tempNameCount;

char* getTempName(){
	sprintf(tempName,"t%i", ++tempNameCount);
	return tempName;
}

char* nodeToString(Node* nodePt){	
	char* strPt = malloc(STR_MAXLENGTH);
	char* strPt1;
	char* strPt2;
	
	switch(nodePt->tag){
		case OP: case REL: case COND:
			strPt1 = nodeToString(nodePt->chNode1);
			strPt2 = nodeToString(nodePt->chNode2);
			sprintf(strPt,"%s %s %s", strPt1, tokenTagToString(nodePt->token->tag), strPt2);
			free(strPt1);	free(strPt2);
			return strPt;
	}
	if (nodePt->token != NULL){
		switch(nodePt->token->tag){
			case ID:
			case TEMP: 
				sprintf(strPt,"%s", nodePt->token->lexeme);
				return strPt;
			case INDEX:
				strPt1 = nodeToString(nodePt->chNode1);
				strPt2 = nodeToString(nodePt->chNode2);
				sprintf(strPt,"%s[%s]", strPt1, strPt2);	
				free(strPt1);	free(strPt2);				
				return strPt;
			case NUM: 
				sprintf(strPt,"%i", nodePt->token->numValue);
				return strPt;
			case REAL: 
				sprintf(strPt,"%f", nodePt->token->realValue);
				return strPt;
			case TRUE: 
				sprintf(strPt,"%s", "true");
				return strPt;
			case FALSE: 
				sprintf(strPt,"%s", "false");
				return strPt;
		}	
	}
	sprintf(strPt,"%s", "");
	return strPt;	
}

Node* generate(Node*,int,int);

Node* reduce(Node* nodePt){
	char* strPt;
	Node *expr, *temp;
	
	if(nodePt->tag == CONST || nodePt->tag == ID) return nodePt;
	
	if(nodePt->tag == OP || nodePt->tag == REL || nodePt->tag == COND || INDEX){
		expr = generate(nodePt,0,0);
		temp = exprNode(tokenWord(TEMP,getTempName()),nodePt->type,NULL,NULL,NULL);
		strPt = nodeToString(expr);
		sprintf(strTemp,"%s = %s\n", temp->token->lexeme, strPt);
		free(strPt);
		emit(strTemp);
		return temp;
	}
	return nodePt;
}

Node* generate(Node* nodePt, int b, int a){
	Node *node1, *node2, *node3, *returnNode;
	char *strPt1, *strPt2;
	int l1, l2;
	
	if(b != 0) emitlabel(b);
	
	if(nodePt->tag == CONST || nodePt->tag == ID) return nodePt;
	if(nodePt->tag == OP || nodePt->tag == REL || nodePt->tag == COND || nodePt->tag == INDEX){
			node1 = reduce(nodePt->chNode1);
			node2 = reduce(nodePt->chNode2);
			if(node1->token->tag == TEMP) tempNameCount--;
			if(node2->token->tag == TEMP) tempNameCount--;
			return exprNode(nodePt->token, nodePt->type, node1, node2, NULL);	
	}
	
	returnNode = NULL;
	switch(nodePt->tag){
		case IF:
			l1 = newLabel();
			node1 = generate(nodePt->chNode1,0,0); strPt1 = nodeToString(node1);
			emitjumps(strPt1, 0, l1); free(strPt1);			
			node2 = generate(nodePt->chNode2, 0, l1);		
			returnNode = stmtNode(nodePt->tag, node1, node2, NULL);
			break;
		case ELSE:
			l1 = newLabel();
			l2 = newLabel();
			node1 = generate(nodePt->chNode1,0,0); strPt1 = nodeToString(node1);
			emitjumps(strPt1, 0, l1); free(strPt1);				
			node2 = generate(nodePt->chNode2, 0, 0);
			emitgoto(l2);
			node3 = generate(nodePt->chNode3, l1, l2);
			returnNode = stmtNode(nodePt->tag, node1, node2, node3);
			break;
		case WHILE:
			l1 = newLabel();
			l2 = newLabel();
			nodePt->after = l2;
			node1 = generate(nodePt->chNode1,l1,0); strPt1 = nodeToString(node1);
			emitjumps(strPt1, 0, l2); free(strPt1);					
			node2 = generate(nodePt->chNode2, 0, 0);
			emitgoto(l1);
			emitlabel(l2);			
			returnNode = stmtNode(nodePt->tag, node1, node2, NULL);
			break;
		case DO:
			l1 = newLabel();
			l2 = newLabel();
			nodePt->after = l2;
			node1 = generate(nodePt->chNode1,l1,0);					
			node2 = generate(nodePt->chNode2, 0, l2); strPt2 = nodeToString(node2);
			emitjumps(strPt2, l1, 0); free(strPt2);					
			returnNode = stmtNode(nodePt->tag, node1, node2, NULL);
			break;
		case BREAK:
			emitgoto(nodePt->chNode1->after);
			break;
		case SET:
			node2 = generate(nodePt->chNode2, 0, 0);
			strPt1 = nodeToString(nodePt->chNode1); strPt2 = nodeToString(node2);
			sprintf(strTemp,"%s = %s\n", strPt1, strPt2);
			free(strPt1); free(strPt2);	
			emit(strTemp);
			returnNode = stmtNode(nodePt->tag, nodePt->chNode1, node2, NULL);
			break;
		case SETELEMENT:
			node1 = generate(nodePt->chNode1,0,0);
			node2 = reduce(nodePt->chNode2);
			if(node2->token->tag == TEMP) tempNameCount--;
			strPt1 = nodeToString(node1); strPt2 = nodeToString(node2);
			sprintf(strTemp,"%s = %s\n", strPt1, strPt2);
			free(strPt1); free(strPt2);	
			emit(strTemp);
			returnNode = stmtNode(nodePt->tag, node1, node2, NULL);
			break;			
		case SEQ:
			if(nodePt->chNode1 != NULL) 
				node1 = generate(nodePt->chNode1,0,0);
			if(nodePt->chNode2 != NULL) 
				node2 = generate(nodePt->chNode2,0,0);
			break;
	}	
	if(a != 0) emitlabel(a);
	return returnNode;
}


int main(int argc, char** argv){
	if(argc < 3){
		throwExeption("Incorrect number of arguments. Expected %i, actual %i", 2, argc);	
	}
	
	char* fileNamePt = argv[1];
	FILE* fpIn = fopen( fileNamePt, "r" );	
	Node* nd = program(fpIn);
	fclose(fpIn);		

	fileNamePt = argv[2];
	fpOut = fopen( fileNamePt, "w" );
	generate(nd,0,0);
	fclose(fpOut);
	
	if((argc > 3) && (strcmp(argv[2],"pst"))){
		writeSyntaxTree(nd, 0);
	}
	
	return 0;
}
