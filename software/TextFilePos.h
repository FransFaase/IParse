#ifndef _INCLUDED_TEXTFILEPOS_H
#define _INCLUDED_TEXTFILEPOS_H

class TextFilePos
{
public:
	TextFilePos() { clear(); }
	void clear()
	{	_pos = (unsigned long)-1;
		_line = 1;
		_column = 1;
	}
	TextFilePos& operator=(const TextFilePos& lhs)
	{	_pos = lhs._pos;
		_line = lhs._line;
		_column = lhs._column;
		return *this;
	}
	inline bool operator==(const TextFilePos& lhs) const { return _pos == lhs._pos; }
	inline bool operator!=(const TextFilePos& lhs) const { return _pos != lhs._pos; }
	inline bool operator<(const TextFilePos& lhs) const { return _pos < lhs._pos; }
	inline bool operator>(const TextFilePos& lhs) const { return _pos > lhs._pos; }
	inline unsigned long position() { return _pos; }
	inline int line() { return _line; }
	inline int column() { return _column; }

protected:
	unsigned long _pos;
	int _line;
	int _column;
};


#endif // _INCLUDED_TEXTFILEPOS_H
