#include <stdbool.h>
#include "lexer.h"
#include "utils.h"
#include "ad.h"

// Entry point
void parse(Token *tokens);

// Token consumption
bool consume(int code);

// Grammar rules
bool unit(void);
bool structDef(void);
bool varDef(void);
bool fnDef(void);
bool fnParam(void);

// Type rules
bool typeBase(Type *t);
bool arrayDecl(Type *t);

// Statement rules
bool stm(void);
bool stmCompound(bool newDomain);
bool stmIf(void);
bool stmWhile(void);
bool stmReturn(void);

// Expression rules
bool expr(void);
bool exprAssign(void);
bool exprOr(void);
bool exprOrPrim(void);
bool exprAnd(void);
bool exprAndPrim(void);
bool exprEq(void);
bool exprEqPrim(void);
bool exprRel(void);
bool exprRelPrim(void);
bool exprAdd(void);
bool exprAddPrim(void);
bool exprMul(void);
bool exprMulPrim(void);
bool exprCast(void);
bool exprUnary(void);
bool exprPostfix(void);
bool exprPostfixPrim(void);
bool exprPrimary(void);