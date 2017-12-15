%{
#include <cstdio>
#include <string>
#include "binarytree.h"

int fieldModulus = 1234577;

extern int yylineno;
int yylex();
int yyerror(char*);
int calculateTree(TreeNode* node, int modulus);
void postfixTree(TreeNode* node, int modulus);
%}
%union {
	TreeNode* node;
}
%type <node> exp line
%token <node> VAL
%token PLUS
%token MINUS
%token MULT
%token DIV
%token EXP
%token LPAR
%token RPAR
%token END
%token ERROR
%token UNARY_NEG
%left PLUS MINUS
%left MULT DIV
%precedence NEG
%right EXP
%%
input:
| input line
;
line: 
  exp END 	{
	postfixTree($$, fieldModulus);
	printf("\n");
	try{
		int result = calculateTree($$, fieldModulus);
		printf("Wynik: %d\n",result);
	} catch (char const* ex){
		printf("%s\n", ex);
	}
	clear($$);
}
| error END	{ printf("Błąd.\n"); }
;
exp:
  VAL			{ $$ = $1; }
| exp PLUS exp		{ $$ = opNode(PLUS, $1, $3); }
| exp MINUS exp		{ $$ = opNode(MINUS, $1, $3); }
| exp MULT exp		{ $$ = opNode(MULT, $1, $3); }
| exp DIV exp		{ $$ = opNode(DIV, $1, $3); }
| MINUS exp  %prec NEG	{ $$ = opNode(UNARY_NEG, $2, NULL); }
| exp EXP exp		{ $$ = opNode(EXP, $1, $3); }
| LPAR exp RPAR		{ $$ = $2; }
;
%%
int phi(int n){
	if (n == 1){
		return 1;
	}
	int result = n;

	for (int p=2; p*p<=n; ++p){
		// p - prime factor of n
		if (n % p == 0)		{
			while (n % p == 0)
				n /= p;
			result = (result / p) * (p-1);
		}
	}
	if (n > 1)
		result = (result / n) * (n-1);
 
	return result;
}
const char* tokenToStr(int token){
	if (token == VAL)
		return "VAL";
	if (token == PLUS)
		return "+";
	if (token == MINUS)
		return "-";
	if (token == MULT)
		return "*";
	if (token == DIV)
		return "/";
	if (token == EXP)
		return "^";
	if (token == UNARY_NEG)
		return "neg";
}

int gcd(int a, int b) {
	int c;
	while(b != 0) {
		c = a % b;
		a = b;
		b = c;
	}
	return a;
}

int mod(long long int dividend, int divisor){
	if (divisor < 0){
		return dividend;
	}
	int result = dividend % (long)divisor;
	if (result < 0){
		result = result + divisor;
	}
	return result;
}

int modpow(int a, int b, int modulus){
	if (b == 0){
		return 1;
	}
	if (a == 0){
		return 0;
	}
	if (b == 1){
		return a;
	}
	if (b%2 == 0){
		int res = modpow(a, b/2, modulus);
		return mod((long long int)res * (long long int)res, modulus);
	} else {
		int res = modpow(a, b/2, modulus);
		return mod((long long int)mod((long long int)res * (long long int)res, modulus) * (long long int)a, modulus);
	}
}

int inverse(int a, int modulus) {
	if (modulus == -1)
		return 0;
	int b = modulus;
	if (gcd(a, modulus) != 1){
		return 0;
	}
	int p = 1, q = 0, r = 0, s = 1;
	int c, quot;
	while (b != 0){
		c = a % b;
		quot = a / b;
		a = b;
		b = c;
		int r_1 = r, s_1 = s;
		r = p - quot * r;
		s = q - quot * s;
		p = r_1;
		q = s_1;
	}
	return mod(p, modulus);
}

int calculateTree(TreeNode* node, int modulus) {
	if (node != NULL){
		int token = node->token;
		if (token == VAL){
			return mod(node->value, modulus);
		}
		int lvalue = calculateTree(node->left, modulus);
		if (token == UNARY_NEG){
			return mod(-lvalue, modulus);
		}
		if (token == EXP){
			// euler's theorem
			//printf("%d - > %d\n", modulus, phi(modulus));
			int rvalue;
			if (gcd(lvalue, modulus) == 1){
				rvalue = calculateTree(node->right, phi(modulus));
			} else {
				rvalue = calculateTree(node->right, -1);
			}
			if (lvalue == 0 && rvalue < 0){
				throw "Nie ma elementu odwrotnego";
			}
			if (rvalue < 0){
				throw "Nie dopuszczalnie działanie na liczbach całkowitych";
			}
			return modpow(lvalue, rvalue, modulus);
		}
		int rvalue = calculateTree(node->right, modulus);
		int inv;
		switch(token) {
			case PLUS: return mod(lvalue + rvalue, modulus);
			case MINUS: return mod(lvalue - rvalue, modulus);
			case MULT: return mod((long long int)lvalue * (long long int)rvalue, modulus);
			case DIV:
				inv = inverse(rvalue, modulus);
				if (inv == 0){
					throw "Nie ma elementu odwrotnego";
				}
				return mod((long long int)lvalue * (long long int)inv, modulus);
			default: return 0;
		}//printf("VAL:%d TOKEN:%s \n", node->value, tokenToStr(node->token));
	}
	return 0;
}

void postfixTree(TreeNode* node, int modulus) {
	if (node != NULL){
		int token = node->token;
		if (token == UNARY_NEG && node->left->token == VAL){
			int value = mod(-node->left->value, modulus);
			printf("%d ", value);
		} else if (token == VAL){ 
			int value = mod(node->value, modulus);
			printf("%d ", value);
		} else if (token == EXP) {
			postfixTree(node->left, modulus);
			// euler's theorem
			postfixTree(node->right, phi(modulus));
			printf("^ ");
		} else {
			postfixTree(node->left, modulus);
			postfixTree(node->right, modulus);
			printf("%s ", tokenToStr(token));
		}
	}
}

int yyerror(char *s)
{
	//printf("%s\n",s);	
	return 0;
}

int main()
{
	yyparse();
	printf("Przetworzono linii: %d\n",yylineno-1);
	return 0;
}

