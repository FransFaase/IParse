#ifndef _INCLUDED_TEXTFILEBUFFER_H
#define _INCLUDED_TEXTFILEBUFFER_H

#include "TextFilePos.h"

class TextFileBuffer : public TextFilePos
{
public:
	TextFileBuffer();

	void assign(const char* str, unsigned long len, bool utf8encoded = false, unsigned int tab_size = 4);
	void release() { delete[] (char*)_buffer; _buffer = 0; }
	unsigned long length() { return _len; }

	TextFileBuffer& operator=(const TextFileBuffer& lhs)
	{
		assign(lhs._buffer, lhs._len, lhs._utf8encoded, lhs._tab_size);
		return *this;
	}
	TextFileBuffer& operator=(const TextFilePos& lhs)
	{	
		*(TextFilePos*)this = lhs;
		_info = _buffer + _pos;
		return *this;
	}
	
	inline bool eof() { return _pos >= _len; }
	inline unsigned long left() { return _len - _pos; }
	inline char operator*() { return *_info; }
	inline char operator[](int i) { return _info[i]; }
	inline operator const char*() { return _info; }
	void next();
	void advance(unsigned int steps);

	void print_state();
	const char* start();
	
	void setTabSize(int tab_size) { _tab_size = tab_size; }

private:
	const char *_buffer;
	const char *_info;
	unsigned long _len;
	bool _utf8encoded;
	unsigned int _tab_size;
};

#endif // _INCLUDED_TEXTFILEBUFFER_H
