#define OP 276
#define COND 277
#define REL 278
#define NOT 279
#define CONST 280

#define SEQ 281
#define STMT 282
#define SET 283
#define SETELEMENT 284


typedef struct Type Type;
typedef struct Identifier Identifier;
typedef struct Node Node;
typedef struct HashtableElement HashtableElement;

struct Type{
	Type* arrayType;
	char* lexeme;
	int width;
	int size;
};

struct Identifier{
	Type* type;
};

struct Node{
	int tag;
	int offset;
	int after;
	Token* token;
	Type* type;
	Node* chNode1;
	Node* chNode2;
	Node* chNode3;
	Node* temp;
};

Node* program(FILE*);
Node* node(Token*, Type*, Node*, Node*);
Node* exprNode(Token*,Type*,Node*,Node*,Node*);
Node* stmtNode(int,Node*,Node*,Node*);