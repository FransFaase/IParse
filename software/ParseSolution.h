#ifndef _INCLUDED_PARSESOLUTION_H
#define _INCLUDED_PARSESOLUTION_H


enum EnumSuccess { s_unknown, s_fail, s_success } ;

class ParseSolution
{	
public:
	ParseSolution* next;
	Ident nt;
	enum EnumSuccess success;
	AbstractParseTree result;
	TextFilePos sp;
};

#endif // _INCLUDED_PARSESOLUTION_H
