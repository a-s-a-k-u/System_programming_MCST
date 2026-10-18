%{
#include <iostream>
#include <string>
#include <vector>
#include "AST.h"

extern int yylex();
extern int yylineno;

void yyerror(const char* msg) {
    std::cerr << "Syntax error: " << msg << "\n";
    exit(1);
}

frontend::ProgramNode* root = nullptr;
using namespace frontend;
%}

%union {
    int int_val; char char_val; std::string* str_val;
    frontend::AstNode* ast_node;
    std::vector<frontend::AstNode*>* node_list;
    frontend::TypeSpec* type_spec;
    frontend::ParamSpec* param;
    std::vector<frontend::ParamSpec>* param_list;
}

%token KW_I32 KW_CHAR KW_BOOL KW_PTR
%token KW_IF KW_ELIF KW_ELSE KW_WHILE KW_RETURN KW_TRUE KW_FALSE
%token OP_ADD_ASSIGN OP_EQ OP_NEQ OP_LEQ OP_GEQ
%token OP_PLUS OP_MINUS OP_STAR OP_SLASH OP_ASSIGN OP_LT OP_GT OP_AMP OP_PIPE OP_CARET
%token SYM_LBRACKET SYM_RBRACKET SYM_LPAREN SYM_RPAREN SYM_LBRACE SYM_RBRACE SYM_SEMI SYM_COMMA
%token <int_val> NUM_INT
%token <char_val> LIT_CHAR
%token <str_val> IDENT LIT_STRING

%type <ast_node> program func_decl stmt block expr var_decl assign while_stmt if_stmt return_stmt
%type <type_spec> type_spec base_type
%type <param> param
%type <param_list> param_list
%type <node_list> arg_list stmt_list

%nonassoc UMINUS UPLUS
%left OP_EQ OP_NEQ
%left OP_LT OP_GT OP_LEQ OP_GEQ
%left OP_AMP
%left OP_PIPE
%left OP_CARET
%left OP_PLUS OP_MINUS
%left OP_STAR OP_SLASH
%precedence SYM_LBRACKET

%%

program
    : /* empty */ { root = new ProgramNode(); }
    | program func_decl { root->add_function($2); }
    ;

func_decl
    : type_spec IDENT SYM_LPAREN param_list SYM_RPAREN block {
        $$ = new FunctionDecl(*$1, *$2, *$4, $6);
        delete $1; delete $2; delete $4;
    }
    ;

base_type
    : KW_I32  { $$ = new TypeSpec{ScalarKind::Int32, {}}; }
    | KW_CHAR { $$ = new TypeSpec{ScalarKind::Character, {}}; }
    | KW_BOOL { $$ = new TypeSpec{ScalarKind::Boolean, {}}; }
    ;

type_spec
    : base_type { $$ = $1; }
    | type_spec KW_PTR { $1->decorators.push_back(TypeDecorator::Pointer); $$ = $1; }
    | type_spec SYM_LBRACKET SYM_RBRACKET { $1->decorators.push_back(TypeDecorator::Array); $$ = $1; }
    ;

param_list
    : /* empty */ { $$ = new std::vector<ParamSpec>(); }
    | param { $$ = new std::vector<ParamSpec>(); $$->push_back(*$1); delete $1; }
    | param_list SYM_COMMA param { $1->push_back(*$3); $$ = $1; delete $3; }
    ;

param
    : type_spec IDENT { $$ = new ParamSpec{*$1, *$2}; delete $1; delete $2; }
    ;

block
    : SYM_LBRACE stmt_list SYM_RBRACE {
        auto* b = new BlockStmt();
        for (auto s : *$2) b->append(s);
        delete $2; $$ = b;
    }
    ;

stmt_list
    : /* empty */ { $$ = new std::vector<AstNode*>(); }
    | stmt_list stmt { $1->push_back($2); $$ = $1; }
    ;

stmt
    : var_decl { $$ = $1; }
    | assign { $$ = $1; }
    | while_stmt { $$ = $1; }
    | if_stmt { $$ = $1; }
    | return_stmt { $$ = $1; }
    | expr SYM_SEMI { $$ = $1; }
    ;

var_decl
    : type_spec IDENT OP_ASSIGN expr SYM_SEMI { $$ = new VarDeclStmt(*$1, *$2, $4); delete $1; delete $2; }
    | type_spec IDENT SYM_SEMI { $$ = new VarDeclStmt(*$1, *$2, nullptr); delete $1; delete $2; }
    ;

assign
    : expr OP_ASSIGN expr SYM_SEMI { $$ = new AssignStmt($1, $3, false); }
    | expr OP_ADD_ASSIGN expr SYM_SEMI { $$ = new AssignStmt($1, $3, true); }
    ;

while_stmt
    : KW_WHILE SYM_LPAREN expr SYM_RPAREN block { $$ = new WhileStmt($3, $5); }
    ;

if_stmt
    : KW_IF SYM_LPAREN expr SYM_RPAREN block {
        $$ = new IfStmt($3, $5);
    }
    | if_stmt KW_ELIF SYM_LPAREN expr SYM_RPAREN block {
        static_cast<IfStmt*>($1)->add_branch($4, $6);
        $$ = $1;
    }
    | if_stmt KW_ELSE block {
        static_cast<IfStmt*>($1)->set_else($3);
        $$ = $1;
    }
    ;

return_stmt
    : KW_RETURN expr SYM_SEMI { $$ = new ReturnStmt($2); }
    | KW_RETURN SYM_SEMI { $$ = new ReturnStmt(nullptr); }
    ;

expr
    : NUM_INT { $$ = new IntConst($1); }
    | LIT_CHAR { $$ = new CharConst($1); }
    | LIT_STRING { $$ = new StrConst(*$1); delete $1; }
    | IDENT { $$ = new Identifier(*$1); delete $1; }
    | KW_TRUE { $$ = new BoolConst(true); }
    | KW_FALSE { $$ = new BoolConst(false); }
    | SYM_LPAREN expr SYM_RPAREN { $$ = $2; } /* <--- ДОБАВЛЕНО ПРАВИЛО ДЛЯ СКОБОК! */
    | expr SYM_LBRACKET expr SYM_RBRACKET { $$ = new SubscriptExpr($1, $3); }
    | expr OP_PLUS expr { $$ = new BinaryExpr(BinKind::Plus, $1, $3); }
    | expr OP_MINUS expr { $$ = new BinaryExpr(BinKind::Minus, $1, $3); }
    | expr OP_STAR expr { $$ = new BinaryExpr(BinKind::Multiply, $1, $3); }
    | expr OP_SLASH expr { $$ = new BinaryExpr(BinKind::Divide, $1, $3); }
    | expr OP_AMP expr { $$ = new BinaryExpr(BinKind::BitAnd, $1, $3); }
    | expr OP_PIPE expr { $$ = new BinaryExpr(BinKind::BitOr, $1, $3); }
    | expr OP_CARET expr { $$ = new BinaryExpr(BinKind::BitXor, $1, $3); }
    | expr OP_LT expr { $$ = new BinaryExpr(BinKind::Less, $1, $3); }
    | expr OP_GT expr { $$ = new BinaryExpr(BinKind::Greater, $1, $3); }
    | expr OP_LEQ expr { $$ = new BinaryExpr(BinKind::LessEq, $1, $3); }
    | expr OP_GEQ expr { $$ = new BinaryExpr(BinKind::GreaterEq, $1, $3); }
    | expr OP_EQ expr { $$ = new BinaryExpr(BinKind::Equal, $1, $3); }
    | expr OP_NEQ expr { $$ = new BinaryExpr(BinKind::NotEqual, $1, $3); }
    | IDENT SYM_LPAREN arg_list SYM_RPAREN { $$ = new CallExpr(*$1, *$3); delete $1; delete $3; }
    | OP_MINUS expr %prec UMINUS { $$ = new UnaryExpr(UnaryKind::Negate, $2); }
    | OP_PLUS expr %prec UPLUS { $$ = new UnaryExpr(UnaryKind::Posit, $2); }
    ;

arg_list
    : /* empty */ { $$ = new std::vector<AstNode*>(); }
    | expr { $$ = new std::vector<AstNode*>(); $$->push_back($1); }
    | arg_list SYM_COMMA expr { $1->push_back($3); $$ = $1; }
    ;
%%