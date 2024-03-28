// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

class sqlrdirectiveprivate {
	friend class sqlrdirective;
	private:
		domnode	*_parameters;
};

sqlrdirective::sqlrdirective(sqlrservercontroller *cont,
					domnode *parameters) {
	pvt=new sqlrdirectiveprivate;
	pvt->_parameters=parameters;
}

sqlrdirective::~sqlrdirective() {
	delete pvt;
}

bool sqlrdirective::run(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				const char *query) {
	return true;
}

domnode *sqlrdirective::getParameters() {
	return pvt->_parameters;
}

bool sqlrdirective::getDirective(const char *line,
					const char **directivestart,
					uint32_t *directivesize,
					const char **newline) {

	const char	*ptr=line;
	const char	*start=ptr;

	// Skip comment marker and spaces after it.
	// If the line didn't start with a comment,
	// then we're done.
	if (!charstring::compare(ptr,"--",2)) {
		ptr+=2;
		while (*ptr && *ptr==' ') {
			ptr++;
		}
		start=ptr;
	} else {
		*directivestart=NULL;
		*directivesize=0;
		*newline=start;
		return false;
	}

	// get the rest of the line and parse the directive
	for (;;) {
		if (*ptr=='\n' || !*ptr) {
			*directivestart=start;
			*directivesize=ptr-start;
			if (*(ptr-1)=='\r') {
				(*directivesize)--;
			}
			if (!*ptr) {
				*newline=ptr;
			} else {
				ptr++;
				if (*ptr=='\r') {
					(*directivesize)++;
				}
				*newline=ptr;
			}
			return true;
		}
		ptr++;
	}
}
