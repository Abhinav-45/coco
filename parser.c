#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "lexer.h"
#include "parser.h"

/* ============================================================ */
/*              NON-TERMINAL NAME STRINGS                       */
/* ============================================================ */

const char *nonTerminalStrings[] = {
    "program",          "otherFunctions",   "function",
    "mainFunction",     "input",            "output",
    "parameterList",    "morePL",           "dataType",
    "stmts",            "stmt",             "typeStmt",
    "assignmentStmt",   "callAssignStmt",   "callStmt",
    "actualParamList",  "moreActual",       "iterativeStmt",
    "conditionalStmt",  "elseStmt",         "ioStmt",
    "returnStmt",       "optReturnVal",     "boolExpr",
    "relOp",            "expr",             "exprPrime",
    "term",             "termPrime",        "factor"
};

/* ============================================================ */
/*                 GRAMMAR INITIALISATION                       */
/* ============================================================ */

/* Helper: add a production to the grammar */
static void addProd(Grammar *G, NonTerminal lhs,
                    int isTerms[], int syms[], int len)
{
    Production *p = &G->prods[G->numProds++];
    p->lhs   = lhs;
    p->rhsLen = len;
    for (int i = 0; i < len; i++) {
        p->rhs[i].isTerminal = isTerms[i];
        p->rhs[i].symbol     = syms[i];
    }
}

/* Shorthand macros */
#define T  1   /* isTerminal = true  */
#define NT 0   /* isTerminal = false */

/*
 * Grammar productions (LL(1)):
 *
 *  P0:  program          → otherFunctions mainFunction
 *  P1:  otherFunctions   → function otherFunctions
 *  P2:  otherFunctions   → ε
 *  P3:  function         → TK_FUNID input output stmts TK_END
 *  P4:  mainFunction     → TK_MAIN stmts TK_END
 *  P5:  input            → TK_INPUT TK_PARAMETER TK_LIST TK_SQL parameterList TK_SQR
 *  P6:  input            → ε
 *  P7:  output           → TK_OUTPUT TK_PARAMETER TK_LIST TK_SQL parameterList TK_SQR TK_SEM
 *  P8:  output           → ε
 *  P9:  parameterList    → dataType TK_ID morePL
 *  P10: morePL           → TK_COMMA dataType TK_ID morePL
 *  P11: morePL           → ε
 *  P12: dataType         → TK_INT
 *  P13: dataType         → TK_REAL
 *  P14: dataType         → TK_ID
 *  P15: dataType         → TK_FIELDID
 *  P16: stmts            → stmt stmts
 *  P17: stmts            → ε
 *  P18: stmt             → typeStmt
 *  P19: stmt             → assignmentStmt
 *  P20: stmt             → callAssignStmt
 *  P21: stmt             → iterativeStmt
 *  P22: stmt             → conditionalStmt
 *  P23: stmt             → ioStmt
 *  P24: stmt             → returnStmt
 *  P25: typeStmt         → TK_TYPE dataType TK_COLON TK_ID TK_SEM
 *  P26: assignmentStmt   → TK_ID TK_ASSIGNOP expr TK_SEM
 *  P27: callAssignStmt   → TK_SQL TK_ID TK_SQR TK_ASSIGNOP callStmt TK_SEM
 *  P28: callStmt         → TK_CALL TK_FUNID TK_WITH TK_PARAMETERS TK_SQL actualParamList TK_SQR
 *  P29: actualParamList  → TK_ID moreActual
 *  P30: moreActual       → TK_COMMA TK_ID moreActual
 *  P31: moreActual       → ε
 *  P32: iterativeStmt    → TK_WHILE TK_OP boolExpr TK_CL stmts TK_ENDWHILE
 *  P33: conditionalStmt  → TK_IF TK_OP boolExpr TK_CL TK_THEN stmts elseStmt TK_ENDIF
 *  P34: elseStmt         → TK_ELSE stmts
 *  P35: elseStmt         → ε
 *  P36: ioStmt           → TK_READ TK_OP TK_ID TK_CL TK_SEM
 *  P37: ioStmt           → TK_WRITE TK_OP expr TK_CL TK_SEM
 *  P38: returnStmt       → TK_RETURN optReturnVal TK_SEM
 *  P39: optReturnVal     → TK_SQL TK_ID TK_SQR
 *  P40: optReturnVal     → ε
 *  P41: boolExpr         → expr relOp expr
 *  P42: boolExpr         → TK_NOT boolExpr
 *  P43: relOp            → TK_LT
 *  P44: relOp            → TK_LE
 *  P45: relOp            → TK_EQ
 *  P46: relOp            → TK_GT
 *  P47: relOp            → TK_GE
 *  P48: relOp            → TK_NE
 *  P49: expr             → term exprPrime
 *  P50: exprPrime        → TK_PLUS  term exprPrime
 *  P51: exprPrime        → TK_MINUS term exprPrime
 *  P52: exprPrime        → ε
 *  P53: term             → factor termPrime
 *  P54: termPrime        → TK_MUL factor termPrime
 *  P55: termPrime        → TK_DIV factor termPrime
 *  P56: termPrime        → ε
 *  P57: factor           → TK_OP expr TK_CL
 *  P58: factor           → TK_ID
 *  P59: factor           → TK_NUM
 *  P60: factor           → TK_RNUM
 */

void initGrammar(Grammar *G)
{
    G->numProds = 0;

    /* P0: program → otherFunctions mainFunction */
    { int it[]={NT,NT}; int sy[]={NT_OTHER_FUNCTIONS,NT_MAIN_FUNCTION};
      addProd(G, NT_PROGRAM, it, sy, 2); }

    /* P1: otherFunctions → function otherFunctions */
    { int it[]={NT,NT}; int sy[]={NT_FUNCTION,NT_OTHER_FUNCTIONS};
      addProd(G, NT_OTHER_FUNCTIONS, it, sy, 2); }

    /* P2: otherFunctions → ε */
    { addProd(G, NT_OTHER_FUNCTIONS, NULL, NULL, 0); }

    /* P3: function → TK_FUNID input output stmts TK_END */
    { int it[]={T,NT,NT,NT,T}; int sy[]={TK_FUNID,NT_INPUT,NT_OUTPUT,NT_STMTS,TK_END};
      addProd(G, NT_FUNCTION, it, sy, 5); }

    /* P4: mainFunction → TK_MAIN stmts TK_END */
    { int it[]={T,NT,T}; int sy[]={TK_MAIN,NT_STMTS,TK_END};
      addProd(G, NT_MAIN_FUNCTION, it, sy, 3); }

    /* P5: input → TK_INPUT TK_PARAMETER TK_LIST TK_SQL parameterList TK_SQR */
    { int it[]={T,T,T,T,NT,T};
      int sy[]={TK_INPUT,TK_PARAMETER,TK_LIST,TK_SQL,NT_PARAMETER_LIST,TK_SQR};
      addProd(G, NT_INPUT, it, sy, 6); }

    /* P6: input → ε */
    { addProd(G, NT_INPUT, NULL, NULL, 0); }

    /* P7: output → TK_OUTPUT TK_PARAMETER TK_LIST TK_SQL parameterList TK_SQR TK_SEM */
    { int it[]={T,T,T,T,NT,T,T};
      int sy[]={TK_OUTPUT,TK_PARAMETER,TK_LIST,TK_SQL,NT_PARAMETER_LIST,TK_SQR,TK_SEM};
      addProd(G, NT_OUTPUT, it, sy, 7); }

    /* P8: output → ε */
    { addProd(G, NT_OUTPUT, NULL, NULL, 0); }

    /* P9: parameterList → dataType TK_ID morePL */
    { int it[]={NT,T,NT}; int sy[]={NT_DATA_TYPE,TK_ID,NT_MORE_PL};
      addProd(G, NT_PARAMETER_LIST, it, sy, 3); }

    /* P10: morePL → TK_COMMA dataType TK_ID morePL */
    { int it[]={T,NT,T,NT}; int sy[]={TK_COMMA,NT_DATA_TYPE,TK_ID,NT_MORE_PL};
      addProd(G, NT_MORE_PL, it, sy, 4); }

    /* P11: morePL → ε */
    { addProd(G, NT_MORE_PL, NULL, NULL, 0); }

    /* P12: dataType → TK_INT */
    { int it[]={T}; int sy[]={TK_INT};
      addProd(G, NT_DATA_TYPE, it, sy, 1); }

    /* P13: dataType → TK_REAL */
    { int it[]={T}; int sy[]={TK_REAL};
      addProd(G, NT_DATA_TYPE, it, sy, 1); }

    /* P14: dataType → TK_ID */
    { int it[]={T}; int sy[]={TK_ID};
      addProd(G, NT_DATA_TYPE, it, sy, 1); }

    /* P15: dataType → TK_FIELDID */
    { int it[]={T}; int sy[]={TK_FIELDID};
      addProd(G, NT_DATA_TYPE, it, sy, 1); }

    /* P16: stmts → stmt stmts */
    { int it[]={NT,NT}; int sy[]={NT_STMT,NT_STMTS};
      addProd(G, NT_STMTS, it, sy, 2); }

    /* P17: stmts → ε */
    { addProd(G, NT_STMTS, NULL, NULL, 0); }

    /* P18: stmt → typeStmt */
    { int it[]={NT}; int sy[]={NT_TYPE_STMT};
      addProd(G, NT_STMT, it, sy, 1); }

    /* P19: stmt → assignmentStmt */
    { int it[]={NT}; int sy[]={NT_ASSIGNMENT_STMT};
      addProd(G, NT_STMT, it, sy, 1); }

    /* P20: stmt → callAssignStmt */
    { int it[]={NT}; int sy[]={NT_CALL_ASSIGN_STMT};
      addProd(G, NT_STMT, it, sy, 1); }

    /* P21: stmt → iterativeStmt */
    { int it[]={NT}; int sy[]={NT_ITERATIVE_STMT};
      addProd(G, NT_STMT, it, sy, 1); }

    /* P22: stmt → conditionalStmt */
    { int it[]={NT}; int sy[]={NT_CONDITIONAL_STMT};
      addProd(G, NT_STMT, it, sy, 1); }

    /* P23: stmt → ioStmt */
    { int it[]={NT}; int sy[]={NT_IO_STMT};
      addProd(G, NT_STMT, it, sy, 1); }

    /* P24: stmt → returnStmt */
    { int it[]={NT}; int sy[]={NT_RETURN_STMT};
      addProd(G, NT_STMT, it, sy, 1); }

    /* P25: typeStmt → TK_TYPE dataType TK_COLON TK_ID TK_SEM */
    { int it[]={T,NT,T,T,T}; int sy[]={TK_TYPE,NT_DATA_TYPE,TK_COLON,TK_ID,TK_SEM};
      addProd(G, NT_TYPE_STMT, it, sy, 5); }

    /* P26: assignmentStmt → TK_ID TK_ASSIGNOP expr TK_SEM */
    { int it[]={T,T,NT,T}; int sy[]={TK_ID,TK_ASSIGNOP,NT_EXPR,TK_SEM};
      addProd(G, NT_ASSIGNMENT_STMT, it, sy, 4); }

    /* P27: callAssignStmt → TK_SQL TK_ID TK_SQR TK_ASSIGNOP callStmt TK_SEM */
    { int it[]={T,T,T,T,NT,T};
      int sy[]={TK_SQL,TK_ID,TK_SQR,TK_ASSIGNOP,NT_CALL_STMT,TK_SEM};
      addProd(G, NT_CALL_ASSIGN_STMT, it, sy, 6); }

    /* P28: callStmt → TK_CALL TK_FUNID TK_WITH TK_PARAMETERS TK_SQL actualParamList TK_SQR */
    { int it[]={T,T,T,T,T,NT,T};
      int sy[]={TK_CALL,TK_FUNID,TK_WITH,TK_PARAMETERS,TK_SQL,NT_ACTUAL_PARAM_LIST,TK_SQR};
      addProd(G, NT_CALL_STMT, it, sy, 7); }

    /* P29: actualParamList → TK_ID moreActual */
    { int it[]={T,NT}; int sy[]={TK_ID,NT_MORE_ACTUAL};
      addProd(G, NT_ACTUAL_PARAM_LIST, it, sy, 2); }

    /* P30: moreActual → TK_COMMA TK_ID moreActual */
    { int it[]={T,T,NT}; int sy[]={TK_COMMA,TK_ID,NT_MORE_ACTUAL};
      addProd(G, NT_MORE_ACTUAL, it, sy, 3); }

    /* P31: moreActual → ε */
    { addProd(G, NT_MORE_ACTUAL, NULL, NULL, 0); }

    /* P32: iterativeStmt → TK_WHILE TK_OP boolExpr TK_CL stmts TK_ENDWHILE */
    { int it[]={T,T,NT,T,NT,T};
      int sy[]={TK_WHILE,TK_OP,NT_BOOL_EXPR,TK_CL,NT_STMTS,TK_ENDWHILE};
      addProd(G, NT_ITERATIVE_STMT, it, sy, 6); }

    /* P33: conditionalStmt → TK_IF TK_OP boolExpr TK_CL TK_THEN stmts elseStmt TK_ENDIF */
    { int it[]={T,T,NT,T,T,NT,NT,T};
      int sy[]={TK_IF,TK_OP,NT_BOOL_EXPR,TK_CL,TK_THEN,NT_STMTS,NT_ELSE_STMT,TK_ENDIF};
      addProd(G, NT_CONDITIONAL_STMT, it, sy, 8); }

    /* P34: elseStmt → TK_ELSE stmts */
    { int it[]={T,NT}; int sy[]={TK_ELSE,NT_STMTS};
      addProd(G, NT_ELSE_STMT, it, sy, 2); }

    /* P35: elseStmt → ε */
    { addProd(G, NT_ELSE_STMT, NULL, NULL, 0); }

    /* P36: ioStmt → TK_READ TK_OP TK_ID TK_CL TK_SEM */
    { int it[]={T,T,T,T,T}; int sy[]={TK_READ,TK_OP,TK_ID,TK_CL,TK_SEM};
      addProd(G, NT_IO_STMT, it, sy, 5); }

    /* P37: ioStmt → TK_WRITE TK_OP expr TK_CL TK_SEM */
    { int it[]={T,T,NT,T,T}; int sy[]={TK_WRITE,TK_OP,NT_EXPR,TK_CL,TK_SEM};
      addProd(G, NT_IO_STMT, it, sy, 5); }

    /* P38: returnStmt → TK_RETURN optReturnVal TK_SEM */
    { int it[]={T,NT,T}; int sy[]={TK_RETURN,NT_OPT_RETURN_VAL,TK_SEM};
      addProd(G, NT_RETURN_STMT, it, sy, 3); }

    /* P39: optReturnVal → TK_SQL TK_ID TK_SQR */
    { int it[]={T,T,T}; int sy[]={TK_SQL,TK_ID,TK_SQR};
      addProd(G, NT_OPT_RETURN_VAL, it, sy, 3); }

    /* P40: optReturnVal → ε */
    { addProd(G, NT_OPT_RETURN_VAL, NULL, NULL, 0); }

    /* P41: boolExpr → expr relOp expr */
    { int it[]={NT,NT,NT}; int sy[]={NT_EXPR,NT_REL_OP,NT_EXPR};
      addProd(G, NT_BOOL_EXPR, it, sy, 3); }

    /* P42: boolExpr → TK_NOT boolExpr */
    { int it[]={T,NT}; int sy[]={TK_NOT,NT_BOOL_EXPR};
      addProd(G, NT_BOOL_EXPR, it, sy, 2); }

    /* P43–P48: relOp → TK_LT | TK_LE | TK_EQ | TK_GT | TK_GE | TK_NE */
    { int it[]={T}; int sy[]={TK_LT}; addProd(G, NT_REL_OP, it, sy, 1); }
    { int it[]={T}; int sy[]={TK_LE}; addProd(G, NT_REL_OP, it, sy, 1); }
    { int it[]={T}; int sy[]={TK_EQ}; addProd(G, NT_REL_OP, it, sy, 1); }
    { int it[]={T}; int sy[]={TK_GT}; addProd(G, NT_REL_OP, it, sy, 1); }
    { int it[]={T}; int sy[]={TK_GE}; addProd(G, NT_REL_OP, it, sy, 1); }
    { int it[]={T}; int sy[]={TK_NE}; addProd(G, NT_REL_OP, it, sy, 1); }

    /* P49: expr → term exprPrime */
    { int it[]={NT,NT}; int sy[]={NT_TERM,NT_EXPR_PRIME};
      addProd(G, NT_EXPR, it, sy, 2); }

    /* P50: exprPrime → TK_PLUS term exprPrime */
    { int it[]={T,NT,NT}; int sy[]={TK_PLUS,NT_TERM,NT_EXPR_PRIME};
      addProd(G, NT_EXPR_PRIME, it, sy, 3); }

    /* P51: exprPrime → TK_MINUS term exprPrime */
    { int it[]={T,NT,NT}; int sy[]={TK_MINUS,NT_TERM,NT_EXPR_PRIME};
      addProd(G, NT_EXPR_PRIME, it, sy, 3); }

    /* P52: exprPrime → ε */
    { addProd(G, NT_EXPR_PRIME, NULL, NULL, 0); }

    /* P53: term → factor termPrime */
    { int it[]={NT,NT}; int sy[]={NT_FACTOR,NT_TERM_PRIME};
      addProd(G, NT_TERM, it, sy, 2); }

    /* P54: termPrime → TK_MUL factor termPrime */
    { int it[]={T,NT,NT}; int sy[]={TK_MUL,NT_FACTOR,NT_TERM_PRIME};
      addProd(G, NT_TERM_PRIME, it, sy, 3); }

    /* P55: termPrime → TK_DIV factor termPrime */
    { int it[]={T,NT,NT}; int sy[]={TK_DIV,NT_FACTOR,NT_TERM_PRIME};
      addProd(G, NT_TERM_PRIME, it, sy, 3); }

    /* P56: termPrime → ε */
    { addProd(G, NT_TERM_PRIME, NULL, NULL, 0); }

    /* P57: factor → TK_OP expr TK_CL */
    { int it[]={T,NT,T}; int sy[]={TK_OP,NT_EXPR,TK_CL};
      addProd(G, NT_FACTOR, it, sy, 3); }

    /* P58: factor → TK_ID */
    { int it[]={T}; int sy[]={TK_ID}; addProd(G, NT_FACTOR, it, sy, 1); }

    /* P59: factor → TK_NUM */
    { int it[]={T}; int sy[]={TK_NUM}; addProd(G, NT_FACTOR, it, sy, 1); }

    /* P60: factor → TK_RNUM */
    { int it[]={T}; int sy[]={TK_RNUM}; addProd(G, NT_FACTOR, it, sy, 1); }
}

#undef T
#undef NT

/* ============================================================ */
/*            FIRST AND FOLLOW SET COMPUTATION                  */
/* ============================================================ */

/*
 * Compute FIRST of a sequence of RHS symbols starting at index `start`.
 * Writes results into `outFirst[NUM_TERMINALS]` and *outHasEps.
 */
static void firstOfSeq(Grammar *G, FirstAndFollow *F,
                       GrammarSymbol *rhs, int len, int start,
                       bool *outFirst, bool *outHasEps)
{
    *outHasEps = true; /* assume epsilon until we find a non-nullable symbol */

    for (int i = start; i < len; i++) {
        GrammarSymbol sym = rhs[i];
        if (sym.isTerminal) {
            outFirst[sym.symbol] = true;
            *outHasEps = false;
            return;
        } else {
            NonTerminal nt = (NonTerminal)sym.symbol;
            for (int t = 0; t < NUM_TERMINALS; t++) {
                if (F->first[nt][t]) outFirst[t] = true;
            }
            if (!F->firstHasEps[nt]) {
                *outHasEps = false;
                return;
            }
        }
    }
    /* reached end of sequence: epsilon is possible */
}

FirstAndFollow computeFirstAndFollowSets(Grammar *G)
{
    FirstAndFollow F;
    memset(&F, 0, sizeof(F));

    /* --- Compute FIRST sets (iterative fixpoint) --- */
    bool changed = true;
    while (changed) {
        changed = false;
        for (int p = 0; p < G->numProds; p++) {
            Production *prod = &G->prods[p];
            NonTerminal A    = prod->lhs;

            if (prod->rhsLen == 0) {
                /* epsilon production */
                if (!F.firstHasEps[A]) { F.firstHasEps[A] = true; changed = true; }
                continue;
            }

            bool seqHasEps = true;
            for (int i = 0; i < prod->rhsLen; i++) {
                GrammarSymbol sym = prod->rhs[i];
                if (sym.isTerminal) {
                    if (!F.first[A][sym.symbol]) {
                        F.first[A][sym.symbol] = true; changed = true;
                    }
                    seqHasEps = false;
                    break;
                } else {
                    NonTerminal nt = (NonTerminal)sym.symbol;
                    for (int t = 0; t < NUM_TERMINALS; t++) {
                        if (F.first[nt][t] && !F.first[A][t]) {
                            F.first[A][t] = true; changed = true;
                        }
                    }
                    if (!F.firstHasEps[nt]) { seqHasEps = false; break; }
                }
            }
            if (seqHasEps && !F.firstHasEps[A]) {
                F.firstHasEps[A] = true; changed = true;
            }
        }
    }

    /* --- Compute FOLLOW sets (iterative fixpoint) --- */
    /* Add $ to FOLLOW(program) */
    F.follow[NT_PROGRAM][TK_EOF] = true;

    changed = true;
    while (changed) {
        changed = false;
        for (int p = 0; p < G->numProds; p++) {
            Production *prod = &G->prods[p];
            NonTerminal A    = prod->lhs;

            for (int i = 0; i < prod->rhsLen; i++) {
                if (prod->rhs[i].isTerminal) continue;
                NonTerminal B = (NonTerminal)prod->rhs[i].symbol;

                /* compute FIRST of the suffix after B */
                bool suffixFirst[NUM_TERMINALS];
                bool suffixHasEps;
                memset(suffixFirst, 0, sizeof(suffixFirst));
                firstOfSeq(G, &F, prod->rhs, prod->rhsLen, i + 1,
                           suffixFirst, &suffixHasEps);

                for (int t = 0; t < NUM_TERMINALS; t++) {
                    if (suffixFirst[t] && !F.follow[B][t]) {
                        F.follow[B][t] = true; changed = true;
                    }
                }
                /* if suffix can derive ε, add FOLLOW(A) to FOLLOW(B) */
                if (suffixHasEps) {
                    for (int t = 0; t < NUM_TERMINALS; t++) {
                        if (F.follow[A][t] && !F.follow[B][t]) {
                            F.follow[B][t] = true; changed = true;
                        }
                    }
                }
            }
        }
    }

    return F;
}

/* ============================================================ */
/*                 PARSE TABLE CONSTRUCTION                     */
/* ============================================================ */

void createParseTable(FirstAndFollow *F, Grammar *G, ParseTable T)
{
    /* initialize all entries to -1 (error) */
    for (int nt = 0; nt < NUM_NON_TERMINALS; nt++)
        for (int t = 0; t < NUM_TERMINALS; t++)
            T[nt][t] = -1;

    for (int p = 0; p < G->numProds; p++) {
        Production *prod = &G->prods[p];
        NonTerminal A    = prod->lhs;

        if (prod->rhsLen == 0) {
            /* A → ε : add A → ε for each token in FOLLOW(A) */
            for (int t = 0; t < NUM_TERMINALS; t++) {
                if (F->follow[A][t]) {
                    if (T[A][t] == -1) T[A][t] = p;
                }
            }
        } else {
            /* compute FIRST of the RHS */
            bool rhsFirst[NUM_TERMINALS];
            bool rhsHasEps;
            memset(rhsFirst, 0, sizeof(rhsFirst));
            firstOfSeq(G, F, prod->rhs, prod->rhsLen, 0, rhsFirst, &rhsHasEps);

            for (int t = 0; t < NUM_TERMINALS; t++) {
                if (rhsFirst[t]) {
                    if (T[A][t] == -1) T[A][t] = p;
                }
            }
            if (rhsHasEps) {
                for (int t = 0; t < NUM_TERMINALS; t++) {
                    if (F->follow[A][t]) {
                        if (T[A][t] == -1) T[A][t] = p;
                    }
                }
            }
        }
    }
}

/* ============================================================ */
/*                 PARSE TREE UTILITIES                         */
/* ============================================================ */

static TreeNode *newNode(int isLeaf, int symbol)
{
    TreeNode *n = (TreeNode *)malloc(sizeof(TreeNode));
    memset(n, 0, sizeof(TreeNode));
    n->isLeaf = isLeaf;
    n->symbol = symbol;
    strcpy(n->lexeme, "----");
    n->lineNo   = 0;
    n->hasNumVal = 0;
    return n;
}

void freeTree(TreeNode *node)
{
    if (!node) return;
    for (int i = 0; i < node->numChildren; i++)
        freeTree(node->children[i]);
    free(node);
}

/* ============================================================ */
/*                       PARSER                                 */
/* ============================================================ */

#define PARSE_STACK_MAX 1024

/* Skip comments and error tokens; return first real token */
static tokenInfo getNextUsableToken(twinBuffer B)
{
    tokenInfo tk;
    do {
        tk = getNextToken(B);
    } while (tk->tokenType == TK_COMMENT || tk->tokenType == TK_ERROR);
    return tk;
}

ParseTree parseInputSourceCode(char *testcaseFile, ParseTable T, Grammar *G)
{
    FILE *fp = fopen(testcaseFile, "r");
    if (!fp) {
        fprintf(stderr, "Error: cannot open '%s'\n", testcaseFile);
        return NULL;
    }

    lineNumber = 1;
    twinBuffer B = initBuffer(fp);

    /* Allocate root node for the start symbol */
    TreeNode *root = newNode(0 /* non-leaf */, (int)NT_PROGRAM);

    /* Symbol stack and parallel tree-node stack */
    GrammarSymbol symStack[PARSE_STACK_MAX];
    TreeNode     *nodeStack[PARSE_STACK_MAX];
    int top = 0;

    /* Push EOF sentinel */
    symStack[top].isTerminal = 1;
    symStack[top].symbol     = TK_EOF;
    nodeStack[top]           = NULL;
    top++;

    /* Push start symbol */
    symStack[top].isTerminal = 0;
    symStack[top].symbol     = (int)NT_PROGRAM;
    nodeStack[top]           = root;
    top++;

    tokenInfo curTok = getNextUsableToken(B);
    int hasError = 0;

    while (top > 0) {
        GrammarSymbol X    = symStack[top - 1];
        TreeNode     *Xnod = nodeStack[top - 1];

        /* ---- EOF sentinel on top ---- */
        if (X.isTerminal && X.symbol == (int)TK_EOF) {
            if (curTok->tokenType == TK_EOF) {
                top--;
                break;  /* successful parse */
            } else {
                printf("Line %d : Syntax Error : Extra tokens after end of program\n",
                       curTok->lineNo);
                hasError = 1;
                break;
            }
        }

        /* ---- Terminal on top ---- */
        if (X.isTerminal) {
            if (curTok->tokenType == (TokenType)X.symbol) {
                /* Match! Fill in the leaf node */
                if (Xnod) {
                    strcpy(Xnod->lexeme, curTok->lexeme);
                    Xnod->lineNo    = curTok->lineNo;
                    Xnod->tokenType = curTok->tokenType;
                    Xnod->isLeaf    = 1;
                    if (curTok->tokenType == TK_NUM) {
                        Xnod->numVal    = (double)atoi(curTok->lexeme);
                        Xnod->hasNumVal = 1;
                    } else if (curTok->tokenType == TK_RNUM) {
                        Xnod->numVal    = atof(curTok->lexeme);
                        Xnod->hasNumVal = 1;
                    }
                }
                top--;
                free(curTok);
                curTok = getNextUsableToken(B);
            } else {
                /* Terminal mismatch error */
                printf("Line %d : Syntax Error : Expected token %s but got %s <%s>\n",
                       curTok->lineNo,
                       tokenStrings[X.symbol],
                       tokenStrings[curTok->tokenType],
                       curTok->lexeme);
                hasError = 1;
                /* Pop the unmatched terminal and continue */
                top--;
            }
            continue;
        }

        /* ---- Non-terminal on top ---- */
        NonTerminal nt = (NonTerminal)X.symbol;
        int prodIdx    = T[nt][curTok->tokenType];

        if (prodIdx == -1) {
            /* No production: syntax error */
            printf("Line %d : Syntax Error : Unexpected token %s <%s> while parsing %s\n",
                   curTok->lineNo,
                   tokenStrings[curTok->tokenType],
                   curTok->lexeme,
                   nonTerminalStrings[nt]);
            hasError = 1;

            /* Panic-mode recovery: skip current token and retry */
            if (curTok->tokenType == TK_EOF) {
                top--;  /* pop NT, will hit EOF sentinel next */
            } else {
                free(curTok);
                curTok = getNextUsableToken(B);
                /* keep same NT on stack; try with next token */
                /* but avoid infinite loop: if still no prod, pop NT */
                if (T[nt][curTok->tokenType] == -1) {
                    top--;
                }
            }
            continue;
        }

        /* Apply production */
        top--;
        Production *prod = &G->prods[prodIdx];

        if (prod->rhsLen == 0) {
            /* Epsilon production: no children pushed to the stack */
            /* We mark Xnod as an internal node with 0 children */
            if (Xnod) Xnod->isLeaf = 0;
        } else {
            if (Xnod) Xnod->isLeaf = 0;

            /* Create child nodes in left-to-right order */
            TreeNode *children[MAX_PROD_LEN];
            for (int i = 0; i < prod->rhsLen; i++) {
                int childIsLeaf  = prod->rhs[i].isTerminal;
                children[i] = newNode(childIsLeaf, prod->rhs[i].symbol);
                if (Xnod) {
                    children[i]->parent = Xnod;
                    Xnod->children[Xnod->numChildren++] = children[i];
                }
            }

            /* Push children in REVERSE order so leftmost is on top */
            for (int i = prod->rhsLen - 1; i >= 0; i--) {
                if (top >= PARSE_STACK_MAX) {
                    fprintf(stderr, "Parse stack overflow!\n");
                    freeTree(root);
                    free(curTok);
                    free(B);
                    fclose(fp);
                    return NULL;
                }
                symStack[top]  = prod->rhs[i];
                nodeStack[top] = children[i];
                top++;
            }
        }
    }

    free(curTok);
    free(B);
    fclose(fp);

    if (!hasError) {
        printf("Input source code is syntactically correct...........\n");
        return root;
    } else {
        return root; /* return partial tree even on error */
    }
}

/* ============================================================ */
/*                 PARSE TREE PRINTING                          */
/* ============================================================ */

/*
 * Pre-order (DFS) traversal of the parse tree.
 *
 * Each line contains:
 *   col1: lexeme (leaf) or "----" (internal)
 *   col2: line number (leaf) or "----" (internal)
 *   col3: token name (leaf) or "----" (internal)
 *   col4: numeric value (TK_NUM/TK_RNUM) or "----"
 *   col5: parent NT name (or "ROOT" for the root)
 *   col6: "yes" / "no"
 *   col7: NT symbol name (internal) or "----" (leaf)
 */
static void traverseTree(TreeNode *node, FILE *out)
{
    if (!node) return;

    /* Determine parent symbol string */
    char parentStr[64];
    if (!node->parent) {
        strcpy(parentStr, "ROOT");
    } else if (!node->parent->isLeaf) {
        strcpy(parentStr, nonTerminalStrings[node->parent->symbol]);
    } else {
        strcpy(parentStr, "----");
    }

    if (node->isLeaf && node->lineNo > 0) {
        /* Leaf node (matched terminal) */
        char valStr[64];
        if (node->hasNumVal) {
            if (node->tokenType == TK_NUM)
                snprintf(valStr, sizeof(valStr), "%d", (int)node->numVal);
            else
                snprintf(valStr, sizeof(valStr), "%g", node->numVal);
        } else {
            strcpy(valStr, "----");
        }

        fprintf(out, "%-20s  %-4d  %-20s  %-12s  %-20s  yes   ----\n",
                node->lexeme,
                node->lineNo,
                tokenStrings[node->tokenType],
                valStr,
                parentStr);
    } else if (!node->isLeaf) {
        /* Internal (non-terminal) node */
        fprintf(out, "%-20s  %-4s  %-20s  %-12s  %-20s  no    %s\n",
                "----",
                "----",
                "----",
                "----",
                parentStr,
                nonTerminalStrings[node->symbol]);

        /* Recurse over children */
        for (int i = 0; i < node->numChildren; i++)
            traverseTree(node->children[i], out);
    }
    /* If isLeaf but lineNo == 0 (unmatched placeholder), skip */
}

void printParseTree(ParseTree PT, char *outfile)
{
    if (!PT) {
        fprintf(stderr, "Parse tree is NULL; nothing to print.\n");
        return;
    }

    FILE *out = fopen(outfile, "w");
    if (!out) {
        fprintf(stderr, "Error: cannot open output file '%s'\n", outfile);
        return;
    }

    /* Header */
    fprintf(out, "%-20s  %-4s  %-20s  %-12s  %-20s  %-5s  %s\n",
            "Lexeme", "Line", "TokenName", "Value", "ParentNT", "Leaf", "NodeNT");
    fprintf(out,
            "---------------------------------------------------------------------"
            "--------------------------------\n");

    traverseTree(PT, out);
    fclose(out);
    printf("Parse tree written to '%s'.\n", outfile);
}
