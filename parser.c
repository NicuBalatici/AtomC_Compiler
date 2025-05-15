#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<stdarg.h>

#include "utils.h"
#include "parser.h"
#include "lexer.h"
#include "ad.h"
#include "at.h"

Token *iTk;
Token *consumedTk;
Symbol* owner = NULL;

void tkerr(const char *fmt,...)
{
	fprintf(stderr,"error in line %d: ",iTk->line);
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
}

void parse(Token *tokens)
{
    iTk = tokens;
    if (!unit())
    {
        err("Error: Syntax analysis failed.\n");
    }
}

bool expr(Ret *r){
	Token *start = iTk;
	if(exprAssign(r)){
		return true;
	}
	iTk = start;
	return false;
}

bool exprAssign(Ret *r){
	Ret rDst;
	Token *start = iTk;
	if(exprUnary(&rDst)){
		if(consume(ASSIGN)){
			if(exprAssign(r)){
				if(!rDst.lval)tkerr("the assign destination must be a left-value");
                if(rDst.ct)tkerr("the assign destination cannot be constant");
                if(!canBeScalar(&rDst))tkerr("the assign destination must be scalar");
                if(!canBeScalar(r))tkerr("the assign source must be scalar");
                if(!convTo(&r->type,&rDst.type))tkerr("the assign source cannot be converted to destination");
                r->lval=false;
                r->ct=true;
				return true;
			}else tkerr("no expression after assign operator");
		}
		iTk=start;
	}
	if(exprOr(r)){
		return true;
	}

    iTk = start;
	return false;
} 

bool exprOr(Ret *r){
	Token* start = iTk;
	if(exprAnd(r)){
		if(exprOrPrim(r)){
			return true;
		}
	}
	iTk = start;
	return false;

}

bool exprOrPrim(Ret *r) {
	Token *start = iTk;
    if (consume(OR)) {
		Ret right;
        if (exprAnd(&right)) {
            Type tDst;
            if(!arithTypeTo(&r->type,&right.type,&tDst)) {
                char errorMsg[100];
                sprintf(errorMsg, "invalid operand type for || at line %d", iTk->line);
                tkerr(errorMsg);
            }
            *r=(Ret){{TB_INT,NULL,-1},false,true};
            exprOrPrim(r);
            return true;
        }else tkerr("no expression after or operator");
    }
	iTk = start;
    return true;
}

bool exprAnd(Ret *r) {
    Token* start = iTk;
	if(exprEq(r)){
		if(exprAndPrim(r)){
			return true;
		}
	}
	iTk = start;
	return false;
}

bool exprAndPrim(Ret *r) {
    if (consume(AND)) {
		Ret right;
        if (exprEq(&right)) {
            Type tDst;
            if (!arithTypeTo(&r->type, &right.type, &tDst))
                tkerr("invalid operand type for &&");
            *r = (Ret) {{TB_INT, NULL, -1}, false, true};
            exprAndPrim(r);
            return true;
        }tkerr("No expression after and operator");
    }
    return true;
}

bool exprEq(Ret *r) {
    Token* start = iTk;
	if(exprRel(r)){
		if(exprEqPrim(r)){
			return true;
		}
	}
	iTk = start;
	return false;
}

bool exprEqPrim(Ret *r) {
    if (consume(EQUAL) || consume(NOTEQ)) { 
		Ret right;
        if (exprRel(&right)) {
            Type tDst;
            if(!arithTypeTo(&r->type,&right.type,&tDst))
                tkerr("invalid operand type for == or!=");
            *r=(Ret){{TB_INT,NULL,-1},false,true};
            exprEqPrim(r);
            return true;
        }else tkerr("No expression after equality operator");
    }
    return true; 
}

bool exprRel(Ret *r) {
    Token* start = iTk;
	if(exprAdd(r)){
		if(exprRelPrim(r)){
			return true;
		}
	}
	iTk = start;
	return false;
}

bool exprRelPrim(Ret *r) {
    if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)) { 
		Ret right;
        if (exprAdd(&right)) {
            Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst))tkerr("invalid operand type for <, <=, >,>=");
			*r=(Ret){{TB_INT,NULL,-1},false,true};
			exprRelPrim(r);
			return true;
        }tkerr("No expression after relation operator");
    }
    return true;
}


bool exprAdd(Ret *r) {
    Token* start = iTk;
	if(exprMul(r)){
		if(exprAddPrim(r)){
			return true;
		}
	}
	iTk = start;
	return false;
}

bool exprAddPrim(Ret *r) {
    if (consume(ADD) || consume(SUB)) { 
		Ret right;
        if (exprMul(&right)) {
            Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst))tkerr("invalid operand type for + or -");
			*r=(Ret){tDst,false,true};
			exprAddPrim(r);
			return true;
        }tkerr("No expression after adder operator");
    }
    return true; 
}


bool exprMul(Ret *r) {
    Token* start = iTk;
	if(exprCast(r)){
		if(exprMulPrim(r)){
			return true;
		}
	}
	iTk = start;
	return false;
}

bool exprMulPrim(Ret *r) {
    if (consume(MUL) || consume(DIV)) {
		Ret right;
        if (exprCast(&right)) {
            Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst))tkerr("invalid operand type for * or /");
			*r=(Ret){tDst,false,true};
			exprMulPrim(r);
			return true;
        }else tkerr("No expression after multiplier operator");
    }
    return true;
}

bool exprCast(Ret *r){
	Token* start = iTk;
	if(consume(LPAR)){
		Type t;
		Ret op;
		if(typeBase(&t)){
			if(arrayDecl(&t)) {}
            if(consume(RPAR)) {
                if(exprCast(&op)) {
                    if(t.tb==TB_STRUCT)tkerr("cannot convert to a struct type");
                    if(op.type.tb==TB_STRUCT)tkerr("cannot convert a struct");
                    if(op.type.n>=0&&t.n<0)tkerr("an array can be converted only to another array");
                    if(op.type.n<0&&t.n>=0)tkerr("a scalar can be converted only to another scalar");
                    *r=(Ret){t,false,true};
                    return true;
                }
            }else tkerr("Missing ) from cast expression");
        }
	}
	
	if (exprUnary(r)){
		return true;
	}
	iTk = start;
	return false;
}

bool exprUnary(Ret *r){
	Token* start = iTk;
	if(consume(SUB)|| consume(NOT)){
		if(exprUnary(r)){
			if(!canBeScalar(r))tkerr("unary - or ! must have a scalar operand");
			r->lval=false;
			r->ct=true;
			return true;
		}else tkerr("no expression after unary operator");
	}
	if(exprPostfix(r)){
		return true;
	}
	iTk = start;
	return false;
}


bool exprPrimary(Ret *r){
	Token* start = iTk;
	if(consume(ID)){
		Token *tkName = consumedTk;
        Symbol *s=findSymbol(tkName->text);
        if(!s)tkerr("undefined id: %s",tkName->text);
		if(consume(LPAR)){
			if(s->kind!=SK_FN)tkerr("only a function can be called");
			Ret rArg;
			Symbol *param=s->fn.params;
			if(expr(&rArg)){
				if(!param)tkerr("too many arguments in function call");
                if(!convTo(&rArg.type,&param->type))
                    tkerr("in call, cannot convert the argument type to the parameter type");
                param=param->next;
				for(;;) {
                    if(consume(COMMA)) {
                        if(expr(&rArg)) {
                            if(!param)tkerr("too many arguments in function call");
                            if(!convTo(&rArg.type,&param->type))
                                tkerr("in call, cannot convert the argument type to the parameter type");
                            param=param->next;
                        }else tkerr("Missing expression after ,");
                    }else break;
                }
			}else tkerr("Missing expression after (");
			if(consume(RPAR)){
				if(param)tkerr("too few arguments in function call");
                *r=(Ret){s->type,false,true};
                return true;
			}else tkerr("Missing )");
		}
		if(s->kind==SK_FN)tkerr("a function can only be called");
        *r=(Ret){s->type,true,s->type.n>=0};
		return true;
	}
	else if(consume(DOUBLE)){
		 *r=(Ret){{TB_DOUBLE,NULL,-1},false,true};
		return true;
	}
	else if(consume(CHAR)){
		*r=(Ret){{TB_CHAR,NULL,-1},false,true};
		return true;
	}
	else if(consume(STRING)){
		*r=(Ret){{TB_CHAR,NULL,0},false,true};
		return true;
	}
	else if(consume(INT)){
		*r=(Ret){{TB_INT,NULL,-1},false,true};
		return true;
	}
	else if(consume(LPAR)){
		if(expr(r)){
			if(consume(RPAR)){
				return true;
			}else tkerr("Missing )");
		}else tkerr("Missing expression after (");
	}

    iTk = start;
	return false;
}

bool exprPostfix(Ret *r) {
    Token* start = iTk;
	if(exprPrimary(r)){
		if(exprPostfixPrim(r)){
			return true;
		}
	}
	iTk = start;
	return false;
}

bool exprPostfixPrim(Ret *r){
	Token* start = iTk;
	if(consume(LBRACKET)){
		Ret idx;
		if(expr(&idx)){
			if(consume(RBRACKET)){
				if(r->type.n<0)
                    tkerr("only an array can be indexed");
                Type tInt={TB_INT,NULL,-1};
                if(!convTo(&idx.type,&tInt))tkerr("the index is not convertible to int");
                r->type.n=-1;
                r->lval=true;
                r->ct=false;
                exprPostfixPrim(r);
                return true;
			}else tkerr("lipseste ]");
		}
		iTk=start;
	}
	if(consume(DOT)){
		if(consume(ID)){
			Token *tkName = consumedTk;
                if(r->type.tb!=TB_STRUCT)tkerr("a field can only be selected from a struct");
                Symbol *s=findSymbolInList(r->type.s->structMembers,tkName->text);
                if(!s)
                    tkerr("the structure %s does not have a field%s",r->type.s->name,tkName->text);
                *r=(Ret){s->type,true,s->type.n>=0};
                exprPostfixPrim(r);
                return true;
		}else tkerr("missing ID after .");
		iTk=start;
	}
	return true;
}

char *tkCodeName(int code)
{
    switch (code)
    {
        case ASSIGN: return "ASSIGN";
        case OR: return "OR";
        case AND: return "AND";
        case EQUAL: return "EQUAL";
        case NOTEQ: return "NOTEQ";
        case LESS: return "LESS";
        case LESSEQ: return "LESSEQ";
        case GREATER: return "GREATER";
        case GREATEREQ: return "GREATEREQ";
        case ADD: return "ADD";
        case SUB: return "SUB";
        case MUL: return "MUL";
        case DIV: return "DIV";
        case LPAR: return "LPAR";
        case RPAR: return "RPAR";
        case LBRACKET: return "LBRACKET";
        case RBRACKET: return "RBRACKET";
        case DOT: return "DOT";
        case COMMA: return "COMMA";
        case SEMICOLON: return "SEMICOLON";
        case ID: return "ID";
        case INT: return "INT";
        case DOUBLE: return "DOUBLE";
        case CHAR: return "CHAR";
        case STRING: return "STRING";
        case STRUCT: return "STRUCT";
        case IF: return "IF";
        case ELSE: return "ELSE";
        case WHILE: return "WHILE";
        case RETURN: return "RETURN";
        case VOID: return "VOID";
        case TYPE_INT: return "TYPE_INT";
        case TYPE_DOUBLE: return "TYPE_DOUBLE";
        case TYPE_CHAR: return "TYPE_CHAR";
        case END: return "END";
        default: return "UNKNOWN";
    }
}

bool consume(int code)
{

    if(iTk->code==code){
        consumedTk=iTk;
        iTk=iTk->next;
        return true;
    }

    return false;
}

bool structDef() {
    Token* start = iTk;

    if (!consume(STRUCT)) {
        iTk = start;
        return false;
    }

    Token* tkName = iTk;
    if (!consume(ID)) {
        tkerr("invalid or missing identifier after struct");
    }

    if (!consume(LACC)) {
        iTk = start;
        return false;
    }

    Symbol* s = findSymbolInDomain(symTable, tkName->text);
    if (s) {
        tkerr("symbol redefinition: %s", tkName->text);
    }

    s = addSymbolToDomain(symTable, newSymbol(tkName->text, SK_STRUCT));
    s->type.tb = TB_STRUCT;
    s->type.s = s;
    s->type.n = -1;

    pushDomain();
    owner = s;

    while (varDef());

    if (!consume(RACC)) {
        tkerr("missing '}' in struct");
    }

    dropDomain();
    owner = NULL;

    if (!consume(SEMICOLON)) {
        tkerr("missing ';' after struct definition");
    }

    return true;
}

bool varDef() {
    Token *start = iTk;
    Type t;

    if (typeBase(&t)) { 
        Token* tkName = iTk;

        if (consume(ID)) {
            if (arrayDecl(&t)) {
                if (t.n == 0)
                    tkerr("a vector variable must have a specified dimension");
            }

            if (consume(SEMICOLON)) {
                Symbol* var = findSymbolInDomain(symTable, tkName->text);
                if (var){
                    tkerr("symbol redefinition: %s", tkName->text);
                }

                var = newSymbol(tkName->text, SK_VAR);
                var->type = t;
                var->owner = owner;
                addSymbolToDomain(symTable, var);

                if (owner) {

					switch (owner->kind) {

						case SK_FN:
                        
                        var->varIdx = symbolsLen(owner->fn.locals);
                        addSymbolToList(&owner->fn.locals, dupSymbol(var));
                        break;
                        
						case SK_STRUCT:
                        var->varIdx = typeSize(&owner->type);
                        addSymbolToList(&owner->structMembers, dupSymbol(var));
						break;
				
						case SK_VAR:
						case SK_PARAM:
							break;
					}
				} else {
                    
                    var->varMem = safeAlloc(typeSize(&t));
				}

                return true;
            } else {

                tkerr("Missing semicolon ; after variable declaration.");
            }
        } else {
            tkerr("Expected identifier after type.");
        }
    }
    

    iTk = start;
    return false;
}

bool fnDef() {
    Token* start = iTk;
    Type t;

    if (consume(VOID)) {
        t.tb = TB_VOID;
        t.n = -1;
    } else if (!typeBase(&t)) {
        iTk = start;
        return false;
    }

    Token* tkName = iTk;
    if (!consume(ID)) {
		iTk = start;
		return false;
	}
	
	if (!consume(LPAR)) {
		iTk = start;
		return false;
	}
    
    Symbol* fn = findSymbolInDomain(symTable, tkName->text);
    if (fn) {
        tkerr("Symbol redefinition: %s", tkName->text);
    }
    
    fn = newSymbol(tkName->text, SK_FN);
    fn->type = t;
    fn->fn.locals = NULL;
    fn->fn.params = NULL;
    addSymbolToDomain(symTable, fn);
    
    owner = fn;
    pushDomain();
    
    if (fnParam()) {
        while (consume(COMMA)) {
            if (!fnParam()) {
                tkerr("Missing or invalid parameter after ','.");
            }
        }
    }

    if (!consume(RPAR)) {
        tkerr("Missing ')' after parameter list.");
    }

    if (!stmCompound(false)) {
        tkerr("Missing or invalid function body.");
    }

    dropDomain();
    owner = NULL;

    return true;
}

bool unit()
{
    while(true)
    { 
        if(structDef()){}
        else if(fnDef()){}
        else if(varDef()){}
        else {
            break;
        }
    }
    
    if(consume(END)){
        return true;
    }
    return true;
}

bool typeBase(Type* t) {
    Token* start = iTk;
    t->n = -1; 

    if (consume(TYPE_INT)) {
        t->tb = TB_INT;
        return true;
    }

    if (consume(TYPE_DOUBLE)) {
        t->tb = TB_DOUBLE;
        return true;
    }

    if (consume(TYPE_CHAR)) {
        t->tb = TB_CHAR;
        return true;
    }

    if (consume(STRUCT)) {
        Token* tkName = iTk;
        if (consume(ID)) {
            t->tb = TB_STRUCT;
            t->s = findSymbol(tkName->text);
            if (!t->s)
                tkerr("undefined struct: %s", tkName->text);
            return true;
        } else {
            tkerr("missing identifier after 'struct'");
        }
    }

    iTk = start;
    return false;
}

bool arrayDecl(Type *t){
	if(consume(LBRACKET)){
		if(consume(INT)){
			Token *tkSize=consumedTk;
			t->n=tkSize->i;
		}else{
			t->n=0; 
		}
		if(consume(RBRACKET)){
			return true;
		}
		else tkerr("missing ] or invalid expression inside [...]");
	}
	return false;
}

bool fnParam(){
	Token *start = iTk;
	Type t;
	if (typeBase(&t)) {
		if (consume(ID)) {
			Token *tkName = consumedTk;
			if (arrayDecl(&t)) {
				t.n=0;
			}
			Symbol *param=findSymbolInDomain(symTable,tkName->text);
			if(param)tkerr("symbol redefinition: %s",tkName->text);
			param=newSymbol(tkName->text,SK_PARAM);
			param->type=t;
			param->owner=owner;
			param->paramIdx=symbolsLen(owner->fn.params);
			addSymbolToDomain(symTable,param);
			addSymbolToList(&owner->fn.params,dupSymbol(param));
			return true;
		} else {
			tkerr("Lipseste numele parametrului");
		}
	}
	iTk = start;
	return false;
}

bool stm(){
	// printf("Stm\n");
	Token* start= iTk;
	Ret rCond,rExpr;
	if(stmCompound(true)){
		// printf("stmCompound->stm\n ");
		return true;
	}
	if(consume(IF)){
		if(consume(LPAR)){
			if(expr(&rCond)){
				if(!canBeScalar(&rCond))
                    tkerr("the if condition must be a scalar value");
				if(consume(RPAR)){
					if(stm()){
						if(consume(ELSE)){
							if(stm()){}else tkerr("else statement mssing");
						}
						return true;
					}else tkerr("if statement mssing");
				}else tkerr("Missing ) for if");
			}else tkerr("Missing if expression");
		}else tkerr("Missing ( after if");
	}
	else if(consume(WHILE)){
		if(consume(LPAR)){
			if(expr(&rCond)){
				if(!canBeScalar(&rCond))
                    tkerr("the while condition must be a scalar value");
				if(consume(RPAR)){
					if(stm()){
						return true;
					}else tkerr("Missing while body");
				}else tkerr("Missing ) for while");
			}else tkerr("Missing while condition");
		}else tkerr("Missing ( after while keyword");
		iTk=start;
	}
	else if(consume(RETURN)){
			if(expr(&rExpr)) {
				if(owner->type.tb==TB_VOID)
					tkerr("a void function cannot return a value");
				if(!canBeScalar(&rExpr))
					tkerr("the return value must be a scalar value");
				if(!convTo(&rExpr.type,&owner->type))
					tkerr("cannot convert the return expression type to the function return type");
			} else {
				if(owner->type.tb!=TB_VOID)
					tkerr("a non-void function must return a value");
				}
			if(consume(SEMICOLON)){
				return true;
			}else tkerr("missing ; at return statement");
	}
	else if(expr(&rExpr)){
		if(consume(SEMICOLON)){
			return true;
		}else tkerr("Missing ; at the end of expression");
	}
	if(consume(SEMICOLON)){
		return true;
	}
	iTk = start;
	return false;
}


bool stmCompound(bool newDomain) {
    Token* start = iTk;

    if (consume(LACC)) {
        if (newDomain){
            pushDomain();
        }

        while (true) {
            if (varDef()) {} 
            else if (stm()){} 
            else {break;}
        }

        if (consume(RACC)) {
            if (newDomain)
                dropDomain();
            return true;
        } else {
            tkerr("missing '}' after compound statement");
        }

        if (newDomain){
            dropDomain();
        }
    }

    iTk = start;
    return false;
}

//fnParam + exprAll