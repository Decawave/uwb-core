#ifndef _CTYPE_H_
#define _CTYPE_H_

#define isspace(c)	((c) == ' ')
static inline int isdigit(int c)
{
	return '0' <= c && c <= '9';
}

#endif
