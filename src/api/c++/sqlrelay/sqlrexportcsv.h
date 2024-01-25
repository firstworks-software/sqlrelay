// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#ifndef SQLREXPORTCSV_H
#define SQLREXPORTCSV_H

#include <sqlrelay/private/sqlrexportcsvincludes.h>

/** The sqlrexportcsv class implements sqlrexport for CSV files. */
class SQLRCLIENT_DLLSPEC sqlrexportcsv : public sqlrexport {
	public:
		/** Creates an instance of the sqlrexportcsv class. */
		sqlrexportcsv();

		/** Destroys this instance of the sqlrexportcsv class. */
		virtual	~sqlrexportcsv();

		/** Exports the result set of the cursor currently in use as
		 *  set by the most recent call to setSqlrCursor() to file
		 *  "filename" in CSV format, or to standard output if
		 *  "filename" is NULL or empty.
		 *
		 *  The following result set:
		 *
		 *  col1,col2,col3,col4
		 *  =====================
		 *  0,0.0,field00,field01
		 *  1,1.1,field10,field11
		 *
		 *  would be exported as:
		 *
		 *  "col1","col2","col3","col4"
		 *  0,0.0,"field00","field01"
		 *  1,1.1,"field10","field11"
		 *
		 *
		 *  The "table" argument is ignored in this implementation.
		 *
		 *  Returns true on success and false if an error occurred. */
		virtual	bool	exportToFile(const char *filename,
							const char *table);

		/** Exports the result set of the cursor currently in use as
		 *  set by the most recent call to setSqlrCursor() to the
		 *  JSON domnode "jsondomnode" in a format the represents the
		 *  CSV using JSON.
		 *
		 *  The following result set:
		 *
		 *  col1,col2,col3,col4
		 *  =====================
		 *  0,0.0,field00,field01
		 *  1,1.1,field10,field11
		 *
		 *  would be represented like:
		 *
		 *  <h t="a">
		 *    <v t="s" v="col1"/>
		 *    <v t="s" v="col2"/>
		 *    <v t="s" v="col3"/>
		 *    <v t="s" v="col4"/>
		 *  </h>
		 *  <r t="a">
		 *    <v t="n" v="0"/>
		 *    <v t="n" v="0.0"/>
		 *    <v t="s" v="field00"/>
		 *    <v t="s" v="field01"/>
		 *  </r>
		 *  <r t="a">
		 *    <v t="n" v="1"/>
		 *    <v t="n" v="1.1"/>
		 *    <v t="s" v="field10"/>
		 *    <v t="s" v="field11"/>
		 *  </r>
		 *
		 *  This format is similar to the DOM tree that the rudiments
		 *  class csvdom defines to represent a CSV, though it uses the
		 *  rules defined for representing JSON by the rudiments class
		 *  jsondom.
		 *
		 *  It consists of a "h"(eader) array, each "v"(alue) of which
		 *  contains a "t"(ype) attribute of either "s"(tring) or
		 *  "n"(umber) and a "v"(alue) attribute containing the column
		 *  name.
		 *
		 *  It also consists of "r"(ecord) arrays, each "v"(alue)
		 *  of which also contains a "t"(ype) attribute of either
		 *  "s"(tring) or "n"(umber) and a "v"(alue).
		 *
		 *  The "h"(eader) and "r"(ecord) nodes are inspired by the
		 *  CSV representation defined by csvdom, but the nodes of the
		 *  tree are all consistent with the JSON representation
		 *  defined by jsondom.  As such, the tree can be operated on
		 *  by methods that can operate on a jsondom tree.
		 *
		 *  Note that since there is no tag representing the set of
		 *  records/rows, rather the tag for an each record/row is just
		 *  appeneded to the top-level tag, there is no "rows domnode".
		 *  As such, this method never calls setRowsDomNode(), and
		 *  getRowsDomNode() always return NULL.
		 *
		 *  The "table" argument is ignored in this implementation.
		 *
		 *  Returns true on success and false if an error occurred. */
		virtual	bool	exportToJsonDomNode(domnode *jsondomnode,
							const char *table);

	#include <sqlrelay/private/sqlrexportcsv.h>
};

#endif
