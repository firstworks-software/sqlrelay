// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrexportcsv.h>
#include <rudiments/process.h>
#include <rudiments/sys.h>
#include <rudiments/file.h>
#include <rudiments/filesystem.h>
#include <rudiments/permissions.h>
#include <rudiments/dictionary.h>
#include <rudiments/randomnumber.h>
#include <rudiments/stdio.h>

//#define ROWS 10000
//#define ROWS 1000
//#define ROWS 100
#define ROWS 10
//#define ROWS 2
//#define ROWS 1

//#define DEBUG 1

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

class testsqlrexportcsv : public sqlrexportcsv {
	public:
		testsqlrexportcsv();
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

		bool	tests(const char *method);

		bool		exportrow;
		uint64_t	currentcol;
		uint32_t	currentrow;
};

testsqlrexportcsv::testsqlrexportcsv() {
	exportrow=true;
	currentcol=0;
	currentrow=0;
}

bool testsqlrexportcsv::tests(const char *method) {
	if (getExportRow()!=exportrow) {
		stdoutput.printf("\n%s - getExportRow(): %d!=%d\n",
					method,getExportRow(),exportrow);
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
	return true;
}

bool testsqlrexportcsv::columnsStart() {
	#ifdef DEBUG
		stdoutput.printf("\ncolumnsStart()...\n");
	#endif
	if (!sqlrexportcsv::columnsStart()) {
		return false;
	}
	if (!tests("columnsStart()")) {
		return false;
	}
	return true;
}

bool testsqlrexportcsv::columnStart() {
	#ifdef DEBUG
		stdoutput.printf("columnStart()...\n");
	#endif
	if (!sqlrexportcsv::columnStart()) {
		return false;
	}
	if (!tests("columnStart()")) {
		return false;
	}
	return true;
}

bool testsqlrexportcsv::columnEnd() {
	#ifdef DEBUG
		stdoutput.printf("columnEnd()...\n");
	#endif
	if (!sqlrexportcsv::columnEnd()) {
		return false;
	}
	if (!tests("columnEnd()")) {
		return false;
	}
	currentcol++;
	return true;
}

bool testsqlrexportcsv::columnsEnd() {
	#ifdef DEBUG
		stdoutput.printf("columnsEnd()...\n");
	#endif
	if (!sqlrexportcsv::columnsEnd()) {
		return false;
	}
	if (!tests("columnsEnd()")) {
		return false;
	}
	currentcol=0;
	return true;
}

bool testsqlrexportcsv::rowsStart() {
	#ifdef DEBUG
		stdoutput.printf("rowsStart()...\n");
	#endif
	if (!sqlrexportcsv::rowsStart()) {
		return false;
	}
	if (!tests("rowsStart()")) {
		return false;
	}
	return true;
}

bool testsqlrexportcsv::rowStart() {
	#ifdef DEBUG
		stdoutput.printf("rowStart()...\n");
	#endif
	if (!sqlrexportcsv::rowStart()) {
		return false;
	}
	if (!tests("rowStart()")) {
		return false;
	}
	currentcol=0;
	return true;
}

bool testsqlrexportcsv::fieldStart() {
	#ifdef DEBUG
		stdoutput.printf("fieldStart()...\n");
	#endif
	if (!sqlrexportcsv::fieldStart()) {
		return false;
	}
	if (!tests("fieldStart()")) {
		return false;
	}
	return true;
}

bool testsqlrexportcsv::fieldEnd() {
	#ifdef DEBUG
		stdoutput.printf("fieldEnd()...\n");
	#endif
	if (!sqlrexportcsv::fieldEnd()) {
		return false;
	}
	if (!tests("fieldEnd()")) {
		return false;
	}
	currentcol++;
	return true;
}

bool testsqlrexportcsv::rowEnd() {
	#ifdef DEBUG
		stdoutput.printf("rowEnd()...\n");
	#endif
	if (!sqlrexportcsv::rowEnd()) {
		return false;
	}
	if (!tests("rowEnd()")) {
		return false;
	}
	currentrow++;
	currentcol=0;
	return true;
}

bool testsqlrexportcsv::rowsEnd() {
	#ifdef DEBUG
		stdoutput.printf("rowsEnd()...\n");
	#endif
	if (!sqlrexportcsv::rowsEnd()) {
		return false;
	}
	if (!tests("rowsEnd()")) {
		return false;
	}
	exportrow=true;
	currentcol=0;
	currentrow=0;
	return true;
}

struct field_t {
	const char	*name;
	const char	*type;
	const char	*pattern;
	bool		quote;
};

field_t field[]={
	{"testsmallint","smallint","%lld",false},
	{"testmediumint","mediumint","%lld",false},
	{"testint","int","%lld",false},
	{"testbigint","bigint","%lld",false},
	{"testfloat","float","%lld.1",false},
	{"testreal","real","%lld.1",false},
	{"testdecimal","decimal(5,1)","%lld.1",false},
	{"testdate","date","%04lld-01-01",true},
	{"testtime","time","01:00:00",true},
	{"testdatetime","datetime","%04lld-01-01 01:00:00",true},
	{"testchar","char(40)","char%lld",true},
	{"testvarchar","varchar(40)","varchar%lld",true},
	{"testtext","text","text%lld",true},
	{"testtinytext","tinytext","tinytext%lld",true},
	{"testmediumtext","mediumtext","mediumtext%lld",true},
	{"testlongtext","longtext","longtext%lld",true},
	{"testblob","blob","blob%lld",true},
	{"testtinyblob","tinyblob","tinyblob%lld",true},
	{"testmediumblob","mediumblob","mediumblob%lld",true,},
	{"testlongblob","longblob","longblob%lld",true},
	{NULL,NULL,NULL,false}
};

void generateComparisonFile(const char *filename,
				bool ignorecolumns,
				const char * const *columnstoignore) {


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
		header.append(field[col].name);
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
			record.printf(field[col].pattern,row);
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

int main(int argc, char **argv) {

	// instantiation
	con=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);
	stringbuffer	query;

	// clean up
	cur->sendQuery("drop table testtable");
	file::remove("testtable.csv");
	file::remove("testtable-comparison.csv");

	// create a new table
	stdoutput.printf("CREATE TEMPTABLE: \n");
	query.append("create table testtable (");
	for (uint64_t col=0; field[col].name; col++) {
		if (col) {
			query.append(',');
		}
		query.append(field[col].name);
		query.append(' ');
		query.append(field[col].type);
	}
	query.append(')');
	checkSuccess(cur->sendQuery(query.getString()),1);
	stdoutput.printf("\n");

	// insert
	stdoutput.printf("INSERT: \n");
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
	stdoutput.printf("\n");

	// set up export
	stdoutput.printf("SET UP EXPORT: \n");
	testsqlrexportcsv	ec;
	ec.setSqlrConnection(con);
	ec.setSqlrCursor(cur);
	checkSuccess((uint64_t)ec.getSqlrConnection(),(uint64_t)con);
	checkSuccess((uint64_t)ec.getSqlrCursor(),(uint64_t)cur);

	// iterate through options
	uint8_t iteration=0;
	for (;;) {

		stdoutput.printf("\n");

		// set options
		const char	*option=NULL;
		bool		ignorecolumns=false;
		const char	**columnstoignore=NULL;
		uint16_t	columnstoignorecount=0;
		stringbuffer	opt;
		const char	*col;
		if (iteration==0) {
			option=charstring::duplicate("");
		} else if (iteration==1) {
			// for iteration 1, ignore columsn
			option=charstring::duplicate("IGNORE COLUMNS - ");
			ignorecolumns=true;
		} else if (iteration>=2 && iteration<=21) {
			// for iterations 2-21, exclude individual columns
			col=field[iteration-2].name;
			columnstoignore=new const char *[2];
			columnstoignore[0]=col;
			columnstoignore[1]=NULL;
			columnstoignorecount=1;
			opt.append("IGNORE ");
			opt.append(col);
			opt.append(" column - ");
			option=opt.detachString();
			ignorecolumns=false;
		} else if (iteration>=22 && iteration<=32) {
			// for iterations 22-32, exclude random sets of
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
			ignorecolumns=false;
		} else {
			break;
		}

		// export CSV
		stdoutput.printf("%sEXPORT CSV: \n",option);
		ec.setIgnoreColumns(ignorecolumns);
		ec.setColumnsToIgnore(columnstoignore);
		checkSuccess(ec.getIgnoreColumns(),ignorecolumns);
		if (columnstoignore) {
			for (uint16_t i=0; i<columnstoignorecount; i++) {
				checkSuccess(ec.getColumnsToIgnore()[i],
							columnstoignore[i]);
			}
			checkSuccess(
				ec.getColumnsToIgnore()[columnstoignorecount],
				NULL);
		} else {
			checkSuccess((uint64_t)ec.getColumnsToIgnore(),
								(uint64_t)0);
		}
		checkSuccess(cur->sendQuery(
			"select * from testtable order by testsmallint"),1);
		checkSuccess(ec.exportToFile("testtable.csv"),1);
		stdoutput.printf("\n");

		// generate comparison CSV
		stdoutput.printf("%sGENERATE COMPARISON CSV: \n",option);
		generateComparisonFile("testtable-comparison.csv",
					ignorecolumns,columnstoignore);
		stdoutput.printf("\n");

		// diff CSV
		stdoutput.printf("%sDIFF CSV: \n",option);
		diffFiles("testtable.csv","testtable-comparison.csv");
		stdoutput.printf("\n");

		// clean up
		file::remove("testtable.csv");
		file::remove("testtable-comparison.csv");
		delete[] option;
		delete[] columnstoignore;

		iteration++;
	}

	// clean up
	cur->sendQuery("drop table testtable");

	delete cur;
	delete con;

	return 0;
}
