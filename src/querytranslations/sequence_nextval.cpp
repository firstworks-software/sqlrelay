// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/stringbuffer.h>
#include <rudiments/character.h>
#include <rudiments/charstring.h>
//#define DEBUG_MESSAGES 1
#include <rudiments/debugprint.h>

enum style_t {
	STYLE_DOT_NEXTVAL=0,
	STYLE_NEXTVAL_PAREN,
	STYLE_NEXT_VALUE_FOR
};

static const char	nextvalparenmark[]="nextval(";
static const char	nextvalueformark[]="next value for ";

class SQLRSERVER_DLLSPEC sqlrquerytranslation_sequence_nextval :
					public sqlrquerytranslation {
	public:
		sqlrquerytranslation_sequence_nextval(
					sqlrservercontroller *cont,
					domnode *parameters);
		bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					const char *query,
					uint32_t querylength,
					stringbuffer *translatedquery);
	private:
		bool		isIdentChar(char c);
		const char	*matchDotNextval(const char *ptr,
						const char *start,
						const char *end,
						const char **namestart,
						const char **nameend);
		const char	*matchNextvalParen(const char *ptr,
						const char *start,
						const char *end,
						const char **namestart,
						const char **nameend);
		const char	*matchNextValueFor(const char *ptr,
						const char *start,
						const char *end,
						const char **namestart,
						const char **nameend);
		const char	*scanDottedPath(const char *ptr,
							const char *end,
							const char **lastcomp);
		void		emit(const char *namestart,
						const char *nameend,
						stringbuffer *out);

		bool	translate;
		style_t	style;
};

sqlrquerytranslation_sequence_nextval::
		sqlrquerytranslation_sequence_nextval(
				sqlrservercontroller *cont,
				domnode *parameters) :
				sqlrquerytranslation(cont,parameters) {
	debugFunction();

	// an unset or unrecognized style passes queries through untranslated
	translate=true;
	const char	*s=parameters->getAttributeValue("style");
	if (!charstring::compareIgnoringCase(s,".nextval")) {
		style=STYLE_DOT_NEXTVAL;
	} else if (!charstring::compareIgnoringCase(s,"nextval()")) {
		style=STYLE_NEXTVAL_PAREN;
	} else if (!charstring::compareIgnoringCase(s,"next value for")) {
		style=STYLE_NEXT_VALUE_FOR;
	} else {
		translate=false;
		style=STYLE_DOT_NEXTVAL;
	}
}

bool sqlrquerytranslation_sequence_nextval::isIdentChar(char c) {
	return character::isAlphanumeric(c) || c=='_';
}

const char *sqlrquerytranslation_sequence_nextval::scanDottedPath(
						const char *ptr,
						const char *end,
						const char **lastcomp) {

	// scan a dotted object path
	// (single dots; the query is assumed already normalized)
	const char	*p=ptr;
	const char	*lc=ptr;
	for (;;) {
		lc=p;
		const char	*compstart=p;
		while (p<end && isIdentChar(*p)) {
			p++;
		}
		if (p==compstart) {
			return NULL;
		}
		if (p<end && *p=='.' && p+1<end && isIdentChar(*(p+1))) {
			p++;
			continue;
		}
		break;
	}
	*lastcomp=lc;
	return p;
}

bool sqlrquerytranslation_sequence_nextval::run(
				sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				const char *query,
				uint32_t querylength,
				stringbuffer *translatedquery) {
	debugFunction();

	if (getDebug()) {
		debugWrite("original query:");
		stringbuffer	b;
		b.safePrint(query,querylength);
		debugWrite("\"%s\"",b.getString());
	}

	// pass through untranslated when no valid target style was configured
	if (!translate) {
		translatedquery->append(query,querylength);
		return true;
	}

	// walk the query, translating any sequence-nextval expressions
	const char	*start=query;
	const char	*end=query+querylength;
	const char	*ptr=query;
	while (ptr<end) {

		// copy out string literals verbatim
		if (*ptr=='\'') {
			ptr=cont->copyStringLiteral(ptr,end,translatedquery,true);
			continue;
		}

		// try each input style
		const char	*namestart;
		const char	*nameend;
		const char	*after=matchDotNextval(ptr,start,end,
							&namestart,&nameend);
		if (!after) {
			after=matchNextvalParen(ptr,start,end,
							&namestart,&nameend);
		}
		if (!after) {
			after=matchNextValueFor(ptr,start,end,
							&namestart,&nameend);
		}
		if (after) {
			emit(namestart,nameend,translatedquery);
			ptr=after;
			continue;
		}

		// otherwise copy out the current character
		translatedquery->append(*ptr);
		ptr++;
	}

	if (getDebug()) {
		debugWrite("translated query:");
		stringbuffer	b;
		b.safePrint(translatedquery->getString(),
					translatedquery->getSize());
		debugWrite("\"%s\"",b.getString());
	}

	return true;
}

const char *sqlrquerytranslation_sequence_nextval::matchDotNextval(
						const char *ptr,
						const char *start,
						const char *end,
						const char **namestart,
						const char **nameend) {

	// "ptr" must point at the start of an identifier
	if (ptr>start && isIdentChar(*(ptr-1))) {
		return NULL;
	}

	// scan the full dotted object path, eg. "myschema.seq.nextval"
	const char	*lastcomp;
	const char	*p=scanDottedPath(ptr,end,&lastcomp);
	if (!p) {
		return NULL;
	}

	// the final component must be exactly "nextval"
	// (so eg. ".nextvalue" doesn't match)
	if (lastcomp==ptr ||
		(size_t)(p-lastcomp)!=7 ||
		charstring::compareIgnoringCase(lastcomp,"nextval",7)) {
		return NULL;
	}

	// the sequence name is everything before the trailing ".nextval"
	*namestart=ptr;
	*nameend=lastcomp-1;
	return p;
}

const char *sqlrquerytranslation_sequence_nextval::matchNextvalParen(
						const char *ptr,
						const char *start,
						const char *end,
						const char **namestart,
						const char **nameend) {

	// require "nextval(" as its own token
	const size_t	marklen=sizeof(nextvalparenmark)-1;
	if (ptr>start && isIdentChar(*(ptr-1))) {
		return NULL;
	}
	if (ptr+marklen>end ||
		charstring::compareIgnoringCase(ptr,nextvalparenmark,marklen)) {
		return NULL;
	}
	const char	*p=ptr+marklen;

	// the argument is the sequence name (a dotted object path),
	// optionally single-quoted
	const char	*ns;
	const char	*ne;
	if (p<end && *p=='\'') {
		ns=p+1;
		p++;
		while (p<end && *p!='\'') {
			p++;
		}
		if (p>=end) {
			return NULL;
		}
		ne=p;
		p++;
	} else {
		const char	*lastcomp;
		ns=p;
		p=scanDottedPath(p,end,&lastcomp);
		if (!p) {
			return NULL;
		}
		ne=p;
	}
	if (ne==ns) {
		return NULL;
	}

	// require the closing ")"
	if (p>=end || *p!=')') {
		return NULL;
	}

	*namestart=ns;
	*nameend=ne;
	return p+1;
}

const char *sqlrquerytranslation_sequence_nextval::matchNextValueFor(
						const char *ptr,
						const char *start,
						const char *end,
						const char **namestart,
						const char **nameend) {

	// require "next value for " as its own token
	// (single spaces; the query is assumed already normalized)
	const size_t	marklen=sizeof(nextvalueformark)-1;
	if (ptr>start && isIdentChar(*(ptr-1))) {
		return NULL;
	}
	if (ptr+marklen>end ||
		charstring::compareIgnoringCase(ptr,nextvalueformark,marklen)) {
		return NULL;
	}
	const char	*p=ptr+marklen;

	// the sequence name (a dotted object path) follows the phrase
	const char	*lastcomp;
	const char	*ns=p;
	p=scanDottedPath(p,end,&lastcomp);
	if (!p) {
		return NULL;
	}
	const char	*ne=p;

	*namestart=ns;
	*nameend=ne;
	return p;
}

void sqlrquerytranslation_sequence_nextval::emit(const char *namestart,
						const char *nameend,
						stringbuffer *out) {
	switch (style) {
		case STYLE_NEXTVAL_PAREN:
			out->append("nextval('");
			out->append(namestart,nameend-namestart);
			out->append("')");
			break;
		case STYLE_NEXT_VALUE_FOR:
			out->append("next value for ");
			out->append(namestart,nameend-namestart);
			break;
		case STYLE_DOT_NEXTVAL:
		default:
			out->append(namestart,nameend-namestart);
			out->append(".nextval");
			break;
	}
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrquerytranslation
		*new_sqlrquerytranslation_sequence_nextval(
					sqlrservercontroller *cont,
					domnode *parameters) {
		return new sqlrquerytranslation_sequence_nextval(
					cont,parameters);
	}
}
