#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"
#include "utils.h"
#include "lexer.h"
#include "ad.h"
#include "at.h"
#include "gc.h"

Token *iTk;		// the iterator in the tokens list
Token *consumedTk;		// the last consumed token
Symbol* owner;

void tkerr(const Token *tk, const char *fmt,...)
{
	fprintf(stderr,"error in line %d: ",iTk->line);
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
}

bool consume(int code)
{
	if(iTk->code == code)
	{
		consumedTk = iTk;
		iTk = iTk->next;
		// printf(" => consumed\n");
		return true;
	}

	return false;
}


// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool typeBase(Type *t)
{
    Token *start = iTk;
    t->n = -1;
    if(consume(TYPE_INT)) 
    { 
        t->tb = TB_INT;
        return true; 
    }
    if(consume(TYPE_DOUBLE)) 
    { 
        t->tb = TB_DOUBLE; 
        return true; 
    }
    if(consume(TYPE_CHAR)) 
    { 
        t->tb = TB_CHAR; 
        return true; 
    }
    if(consume(STRUCT))
    {
        if(consume(ID))
        {
            Token *tkName = consumedTk;
            t->tb = TB_STRUCT;
            t->s = findSymbol(tkName->text);
            if(!t->s)
            {
                tkerr(iTk, "undefined struct: %s", tkName->text);
            }
            return true;
        }
        else tkerr(iTk, "struct identifier missing");
    }

    iTk = start;
    return false;
}

// unit: ( structDef | fnDef | varDef )* END
bool unit()
{
	for(;;)
	{
		if(structDef()){}
		else if(fnDef()){}
		else if(varDef()){}
		else break;
	}
	if(consume(END))
	{
		return true;
	}
    
	return false;
}

// structDef: STRUCT ID LACC varDef* RACC SEMICOLON
bool structDef()
{
	Token* start = iTk;
    Type t;

	if(consume(STRUCT))
    {
		if(consume(ID))
        {
            Token *tkName = consumedTk;
            if(consume(LACC))
            {
                Symbol *s = findSymbolInDomain(symTable, tkName->text);
                if(s) 
                {
                    tkerr(tkName, "symbol redefinition: %s", tkName->text);
                }
                s = addSymbolToDomain(symTable, newSymbol(tkName->text, SK_STRUCT));
                s->type.tb = TB_STRUCT;
                s->type.s  = s;
                s->type.n  = -1;
                pushDomain();
                owner = s; 
				while(varDef());
				if(consume(RACC))
                {
					if(consume(SEMICOLON))
                    {
                        owner=NULL;
                        dropDomain();
                        return true;
					}
					else tkerr(iTk, "missing ';' after '}' struct");
				} 
				else tkerr(iTk, "missing '}' struct");
			}
			else
            {
				if(typeBase(&t))
                {
					tkerr(iTk, "missing { after struct id");
				}
			}
		} 
        else tkerr(iTk, "struct identifier missing");
	}

	iTk = start;
	return false;
}

// fnDef: ( typeBase | VOID ) ID
// LPAR ( fnParam ( COMMA fnParam )* )? RPAR
// stmCompound
bool fnDef()
{
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
    Type t;

    if(typeBase(&t) || consume(VOID)){
        if(start->code == VOID) {
            t.tb = TB_VOID;
        }
        if(consume(ID)){
            Token* tkName = consumedTk;
            if(consume(LPAR)){
                Symbol *fn = findSymbolInDomain(symTable, tkName->text);
                if(fn){
                    tkerr(iTk,"symbol redefinition: %s", tkName->text);
                }
                fn = newSymbol(tkName->text, SK_FN);
                fn->type = t;
                addSymbolToDomain(symTable, fn);
                owner = fn;
                pushDomain();
                
                // Add ENTER instruction
                addInstr(&fn->fn.instr,OP_ENTER);

                if(fnParam()){
                    for(;;){
                        if(consume(COMMA)){
                            if(fnParam()){}
                            else tkerr(iTk, "missing/invalid parameter after ',' in function args");
                        }
                        else break;
                    }
                }
                if(consume(RPAR)){
                    if(stmCompound(false)){
                        fn->fn.instr->arg.i = symbolsLen(fn->fn.locals);
                        if(fn->type.tb==TB_VOID){
                            addInstrWithInt(&fn->fn.instr,OP_RET_VOID, symbolsLen(fn->fn.params));
                        }
                        
                        dropDomain();
                        owner = NULL;
                        return true;
                    }
                    else tkerr(iTk, "need function implementation");
                }
                else tkerr(iTk, "missing ')' function closing");
            }
        }
        else{
            if(consume(ID)){
                if(consume(LPAR)){
                    tkerr(iTk, "need type in function declaration");
                }
            }
        }
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return false;
}

// varDef: typeBase ID arrayDecl? SEMICOLON
bool varDef()
{
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
    Type t;

    if(typeBase(&t))
    {
        if(consume(ID))
        {
            Token* tkName = consumedTk;
            if(arrayDecl(&t))
            {
                if(t.n==0)
                {
                    tkerr(iTk,"a vector variable must have a specified dimension");
                }
            }
            if(consume(SEMICOLON))
            {
                Symbol *var = findSymbolInDomain(symTable, tkName->text);
                if(var)
                {
                    tkerr(iTk,"symbol redefinition: %s", tkName->text);
                }
                var = newSymbol(tkName->text, SK_VAR);
                var->type = t;
                var->owner = owner;
                addSymbolToDomain(symTable, var);
                if(owner)
                {
                    switch(owner->kind)
                    {
                        case SK_FN:
                            var->varIdx = symbolsLen(owner->fn.locals);
                            addSymbolToList(&owner->fn.locals, dupSymbol(var));
                            break;
                        case SK_STRUCT:
                            var->varIdx = typeSize(&owner->type);
                            addSymbolToList(&owner->structMembers, dupSymbol(var));
                            break;
                        case SK_VAR: break;
                        case SK_PARAM: break;
                    }
                }
                else
                {
                    var->varMem=safeAlloc(typeSize(&t));
                }

                return true;
            }
            else tkerr(iTk, "missing ';' after identifier declaration");
        }
        else tkerr(iTk, "need identifier after type declaration");
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return false;
}

// arrayDecl: LBRACKET INT? RBRACKET
bool arrayDecl(Type* t)
{
    Token *start = iTk;

    if(consume(LBRACKET))
    {
        if(consume(INT))
        {
            Token* tkSize = consumedTk;
            t->n = tkSize->i;
        }
        if(consume(RBRACKET)){
            return true;
        }
        else tkerr(iTk, " ']' missing!");
    }

    iTk = start;
    return false;
}

// fnParam: typeBase ID arrayDecl?
bool fnParam()
{
    Token *start = iTk;
    Type t;

    if(typeBase(&t))
    {
        if(consume(ID))
        {
            Token* tkName = consumedTk;
            if(arrayDecl(&t))
            {
                t.n = 0;
            }

            Symbol *param = findSymbolInDomain(symTable, tkName->text);
            if(param)
            {
                tkerr(iTk, "symbol redefinition: %s", tkName->text);
            }
            param = newSymbol(tkName->text,SK_PARAM);
            param->type=t;
            param->owner=owner;
            param->paramIdx = symbolsLen(owner->fn.params);

            addSymbolToDomain(symTable,param);
            addSymbolToList(&owner->fn.params,dupSymbol(param));

            return true;
        }
        else tkerr(iTk, "missing parameter identifier after type declaration");
    }

    iTk = start;
    return false;
}

// stmCompound: LACC ( varDef | stm )* RACC
bool stmCompound(bool newDomain)
{
    Token *start = iTk;
    Instr *startInstr=owner?lastInstr(owner->fn.instr):NULL;

    if(consume(LACC))
    {
        if(newDomain)
        {
            pushDomain();
        }
        for(;;)
        {
            if(varDef()){}
            else if(stm()){}
            else break;
        }
        if(consume(RACC))
        {
            if(newDomain)
            {
                dropDomain();
            }
            return true;
        }
        else tkerr(iTk, "missing '}'");
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return false;
}

// stm: stmCompound
// | IF LPAR expr RPAR stm ( ELSE stm )?
// | WHILE LPAR expr RPAR stm
// | RETURN expr? SEMICOLON
// | expr? SEMICOLON
int returnFound = 0;
bool stm()
{
	Token* start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
    Ret rCond, rExpr;

	if(stmCompound(true))
    {
		return true;
	}

	if(consume(IF))
    {
		if(consume(LPAR))
        {
			if(expr(&rCond))
            {
                if(!canBeScalar(&rCond))
                {
                    tkerr(iTk, "the if condition must be a scalar value");
                }

				if(consume(RPAR))
                {
                    addRVal(&owner->fn.instr, rCond.lval, &rCond.type);
                    Type intType = {TB_INT,NULL,-1};
                    insertConvIfNeeded(lastInstr(owner->fn.instr), &rCond.type, &intType);
                    Instr *ifJF = addInstr(&owner->fn.instr, OP_JF);

					if(stm())
                    {
						if(consume(ELSE))
                        {
                            Instr *ifJMP = addInstr(&owner->fn.instr, OP_JMP);
                            ifJF->arg.instr = addInstr(&owner->fn.instr, OP_NOP);

							if(stm())
                            {
                                ifJMP->arg.instr = addInstr(&owner->fn.instr, OP_NOP);
								return true;
							} 
							else tkerr(iTk, "missing else branch");
						} 
						else 
                        {
                            ifJF->arg.instr=addInstr(&owner->fn.instr,OP_NOP);
                            return true;
                        }
					} 
					else tkerr(iTk, "missing if branch");
				} 
				else tkerr(iTk, "missing ')' after expression in if");
			} 
			else tkerr(iTk, "invalid/missing condition in if");
		} 
		else tkerr(iTk, "missing '(' after if");
	}

	iTk = start;
    if(owner)delInstrAfter(startInstr);

	if(consume(WHILE))
    {
        Instr *beforeWhileCond = lastInstr(owner->fn.instr);

		if(consume(LPAR))
        {
			if(expr(&rCond))
            {
                if(!canBeScalar(&rCond))
                {
                    tkerr(iTk, "the while condition must be a scalar value");
                }

				if(consume(RPAR))
                {
                    addRVal(&owner->fn.instr, rCond.lval, &rCond.type);
                    Type intType={TB_INT, NULL, -1};
                    insertConvIfNeeded(lastInstr(owner->fn.instr), &rCond.type, &intType);
                    Instr *whileJF = addInstr(&owner->fn.instr, OP_JF);

					if(stm())
                    {
                        addInstr(&owner->fn.instr,OP_JMP)->arg.instr=beforeWhileCond->next;
                        whileJF->arg.instr=addInstr(&owner->fn.instr,OP_NOP);
						return true;
					} 
					else tkerr(iTk, "missing statement in while");
				}
				else tkerr(iTk, "missing ')' after expression in while");
			} 
			else tkerr(iTk, "missing while condition");
		} 
		else
        {
			tkerr(iTk, "missing '(' after while");
		}
	}

	iTk = start;
    if(owner)delInstrAfter(startInstr);
	if(consume(RETURN))
    {
		returnFound = 1;
        if(expr(&rExpr))
        {
            if(owner->type.tb == TB_VOID)
            {
                tkerr(iTk, "a void function cannot return a value");
            }
            if(!canBeScalar(&rExpr))
            {
                tkerr(iTk, "the return value must be a scalar value");
            }
            if(!convTo(&rExpr.type, &owner->type))
            {
                tkerr(iTk, "cannot convert the return expression type to the function return type");
            }
            
            addRVal(&owner->fn.instr, rExpr.lval, &rExpr.type);
            insertConvIfNeeded(lastInstr(owner->fn.instr), &rExpr.type, &owner->type);
            addInstrWithInt(&owner->fn.instr, OP_RET, symbolsLen(owner->fn.params));
        }
        else 
        {
            if(owner->type.tb != TB_VOID)
            {
                tkerr(iTk, "a non-void function must return a value");
            }
            addInstr(&owner->fn.instr,OP_RET_VOID);
        }

		if(consume(SEMICOLON))
        {
			return true;
		} 
		else tkerr(iTk, "missing ';' after return");
	}

	iTk = start;
    if(owner)delInstrAfter(startInstr);
	if (expr(&rExpr)) 
    {
        if(rExpr.type.tb!=TB_VOID)
        {
            addInstr(&owner->fn.instr,OP_DROP);
        }
            
		if (consume(SEMICOLON)) 
        {
			return true;
		}
		else tkerr(iTk, "missing ';' after expression");
	}

	iTk = start;
    if(owner)delInstrAfter(startInstr);
	if(consume(SEMICOLON))
    {
		return true;
	}
    
    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return false;
}

// exprAssign: exprUnary ASSIGN exprAssign | exprOr
bool exprAssign(Ret *r)
{
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
    Ret rDst;

    if(exprUnary(&rDst))
    {
        if(consume(ASSIGN))
        {
            if(exprAssign(r))
            {
                if(!rDst.lval)
                {
                    tkerr(iTk, "the assign destination must be a left-value");
                }
                if(rDst.ct)
                {
                    tkerr(iTk, "the assign destination cannot be constant");
                }
                if(!canBeScalar(&rDst))
                {
                    tkerr(iTk, "the assign destination must be scalar");
                }
                if(!canBeScalar(r))
                {
                    tkerr(iTk, "the assign source must be scalar");
                }
                if(!convTo(&r->type, &rDst.type))
                {
                    tkerr(iTk, "the assign source cannot be converted to destination");
                }
                r->lval = false;
                r->ct = true;
                
                addRVal(&owner->fn.instr,  r->lval,&r->type);
                insertConvIfNeeded(lastInstr(owner->fn.instr), &r->type, &rDst.type);
                switch(rDst.type.tb)
                {
                    case TB_INT: addInstr(&owner->fn.instr,OP_STORE_I); break;
                    case TB_DOUBLE: addInstr(&owner->fn.instr,OP_STORE_F); break;
                    case TB_CHAR: /* not implemented */ break;
                    case TB_VOID: /* not applicable */ break;
                    case TB_STRUCT: /* not applicable */ break;
                    default: break;
                }

                return true;
            }
            else tkerr(iTk, "need right operand");
        }
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);

    if(exprOr(r))
    {
        return true;
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return false;
}

// expr: exprAssign
bool expr(Ret *r)
{
    Token *start = iTk;
    Instr *startInstr=owner?lastInstr(owner->fn.instr):NULL;

    if(exprAssign(r))
    {
        return true;
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return false;
}

// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
bool exprUnary(Ret *r)
{
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;

    char c;
    if(start->code == SUB) c = '-';
    else c='!';

    if(consume(SUB)||consume(NOT))
	{
        if(exprUnary(r))
		{
            if(!canBeScalar(r))
            {
                tkerr(iTk, "unary - or ! must have a scalar operand");
            }
            r->lval = false;
            r->ct = true;

            return true;
        }
        else tkerr(iTk, "need expression after %c", c);
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);

    if(exprPostfix(r))
	{
        return true;
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return false;
}

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast(Ret *r)
{
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;

    if(consume(LPAR))
    {
        Type t;
        Ret op;
        if(typeBase(&t))
        {
            if(arrayDecl(&t)){}
            
            if(consume(RPAR))
            {
                if(exprCast(&op))
                {
                    if(t.tb == TB_STRUCT)
                    {
                        tkerr(iTk, "cannot convert to a struct type");
                    }
                    if(op.type.tb == TB_STRUCT)
                    {
                        tkerr(iTk, "cannot convert a struct");
                    }
                    if(op.type.n >= 0 && t.n < 0)
                    {
                        tkerr(iTk, "an array can be converted only to another array");
                    }
                    if(op.type.n < 0 && t.n >= 0)
                    {
                        tkerr(iTk, "a scalar can be converted only to another scalar");
                    }
                    *r = (Ret){t, false, true};

                    return true;
                }
                iTk = start;
                if(owner)delInstrAfter(startInstr);
                return false;
            }
            else tkerr(iTk, "missing ')'");
        }
    }
    iTk = start;
    if(owner)delInstrAfter(startInstr);
    
    if(exprUnary(r))
    {
        return true;
    }
    
    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return false;
}

// exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
// | INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
bool exprPrimary(Ret *r)
{
	Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
    
	if (consume(ID))
	{
		Token *tkName = consumedTk;
		Symbol *s = findSymbol(tkName->text);

		if (!s)
		{
            tkerr(iTk, "undefined id: %s", tkName->text);
        }

		if (consume(LPAR))
		{
			if (s->kind != SK_FN)
			{
                tkerr(iTk, "only a function can be called");
            }

			Ret rArg;
			Symbol *param = s->fn.params;
			if (consume(RPAR))
			{
                if(s->fn.extFnPtr)
                {
                    addInstr(&owner->fn.instr,OP_CALL_EXT)->arg.extFnPtr=s->fn.extFnPtr;
                }
                else
                {
                    addInstr(&owner->fn.instr,OP_CALL)->arg.instr=s->fn.instr;
                }
                *r = (Ret){s->type, false, true};
				return true;
			}

			if (expr(&rArg))
			{
				if (!param)
				{
                    tkerr(iTk, "too many arguments in function call");
                }
				if (!convTo(&rArg.type, &param->type))
				{
                    tkerr(iTk, "in call, cannot convert the argument type to the parameter type");
                }
                
                addRVal(&owner->fn.instr, rArg.lval, &rArg.type);
                insertConvIfNeeded(lastInstr(owner->fn.instr), &rArg.type, &param->type);
                
                param = param->next;
				while (consume(COMMA))
				{
					if (expr(&rArg))
					{
						if (!param)
						{
                            tkerr(iTk, "too many arguments in function call");
                        }
                        if (!convTo(&rArg.type, &param->type))
						{
                            tkerr(iTk, "in call, cannot convert the argument type to the parameter type");
                        }
                        
                        addRVal(&owner->fn.instr,rArg.lval,&rArg.type);
                        insertConvIfNeeded(lastInstr(owner->fn.instr),&rArg.type,&param->type);
                        
                        param = param->next;
						continue;
					}
					else
					{
						tkerr(iTk, "missing function argument after ','");
					}

					iTk = start;
                    if(owner)delInstrAfter(startInstr);
					return false;
				}

				if (consume(RPAR))
				{
					if (param)
					{
                        tkerr(iTk, "too few arguments in function call");
                    }
                    
                    if(s->fn.extFnPtr)
                    {
                        addInstr(&owner->fn.instr, OP_CALL_EXT)->arg.extFnPtr = s->fn.extFnPtr;
                    }
                    else 
                    {
                        addInstr(&owner->fn.instr, OP_CALL)->arg.instr = s->fn.instr;
                    }
                    
					*r = (Ret){s->type, false, true};
					return true;
				}
				else
				{
					tkerr(iTk, "missing ')' in function call");
				}
			}
		}
		if (s->kind == SK_FN)
		{
            tkerr(iTk, "a function can only be called");
        }
        
        if(s->kind==SK_VAR)
        {
            if(s->owner==NULL)
            {
                addInstr(&owner->fn.instr, OP_ADDR)->arg.p = s->varMem;
            }
            else
            {
                switch(s->type.tb)
                {
                    case TB_INT: addInstrWithInt(&owner->fn.instr,OP_FPADDR_I,s->varIdx+1); break;
                    case TB_DOUBLE: addInstrWithInt(&owner->fn.instr,OP_FPADDR_F,s->varIdx+1); break;
                    case TB_CHAR: /* not implemented */ break;
                    case TB_VOID: /* not applicable */ break;
                    case TB_STRUCT: /* not applicable */ break;
                    default: break;
                }
            }
        }
        if(s->kind==SK_PARAM)
        {
            switch(s->type.tb)
            {
                case TB_INT:
                    addInstrWithInt(&owner->fn.instr, OP_FPADDR_I, s->paramIdx-symbolsLen(s->owner->fn.params)-1);
                    break;
                case TB_DOUBLE:
                    addInstrWithInt(&owner->fn.instr, OP_FPADDR_F, s->paramIdx-symbolsLen(s->owner->fn.params)-1);
                    break;
                case TB_CHAR: /* not implemented */ break;
                case TB_VOID: /* not applicable */ break;
                case TB_STRUCT: /* not applicable */ break;
                default: break;
            }
        }
        
		*r = (Ret){s->type, true, s->type.n >= 0};
		return true;
	}

	iTk = start;
    if(owner)delInstrAfter(startInstr);
	if (consume(INT))
	{
        Token *ct = consumedTk;
        addInstrWithInt(&owner->fn.instr,OP_PUSH_I,ct->i);
		*r = (Ret){{TB_INT, NULL, -1}, false, true};
		return true;
	}
	if (consume(DOUBLE))
	{
        Token *ct = consumedTk;
        addInstrWithDouble(&owner->fn.instr,OP_PUSH_F,ct->d);
		*r = (Ret){{TB_DOUBLE, NULL, -1}, false, true};
		return true;
	}
	if (consume(CHAR))
	{
		*r = (Ret){{TB_CHAR, NULL, -1}, false, true};
		return true;
	}
	if (consume(STRING))
	{
		*r = (Ret){{TB_CHAR, NULL, 0}, false, true};
		return true;
	}

	if (consume(LPAR))
	{
		if (expr(r))
		{
			if (consume(RPAR))
			{
				return true;
			}
			else
			{
				tkerr(iTk, "missing ')' after expression");
			}
		}
	}

	iTk = start;
    if(owner)delInstrAfter(startInstr);
	return false;
}

// exprOr: exprOr OR exprAnd | exprAnd
bool exprOr(Ret *r)
{
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;

    if(exprAnd(r))
    {
        if(exprOrPrim(r))
        {
            return true;
        }
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return false;
}

bool exprOrPrim(Ret *r)
{
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;

    if(consume(OR))
    {
        Ret right;
        if(exprAnd(&right))
        {
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type ,&tDst))
            {
                tkerr(iTk,"invalid operand type for ||");
            }
            *r = (Ret){{TB_INT, NULL, -1}, false, true};

            if(exprOrPrim(r))
            {
                return true;
            }
        }
        else tkerr(iTk, " invalid or missing expression after '||'");
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return true;
}

// exprAnd: exprAnd AND exprEq | exprEq
bool exprAnd(Ret *r)
{
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;

    if(exprEq(r))
    {
        if(exprAndPrim(r))
        {
            return true;
        }
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return false;
}

bool exprAndPrim(Ret *r)
{
    Token *start = iTk;
    Instr *startInstr=owner?lastInstr(owner->fn.instr):NULL;

    if(consume(AND))
    {
        Ret right;
        if(exprEq(&right))
        {
            Type tDst;
            if(!arithTypeTo(&r->type,&right.type,&tDst))
            {
                tkerr(iTk, "invalid operand type for &&");
            }
            *r = (Ret){{TB_INT, NULL, -1}, false, true};

            if(exprAndPrim(r))
            {
                return true;
            }
        }
        else tkerr(iTk, " invalid or missing expression after '&&'!");
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return true;
}

// exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel
bool exprEq(Ret *r)
{
    Token *start = iTk;
    Instr *startInstr=owner?lastInstr(owner->fn.instr):NULL;

    if(exprRel(r))
    {
        if(exprEqPrim(r))
        {
            return true;
        }
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return false;
}

bool exprEqPrim(Ret *r) 
{
    Token *start = iTk;
    Instr *startInstr=owner?lastInstr(owner->fn.instr):NULL;
    
    if (consume(EQUAL) || consume(NOTEQ))
    {
        Ret right;
        if (exprRel(&right)) 
        {
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type, &tDst))
            {
                tkerr(iTk, "invalid operand type for == or !=");
            }
            *r = (Ret){{TB_INT, NULL, -1}, false, true};

            if (exprEqPrim(r)) 
            {
                return true;
            }
        } 
        else 
        {
            iTk = start;
            if (consume(EQUAL)) 
            {
                tkerr(iTk, "missing expression after ==");
            }
            else if (consume(NOTEQ)) 
            {
                tkerr(iTk, "missing expression after !=");
            }
        }
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return true;
}

// exprRel: exprRel ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd
bool exprRel(Ret *r)
{
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;

    if(exprAdd(r))
    {
        if(exprRelPrim(r))
        {
            return true;
        }
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return false;
}

bool exprRelPrim(Ret *r) 
{
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
    Token *op;
    
    if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ))
    {
        op = consumedTk;
        Instr *lastLeft = lastInstr(owner->fn.instr);
        addRVal(&owner->fn.instr, r->lval, &r->type);
        
        Ret right;
        if (exprAdd(&right)) 
        {
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type, &tDst))
            {
                tkerr(iTk, "invalid operand type for <, <=, >,>=");
            }
            
            addRVal(&owner->fn.instr,right.lval,&right.type);
            insertConvIfNeeded(lastLeft,&r->type,&tDst);
            insertConvIfNeeded(lastInstr(owner->fn.instr),&right.type,&tDst);
            
            switch(op->code)
            {
                case LESS:
                    switch(tDst.tb) 
                    {
                        case TB_INT: addInstr(&owner->fn.instr, OP_LESS_I); break;
                        case TB_DOUBLE: addInstr(&owner->fn.instr, OP_LESS_F); break;
                        case TB_CHAR: /* not implemented */ break;
                        case TB_VOID: /* not applicable */ break;
                        case TB_STRUCT: /* not applicable */ break;
                        default: break;
                    }
                    break;
                case LESSEQ:
                    switch(tDst.tb)
                    {
                        case TB_INT: addInstr(&owner->fn.instr, OP_LESSEQ_I); break;
                        case TB_DOUBLE: addInstr(&owner->fn.instr, OP_LESSEQ_F); break;
                        case TB_CHAR: /* not implemented */ break;
                        case TB_VOID: /* not applicable */ break;
                        case TB_STRUCT: /* not applicable */ break;
                        default: break;
                    }
                    break;
                case GREATER:
                    switch(tDst.tb)
                    {
                        case TB_INT: addInstr(&owner->fn.instr, OP_GREATER_I); break;
                        case TB_DOUBLE: addInstr(&owner->fn.instr, OP_GREATER_F); break;
                        case TB_CHAR: /* not implemented */ break;
                        case TB_VOID: /* not applicable */ break;
                        case TB_STRUCT: /* not applicable */ break;
                        default: break;
                    }
                    break;
                case GREATEREQ:
                    switch(tDst.tb)
                    {
                        case TB_INT: addInstr(&owner->fn.instr, OP_GREATEREQ_I); break;
                        case TB_DOUBLE: addInstr(&owner->fn.instr, OP_GREATEREQ_F); break;
                        case TB_CHAR: /* not implemented */ break;
                        case TB_VOID: /* not applicable */ break;
                        case TB_STRUCT: /* not applicable */ break;
                        default: break;
                    }
                    break;
                case TB_CHAR: /* not implemented */ break;
                case TB_VOID: /* not applicable */ break;
                case TB_STRUCT: /* not applicable */ break;
                default: break;
            }
            
            *r = (Ret){{TB_INT, NULL, -1}, false, true};

            if (exprRelPrim(r)) 
            {
                return true;
            }
        } 
        else 
        {
            iTk = start;
            if (op->code == LESS) 
            {
                tkerr(iTk, "missging expression after '<'");
            }
            else if (op->code == LESSEQ) 
            {
                tkerr(iTk, "missing expression after '<='");
            }
            else if (op->code == GREATER) 
            {
                tkerr(iTk, "Missing expression after '>'");
            }
            else if (op->code == GREATEREQ) 
            {
                tkerr(iTk, "missing expression after '>='");
            }
        }
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return true;
}

// exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul
bool exprAdd(Ret *r)
{
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;

    if(exprMul(r))
    {
        if(exprAddPrim(r))
        {
            return true;
        }
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return false;
}

bool exprAddPrim(Ret *r) 
{
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
    Token *op;
    
    if (consume(ADD) || consume(SUB)) 
    {
        op = consumedTk;
        Instr *lastLeft = lastInstr(owner->fn.instr);
        addRVal(&owner->fn.instr, r->lval, &r->type);
        
        Ret right;
        if (exprMul(&right)) 
        {
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type, &tDst))
            {
                tkerr(iTk, "invalid operand type for + or -");
            }
            
            addRVal(&owner->fn.instr,right.lval, &right.type);
            insertConvIfNeeded(lastLeft, &r->type, &tDst);
            insertConvIfNeeded(lastInstr(owner->fn.instr), &right.type, &tDst);
            
            switch(op->code)
            {
                case ADD:
                    switch(tDst.tb)
                    {
                        case TB_INT: addInstr(&owner->fn.instr, OP_ADD_I); break;
                        case TB_DOUBLE: addInstr(&owner->fn.instr, OP_ADD_F); break;
                        case TB_CHAR: /* not implemented */ break;
                        case TB_VOID: /* not applicable */ break;
                        case TB_STRUCT: /* not applicable */ break;
                        default: break;
                    }
                    break;
                case SUB:
                    switch(tDst.tb)
                    {
                        case TB_INT: addInstr(&owner->fn.instr, OP_SUB_I);break;
                        case TB_DOUBLE: addInstr(&owner->fn.instr, OP_SUB_F);break;
                        case TB_CHAR: /* not implemented */ break;
                        case TB_VOID: /* not applicable */ break;
                        case TB_STRUCT: /* not applicable */ break;
                        default: break;
                    }
                    break;
                case TB_CHAR: /* not implemented */ break;
                case TB_VOID: /* not applicable */ break;
                case TB_STRUCT: /* not applicable */ break;
                default: break;
            }
            
            *r = (Ret){tDst, false, true};

            if (exprAddPrim(r)) 
            {
                return true;
            }
        } 
        else 
        {
            iTk = start;
            if (op->code == ADD) 
            {
                tkerr(iTk, "missing expression after '+'");
            }
            else if (op->code == SUB) 
            {
                tkerr(iTk, "missing expression after '-'");
            }
        }
    }
    
    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return true;
}

// exprMul: exprMul ( MUL | DIV ) exprCast | exprCast
bool exprMul(Ret *r)
{
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;

    if(exprCast(r))
    {
        if(exprMulPrim(r))
        {
            return true;
        }
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return false;
}

bool exprMulPrim(Ret *r)
{
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
    Token *op;

    if(consume(MUL) || consume(DIV))
    {
        op = consumedTk;
        Instr *lastLeft = lastInstr(owner->fn.instr);
        addRVal(&owner->fn.instr, r->lval, &r->type);
        
        Ret right;
        if(exprCast(&right))
        {
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type, &tDst))
            {
                tkerr(iTk, "invalid operand type for * or /");
            }
            
            addRVal(&owner->fn.instr,right.lval, &right.type);
            insertConvIfNeeded(lastLeft, &r->type, &tDst);
            insertConvIfNeeded(lastInstr(owner->fn.instr), &right.type, &tDst);
            
            switch(op->code)
            {
                case MUL:
                    switch(tDst.tb) 
                    {
                        case TB_INT: addInstr(&owner->fn.instr, OP_MUL_I); break;
                        case TB_DOUBLE: addInstr(&owner->fn.instr, OP_MUL_F); break;
                        case TB_CHAR: /* not implemented */ break;
                        case TB_VOID: /* not applicable */ break;
                        case TB_STRUCT: /* not applicable */ break;
                        default: break;
                    }
                    break;
                case DIV:
                    switch(tDst.tb)
                    {
                        case TB_INT: addInstr(&owner->fn.instr, OP_DIV_I); break;
                        case TB_DOUBLE: addInstr(&owner->fn.instr, OP_DIV_F); break;
                        case TB_CHAR: /* not implemented */ break;
                        case TB_VOID: /* not applicable */ break;
                        case TB_STRUCT: /* not applicable */ break;
                        default: break;
                    }
                    break;
                case TB_CHAR: /* not implemented */ break;
                case TB_VOID: /* not applicable */ break;
                case TB_STRUCT: /* not applicable */ break;
                default: break;
            }
            
            *r = (Ret){tDst, false, true};

            if(exprMulPrim(r))
            {
                return true;
            }
        }
        else 
        {
            iTk = start;
            if (op->code == MUL) 
            {
                tkerr(iTk, "missing expression after '*'");
            }
            else if (op->code == DIV) 
            {
                tkerr(iTk, "missing expression after '/'");
            }
        }    
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return true;
}

// exprPostfix: exprPostfix LBRACKET expr RBRACKET
// | exprPostfix DOT ID
// | exprPrimary
bool exprPostfix(Ret *r)
{
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;

    if(exprPrimary(r))
    {
        if(exprPostfixPrim(r))
        {
            return true;
        }
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return false;
}

bool exprPostfixPrim(Ret *r)
{
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;

    if(consume(LBRACKET))
    {
        Ret idx;

        if(expr(&idx))
        {
            if(consume(RBRACKET))
            {
                if(r->type.n < 0)
                {
                    tkerr(iTk, "only an array can be indexed");
                }
                Type tInt = {TB_INT, NULL, -1};
                if(!convTo(&idx.type, &tInt))
                {
                    tkerr(iTk, "the index is not convertible to int");
                }
                r->type.n = -1;
                r->lval = true;
                r->ct = false;

                if(exprPostfixPrim(r))
                {
                    return true;
                }
            }
            else tkerr(iTk, " ']' missing");
        }
        else tkerr(iTk, " expression missing between []");
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    if(consume(DOT))
    {
        if(consume(ID))
        {
            Token *tkName = consumedTk;
            if(r->type.tb != TB_STRUCT)
            {
                tkerr(iTk, "a field can only be selected from a struct");
            }
            Symbol *s = findSymbolInList(r->type.s->structMembers, tkName->text);
            if(!s)
            {
                tkerr(iTk, "the structure %s does not have a field %s", r->type.s->name, tkName->text);
            }
            *r=(Ret){s->type,true,s->type.n>=0};

            if(exprPostfixPrim(r))
            {
                return true;
            }
        }
        else tkerr(iTk, " identifier missing after . (dot)");
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    if(exprPrimary(r))
    {
        return true;
    }

    iTk = start;
    if(owner)delInstrAfter(startInstr);
    return true;
}


void parse(Token *tokens)
{
	iTk = tokens;
	if(!unit())
	{
		tkerr(iTk,"syntax error");
	}
}