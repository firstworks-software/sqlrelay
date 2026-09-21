// Copyrifht (c) 2000-2016  David Muse
// See the file COPYING for more information
#ifndef COUNT_BIND_VARIABLES_H
#define COUNT_BIND_VARIABLES_H
#include <rudiments/character.h>

enum queryparsestate_t {
	IN_QUERY=0,
	IN_QUOTES,
	BEFORE_BIND,
	IN_BIND
};

#ifdef NEED_BEFORE_BIND_VARIABLE
static bool beforeBindVariable(const char *c) {
	return character::isInSet(*c," \t\n\r=<>,(+-*/%|&!~^");
}
#endif

#ifdef NEED_IS_BIND_DELIMITER
static bool isBindDelimiter(const char *c,
				bool questionmark,
				bool colon,
				bool atsign,
				bool dollarsign) {
	return (questionmark && *c=='?') ||
		(colon && *c==':' && *(c+1)!='=') ||
		(atsign && *c=='@' && *(c+1)!='@') ||
		(dollarsign && *c=='$');
}
#endif

#if defined(NEED_AFTER_BIND_VARIABLE) || defined(NEED_WHOLE_BIND_VARIABLE)
// the one definition of what can appear inside a bind variable name:
// alphanumerics and '_' everywhere, plus '$' and '#' for oracle identifiers.
static bool isBindVariableNameCharacter(char c) {
	return (character::isAlphanumeric(c) ||
			c=='_' || c=='$' || c=='#');
}
#endif

#ifdef NEED_AFTER_BIND_VARIABLE
// deliberately lenient - this walks a whole query, so it only has to find
// *an* end, and over-consuming into what looks like a name is harmless here.
// a name character never terminates a name, so this can only run long, never
// split one - narrowing it to !isBindVariableNameCharacter() would be a
// real behavior change for its callers, not just a de-duplication.
static bool afterBindVariable(const char *c) {
	if (isBindVariableNameCharacter(*c)) {
		return false;
	}
	return (character::isInSet(*c," \t\n\r,);=<>!") ||
				(*c==':' && *(c+1)=='='));
}
#endif

#ifdef NEED_WHOLE_BIND_VARIABLE
// true if [start,contentend) is entirely a postgres dollar-quoted string:
// $$...$$ or $tag$...$tag$, tag being alphanumeric/'_'.  postgres closes the
// string at the first matching closing delimiter, so this requires that same
// first match to land exactly on "contentend".
static bool isDollarQuotedLiteral(const char *start, const char *contentend) {

	// opening delimiter: '$', an optional tag, then '$'
	const char	*tag=start+1;
	const char	*p=tag;
	while (p<contentend &&
			(character::isAlphanumeric(*p) || *p=='_')) {
		p++;
	}
	if (p>=contentend || *p!='$') {
		return false;
	}
	size_t		taglen=p-tag;
	const char	*bodystart=p+1;

	// find the first closing delimiter - the same tag, between two '$'s
	size_t		delimlen=taglen+2;
	for (const char *q=bodystart;
			(size_t)(contentend-q)>=delimlen; q++) {
		if (*q!='$' || *(q+delimlen-1)!='$') {
			continue;
		}
		bool	tagmatches=true;
		for (size_t i=0; i<taglen; i++) {
			if (*(q+1+i)!=*(tag+i)) {
				tagmatches=false;
				break;
			}
		}
		if (tagmatches) {
			return q+delimlen==contentend;
		}
	}
	return false;
}

// (requires NEED_IS_BIND_DELIMITER)
// returns the start of the bind variable in "var", and its length in "len", if
// "var" is nothing but a single bind variable, optionally surrounded by
// whitespace, or NULL otherwise - a bind inside an expression, "?+1", feeds no
// column on its own.  the result is not NULL-terminated, use it with "len".
// "len" may be NULL if the caller only needs a yes/no answer.
static const char *wholeBindVariable(const char *var,
					bool questionmark,
					bool colon,
					bool atsign,
					bool dollarsign,
					size_t *len) {

	if (!var) {
		return NULL;
	}

	// skip leading whitespace
	const char	*start=var;
	while (character::isWhitespace(*start)) {
		start++;
	}

	if (!isBindDelimiter(start,questionmark,colon,atsign,dollarsign)) {
		return NULL;
	}

	// find the end of the string, then back off any trailing whitespace,
	// to get the bounds of the content: "start" through "contentend"
	const char	*strend=start;
	while (*strend) {
		strend++;
	}
	const char	*contentend=strend;
	while (contentend>start &&
			character::isWhitespace(*(contentend-1))) {
		contentend--;
	}

	// a postgres dollar-quoted literal, e.g. $$abc$$ or $tag$abc$tag$, is
	// alphanumeric-and-'$' start to end, just like a bind name below would
	// take it to be - rule it out first
	if (*start=='$' && isDollarQuotedLiteral(start,contentend)) {
		return NULL;
	}

	// scan the name
	const char	*p=start+1;
	while (isBindVariableNameCharacter(*p)) {
		p++;
	}
	const char	*end=p;

	// skip trailing whitespace
	while (character::isWhitespace(*p)) {
		p++;
	}
	if (*p) {
		return NULL;
	}

	if (len) {
		*len=end-start;
	}
	return start;
}
#endif

#ifdef NEED_COUNT_BIND_VARIABLES
static uint16_t countBindVariables(const char *query,
					uint32_t querylen,
					bool questionmark,
					bool colon,
					bool atsign,
					bool dollarsign,
					bool backslash) {

	if (!query) {
		return 0;
	}

	uint16_t	questionmarkcount=0;
	uint16_t	coloncount=0;
	uint16_t	atsigncount=0;
	uint16_t	dollarsigncount=0;

	queryparsestate_t	parsestate=IN_QUERY;

	const char	*ptr=query;
	const char	*endptr=query+querylen;
	char		prev='\0';
	do {

		// if we're in the query...
		if (parsestate==IN_QUERY) {

			// if we find a quote, we're in quotes
			if (*ptr=='\'') {
				parsestate=IN_QUOTES;
			}

			// if we find whitespace or a couple of other things
			// then the next thing could be a bind variable
			if (beforeBindVariable(ptr)) {
				parsestate=BEFORE_BIND;
			}

			// move on
			if (*ptr=='\\' && prev=='\\') {
				prev='\0';
			} else {
				prev=*ptr;
			}
			ptr++;
			continue;
		}

		// ignore anything in quotes
		if (parsestate==IN_QUOTES) {

			// if we find a quote, but not an escaped quote,
			// then we're back in the query
			// (or we're in between one of these: '...''...'
			// which is functionally the same)
			if (*ptr=='\'' && (!backslash || prev!='\\')) {
				parsestate=IN_QUERY;
			}

			// move on
			if (*ptr=='\\' && prev=='\\') {
				prev='\0';
			} else {
				prev=*ptr;
			}
			ptr++;
			continue;
		}

		if (parsestate==BEFORE_BIND) {

			// if we find a bind variable...
			if (questionmark && isBindDelimiter(
						ptr,true,false,false,false)) {
				questionmarkcount++;
				parsestate=IN_BIND;
				continue;
			} else if (colon && isBindDelimiter(
						ptr,false,true,false,false)) {
				coloncount++;
				parsestate=IN_BIND;
				continue;
			} else if (atsign && isBindDelimiter(
						ptr,false,false,true,false)) {
				atsigncount++;
				parsestate=IN_BIND;
				continue;
			} else if (dollarsign && isBindDelimiter(
						ptr,false,false,false,true)) {
				dollarsigncount++;
				parsestate=IN_BIND;
				continue;
			}

			// if we didn't find a bind variable then we're just
			// back in the query
			parsestate=IN_QUERY;
			continue;
		}

		// if we're in a bind variable...
		if (parsestate==IN_BIND) {

			// If we find whitespace or a few other things
			// then we're done with the bind variable.
			if (afterBindVariable(ptr)) {

				parsestate=IN_QUERY;

			} else {

				// move on
				if (*ptr=='\\' && prev=='\\') {
					prev='\0';
				} else {
					prev=*ptr;
				}
				ptr++;
			}
			continue;
		}

	} while (ptr<endptr);

	// if we got $'s or ?'s, ignore the :'s or @'s
	if (dollarsigncount) {
		return dollarsigncount;
	}
	if (questionmarkcount) {
		return questionmarkcount;
	}
	if (coloncount) {
		return coloncount;
	}
	if (atsigncount) {
		return atsigncount;
	}
	return 0;
}
#endif

#ifdef NEED_SUBSTITUTE_NULL_FOR_BIND_VARIABLES
#include <rudiments/stringbuffer.h>
// (requires NEED_COUNT_BIND_VARIABLES)
static uint16_t substituteNullForBindVariables(const char *query,
						uint32_t querylen,
						bool questionmark,
						bool colon,
						bool atsign,
						bool dollarsign,
						bool backslash,
						stringbuffer *output) {

	if (!query || !querylen || !output) {
		return 0;
	}

	// only one kind of marker gets substituted - the same one
	// countBindVariables() would have counted, found by running the
	// counter for one kind at a time, in the order it prefers them
	bool	q=false;
	bool	c=false;
	bool	a=false;
	bool	d=false;
	if (dollarsign &&
		countBindVariables(query,querylen,
				false,false,false,true,backslash)) {
		d=true;
	} else if (questionmark &&
		countBindVariables(query,querylen,
				true,false,false,false,backslash)) {
		q=true;
	} else if (colon &&
		countBindVariables(query,querylen,
				false,true,false,false,backslash)) {
		c=true;
	} else if (atsign &&
		countBindVariables(query,querylen,
				false,false,true,false,backslash)) {
		a=true;
	} else {
		// nothing to substitute
		output->append(query,querylen);
		return 0;
	}

	uint16_t	count=0;

	queryparsestate_t	parsestate=IN_QUERY;

	const char	*ptr=query;
	const char	*endptr=query+querylen;
	char		prev='\0';
	do {

		// if we're in the query...
		if (parsestate==IN_QUERY) {

			// if we find a quote, we're in quotes
			if (*ptr=='\'') {
				parsestate=IN_QUOTES;
			}

			// if we find whitespace or a couple of other things
			// then the next thing could be a bind variable
			if (beforeBindVariable(ptr)) {
				parsestate=BEFORE_BIND;
			}

			// copy it through and move on
			output->append(*ptr);
			if (*ptr=='\\' && prev=='\\') {
				prev='\0';
			} else {
				prev=*ptr;
			}
			ptr++;
			continue;
		}

		// ignore anything in quotes
		if (parsestate==IN_QUOTES) {

			// if we find a quote, but not an escaped quote,
			// then we're back in the query
			// (or we're in between one of these: '...''...'
			// which is functionally the same)
			if (*ptr=='\'' && (!backslash || prev!='\\')) {
				parsestate=IN_QUERY;
			}

			// copy it through and move on
			output->append(*ptr);
			if (*ptr=='\\' && prev=='\\') {
				prev='\0';
			} else {
				prev=*ptr;
			}
			ptr++;
			continue;
		}

		if (parsestate==BEFORE_BIND) {

			// if we find a bind variable then NULL goes in its
			// place, and the marker itself is skipped below
			if (isBindDelimiter(ptr,q,c,a,d)) {
				output->append("NULL");
				count++;
				parsestate=IN_BIND;
				continue;
			}

			// if we didn't find a bind variable then we're just
			// back in the query
			parsestate=IN_QUERY;
			continue;
		}

		// if we're in a bind variable...
		if (parsestate==IN_BIND) {

			// If we find whitespace or a few other things
			// then we're done with the bind variable.
			if (afterBindVariable(ptr)) {

				parsestate=IN_QUERY;

			} else {

				// skip it and move on
				if (*ptr=='\\' && prev=='\\') {
					prev='\0';
				} else {
					prev=*ptr;
				}
				ptr++;
			}
			continue;
		}

	} while (ptr<endptr);

	return count;
}
#endif

#endif
