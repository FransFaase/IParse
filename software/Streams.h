#ifndef _INCLUDED_STREAMS_H
#define _INCLUDED_STREAMS_H

#include "CodePages.h"

template<class T>
class AbstractStream
{
public:
	virtual void emit(const T symbol) = 0;
};

template<class T>
class NullStream : public AbstractStream<T>
{
public:
	virtual void emit(const T symbol) {}
};

template<class T>
class StreamCounter : public AbstractStream<T>
{
public:
	StreamCounter() : _count(0) {}
	virtual void emit(const T symbol) { if (symbol != '\r') _count++; }
	unsigned long count() { return _count; }
private:
	unsigned long _count;
};

template<class T>
class StreamToString : public AbstractStream<T>
{
public:
	StreamToString(T* str) : _str(str) {}
	virtual void emit(const T symbol) { if (symbol != '\r') *_str++ = symbol; }
private:
	T* _str;
};

class CharStreamToFile : public AbstractStream<char>
{
public:
	CharStreamToFile(FILE *f, bool text) : _f(f), _text(text) {}
	virtual void emit(const char symbol);
private:
	FILE *_f;
	bool _text;
};

template<class T, class S>
class ConverterStream : public AbstractStream<T>
{
public:
	ConverterStream() { _out = &_null_stream; }
	virtual void setOutputStream(AbstractStream<S> *out) { _out = out; }
protected:
	AbstractStream<S> *_out;
private:
	NullStream<S> _null_stream;
};

#ifndef __GNUC__ // For some reason the g++ (3.4.4) does not recognize _out in emit method
template<class T>
class LineColTrackerStream : public ConverterStream<T, T>
{
public:
	LineColTrackerStream(T newline) : _newline(newline), _line(0), _col(0) {}
	virtual void setOutputStream(AbstractStream<T> *out)
	{ 
		ConverterStream<T,T>::setOutputStream(out);
		_line = 1;
		_col = 1;
	}
	virtual void emit(const T symbol)
	{
		_out->emit(symbol);
		if (symbol == _newline)
		{
			_line++;
			_col = 1;
		}
		else
			_col++;

	}

	int line() { return _line; };
	int col() { return _col; }
private:
	T _newline;
	int _line;
	int _col;
};
#else // Less orthogonal version for g++
template<class T>
class LineColTrackerStream : public AbstractStream<T>
{
public:
	LineColTrackerStream(T newline) : _newline(newline), _line(0), _col(0) { _out = &_null_stream; }
	virtual void setOutputStream(AbstractStream<T> *out)
	{ 
		_out = out;
		_line = 1;
		_col = 1;
	}
	virtual void emit(const T symbol)
	{
		_out->emit(symbol);
		if (symbol == _newline)
		{
			_line++;
			_col = 1;
		}
		else
			_col++;

	}

	int line() { return _line; };
	int col() { return _col; }
private:
	AbstractStream<T> *_out;
	NullStream<T> _null_stream;
	T _newline;
	int _line;
	int _col;
};
#endif

class UTF8LineColTrackerStream : public ConverterStream<char, char>
{
public:
	UTF8LineColTrackerStream(bool text) : _text(text), _line(0), _col(0) {}
	virtual void setOutputStream(AbstractStream<char> *out)
	{ 
		ConverterStream<char,char>::setOutputStream(out);
		_line = 1;
		_col = 1;
	}
	virtual void emit(const char symbol)
	{
		if (_text && symbol == '\n')
		{	
			_out->emit('\r'); 
			_out->emit('\n');
		}
		else if (_text && symbol == '\r')
		{	
			_out->emit('\\'); 
			_out->emit('r');
		}
		else 
			_out->emit(symbol);
		if (symbol == '\n')
		{
			_line++;
			_col = 1;
		}
		else if ((symbol & 0xC0) != 0x80)
			_col++;
	}

	int line() { return _line; };
	int col() { return _col; }
private:
	bool _text;
	int _line;
	int _col;
};

class CodePageToUnicodeConverterStream : public ConverterStream<char, UnicodeChar>
{
public:
	CodePageToUnicodeConverterStream(AbstractCodePage &codePage) : _codePage(codePage) {}
	virtual void emit(const char symbol);
private:
	AbstractCodePage &_codePage;
};

class UnicodeToUTF8ConverterStream : public ConverterStream<UnicodeChar, char>
{
public:
	virtual void emit(const UnicodeChar symbol);
};

class CharToCharConverterStream : public ConverterStream<char, char>
{
public:
	virtual bool isUTF8Encoded() { return false; }
};
class ToUTF8ConverterStream : public CharToCharConverterStream
{
public:
	virtual bool isUTF8Encoded() { return true; }
	virtual void setOutputStream(AbstractStream<char> *out)
	{ 
		_unicodeToUTF8ConverterStream.setOutputStream(out);
	}
protected:
	UnicodeToUTF8ConverterStream _unicodeToUTF8ConverterStream;
};

class CodePageToUF8ConverterStream : public ToUTF8ConverterStream
{
public:
	CodePageToUF8ConverterStream(AbstractCodePage &codePage)
	 : _codePageToUnicodeConverterStream(codePage)
	{
		_codePageToUnicodeConverterStream.setOutputStream(&_unicodeToUTF8ConverterStream);
	}
	virtual void emit(const char symbol) { _codePageToUnicodeConverterStream.emit(symbol); }
private:
	CodePageToUnicodeConverterStream _codePageToUnicodeConverterStream;
};
		
class UTF16ToUTF8ConverterStream : public ToUTF8ConverterStream
{
public:
	UTF16ToUTF8ConverterStream()
	 : _count(0)
	{
	}
	virtual void setOutputStream(AbstractStream<char> *out)
	{ 
		ToUTF8ConverterStream::setOutputStream(out);
		_count = 0;
	}
	virtual void emit(const char symbol);
private:
	long _count;
	char _prev;
};

class UTF8ToUnicodeConverterStream : public ConverterStream<char, UnicodeChar>
{
public:
	UTF8ToUnicodeConverterStream() : _following(0) {}
	virtual void emit(const char symbol);
private:
	int _following;
	UnicodeChar _code_point;
};

class UnicodeToCodePageConverterStream : public ConverterStream<UnicodeChar, char>
{
public:
	UnicodeToCodePageConverterStream(AbstractCodePage &codePage) : _codePage(codePage) {}
	virtual void emit(const UnicodeChar symbol);
private:
	AbstractCodePage &_codePage;
};

class UTF8ToCodePageConverterStream : public ConverterStream<char, char>
{
public:
	UTF8ToCodePageConverterStream(AbstractCodePage &codePage)
	 : _unicodeToCodePageConverterStream(codePage)
	{
		_utf8ToUnicodeConverterStream.setOutputStream(&_unicodeToCodePageConverterStream);
	}
	virtual void setOutputStream(AbstractStream<char>* out) { _unicodeToCodePageConverterStream.setOutputStream(out); }
	virtual void emit(const char symbol) { _utf8ToUnicodeConverterStream.emit(symbol); }
private:
	UTF8ToUnicodeConverterStream _utf8ToUnicodeConverterStream;
	UnicodeToCodePageConverterStream _unicodeToCodePageConverterStream;
};

class UnicodeToUTF16ConverterStream : public ConverterStream<UnicodeChar, char>
{
public:
	virtual void emit(const UnicodeChar symbol);
};

class UTF8ToUTF16ConverterStream : public ConverterStream<char, char>
{
public:
	UTF8ToUTF16ConverterStream()
	{
		_utf8ToUnicodeConverterStream.setOutputStream(&_unicodeToUTF16ConverterStream);
	}
	virtual void setOutputStream(AbstractStream<char>* out)
	{ 
		_unicodeToUTF16ConverterStream.setOutputStream(out);
		// Emit BOM character
		_unicodeToUTF16ConverterStream.emit(0xFEFF);
	}
	virtual void emit(const char symbol) { _utf8ToUnicodeConverterStream.emit(symbol); }
private:
	UTF8ToUnicodeConverterStream _utf8ToUnicodeConverterStream;
	UnicodeToUTF16ConverterStream _unicodeToUTF16ConverterStream;
};


#endif // _INCLUDED_STREAMS_H
