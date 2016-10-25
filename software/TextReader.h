#ifndef _INCLUDED_TEXTREADER_H
#define _INCLUDED_TEXTREADER_H

#include "Streams.h"
#include "TextFileBuffer.h"

class AbstractFileReader
{
public:
	virtual void read(FILE *f, TextFileBuffer &textBuffer) = 0;
};

class PlainFileReader : public AbstractFileReader
{
public:
	virtual void read(FILE *f, TextFileBuffer &textBuffer);
};

class ConverterFileReader : public AbstractFileReader
{
public:
	ConverterFileReader(CharToCharConverterStream &converter) : _converter(converter) {}
	virtual void read(FILE *f, TextFileBuffer &textBuffer);
private:
	CharToCharConverterStream &_converter;
};

#endif // _INCLUDED_TEXTREADER_H
