#pragma once
#include <IAst.h>
#include <unordered_map>
#include "LibLsp/lsp/lsPosition.h"
#include "parser/LPGParser.h"
#include "parser/LPGParser_top_level_ast.h"
using namespace  LPGParser_top_level_ast;
struct ASTUtils
{
	template <typename T>
static	void allDefsOfType(std::vector<IAst*>& vec, std::unordered_multimap<std::wstring, IAst*>& s) {
		for (auto& it : s)
		{
			if (dynamic_cast<T*>(it.second))
			{
				vec.emplace_back(it.second);
			}
		}
	}
	static LPG *getRoot(IAst* node) {
		while (node != nullptr && !(dynamic_cast<LPG*>(node)))
			node = node->getParent();
		return static_cast<LPG*>(node);
	}
	static void getMacros(LPG* root, std::vector<ASTNodeToken*>& vec) {
		if (!root || !root->environment) return;
		// DO NOT pick up macros from any imported file! They shouldn't be
		// treated as defined in this scope!
		vec = root->environment->_macro_name_symbo;

	}
	static void getNonTerminals(LPG* root, std::vector<nonTerm*>& vec) {
		if (!root || !root->environment) return;
		// TODO: pick up non-terminals from imported files
		for(auto& it : root->environment->_non_terms)
		{
			vec.push_back(it.second);
		}
		
		
	}
	static void getTerminals(LPG* root, std::vector<terminal*>& vec) {
		if (!root || !root->environment) return;
		// TODO: pick up non-terminals from imported files
		for (auto& it : root->environment->_terms)
		{
			vec.push_back(it.second);
		}
	}

	static void findRefsOf(std::vector<ASTNode*>& result, nonTerm* nonTerm);

	static void findRefsOf(std::vector<ASTNode*>& result, terminal* term);

	static std::wstring getLine(ILexStream* lex, int line);
	static int toOffset(ILexStream* lex, const lsPosition& pos)
	{
		return  toOffset(lex, pos.line+1, pos.character);
	}
	static int toOffset(ILexStream* lex, int line, int column);
	static boost::optional<lsPosition> toPosition(ILexStream* lex, int offset);

	static std::string getLabelFor(ASTNode *n);
};

class Util
{
public:

	static int INFINITY_,
		OMEGA,
		NIL;

	static int Max(int a, int b) { return (a > b ? a : b); }
	static int Min(int a, int b) { return (a < b ? a : b); }

	static short Abs(short x) { return x < 0 ? -x : x; }
	static int Abs(int x) { return x < 0 ? -x : x; }
	static float Abs(float x) { return x < 0 ? -x : x; }
	static double Abs(double x) { return x < 0 ? -x : x; }

	static void QuickSort(Tuple<int>&, int, int);

	//
	// FILL_IN is a function that pads a buffer, STRING,  with CHARACTER a
	// certain AMOUNT of times.
	//
	static void FillIn(char string[], int amount, char character)
	{
		for (int i = 0; i <= amount; i++)
			string[i] = character;
		string[amount + 1] = '\0';

		return;
	}


	//
	// NUMBER_LEN takes a state number and returns the number of digits in that
	// number.
	//
	static int NumberLength(int state_no)
	{
		int num = 0;

		do
		{
			state_no /= 10;
			num++;
		} while (state_no != 0);

		return num;
	}
};

//
// Convert an integer to its character string representation.
//
class IntToString
{
public:
	IntToString(int num) : value(num)
	{
		if ((unsigned)num == 0x80000000)
		{
			str = info;
			strcpy(str, "-2147483648");
		}
		else
		{
			str = &info[TAIL_INDEX];
			*str = '\0';
			int n = (num < 0 ? -num : num);
			do
			{
				*--str = ('0' + n % 10);
				n /= 10;
			} while (n != 0);

			if (num < 0)
				*--str = '-';
		}
		return;
	}

	int Value() { return value; }
	char* String() { return str; }
	int Length() { return (&info[TAIL_INDEX]) - str; }

private:
	enum { TAIL_INDEX = 1 + 10 }; // 1 for sign, +10 significant digits
	int value;
	char info[TAIL_INDEX + 1], // +1 for '\0'
		* str;
};
enum
{
	SYMBOL_SIZE = 256,
	PRINT_LINE_SIZE = 80
};
