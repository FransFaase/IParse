#include <stdio.h>
#include <io.h>

#include "TextReader.h"

void PlainFileReader::read(FILE *f, TextFileBuffer &textBuffer)
{
	int fh = fileno(f);
	unsigned long len = lseek(fh, 0L, SEEK_END);
	lseek(fh, 0L, SEEK_SET);
	char *str = new char[len+1];
	len = ::read(fh, str, len);
	str[len] = '\0';
	textBuffer.assign(str, len, /* utf8encoded */false);
}

void ConverterFileReader::read(FILE *f, TextFileBuffer &textBuffer)
{
	TextFileBuffer plainTextBuffer;
	PlainFileReader plainFileReader;
	plainFileReader.read(f, plainTextBuffer);

	StreamCounter<char> streamCounter;
	_converter.setOutputStream(&streamCounter);
	TextFilePos startPos = plainTextBuffer;
	for (; !plainTextBuffer.eof(); plainTextBuffer.next())
		_converter.emit(*plainTextBuffer);
	unsigned long len = streamCounter.count();

	char *str = new char[len+1];

	StreamToString<char> streamToString(str);
	_converter.setOutputStream(&streamToString);
	plainTextBuffer = startPos;
	for (; !plainTextBuffer.eof(); plainTextBuffer.next())
		_converter.emit(*plainTextBuffer);
	streamToString.emit('\0');

	textBuffer.assign(str, len, _converter.isUTF8Encoded());

	plainTextBuffer.release();
}

