#include<stdbool.h>
#include "lexer.h"

void parse(Token *tokens);
bool consume(int code);
bool unit();
bool structDef();
bool varDef();
bool fnDef();
bool fnParam();
bool typeBase();
bool arrayDecl();
bool stm();
bool stmCompound();
bool stmIf();
bool stmWhile();
bool stmReturn();
bool expr();
bool exprAssign();
bool exprOr();
bool exprOrPrim();
bool exprAnd();
bool exprAndPrim();
bool exprEq();
bool exprEqPrim();
bool exprRel();
bool exprRelPrim();
bool exprAdd();
bool exprAddPrim();
bool exprMul();
bool exprMulPrim();
bool exprCast();
bool exprUnary();
bool exprPostfix();
bool exprPostfixPrim();
bool exprPrimary();