/*	see copyright notice in squirrel.h */
#ifndef _SQSTRING_H_
#define _SQSTRING_H_

inline SQHash _hashstr (const SQChar *s, size_t l)
{
		SQHash h = (SQHash)l;  /* seed */
		size_t step = (l>>5)|1;  /* if string is too long, don't hash all its chars */
		for (; l>=step; l-=step)
			h = h ^ ((h<<5)+(h>>2)+(unsigned short)*(s++));
		return h;
}

struct SQString : public SQRefCounted
{
	SQString(){}
	~SQString(){}
public:
	static SQString *Create(SQSharedState *ss, const SQChar *, SQInteger len = -1,SQBool isconst = SQFalse );
	SQInteger Next(const SQObjectPtr &refpos, SQObjectPtr &outkey, SQObjectPtr &outval);
	void Release();
	static SQString *Alloc(const SQChar *s, SQInteger len, SQBool isconst, SQHash hash);
	static void Free(SQString *s);
#ifdef UNIQUE_STRINGS
#ifndef GLOBAL_STRINGTABLE
	SQSharedState *_sharedstate;
#endif
	SQString *_next; //chain for the string table
#endif
	SQInteger _len;
	SQHash _hash;
	SQChar *_val;
};



#endif //_SQSTRING_H_
