// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/stringbuffer.h>
#include <rudiments/character.h>
#include <rudiments/charstring.h>
//#define DEBUG_MESSAGES 1
#include <rudiments/debugprint.h>

static const char	keepmark[]="keep";
static const char	denserankmark[]="dense_rank ";
static const char	firstmark[]="first ";
static const char	lastmark[]="last ";
static const char	orderbymark[]="order by ";
static const char	ascmark[]=" asc";
static const char	descmark[]=" desc";
static const char	nullsfirstmark[]=" nulls first";
static const char	nullslastmark[]=" nulls last";
static const char	overmark[]="over";

class SQLRSERVER_DLLSPEC sqlrquerytranslation_keep_dense_rank_to_array_agg :
					public sqlrquerytranslation {
	public:
		sqlrquerytranslation_keep_dense_rank_to_array_agg(
					sqlrservercontroller *cont,
					domnode *parameters);
		bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					const char *query,
					uint32_t querylength,
					stringbuffer *translatedquery);
	private:
		bool		isIdentChar(char c);
		void		translateRange(const char *ptr,
						const char *end,
						stringbuffer *out);
		const char	*translateKeep(const char *ptr,
						const char *end,
						stringbuffer *out);
};

// normalize collapses runs of whitespace to a single space, but leaves the
// space before "(" and after ")" however the original query had it, so at
// those joints a space may or may not be there at all
static const char *skipOptionalSpace(const char *ptr, const char *end) {
	return (ptr<end && *ptr==' ')?ptr+1:ptr;
}

static bool endsWith(const char *start, const char *end,
					const char *mark, size_t marklen) {
	return (size_t)(end-start)>marklen &&
		!charstring::compare(end-marklen,mark,marklen);
}

sqlrquerytranslation_keep_dense_rank_to_array_agg::
		sqlrquerytranslation_keep_dense_rank_to_array_agg(
				sqlrservercontroller *cont,
				domnode *parameters) :
				sqlrquerytranslation(cont,parameters) {
	debugFunction();
}

bool sqlrquerytranslation_keep_dense_rank_to_array_agg::isIdentChar(char c) {
	return character::isAlphanumeric(c) || c=='_';
}

bool sqlrquerytranslation_keep_dense_rank_to_array_agg::run(
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

	translateRange(query,query+querylength,translatedquery);

	if (getDebug()) {
		debugWrite("translated query:");
		stringbuffer	b;
		b.safePrint(translatedquery->getString(),
					translatedquery->getSize());
		debugWrite("\"%s\"",b.getString());
	}

	return true;
}

void sqlrquerytranslation_keep_dense_rank_to_array_agg::translateRange(
					const char *ptr,
					const char *end,
					stringbuffer *out) {

	// walk [ptr, end), translating any max/min ... keep (dense_rank ...)
	const char	*start=ptr;
	while (ptr<end) {

		// copy out string literals verbatim
		if (*ptr=='\'') {
			ptr=cont->copyStringLiteral(ptr,end,out,true);
			continue;
		}

		// match "max" or "min" (skipping, eg. "mymax(...)")
		if ((ptr==start || !isIdentChar(*(ptr-1))) &&
				ptr+3<=end &&
				(!charstring::compare(ptr,"max",3) ||
					!charstring::compare(ptr,"min",3))) {

			// translate the keep (dense_rank ...) construct
			const char	*after=translateKeep(ptr,end,out);
			if (after) {
				ptr=after;
				continue;
			}
		}

		// otherwise copy out the current character
		out->append(*ptr);
		ptr++;
	}
}

const char *sqlrquerytranslation_keep_dense_rank_to_array_agg::translateKeep(
							const char *ptr,
							const char *end,
							stringbuffer *out) {

	// "ptr" points to "max" or "min" - normalize leaves the space before an
	// aggregate's own "(" as the original query had it, so allow one here
	// the same way the "keep(" and "keep("'s own "(" joints do below
	bool		ismax=!charstring::compare(ptr,"max",3);
	const char	*exprstart=skipOptionalSpace(ptr+3,end);
	if (exprstart>=end || *exprstart!='(') {
		return NULL;
	}
	exprstart++;

	// find the end of EXPR, bailing on a multi-argument call
	const char	*exprend=cont->findCommaOrCloseParen(exprstart,end,true);
	if (!exprend || *exprend!=')' || exprend==exprstart) {
		return NULL;
	}
	const char	*p=exprend+1;

	// require "keep(" after the closing ")"
	p=skipOptionalSpace(p,end);
	const size_t	keepmarklen=sizeof(keepmark)-1;
	if (p+keepmarklen>end || charstring::compare(p,keepmark,keepmarklen)) {
		return NULL;
	}
	p=skipOptionalSpace(p+keepmarklen,end);
	if (p>=end || *p!='(') {
		return NULL;
	}
	p++;

	// require "dense_rank "
	const size_t	denserankmarklen=sizeof(denserankmark)-1;
	if (p+denserankmarklen>end ||
		charstring::compare(p,denserankmark,denserankmarklen)) {
		return NULL;
	}
	p+=denserankmarklen;

	// require "first " or "last "
	const size_t	firstmarklen=sizeof(firstmark)-1;
	const size_t	lastmarklen=sizeof(lastmark)-1;
	bool		isfirst;
	if (p+firstmarklen<=end &&
		!charstring::compare(p,firstmark,firstmarklen)) {
		isfirst=true;
		p+=firstmarklen;
	} else if (p+lastmarklen<=end &&
			!charstring::compare(p,lastmark,lastmarklen)) {
		isfirst=false;
		p+=lastmarklen;
	} else {
		return NULL;
	}

	// require "order by "
	const size_t	orderbymarklen=sizeof(orderbymark)-1;
	if (p+orderbymarklen>end ||
		charstring::compare(p,orderbymark,orderbymarklen)) {
		return NULL;
	}
	p+=orderbymarklen;

	// scan for the matching ")" of the keep clause, bailing on a top-level
	// comma - only a single sort key is supported
	const char	*sortstart=p;
	const char	*keepend=cont->findCommaOrCloseParen(sortstart,end,true);
	if (!keepend || *keepend!=')') {
		return NULL;
	}
	const char	*after=keepend+1;

	// bail on an analytic keep, it can't be expressed this way
	const char	*o=skipOptionalSpace(after,end);
	const size_t	overmarklen=sizeof(overmark)-1;
	if (o+overmarklen<=end &&
		!charstring::compare(o,overmark,overmarklen)) {
		o=skipOptionalSpace(o+overmarklen,end);
		if (o<end && *o=='(') {
			return NULL;
		}
	}

	// pull an optional "nulls first"/"nulls last" off the end of SORTEXPR
	const char	*sortend=keepend;
	bool		nullsfirst=false;
	bool		nullswritten=false;
	if (endsWith(sortstart,sortend,
			nullsfirstmark,sizeof(nullsfirstmark)-1)) {
		nullsfirst=true;
		nullswritten=true;
		sortend-=sizeof(nullsfirstmark)-1;
	} else if (endsWith(sortstart,sortend,
			nullslastmark,sizeof(nullslastmark)-1)) {
		nullswritten=true;
		sortend-=sizeof(nullslastmark)-1;
	}

	// pull an optional "asc"/"desc" off the end of SORTEXPR
	bool		desc=false;
	if (endsWith(sortstart,sortend,descmark,sizeof(descmark)-1)) {
		desc=true;
		sortend-=sizeof(descmark)-1;
	} else if (endsWith(sortstart,sortend,ascmark,sizeof(ascmark)-1)) {
		sortend-=sizeof(ascmark)-1;
	}

	// an unwritten nulls placement follows the direction
	if (!nullswritten) {
		nullsfirst=desc;
	}

	// Oracle's "dense_rank last" takes the row at the far end of the sort,
	// which array_agg()[1] can only reach by reversing the sort - and the
	// reversal has to flip the nulls placement too.  Flipping only the
	// direction leaves nulls at the same end, silently returning a null
	// instead of the last non-null value.
	bool		primarydesc=desc;
	bool		primarynullsfirst=nullsfirst;
	if (!isfirst) {
		primarydesc=!desc;
		primarynullsfirst=!nullsfirst;
	}

	// write out (array_agg(EXPR order by SORTEXPR DIR, EXPR DIR))[1]
	//
	// EXPR appears twice, so a volatile EXPR (eg. a random() call) would
	// be evaluated twice here versus once by Oracle's keep() - not a
	// concern for a plain column reference, which is the only form seen
	// in practice
	out->append("(array_agg(");
	translateRange(exprstart,exprend,out);
	out->append(" order by ");
	translateRange(sortstart,sortend,out);
	out->append((primarydesc)?" desc":" asc");
	out->append((primarynullsfirst)?" nulls first":" nulls last");
	out->append(", ");
	translateRange(exprstart,exprend,out);
	out->append((ismax)?" desc nulls last":" asc nulls last");
	out->append("))[1]");

	return after;
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrquerytranslation
		*new_sqlrquerytranslation_keep_dense_rank_to_array_agg(
					sqlrservercontroller *cont,
					domnode *parameters) {
		return new sqlrquerytranslation_keep_dense_rank_to_array_agg(
					cont,parameters);
	}
}
