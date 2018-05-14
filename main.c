#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "datastructs.h"
#include "lexer.h"
#include "parser.h"
#include "compiler.h"

void throwExeption(char* msg, ...){
	va_list ptr;	
	va_start(ptr, msg);
	
	printf("%s. ","Error");
	vprintf(msg, ptr);
	
	va_end(ptr);
	
	getchar();
	exit(-1);
}

int main(int argc, char** argv){
	if(argc < 3){
		throwExeption("Incorrect number of arguments. Expected %i, actual %i", 2, argc);	
	}
	
	char* fileNamePt = argv[1];
	FILE* fpIn = fopen( fileNamePt, "r" );	
	Node* nd = program(fpIn); 		//Parse input, return syntax tree
	fclose(fpIn);		

	fileNamePt = argv[2];
	FILE* fpOut = fopen( fileNamePt, "w" );
	generateOutput(fpOut,nd,0,0); 	//Generate output, 3 address language
	fclose(fpOut);
	
	if((argc > 3) && (strcmp(argv[2],"pst"))){
		writeSyntaxTree(nd, 0); 	//If 3rd parameter == pst, print syntax tree
	}
	
	return 0;
}
