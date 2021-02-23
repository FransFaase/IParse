#ifndef _INCLUDED_TEXTFILEPOS_H
#define _INCLUDED_TEXTFILEPOS_H

class TextFilePos
{
public:
	TextFilePos() { clear(); }
	void clear()
	{	_pos = (size_t)-1;
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
	inline size_t position() { return _pos; }
	inline int line() { return _line; }
	inline int column() { return _column; }

protected:
	size_t _pos;
	int _line;
	int _column;
};


#endif // _INCLUDED_TEXTFILEPOS_H
