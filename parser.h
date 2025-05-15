#include <stdbool.h>
#include "lexer.h"
#include "utils.h"
#include "ad.h"
#include "at.h"

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
bool expr(Ret *r);
bool exprAssign(Ret *r);
bool exprOr(Ret *r);
bool exprOrPrim(Ret *r);
bool exprAnd(Ret *r);
bool exprAndPrim(Ret *r);
bool exprEq(Ret *r);
bool exprEqPrim(Ret *r);
bool exprRel(Ret *r);
bool exprRelPrim(Ret *r);
bool exprAdd(Ret *r);
bool exprAddPrim(Ret *r);
bool exprMul(Ret *r);
bool exprMulPrim(Ret *r);
bool exprCast(Ret *r);
bool exprUnary(Ret *r);
bool exprPostfix(Ret *r);
bool exprPostfixPrim(Ret *r);
bool exprPrimary(Ret *r);