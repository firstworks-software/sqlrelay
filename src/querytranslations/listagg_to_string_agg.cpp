// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/stringbuffer.h>
#include <rudiments/character.h>
#include <rudiments/charstring.h>
//#define DEBUG_MESSAGES 1
#include <rudiments/debugprint.h>

class SQLRSERVER_DLLSPEC sqlrquerytranslation_listagg_to_string_agg :
					public sqlrquerytranslation {
	public:
		sqlrquerytranslation_listagg_to_string_agg(
					sqlrservercontroller *cont,
					domnode *parameters);
		bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					const char *query,
					uint32_t querylength,
					stringbuffer *translatedquery);
	private:
		void		translateRange(const char *ptr,
						const char *end,
						stringbuffer *out);
		const char	*translateListagg(const char *ptr,
						const char *end,
						stringbuffer *out);
};

sqlrquerytranslation_listagg_to_string_agg::
		sqlrquerytranslation_listagg_to_string_agg(
				sqlrservercontroller *cont,
				domnode *parameters) :
				sqlrquerytranslation(cont,parameters) {
	debugFunction();
}

static bool isIdentChar(char c) {
	return character::isAlphanumeric(c) || c=='_';
}

bool sqlrquerytranslation_listagg_to_string_agg::run(
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

void sqlrquerytranslation_listagg_to_string_agg::translateRange(
					const char *ptr,
					const char *end,
					stringbuffer *out) {

	// walk [ptr, end), copying to "out",
	// translating any listagg() calls we encounter
	const char	*start=ptr;
	while (ptr<end) {

		// copy out string literals verbatim
		if (*ptr=='\'') {
			ptr=cont->copyStringLiteral(ptr,end,out);
			continue;
		}

		// match "listagg(" (skipping, eg. "mylistagg(...)")
		if ((ptr==start || !isIdentChar(*(ptr-1))) &&
				ptr+8<=end &&
				!charstring::compare(ptr,"listagg(",8)) {

			// translate the listagg() call
			const char	*after=translateListagg(ptr,end,out);
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

const char *sqlrquerytranslation_listagg_to_string_agg::translateListagg(
							const char *ptr,
							const char *end,
							stringbuffer *out) {

	// "ptr" points to "listagg(", step past it
	const char	*exprstart=ptr+8;

	// find the comma that separates EXPR from SEP
	const char	*exprend=cont->findCommaOrCloseParen(exprstart,end);
	if (!exprend || *exprend!=',') {
		return NULL;
	}
	const char	*separatorstart=exprend+1;

	// find the matching ")" of the listagg(...) call
	const char	*separatorend=cont->findCommaOrCloseParen(
							separatorstart,end);
	if (!separatorend || *separatorend!=')') {
		return NULL;
	}
	const char	*p=separatorend+1;

	// require " within group (" immediately after the closing ")"
	static const char	withinmark[]=" within group (";
	static const size_t	withinmarklen=sizeof(withinmark)-1;
	if (p+withinmarklen>end ||
		charstring::compare(p,withinmark,withinmarklen)) {
		return NULL;
	}
	p+=withinmarklen;

	// scan for the matching ")" of the "within group" clause,
	// stepping past any top-level commas in the "order by" list
	const char	*ogstart=p;
	const char	*ogend=ogstart;
	for (;;) {
		ogend=cont->findCommaOrCloseParen(ogend,end);
		if (!ogend) {
			return NULL;
		}
		if (*ogend==')') {
			break;
		}
		ogend++;
	}
	p=ogend+1;

	// bail if the "within group" contents don't begin with "order by "
	static const char	orderbymark[]="order by ";
	static const size_t	orderbymarklen=sizeof(orderbymark)-1;
	if ((size_t)(ogend-ogstart)<orderbymarklen ||
		charstring::compare(ogstart,orderbymark,orderbymarklen)) {
		return NULL;
	}

	// write out string_agg(cast(EXPR as text),SEP order by SORT)
	out->append("string_agg(cast(");
	translateRange(exprstart,exprend,out);
	out->append(" as text),");
	out->append(separatorstart,separatorend-separatorstart);
	out->append(' ');
	out->append(ogstart,orderbymarklen);
	translateRange(ogstart+orderbymarklen,ogend,out);
	out->append(')');

	return p;
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrquerytranslation
		*new_sqlrquerytranslation_listagg_to_string_agg(
					sqlrservercontroller *cont,
					domnode *parameters) {
		return new sqlrquerytranslation_listagg_to_string_agg(
					cont,parameters);
	}
}
