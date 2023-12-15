// Copyright (c) 1999-2018  David Muse
// All rights reserved

#include <sqlrelay/sqlrserver.h>
#include <rudiments/regularexpression.h>
#include <rudiments/linkedlist.h>
#include <rudiments/error.h>
#include <rudiments/snooze.h>

#include <defines.h>

class SQLRSERVER_DLLSPEC sqlrtrigger_splitmultiinsert : public sqlrtrigger {
	public:
		sqlrtrigger_splitmultiinsert(sqlrservercontroller *cont,
						sqlrtriggers *ts,
						domnode *parameters);

		bool	runBefore(sqlrserverconnection *sqlrcon,
						sqlrservercursor *micur);
	private:
		void	parsePrefix(const char *query,
					const char **ptr,
					stringbuffer *prefix);
		void	parseSuffix(const char *startofvalues,
					const char **ptr,
					stringbuffer *suffix);
		void	parseValues(const char **ptr,
					stringbuffer *values);

		sqlrservercontroller	*cont;

		bool	debug;
};

sqlrtrigger_splitmultiinsert::sqlrtrigger_splitmultiinsert(
					sqlrservercontroller *cont,
					sqlrtriggers *ts,
					domnode *parameters) :
					sqlrtrigger(cont,ts,parameters) {
	this->cont=cont;

	debug=cont->getConfig()->getDebugTriggers();
}

bool sqlrtrigger_splitmultiinsert::runBefore(sqlrserverconnection *sqlrcon,
						sqlrservercursor *micur) {

	// bail if the query was suppressed
	if (cont->getQuerySuppressed(micur)) {
		return true;
	}

	// get the query and query type
	// NOTE: for now determineQueryType() groups simple insert,
	// multi-insert, insert/select and select-into into SQLRQUERYTYPE_INSERT
	const char		*query=cont->getQueryBuffer(micur);
	uint32_t		querylen=cont->getQuerySize(micur);
	sqlrquerytype_t		querytype=micur->getQueryType();
	if (debug) {
		stdoutput.printf("splitmultiinsert {\n");
		stdoutput.printf("	query:\n%.*s\n",querylen,query);
		stdoutput.printf("	query type: %d\n",querytype);
	}

	// bail if the query wasn't an insert
	if (querytype!=SQLRQUERYTYPE_INSERT) {
		if (debug) {
			stdoutput.printf(
				"	query was not an insert\n}\n");
		}
		return true;
	}

	// NOTE: parseInsert will populate querytype with a more specific value
	cont->parseInsert(query,querylen,&querytype,
				NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	if (debug) {
		stdoutput.printf("	query type: %d\n",querytype);
	}

	// bail if the query wasn't a multi-insert
	if (querytype!=SQLRQUERYTYPE_MULTIINSERT) {
		if (debug) {
			stdoutput.printf("	query was not a "
						"multi-insert\n}\n");
		}
		return true;
	}

	// suppress execution of the original query
	cont->setQuerySuppressed(micur,true);

	// buffers that we'll need later
	stringbuffer	prefix;
	stringbuffer	suffix;
	stringbuffer	values;
	stringbuffer	singleinsert;

	// parse out the prefix
	const char	*ptr=NULL;
	parsePrefix(query,&ptr,&prefix);

	// parse out the suffix
	const char	*suffixptr=NULL;
	parseSuffix(ptr,&suffixptr,&suffix);

	// Create an separate cursor to run the single-inserts rather than
	// just using the cursor that ran the original multi-insert.  This
	// preserves the insert cursor in case the client wants to use it to
	// reexecute the multi-insert.
	sqlrservercursor	*sicur=cont->newCursor();
	if (!sicur) {
		cont->setError(micur,"splitmultiinsert failed - "
					"failed to create single-insert cursor",
					SQLR_ERROR_TRIGGER,true);
		return false;
	}

	// open the single-insert cursor
	bool	success=cont->open(sicur);
	if (!success) {
		cont->setError(micur,"splitmultiinsert failed - "
					"failed to open single-insert cursor",
					SQLR_ERROR_TRIGGER,true);
		return false;
	}

	if (debug) {
		stdoutput.printf("	single inserts:\n");
	}

	// split out each insert and run them individually...
	uint64_t	affectedrows=0;
	for (;;) {

		// parse out the values for this insert
		parseValues(&ptr,&values);
		
		// build the query
		singleinsert.append(prefix.getString(),prefix.getSize());
		singleinsert.append(values.getString(),values.getSize());
		singleinsert.append(suffix.getString(),suffix.getSize());
		if (debug) {
			stdoutput.printf("%.*s\n",
					singleinsert.getSize(),
					singleinsert.getString());
		}

		// copy input binds from micur to sicur
		// FIXME: see #7430
		//cont->copyInputBinds(micur,sicur);
		
		// prepare and execute the single-insert query
		success=cont->prepareQuery(sicur,
					singleinsert.getString(),
					singleinsert.getStringLength()) &&
				cont->executeQuery(sicur,true,true,true,true);

		// copy the error from the cursor used to run the
		// single-insert to the cursor used to run the original
		// multi-insert
		if (!success) {
        		const char      *errorstring;
        		uint32_t        errorsize;
        		int64_t         errnum;
        		bool            liveconnection;
        		cont->getError(sicur,&errorstring,
						&errorsize,
                                        	&errnum,
						&liveconnection);
			cont->setError(micur,errorstring,errorsize,
						errnum,liveconnection);
			if (debug) {
				stdoutput.printf("error: %d - %.*s\n",
						errnum,errorsize,errorstring);
			}
			break;
		}

		// tally affected rows
		affectedrows+=cont->getAffectedRows(sicur);

		// clean up
		values.clear();
		singleinsert.clear();

		// continue if there are any more sets of values, otherwise bail
		if (*ptr==',') {
			ptr++;
		} else {
			break;
		}
	}

	// copy affected rows back to micur
	cont->setAffectedRows(micur,affectedrows);

	if (debug) {
		stdoutput.printf("	affected rows: %lld\n",affectedrows);
		stdoutput.printf("}\n");
	}

	// clean up
	cont->closeResultSet(sicur);
	cont->close(sicur);
	cont->deleteCursor(sicur);
	return success;
}

void sqlrtrigger_splitmultiinsert::parsePrefix(const char *query,
						const char **ptr,
						stringbuffer *prefix) {

	// FIXME: assumes normalized query...

	// skip whitespace and comments
	*ptr=cont->skipWhitespaceAndComments(query);

	// skip insert into
	(*ptr)+=12;

	// find first space after table name
	// FIXME: the table name could be quoted and contain a space
	*ptr=charstring::findFirst(*ptr,' ');

	// skip space
	(*ptr)++;

	// skip columns
	if (**ptr=='(') {

		// skip until closing paren
		*ptr=charstring::findFirst(*ptr,')');

		// skip closing paren
		(*ptr)++;

		// skip space after columns
		(*ptr)++;
	}

	// skip values keyword
	// FIXME: the below is kind-of a kludge...
	// sometimes queries are written:
	//	insert into blah values(...);
	// with no space after "values", and the normalize translation
	// doesn't fix this (though it ought to)
	if (!charstring::compare(*ptr,"values(",7)) {
		(*ptr)+=6;
	} else if (!charstring::compare(*ptr,"values (",8)) {
		(*ptr)+=7;
	}

	// *ptr should now be on the open-paren of the first set of values

	// append the prefix to the stringbuffer
	prefix->append(query,*ptr-query);
}

void sqlrtrigger_splitmultiinsert::parseSuffix(const char *startofvalues,
							const char **ptr,
							stringbuffer *suffix) {

	// skip values
	*ptr=startofvalues;
	for (;;) {
		parseValues(ptr,NULL);
		if (**ptr!=',') {
			break;
		}
		(*ptr)++;
	}

	// append whatever is after the values
	suffix->append(*ptr);
}

void sqlrtrigger_splitmultiinsert::parseValues(const char **ptr,
							stringbuffer *values) {

	// we should be on the opening parentheses of a set of values...

	// keep track of this position
	const char	*start=*ptr;

	// skip opening paren
	(*ptr)++;

	// skip to the closing paren, accounting for nested parens and quotes
	uint16_t	depth=0;
	bool		inquotes=false;
	for (;;) {
		if (**ptr=='\'') {
			inquotes=!inquotes;
		} else if (!inquotes) {
			if (**ptr==')') {
				if (!depth) {
					break;
				} else {
					depth--;
				}
			} else if (**ptr=='(') {
				depth++;
			}
		}
		(*ptr)++;
	}

	// we should be on the closing paren...

	// write this set of values to the stringbuffer
	if (values) {
		values->append(start,(*ptr-start)+1);
	}

	// advance to comma or space
	(*ptr)++;
}

extern "C" {
	SQLRSERVER_DLLSPEC
	sqlrtrigger	*new_sqlrtrigger_splitmultiinsert(
						sqlrservercontroller *cont,
						sqlrtriggers *ts,
						domnode *parameters) {

		return new sqlrtrigger_splitmultiinsert(cont,ts,parameters);
	}
}
