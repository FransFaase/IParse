#ifndef INCLUDED_IDENT_H
#define INCLUDED_IDENT_H

class Ident
{
public:
	Ident()
	{	_id = _empty;
	}
	Ident(const char *id)
	{	_id = ident_unify(id);
	}
	
	Ident(const Ident &ident)
	{	_id = ident._id;
	}
	inline Ident &operator=(const Ident &rhs)
	{	_id = rhs._id;
		return *this;
	}
	inline Ident &operator=(const char *rhs)
	{	_id = ident_unify(rhs);
		return *this;
	}
	inline bool operator==(const char *rhs) const
	{	return strcmp(_id, rhs) == 0;
	}
	inline bool operator==(const Ident &rhs) const
	{	return _id == rhs._id;
	}
	inline bool operator!=(const char *rhs) const
	{	return strcmp(_id, rhs) != 0;
	}
	inline bool operator!=(const Ident &rhs) const
	{	return _id != rhs._id;
	}
	inline bool operator>=(const char *rhs) const
	{	return _id >= ident_unify(rhs);
	}
	inline bool operator>=(const Ident &rhs) const
	{	return _id >= rhs._id;
	}
	inline int cmp(const Ident &lhs, const Ident &rhs) const
	{	return (lhs._id == rhs._id) ? 0 : strcmp(lhs._id, rhs._id);
	}
	inline const char *val() const
	{	return _id;
	}
	inline bool empty() const
	{	return _id == _empty;
	}

private:
	const char *_id;
	static char *_empty;

	const char* ident_unify(const char* id) const;
};

#endif // INCLUDED_IDENT_H
