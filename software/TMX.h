#ifndef _INCLUDED_TMX_H
#define _INCLUDED_TMX_H

#include "Ident.h"
#include "AbstractParseTree.h"
#include "Dictionary.h"

class TMX
{
public:
	static void loadFromOmegaT(FILE *fin, Dictionary &dictionary);
	static void saveToOmegaT(FILE *fout, const Dictionary &dictionary, const char* source_file_name);
};


#endif // _INCLUDED_TMX_H
