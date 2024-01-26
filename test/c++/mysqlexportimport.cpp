// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrexportcsv.h>
#include <sqlrelay/sqlrexportxml.h>
#include <sqlrelay/sqlrexporttable.h>
#define NEED_IS_NUMBER_TYPE_CHAR 1
#include "../../src/common/datatypes.h"
#include <rudiments/process.h>
#include <rudiments/sys.h>
#include <rudiments/file.h>
#include <rudiments/filesystem.h>
#include <rudiments/permissions.h>
#include <rudiments/dictionary.h>
#include <rudiments/randomnumber.h>
#include <rudiments/dynamicarray.h>
#include <rudiments/stdio.h>

//#define ROWS 10000
//#define ROWS 1000
//#define ROWS 100
#define ROWS 10
//#define ROWS 2
//#define ROWS 1

//#define DEBUG 1

stringbuffer	createquery;
sqlrconnection	*con;
sqlrcursor	*cur;

void checkSuccess(const char *value, const char *success) {

	if (!success) {
		if (!value) {
			stdoutput.printf("success ");
			stdoutput.flush();
			return;
		} else {
			stdoutput.printf("\"%s\"!=\"%s\"\n",value,success);
			stdoutput.printf("failure: %s",cur->errorMessage());
			stdoutput.flush();
			delete cur;
			delete con;
			process::exit(1);
		}
	}

	if (!charstring::compare(value,success)) {
		stdoutput.printf("success ");
		stdoutput.flush();
	} else {
		stdoutput.printf("\"%s\"!=\"%s\"\n",value,success);
		stdoutput.printf("failure: %s",cur->errorMessage());
		stdoutput.flush();
		delete cur;
		delete con;
		process::exit(1);
	}
}

void checkSuccess(int value, int success) {

	if (value==success) {
		stdoutput.printf("success ");
		stdoutput.flush();
	} else {
		stdoutput.printf("\"%d\"!=\"%d\"\n",value,success);
		stdoutput.printf("failure: %s",cur->errorMessage());
		stdoutput.flush();
		delete cur;
		delete con;
		process::exit(1);
	}
}

void checkSuccess(uint64_t value, uint64_t success) {

	if (value==success) {
		stdoutput.printf("success ");
		stdoutput.flush();
	} else {
		stdoutput.printf("\"%lld\"!=\"%lld\"\n",value,success);
		stdoutput.printf("failure: %s",cur->errorMessage());
		stdoutput.flush();
		delete cur;
		delete con;
		process::exit(1);
	}
}


// define the fields for the table that we're going to export
struct field_t {
	const char	*name;
	const char	*type;
	const char	*dbtype;
	const char	*pattern;
	bool		quote;
};

field_t field[]={
	{"testsmallint","smallint","SMALLINT","%lld",false},
	{"testmediumint","mediumint","MEDIUMINT","%lld",false},
	{"testint","int","INT","%lld",false},
	{"testbigint","bigint","BIGINT","%lld",false},
	{"testfloat","float","FLOAT","%lld.1",false},
	{"testreal","real","REAL","%lld.1",false},
	{"testdecimal","decimal(5,1)","DECIMAL","%lld.1",false},
	{"testdate","date","DATE","%04lld-01-01",true},
	{"testtime","time","TIME","01:00:00",true},
	{"testdatetime","datetime","DATETIME","%04lld-01-01 01:00:00",true},
	{"testchar","char(40)","STRING","char%lld",true},
	{"testvarchar","varchar(40)","VARSTRING","varchar%lld",true},
	{"testtext","text","BLOB","text%lld",true},
	{"testtinytext","tinytext","TINYBLOB","tinytext%lld",true},
	{"testmediumtext","mediumtext","MEDIUMBLOB","mediumtext%lld",true},
	{"testlongtext","longtext","LONGBLOB","longtext%lld",true},
	{"testblob","blob","BLOB","blob%lld",true},
	{"testtinyblob","tinyblob","TINYBLOB","tinyblob%lld",true},
	{"testmediumblob","mediumblob","MEDIUMBLOB","mediumblob%lld",true,},
	{"testlongblob","longblob","LONGBLOB","longblob%lld",true},
	{NULL,NULL,NULL,NULL,false}
};


// define a class that overrides the various event methods, runs a set of tests
// in each one, and bails if any of the tests fail
class testsqlrexport : virtual public sqlrexport {
	public:
		testsqlrexport();

		void	setColumnsToModify(const char * const *columnstomodify);
		void	setRowsToIgnore(dynamicarray<bool> *rowstoignore);
		void	setTestFileName(const char *testfilename);
		void	setTestTable(const char *testtable);

		bool	exportStart();
		bool	columnsStart();
		bool	columnStart();
		bool	columnEnd();
		bool	columnsEnd();
		bool	rowsStart();
		bool	rowStart();
		bool	fieldStart();
		bool	fieldEnd();
		bool	rowEnd();
		bool	rowsEnd();
		bool	exportEnd();
		bool	error(int64_t errornumber, const char *errormessage);

		bool	tests(const char *method);

	private:
		bool		ignorerow;
		uint32_t	currentcol;
		uint64_t	currentrow;
		uint64_t	exportedrowcount;
		bool		inrows;

		const char * const	*columnstomodify;
		dynamicarray<bool>	*rowstoignore;
		const char		*testfilename;
		const char		*testtable;
};

testsqlrexport::testsqlrexport() : sqlrexport() {
	ignorerow=false;
	currentcol=0;
	currentrow=0;
	exportedrowcount=0;
	inrows=false;
	columnstomodify=NULL;
	rowstoignore=NULL;
	testfilename=NULL;
	testtable=NULL;
}

void testsqlrexport::setColumnsToModify(
				const char * const *columnstomodify) {
	this->columnstomodify=columnstomodify;
}

void testsqlrexport::setRowsToIgnore(dynamicarray<bool> *rowstoignore) {
	this->rowstoignore=rowstoignore;
}

void testsqlrexport::setTestFileName(const char *testfilename) {
	this->testfilename=testfilename;
}

void testsqlrexport::setTestTable(const char *testtable) {
	this->testtable=testtable;
}

bool testsqlrexport::tests(const char *method) {
	if (getFileName()) {
		if (charstring::compare(getFileName(),testfilename)) {
			stdoutput.printf("\n%s - getFileName(): %s!=%s\n",
					method,getFileName(),testfilename);
			return false;
		}
	} else {
		if (charstring::compare(getTable(),testtable)) {
			stdoutput.printf("\n%s - getTable(): %s!=%s\n",
					method,getTable(),testtable);
			return false;
		}
	}
	if (getIgnoreRow()!=ignorerow) {
		stdoutput.printf("\n%s - getIgnoreRow(): %d!=%d\n",
					method,getIgnoreRow(),ignorerow);
		return false;
	}
	if (getCurrentRow()!=currentrow) {
		stdoutput.printf("\n%s - getCurrentRow(): %lld!=%lld\n",
					method,getCurrentRow(),currentrow);
		return false;
	}
	if (getCurrentColumn()!=currentcol) {
		stdoutput.printf("\n%s - getCurrentColumn(): %ld!=%ld\n",
					method,getCurrentColumn(),currentcol);
		return false;
	}
	const char	*colname=getSqlrCursor()->getColumnName(currentcol);
	if (!charstring::compare(method,"columnEnd()") &&
			charstring::isInSet(colname,columnstomodify)) {
		colname="MODIFIED";
	}
	if (charstring::compare(getCurrentColumnName(),colname)) {
		stdoutput.printf("\n%s - getCurrentColumnName(): %s!=%s\n",
					method,getCurrentColumnName(),colname);
		return false;
	}
	if (inrows && field[currentcol].name!=NULL) {
		stringbuffer	f;
		if (!charstring::compare(method,"fieldEnd()") &&
			charstring::isInSet(getCurrentColumnName(),
							columnstomodify)) {
			f.write("MODIFIED");
		} else {
			f.printf(field[currentcol].pattern,currentrow);
		}
		if (charstring::compare(getCurrentField(),f.getString())) {
			stdoutput.printf("\n%s - getCurrentField(): %s!=%s\n",
					method,getCurrentField(),
					f.getString());
			return false;
		}
	} else {
		if (charstring::compare(getCurrentField(),
					getCurrentColumnName())) {
			stdoutput.printf("\n%s - getCurrentField(): %s!=%s\n",
					method,getCurrentField(),
					getCurrentColumnName());
			return false;
		}
	}
	if (getIsNumericColumn(currentcol)!=
			isNumberTypeChar(getSqlrCursor()->
						getColumnType(
							getCurrentColumn()))) {
		stdoutput.printf("\n%s - getIsNumericColumn(%d): %d!=%d\n",
				method,currentcol,
				getIsNumericColumn(currentcol),
				isNumberTypeChar(
					getSqlrCursor()->
						getColumnType(
							getCurrentColumn())));
		return false;
	}
	if (getExportedRowCount()!=exportedrowcount) {
		stdoutput.printf("\n%s - getExportedRowCount(): %lld!=%lld\n",
				method,getExportedRowCount(),exportedrowcount);
		return false;
	}
	return true;
}

bool testsqlrexport::exportStart() {
	#ifdef DEBUG
		stdoutput.printf("\nexportStart()...\n");
	#endif
	ignorerow=false;
	currentcol=0;
	currentrow=0;
	exportedrowcount=0;
	inrows=false;
	if (!sqlrexport::exportStart()) {
		return false;
	}
	if (!tests("exportStart()")) {
		return false;
	}
	return true;
}

bool testsqlrexport::columnsStart() {
	#ifdef DEBUG
		stdoutput.printf("\ncolumnsStart()...\n");
	#endif
	if (!sqlrexport::columnsStart()) {
		return false;
	}
	if (!tests("columnsStart()")) {
		return false;
	}
	return true;
}

bool testsqlrexport::columnStart() {
	#ifdef DEBUG
		stdoutput.printf("columnStart()...\n");
	#endif
	if (!sqlrexport::columnStart()) {
		return false;
	}
	if (!tests("columnStart()")) {
		return false;
	}
	if (charstring::isInSet(getCurrentColumnName(),columnstomodify)) {
		setCurrentColumnName("MODIFIED");
	}
	return true;
}

bool testsqlrexport::columnEnd() {
	#ifdef DEBUG
		stdoutput.printf("columnEnd()...\n");
	#endif
	if (!sqlrexport::columnEnd()) {
		return false;
	}
	if (!tests("columnEnd()")) {
		return false;
	}
	currentcol++;
	return true;
}

bool testsqlrexport::columnsEnd() {
	#ifdef DEBUG
		stdoutput.printf("columnsEnd()...\n");
	#endif
	if (!sqlrexport::columnsEnd()) {
		return false;
	}
	if (!tests("columnsEnd()")) {
		return false;
	}
	return true;
}

bool testsqlrexport::rowsStart() {
	#ifdef DEBUG
		stdoutput.printf("rowsStart()...\n");
	#endif
	ignorerow=false;
	currentcol=0;
	currentrow=0;
	exportedrowcount=0;
	inrows=true;
	if (!sqlrexport::rowsStart()) {
		return false;
	}
	if (!tests("rowsStart()")) {
		return false;
	}
	return true;
}

bool testsqlrexport::rowStart() {
	#ifdef DEBUG
		stdoutput.printf("rowStart()...\n");
	#endif
	ignorerow=false;
	currentcol=0;
	if (!sqlrexport::rowStart()) {
		return false;
	}
	if (rowstoignore && (*rowstoignore)[currentrow]) {
		setIgnoreRow(true);
		ignorerow=true;
	}
	if (!tests("rowStart()")) {
		return false;
	}
	return true;
}

bool testsqlrexport::fieldStart() {
	#ifdef DEBUG
		stdoutput.printf("fieldStart()...\n");
	#endif
	if (!sqlrexport::fieldStart()) {
		return false;
	}
	if (!tests("fieldStart()")) {
		return false;
	}
	if (charstring::isInSet(getCurrentColumnName(),columnstomodify)) {
		setCurrentField("MODIFIED");
	}
	return true;
}

bool testsqlrexport::fieldEnd() {
	#ifdef DEBUG
		stdoutput.printf("fieldEnd()...\n");
	#endif
	if (!sqlrexport::fieldEnd()) {
		return false;
	}
	if (!tests("fieldEnd()")) {
		return false;
	}
	currentcol++;
	return true;
}

bool testsqlrexport::rowEnd() {
	#ifdef DEBUG
		stdoutput.printf("rowEnd()...\n");
	#endif
	if (!sqlrexport::rowEnd()) {
		return false;
	}
	if (!tests("rowEnd()")) {
		return false;
	}
	if (!ignorerow) {
		exportedrowcount++;
	}
	currentrow++;
	return true;
}

bool testsqlrexport::rowsEnd() {
	#ifdef DEBUG
		stdoutput.printf("rowsEnd()...\n");
	#endif
	if (!sqlrexport::rowsEnd()) {
		return false;
	}
	if (!tests("rowsEnd()")) {
		return false;
	}
	return true;
}

bool testsqlrexport::exportEnd() {
	#ifdef DEBUG
		stdoutput.printf("exportEnd()...\n");
	#endif
	if (!sqlrexport::exportEnd()) {
		return false;
	}
	if (!tests("exportEnd()")) {
		return false;
	}
	return true;
}

bool testsqlrexport::error(int64_t errornumber, const char *errormessage) {
	stdoutput.printf("\n%lld: %s\n",errornumber,errormessage);
	return false;
}


// define a set of classes that inherit from our base class, to get the event
// methods, but also inherit from classes that export to csv, xml, and table,
// to get the export methods
class testsqlrexportcsv : virtual public testsqlrexport,
					virtual public sqlrexportcsv {
};

class testsqlrexportxml : virtual public testsqlrexport,
					virtual public sqlrexportxml {
};

class testsqlrexporttable : virtual public testsqlrexport,
					virtual public sqlrexporttable {
};


// define a set of methods that generate something to compare our export to
void generateComparisonCsv(const char *filename,
				bool ignorecolumns,
				const char * const *columnstoignore,
				const char * const *columnstomodify,
				dynamicarray<bool> *rowstoignore) {

	// create file
	file	comparison;
	checkSuccess(comparison.create(filename,
				permissions::parsePermString("rw-rw-r--")),1);
	comparison.setWriteBufferSize(
		filesystem::getOptimumTransferBlockSize(filename));

	// write header, ignoring columns as appropriate
	stringbuffer	header;
	for (uint64_t col=0; field[col].name; col++) {
		if (charstring::isInSet(field[col].name,columnstoignore)) {
			continue;
		}
		if (header.getSize()) {
			header.append(',');
		}
		header.append('"');
		if (charstring::isInSet(field[col].name,columnstomodify)) {
			header.append("MODIFIED");
		} else {
			header.append(field[col].name);
		}
		header.append('"');
	}
	if (!ignorecolumns) {
		header.append('\n');
		checkSuccess(comparison.write(header.getString()),
					header.getStringLength());
	}

	// write records, ignoring columns as appropriate
	stringbuffer	record;
	for (uint64_t row=0; row<ROWS; row++) {
		if ((*rowstoignore)[row]) {
			continue;
		}
		for (uint32_t col=0; field[col].name; col++) {
			if (charstring::isInSet(field[col].name,
							columnstoignore)) {
				continue;
			}
			if (record.getSize()) {
				record.append(',');
			}
			if (field[col].quote) {
				record.append('"');
			}
			if (charstring::isInSet(field[col].name,
							columnstomodify)) {
				record.write("MODIFIED");
			} else {
				record.printf(field[col].pattern,row);
			}
			if (field[col].quote) {
				record.append('"');
			}
		}
		record.append('\n');
		if (comparison.write(record.getString(),
					record.getStringLength())!=
					(ssize_t)record.getStringLength()) {
			checkSuccess(0,1);
		}
		record.clear();
	}
	checkSuccess(1,1);

	comparison.flushWriteBuffer(-1,-1);
}

void generateComparisonXml(const char *filename,
				uint32_t colcount, bool ignorecolumns,
				const char * const *columnstoignore,
				const char * const *columnstomodify,
				dynamicarray<bool> *rowstoignore) {

	// create file
	file	comparison;
	checkSuccess(comparison.create(filename,
				permissions::parsePermString("rw-rw-r--")),1);
	comparison.setWriteBufferSize(
		filesystem::getOptimumTransferBlockSize(filename));

	// write header, ignoring columns as appropriate
	comparison.write("<?xml version=\"1.0\"?>\n");
	comparison.write("<table>\n");
	if (!ignorecolumns) {
		stringbuffer	header;
		header.printf("<columns count=\"%ld\">\n",colcount);
		for (uint64_t col=0; field[col].name; col++) {
			if (charstring::isInSet(field[col].name,
							columnstoignore)) {
				continue;
			}
			const char	*colname=field[col].name;
			if (charstring::isInSet(field[col].name,
							columnstomodify)) {
				colname="MODIFIED";
			}
			header.printf("	<column name=\"%s\" "
						"type=\"%s\"/>\n",
						colname,field[col].dbtype);
		}
		header.write("</columns>\n");
		checkSuccess(comparison.write(header.getString()),
					header.getStringLength());
	}

	// write records, ignoring columns as appropriate
	stringbuffer	record;
	comparison.write("<rows>\n");
	for (uint64_t row=0; row<ROWS; row++) {
		if ((*rowstoignore)[row]) {
			continue;
		}
		record.write("	<row>\n");
		for (uint32_t col=0; field[col].name; col++) {
			if (charstring::isInSet(field[col].name,
							columnstoignore)) {
				continue;
			}
			record.write("	<field>");
			if (charstring::isInSet(field[col].name,
							columnstomodify)) {
				record.write("MODIFIED");
			} else {
				record.printf(field[col].pattern,row);
			}
			record.write("</field>\n");
		}
		record.write("	</row>\n");
		if (comparison.write(record.getString(),
					record.getStringLength())!=
					(ssize_t)record.getStringLength()) {
			checkSuccess(0,1);
		}
		record.clear();
	}
	comparison.write("</rows>\n");
	comparison.write("</table>\n");
	checkSuccess(1,1);

	comparison.flushWriteBuffer(-1,-1);
}

void createTable(const char *tablename,
			const char * const *columnstoignore,
			uint32_t *colcount) {

	stringbuffer	query;
	query.append("create table ")->append(tablename)->append(" (");
	uint32_t	ccount=0;
	for (uint64_t col=0; field[col].name; col++) {
		if (charstring::isInSet(field[col].name,columnstoignore)) {
			continue;
		}
		if (ccount) {
			query.append(',');
		}
		query.append(field[col].name);
		query.append(' ');
		query.append(field[col].type);
		ccount++;
	}
	query.append(')');
	checkSuccess(cur->sendQuery(query.getString()),1);
	if (colcount) {
		*colcount=ccount;
	}
}

void generateComparisonTable(const char *tablename,
				bool ignorecolumns,
				const char * const *columnstoignore,
				const char * const *columnstomodify,
				dynamicarray<bool> *rowstoignore) {

	// connect to db
	sqlrconnection	econ("sqlrelay",9000,"/tmp/test.socket",
					"testuser","testpassword",0,1);
	sqlrcursor	ecur(&econ);

	// create table
	createTable(tablename,columnstoignore,NULL);

	// populate table
	stringbuffer	query;
	for (uint64_t row=0; row<ROWS; row++) {
		if ((*rowstoignore)[row]) {
			continue;
		}
		query.clear();
		query.append("insert into ");
		query.append(tablename);
		query.append(" testtable values (");
		bool	first=true;
		for (uint32_t col=0; field[col].pattern; col++) {
			if (charstring::isInSet(field[col].name,
							columnstoignore)) {
				continue;
			}
			if (first) {
				first=false;
			} else {
				query.append(',');
			}
			if (field[col].quote) {
				query.append('\'');
			}
			if (charstring::isInSet(field[col].name,
							columnstomodify)) {
				query.printf(field[col].pattern,row+1);
			} else {
				query.printf(field[col].pattern,row);
			}
			if (field[col].quote) {
				query.append('\'');
			}
		}
		query.append(')');
		if (!cur->sendQuery(query.getString())) {
			return;
		}
	}
}


// define methods to diff our export against a comparison
void diffFiles(const char *filename1, const char *filename2) {

	// open file 1
	file	f1;
	checkSuccess(f1.open(filename1,O_RDONLY),1);
	f1.setReadBufferSize(
		filesystem::getOptimumTransferBlockSize(filename1));

	// open file 2
	file	f2;
	checkSuccess(f2.open(filename2,O_RDONLY),1);
	f2.setReadBufferSize(
		filesystem::getOptimumTransferBlockSize(filename2));

	for (;;) {

		// lines
		char	*line1;
		char	*line2;

		// read a line from each file
		ssize_t	size1=f1.read(&line1,"\n");
		ssize_t	size2=f2.read(&line2,"\n");

		// fail if the sizes are different then
		if (size1!=size2) {
			delete[] line1;
			delete[] line2;
			checkSuccess(0,1);
		}

		// fail if the lines are not the same
		if (charstring::compare(line1,line2)) {
			delete[] line1;
			delete[] line2;
			checkSuccess(0,1);
		}

		// bail if we failed to read either line
		if (!size1 || !size2) {
			delete[] line1;
			delete[] line2;
			break;
		}
	}
	checkSuccess(1,1);
}

void diffTables(const char *table1, const char *table2) {
}



int main(int argc, char **argv) {

	// instantiation
	con=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);

	// clean up
	cur->sendQuery("drop table testtable");
	cur->sendQuery("drop table testtable_export");
	cur->sendQuery("drop table testtable_comparison");
	file::remove("testtable.csv");
	file::remove("testtable-comparison.csv");
	file::remove("testtable.xml");
	file::remove("testtable-comparison.xml");

	// create a new table
	stdoutput.printf("CREATE TESTTABLE: \n");
	uint32_t	colcount=0;
	createTable("testtable",NULL,&colcount);
	stdoutput.printf("\n");

	// insert
	stdoutput.printf("INSERT: \n");
	stringbuffer	query;
	for (uint64_t row=0; row<ROWS; row++) {
		query.clear();
		query.append("insert into testtable values (");
		for (uint32_t col=0; field[col].pattern; col++) {
			if (col) {
				query.append(',');
			}
			if (field[col].quote) {
				query.append('\'');
			}
			query.printf(field[col].pattern,row);
			if (field[col].quote) {
				query.append('\'');
			}
		}
		query.append(')');
		if (!cur->sendQuery(query.getString())) {
			checkSuccess(0,1);
		}
	}
	checkSuccess(1,1);
	stdoutput.printf("\n\n");

	// set up csv export
	stdoutput.printf("SET UP CSV EXPORT: \n");
	testsqlrexportcsv	tsec;
	tsec.setSqlrConnection(con);
	tsec.setSqlrCursor(cur);
	checkSuccess((uint64_t)tsec.getSqlrConnection(),(uint64_t)con);
	checkSuccess((uint64_t)tsec.getSqlrCursor(),(uint64_t)cur);
	stdoutput.printf("\n\n");

	// set up xml export
	stdoutput.printf("SET UP CSV EXPORT: \n");
	testsqlrexportxml	tsex;
	tsex.setSqlrConnection(con);
	tsex.setSqlrCursor(cur);
	checkSuccess((uint64_t)tsex.getSqlrConnection(),(uint64_t)con);
	checkSuccess((uint64_t)tsex.getSqlrCursor(),(uint64_t)cur);
	stdoutput.printf("\n\n");

	// set up table export
	stdoutput.printf("SET UP TABLE EXPORT: \n");
	testsqlrexporttable	tset;
	tset.setSqlrConnection(con);
	tset.setSqlrCursor(cur);
	checkSuccess((uint64_t)tset.getSqlrConnection(),(uint64_t)con);
	checkSuccess((uint64_t)tset.getSqlrCursor(),(uint64_t)cur);
	stdoutput.printf("\n\n");

	// iterate through options...
	uint8_t oiter=0;
	for (;;) {

		// set options
		const char		*option=NULL;
		bool			ignorecolumns=false;
		const char		**columnstoignore=NULL;
		uint16_t		columnstoignorecount=0;
		const char		**columnstomodify=NULL;
		dynamicarray<bool>	rowstoignore;
		stringbuffer		opt;
		const char		*col;
		if (oiter==0) {
			option=charstring::duplicate("");
		} else if (oiter==1) {
			// for iteration 1, ignore columsn
			option=charstring::duplicate("IGNORE COLUMNS - ");
			ignorecolumns=true;
		} else if (oiter>=2 && oiter<=21) {
			// for iterations 2-21, ignore individual columns
			col=field[oiter-2].name;
			columnstoignore=new const char *[2];
			columnstoignore[0]=col;
			columnstoignore[1]=NULL;
			columnstoignorecount=1;
			opt.append("IGNORE ");
			opt.append(col);
			opt.append(" column - ");
			option=opt.detachString();
		} else if (oiter>=22 && oiter<=32) {
			// for iterations 22-32, ignore random sets of
			// columns, possibly with repetitions
			randomnumber	r;
			r.setSeed(randomnumber::getSeed());
			opt.append("IGNORE ");
			columnstoignore=new const char *[11];
			uint32_t	rn;
			for (uint8_t i=0; i<10; i++) {
				r.generate(&rn);
				r.setSeed(rn);
				rn=r.scale(rn,0,19);
				col=field[rn].name;
				columnstoignore[i]=col;
				if (i) {
					opt.append(',');
				}
				opt.append(col);
			}
			columnstoignore[10]=NULL;
			columnstoignorecount=10;
			opt.append(" column - ");
			option=opt.detachString();
		} else if (oiter==33) {
			// for iteration 33, don't export various rows
			opt.append("IGNORE ");
			randomnumber	r;
			r.setSeed(randomnumber::getSeed());
			uint32_t	rn;
			uint64_t	ircount=0;
			for (uint64_t row=0; row<ROWS; row++) {
				r.generate(&rn);
				r.setSeed(rn);
				rn=r.scale(rn,0,1);
				if (rn) {
					rowstoignore[row]=true;
					ircount++;
				} else {
					rowstoignore[row]=false;
				}
			}
			opt.append(ircount);
			opt.append(" ROWS - ");
			option=opt.detachString();
		} else if (oiter>=34 && oiter<=44) {
			// for iterations 22-32, modify random sets of
			// columns and fields, possibly with repetitions
			randomnumber	r;
			r.setSeed(randomnumber::getSeed());
			opt.append("MODIFY ");
			columnstomodify=new const char *[11];
			uint32_t	rn;
			for (uint8_t i=0; i<10; i++) {
				r.generate(&rn);
				r.setSeed(rn);
				rn=r.scale(rn,0,19);
				col=field[rn].name;
				columnstomodify[i]=col;
				if (i) {
					opt.append(',');
				}
				opt.append(col);
			}
			columnstomodify[10]=NULL;
			opt.append(" column - ");
			option=opt.detachString();
		} else {
			break;
		}

		// iterate through formats...
		for (uint8_t fiter=0; fiter<3; fiter++) {

			// csv, xml, or table
			testsqlrexport	*e;
			const char	*format;
			const char	*exp;
			const char	*comp;
			if (fiter==0) {
				e=&tsec;
				format="CSV";
				exp="testtable.csv";
				comp="testtable-comparison.csv";
			} else if (fiter==1) {
				e=&tsex;
				format="XML";
				exp="testtable.xml";
				comp="testtable-comparison.xml";
			} else if (fiter==2) {
// for tables, skip modified colnames/fields, for now
if (oiter>=34 && oiter<=44) {
	continue;
}
				e=&tset;
				format="TABLE";
				exp="testtable_export";
				comp="testtable_comparison";
				createTable(exp,columnstoignore,NULL);
			}

			// export file or table
			stdoutput.printf("%sEXPORT %s: \n",option,format);
			e->setIgnoreColumns(ignorecolumns);
			e->setColumnsToIgnore(columnstoignore);
			e->setColumnsToModify(columnstomodify);
			e->setRowsToIgnore(&rowstoignore);
			e->setTestFileName(exp);
			e->setTestTable(exp);
			checkSuccess(e->getIgnoreColumns(),ignorecolumns);
			if (columnstoignore) {
				for (uint16_t i=0;
					i<columnstoignorecount; i++) {
					checkSuccess(
						e->getColumnsToIgnore()[i],
						columnstoignore[i]);
				}
				checkSuccess(
					e->getColumnsToIgnore()[
						columnstoignorecount],
					NULL);
			} else {
				checkSuccess(
					(uint64_t)e->getColumnsToIgnore(),
					(uint64_t)0);
			}
			checkSuccess(cur->sendQuery(
				"select * from testtable "
				"order by testsmallint"),1);
			if (fiter==0) {
				checkSuccess(tsec.exportToFile(exp),1);
			} else if (fiter==1) {
				checkSuccess(tsex.exportToFile(exp),1);
			} else if (fiter==2) {
				sqlrconnection	econ("sqlrelay",9000,
							"/tmp/test.socket",
							"testuser",
							"testpassword",0,1);
				sqlrcursor	ecur(&econ);
				checkSuccess(tset.exportToTable(&econ,&ecur,
								exp,500),1);
			}
			stdoutput.printf("\n");

			// generate comparison file
			stdoutput.printf(
				"%sGENERATE COMPARISON %s: \n",option,format);
			if (fiter==0) {
				generateComparisonCsv(comp,
						ignorecolumns,columnstoignore,
						columnstomodify,&rowstoignore);
			} else if (fiter==1) {
				generateComparisonXml(comp,colcount,
						ignorecolumns,columnstoignore,
						columnstomodify,&rowstoignore);
			} else if (fiter==2) {
				generateComparisonTable(comp,
						ignorecolumns,columnstoignore,
						columnstomodify,&rowstoignore);
			}
			stdoutput.printf("\n");

			// diff files
			stdoutput.printf("%sDIFF %s: \n",option,format);
			if (fiter<2) {
				diffFiles(exp,comp);
			} else {
				diffTables(exp,comp);
			}
			stdoutput.printf("\n");

			// clean up
			if (fiter==2) {
				sqlrconnection	econ("sqlrelay",9000,
							"/tmp/test.socket",
							"testuser",
							"testpassword",0,1);
				sqlrcursor	ecur(&econ);

				ecur.prepareQuery("drop table $(EXP)");
				ecur.substitution("EXP",exp);
				ecur.executeQuery();
				
				ecur.prepareQuery("drop table $(COMP)");
				ecur.substitution("COMP",comp);
				ecur.executeQuery();
			}
			file::remove(exp);
			file::remove(comp);

			stdoutput.printf("\n");
		}

		// clean up
		delete[] option;
		delete[] columnstoignore;

		oiter++;
	}

	// clean up
	cur->sendQuery("drop table testtable");

	delete cur;
	delete con;

	return 0;
}
