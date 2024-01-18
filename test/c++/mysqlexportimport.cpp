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

#define ROWS 10000
//#define ROWS 1000
//#define ROWS 100
//#define ROWS 10
//#define ROWS 2
//#define ROWS 1

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

const char * const columns[]={
	"testsmallint",
	"testmediumint",
	"testint",
	"testbigint",
	"testfloat",
	"testreal",
	"testdecimal",
	"testdate",
	"testtime",
	"testdatetime",
	"testchar",
	"testvarchar",
	"testtext",
	"testtinytext",
	"testmediumtext",
	"testlongtext",
	"testblob",
	"testtinyblob",
	"testmediumblob",
	"testlongblob",
	NULL
};

void generateComparisonFile(const char *filename,
				bool ignorecolumns,
				const char * const *columnstoignore) {

	// get optimium block size
	filesystem	fs;
	off64_t		optblocksize;
	if (fs.open(filename)) {
		optblocksize=fs.getOptimumTransferBlockSize();
	} else {
		optblocksize=sys::getPageSize();
	}

	// create file
	file	comparison;
	checkSuccess(comparison.create(filename,
				permissions::parsePermString("rw-rw-r--")),1);
	comparison.setWriteBufferSize(optblocksize);

	// write header, ignoring columns to ignore,
	// determining indexes to ignore
	uint16_t			index=0;
	dictionary<uint32_t,bool>	indexestoignore;
	bool				first=true;
	stringbuffer			header;
	for (const char * const *c=columns; *c; c++) {
		if (!charstring::isInSet(*c,columnstoignore)) {
			if (!first) {
				header.append(',');
			}
			header.append('"')->append(*c)->append('"');
			first=false;
			indexestoignore.setValue(index,false);
		} else {
			indexestoignore.setValue(index,true);
		}
		index++;
	}
	if (!ignorecolumns) {
		header.append('\n');
		checkSuccess(comparison.write(header.getString()),
					header.getStringLength());
	}

	// write records, ignoring index to ignore
	bool	success=true;
	stringbuffer	record;
	for (uint64_t i=0; i<ROWS && success; i++) {
		first=true;
		if (!indexestoignore.getValue(0)) {
			record.printf("%lld",i);
			first=false;
		}
		if (!indexestoignore.getValue(1)) {
			if (!first) {
				record.append(',');
			}
			record.printf("%lld",i);
			first=false;
		}
		if (!indexestoignore.getValue(2)) {
			if (!first) {
				record.append(',');
			}
			record.printf("%lld",i);
			first=false;
		}
		if (!indexestoignore.getValue(3)) {
			if (!first) {
				record.append(',');
			}
			record.printf("%lld",i);
			first=false;
		}
		if (!indexestoignore.getValue(4)) {
			if (!first) {
				record.append(',');
			}
			record.printf("%.*f",1,((double)i+0.1));
			first=false;
		}
		if (!indexestoignore.getValue(5)) {
			if (!first) {
				record.append(',');
			}
			record.printf("%.*f",1,((double)i+0.1));
			first=false;
		}
		if (!indexestoignore.getValue(6)) {
			if (!first) {
				record.append(',');
			}
			record.printf("%.*f",1,((double)i+0.1));
			first=false;
		}
		if (!indexestoignore.getValue(7)) {
			if (!first) {
				record.append(',');
			}
			record.printf("\"%04d-01-01\"",i);
			first=false;
		}
		if (!indexestoignore.getValue(8)) {
			if (!first) {
				record.append(',');
			}
			record.printf("\"01:00:00\"",i);
			first=false;
		}
		if (!indexestoignore.getValue(9)) {
			if (!first) {
				record.append(',');
			}
			record.printf("\"%04d-01-01 01:00:00\"",i);
			first=false;
		}
		if (!indexestoignore.getValue(10)) {
			if (!first) {
				record.append(',');
			}
			record.printf("\"char%d\"",i);
			first=false;
		}
		if (!indexestoignore.getValue(11)) {
			if (!first) {
				record.append(',');
			}
			record.printf("\"varchar%d\"",i);
			first=false;
		}
		if (!indexestoignore.getValue(12)) {
			if (!first) {
				record.append(',');
			}
			record.printf("\"text%d\"",i);
			first=false;
		}
		if (!indexestoignore.getValue(13)) {
			if (!first) {
				record.append(',');
			}
			record.printf("\"tinytext%d\"",i);
			first=false;
		}
		if (!indexestoignore.getValue(14)) {
			if (!first) {
				record.append(',');
			}
			record.printf("\"mediumtext%d\"",i);
			first=false;
		}
		if (!indexestoignore.getValue(15)) {
			if (!first) {
				record.append(',');
			}
			record.printf("\"longtext%d\"",i);
			first=false;
		}
		if (!indexestoignore.getValue(16)) {
			if (!first) {
				record.append(',');
			}
			record.printf("\"blob%d\"",i);
			first=false;
		}
		if (!indexestoignore.getValue(17)) {
			if (!first) {
				record.append(',');
			}
			record.printf("\"tinyblob%d\"",i);
			first=false;
		}
		if (!indexestoignore.getValue(18)) {
			if (!first) {
				record.append(',');
			}
			record.printf("\"mediumblob%d\"",i);
			first=false;
		}
		if (!indexestoignore.getValue(19)) {
			if (!first) {
				record.append(',');
			}
			record.printf("\"longblob%d\"",i);
			first=false;
		}
		record.append('\n');
		if (comparison.write(record.getString(),
					record.getStringLength())!=
					(ssize_t)record.getStringLength()) {
			success=false;
		}
		record.clear();
	}
	checkSuccess(success,1);

	comparison.flushWriteBuffer(-1,-1);
}

void diffFiles(const char *filename1, const char *filename2) {

	// get optimium block size
	filesystem	fs;
	off64_t		obs1;
	if (fs.open(filename1)) {
		obs1=fs.getOptimumTransferBlockSize();
	} else {
		obs1=sys::getPageSize();
	}
	off64_t		obs2;
	if (fs.open(filename2)) {
		obs2=fs.getOptimumTransferBlockSize();
	} else {
		obs2=sys::getPageSize();
	}

	// files
	file	f1;
	file	f2;

	// open file 1
	checkSuccess(f1.open(filename1,O_RDONLY),1);
	f1.setReadBufferSize(obs1);

	// open file 2
	checkSuccess(f2.open(filename2,O_RDONLY),1);
	f2.setReadBufferSize(obs2);

	bool	success=true;
	for (;;) {

		// lines
		char	*line1;
		char	*line2;

		// read a line from each file
		ssize_t	size1=f1.read(&line1,"\n");
		ssize_t	size2=f2.read(&line2,"\n");

		// fail if the sizes are different then
		if (size1!=size2) {
			success=false;
			delete[] line1;
			delete[] line2;
			break;
		}

		// fail if the lines are not the same
		if (charstring::compare(line1,line2)) {
			success=false;
			delete[] line1;
			delete[] line2;
			break;
		}

		// bail if we failed to read either line
		if (!size1 || !size2) {
			delete[] line1;
			delete[] line2;
			break;
		}
	}
	checkSuccess(success,1);
}

int main(int argc, char **argv) {

	// instantiation
	con=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);

	// clean up
	cur->sendQuery("drop table testtable");
	file::remove("testtable.csv");
	file::remove("testtable-comparison.csv");

	// create a new table
	stdoutput.printf("CREATE TEMPTABLE: \n");
	checkSuccess(cur->sendQuery(
			"create table testtable ("
			"testsmallint smallint, "
			"testmediumint mediumint, "
			"testint int, "
			"testbigint bigint, "
			"testfloat float, "
			"testreal real, "
			"testdecimal decimal(5,1), "
			"testdate date, "
			"testtime time, "
			"testdatetime datetime, "
			"testchar char(40), "
			"testvarchar varchar(40), "
			"testtext text, "
			"testtinytext tinytext, "
			"testmediumtext mediumtext, "
			"testlongtext longtext, "
			"testblob blob, "
			"testtinyblob tinyblob, "
			"testmediumblob mediumblob, "
			"testlongblob longblob)"),1);
	stdoutput.printf("\n");

	// insert
	stdoutput.printf("INSERT: \n");
	stringbuffer	query;
	bool success=true;
	for (uint64_t i=0; i<ROWS && success; i++) {
		query.printf("insert into testtable values "
			"(%lld,%lld,%lld,%lld,"
			"%.*f,%.*f,%.*f,"
			"'%04d-01-01','01:00:00','%04d-01-01 01:00:00',"
			"'char%d','varchar%d','text%d','tinytext%d',"
			"'mediumtext%d','longtext%d','blob%d','tinyblob%d',"
			"'mediumblob%d','longblob%d')",
			i,i,i,i,
			1,((double)i+0.1),1,((double)i+0.1),1,((double)i+0.1),
			i,i,
			i,i,i,i,
			i,i,i,i,
			i,i);
		if (!cur->sendQuery(query.getString())) {
			success=false;
		}
		query.clear();
	}
	checkSuccess(success,1);
	stdoutput.printf("\n");

	// set up export
	stdoutput.printf("SET UP EXPORT: \n");
	sqlrexportcsv	ec;
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
		if (iteration==0) {
			option=charstring::duplicate("");
		} else if (iteration==1) {
			// for iteration 1, ignore columsn
			option=charstring::duplicate("IGNORE COLUMNS - ");
			ignorecolumns=true;
		} else if (iteration>=2 && iteration<=21) {
			// for iterations 2-21, exclude individual columns
			const char	*col=columns[iteration-2];
			columnstoignore=new const char *[2];
			columnstoignore[0]=col;
			columnstoignore[1]=NULL;
			columnstoignorecount=1;
			stringbuffer	opt;
			opt.append("IGNORE ");
			opt.append(col);
			opt.append(" column - ");
			option=opt.detachString();
			ignorecolumns=false;
		} else if (iteration>=22 && iteration<=32) {
			// for iterations 22-21, exclude random sets of
			// columns, possibly with repetitions
			randomnumber	r;
			r.setSeed(randomnumber::getSeed());
			stringbuffer	opt;
			opt.append("IGNORE ");
			columnstoignore=new const char *[11];
			for (uint8_t i=0; i<10; i++) {
				uint32_t	rn;
				r.generate(&rn);
				r.setSeed(rn);
				rn=r.scale(rn,0,19);
				const char	*col=columns[rn];
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
