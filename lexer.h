#define AND 256
#define BASIC 257
#define BREAK 258
#define DO 259
#define ELSE 260
#define EQ 261
#define FALSE 262
#define GE 263
#define ID 264
#define IF 265
#define INDEX 266
#define LE 267
#define MINUS 268
#define NE 269
#define NUM 270
#define OR 271
#define REAL 272
#define TEMP 273
#define TRUE 274
#define WHILE 275

typedef struct Token Token;
typedef struct HashtableElement HashtableElement;

struct Token{
	int tag;
	int numValue; 		//For integer values
	float realValue; 	//For float values
	char* lexeme; 		//for ids and reserved words
};

int line;
Token* token(int);
Token* tokenNum(int);
Token* tokenWord(int, char*);

char* tokenToString(Token*);
char* tokenTagToString(int);

Token* scan(FILE*, HashtableElement**);
void reserve(HashtableElement**, Token*);
HashtableElement** idHashtableInitialize(int);