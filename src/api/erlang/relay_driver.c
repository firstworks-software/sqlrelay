#include <ei.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlrelay/sqlrclientwrapper.h>

#define BUF_SIZE 2000 
#define COL_NAME_SIZE 512 	// max length of column names
#define FILE_NAME_SIZE 512 	// max length of file name
#define TRUE 0
#define FALSE 1
#define DEBUG 0		// set to 1 for debugging, otherwise set to 0

// define error codes
#define ERR_BINARY_TERM 	-1	
#define ERR_PROTOCOL 		-2	
#define ERR_NUMBER_OF_ARGS 	-3
#define ERR_DECODING_ARGS	-4
#define ERR_ENCODING_ARGS	-5
#define ERR_PREPARING_RESULTS	-6
#define ERR_PROCESSING_QUERY	-7
#define ERR_ROW_OUT_OF_RANGE	-8 
#define ERR_COL_OUT_OF_RANGE	-9 

// type definitions
typedef char byte;
typedef unsigned long ulong;

// global variables
sqlrcon con;
sqlrcur cur=NULL;

// define functions
int read_cmd(byte **buf, int *size);
int write_cmd(ei_x_buff* x);
int read_exact(byte *buf, int len);
int write_exact(byte *buf, int len);
int rowCount();
int colCount();

#define ENCODE_VOID 	if (ei_x_encode_atom(&result, "ok") || ei_x_encode_string(&result, "void")) { return ERR_ENCODING_ARGS; }

// Encode a C string that may be NULL.  A NULL pointer becomes the
// Erlang atom 'undefined' so callers can distinguish it from an empty
// string (getNullsAsNulls vs getNullsAsEmptyStrings).
#define ENCODE_STRING_OR_UNDEFINED(b, s) \
	((s) ? ei_x_encode_string(b, s) : ei_x_encode_atom(b, "undefined"))

// Length-aware variant: preserves embedded nulls so callers can receive
// binary data (e.g. BLOB contents) without truncation at the first 0 byte.
#define ENCODE_BYTES_OR_UNDEFINED(b, s, len) \
	((s) ? ei_x_encode_string_len(b, s, len) : ei_x_encode_atom(b, "undefined"))


/*-----------------------------------------------------------------
 * Utility functions
 *----------------------------------------------------------------*/

// check that the given row is within the limits of the cursor.
// Only the lower bound is enforced — the upper bound depends on
// the result set buffer size / streaming state and the client lib
// scrolls the buffer forward automatically, returning NULL past the
// actual end of results.
int checkRowLimitsOK(int row) {
	if (row < 0) {
		return FALSE;
	} else {
		return TRUE;
	}
}

// check that the given column is within the limits of the cursor
int checkColLimitsOK(int col) {
	if (col < 0 || col >= colCount(cur)) {
		return FALSE;
	} else {
		return TRUE;
	}
}

void signalError(ei_x_buff *result, long err) {
	ei_x_encode_atom(result, "error");
	ei_x_encode_long(result, err); 
}

/*-----------------------------------------------------------------
 * API functions
 *----------------------------------------------------------------*/

long alloc(char *server, ulong port, char *socket, char *user, char *password, ulong retrytime, ulong tries) {
	if (DEBUG) {
		fprintf(stderr, "Processing alloc with arguments: %s, %ld, %s, %s, %s, %ld, %ld\n\r", server, port, socket, user, password, retrytime, tries);
	}

	// Use copyrefs=1 so the C++ library duplicates any string
	// arguments we pass in later (e.g. enableTls cert/ca paths,
	// inputBindString values).  Without this, the library stores
	// raw pointers into the driver's stack buffers and they go
	// stale on subsequent commands.
	con = sqlrcon_alloc_copyrefs(server, port, socket, user, password,
	                             retrytime, tries, 1);

	return 0;
}



// Lazily allocate the cursor the first time it's needed, then reuse
// it.  Allocating a fresh cursor on every send/prepare leaked the
// previous cursor (and its server-side cursor handle), which exhausted
// backend cursor pools after a small number of iterations.
static void ensureCursor() {
	if (!cur) {
		cur = sqlrcur_alloc_copyrefs(con, 1);
	}
}

int sendQuery(char *sql) {
	if (DEBUG) {
		fprintf(stderr, "Processing sendQuery %s\n\r", sql);
	}

	ensureCursor();
	return(sqlrcur_sendQuery(cur, sql));
}

int sendQueryWithLength(char *sql, uint32_t length) {
	ensureCursor();
	return(sqlrcur_sendQueryWithLength(cur, sql, length));
}

int sendFileQuery(char *path, char *filename) {
	ensureCursor();
	return(sqlrcur_sendFileQuery(cur, path, filename));
}

int prepareQuery(char *sql) {
	if (DEBUG) {
		fprintf(stderr, "Processing prepareQuery %s\n\r", sql);
	}

	ensureCursor();
	sqlrcur_prepareQuery(cur, sql);
	return 0;
}

int prepareQueryWithLength(char *sql, uint32_t length) {
	ensureCursor();
	sqlrcur_prepareQueryWithLength(cur, sql, length);
	return 0;
}

int prepareFileQuery(char *path, char *filename) {
	ensureCursor();
	sqlrcur_prepareFileQuery(cur, path, filename);
	return 0;
}



int colCount() {
	return (sqlrcur_colCount(cur)); 	
}

int rowCount() {
	return (sqlrcur_rowCount(cur)); 	
}





/*-----------------------------------------------------------------
 * MAIN
 *----------------------------------------------------------------*/
int main() {
  	byte 	  *buf;
  	int       size = BUF_SIZE;
  	char      command[MAXATOMLEN];
  	int       index, version, arity;
	int 	  err;
  	ei_x_buff result;

  	if ((buf = (byte *) malloc(size)) == NULL) 
    		return -1;
   
	// main loop 
  	while (read_cmd(&buf, &size) > 0) {
    		// Reset the index, so that ei functions can decode terms from the  beginning of the buffer 
    		index = 0;

    		// Ensure that we are receiving the binary term by reading and stripping the version byte 
    		if (ei_decode_version(buf, &index, &version)) {
			return ERR_BINARY_TERM;
		}
    
    		// Get the number of arguments
    		if (ei_decode_tuple_header(buf, &index, &arity)) {
			return ERR_PROTOCOL;
		} else {
			--arity;	// remove the command name from the argument count
		}

		// Get the command  
    		if (ei_decode_atom(buf, &index, command)) {
			return ERR_PROTOCOL;
		}
 
    		// Prepare the output buffer that will hold {ok, Result} or {error, Reason} 
    		if (ei_x_new_with_version(&result) || ei_x_encode_tuple_header(&result, 2)) {
			return ERR_PREPARING_RESULTS;
		}


		// 
		// process command
		//	
		if (DEBUG) {
			fprintf(stderr, "Received command %s, number of arguments is %d\n\r", command, arity);
   		}
		
		if (strcmp("alloc", command) == TRUE) {
			char server[30]; 
                	unsigned long port; 
                	char socket[30];
                	char user[30]; 
                	char password[30]; 
                	unsigned long retrytime; 
                	unsigned long tries; 			
			long c;

			// check number of arguments
		    	if (arity != 7) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &server[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_ulong(buf, &index, &port)) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &socket[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &user[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &password[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_ulong(buf, &index, &retrytime)) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_ulong(buf, &index, &tries)) { 
				return ERR_DECODING_ARGS;
			}
       
			// call function 
			c = alloc(server, port, socket, user, password, retrytime, tries);
     
			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, c)) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("ping", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcon_ping(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("connectionFree", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result
			if (con) {
				sqlrcon_free(con);
				con = NULL;
			}
			ENCODE_VOID;
		}
		
		if (strcmp("cursorFree", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result
			if (cur) {
				sqlrcur_free(cur);
				cur = NULL;
			}
			ENCODE_VOID;
		}

		if (strcmp("setConnectTimeout", command) == TRUE) {
                	long timeoutsec;
                	long timeoutusec;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			if (ei_decode_long(buf, &index, &timeoutsec)) { 
				return ERR_DECODING_ARGS;
			}

			if (ei_decode_long(buf, &index, &timeoutusec)) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			sqlrcon_setConnectTimeout(con,timeoutsec,timeoutusec);
			ENCODE_VOID;   
		}

		if (strcmp("getConnectTimeoutSeconds", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcon_getConnectTimeoutSeconds(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getConnectTimeoutMicroseconds", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcon_getConnectTimeoutMicroseconds(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("setResponseTimeout", command) == TRUE) {
                	long timeoutsec;
                	long timeoutusec;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			if (ei_decode_long(buf, &index, &timeoutsec)) { 
				return ERR_DECODING_ARGS;
			}

			if (ei_decode_long(buf, &index, &timeoutusec)) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			sqlrcon_setResponseTimeout(con,timeoutsec,timeoutusec);
			ENCODE_VOID;
		}

		if (strcmp("getResponseTimeoutSeconds", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcon_getResponseTimeoutSeconds(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getResponseTimeoutMicroseconds", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcon_getResponseTimeoutMicroseconds(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("setBindVariableDelimiters", command) == TRUE) {
			char delimiters[128];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &delimiters[0])) { 
				return ERR_DECODING_ARGS;
			}

			sqlrcon_setBindVariableDelimiters(con, delimiters);
			ENCODE_VOID;
		}

		if (strcmp("getBindVariableDelimiterQuestionMarkSupported", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcon_getBindVariableDelimiterQuestionMarkSupported(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getBindVariableDelimiterColonSupported", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcon_getBindVariableDelimiterColonSupported(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getBindVariableDelimiterAtSignSupported", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcon_getBindVariableDelimiterAtSignSupported(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getBindVariableDelimiterDollarSignSupported", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcon_getBindVariableDelimiterDollarSignSupported(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("enableKerberos", command) == TRUE) {
                	char service[128];
			char mech[128];
			char flags[512];

			// check number of arguments
		    	if (arity != 3) return ERR_NUMBER_OF_ARGS;

			// get arguments
			if (ei_decode_string(buf, &index, &service[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &mech[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &flags[0])) { 
				return ERR_DECODING_ARGS;
			}

			// Convert empty strings to NULL so the underlying C++
			// library applies its "use defaults" behavior.
			sqlrcon_enableKerberos(con,
				service[0] ? service : NULL,
				mech[0] ? mech : NULL,
				flags[0] ? flags : NULL);
			ENCODE_VOID;
		}

		if (strcmp("enableTls", command) == TRUE) {
                	char version[128];
                	char cert[1024];
			char password[128];
			char ciphers[1024];
			char validate[16];
			char ca[1024];
			long depth;

			// check number of arguments
		    	if (arity != 7) return ERR_NUMBER_OF_ARGS;

			// get arguments
			if (ei_decode_string(buf, &index, &version[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &cert[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &password[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &ciphers[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &validate[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &ca[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &depth)) { 
				return ERR_DECODING_ARGS;
			}

			// Convert empty strings to NULL so the underlying C++
			// library applies its "use defaults" behavior (matches
			// how the cs binding passes (String)null).
			sqlrcon_enableTls(con,
				version[0] ? version : NULL,
				cert[0] ? cert : NULL,
				password[0] ? password : NULL,
				ciphers[0] ? ciphers : NULL,
				validate[0] ? validate : NULL,
				ca[0] ? ca : NULL,
				depth);
			ENCODE_VOID;
		}

		if (strcmp("disableEncryption", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			sqlrcon_disableEncryption(con);
			ENCODE_VOID;   
		}

		if (strcmp("endSession", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result
			if (con) {
				sqlrcon_endSession(con);
			}
			ENCODE_VOID;
		}

		if (strcmp("suspendSession", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result,
					sqlrcon_suspendSession(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getConnectionPort", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcon_getConnectionPort(con))) {
				return ERR_ENCODING_ARGS;
			}
		}
		
		if (strcmp("getConnectionSocket", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_string(&result, sqlrcon_getConnectionSocket(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("resumeSession", command) == TRUE) {
			unsigned long port;
			int vtype, vsize;
			char *socket;
			int rs;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get arguments
			if (ei_decode_ulong(buf, &index, &port)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_get_type(buf, &index, &vtype, &vsize)) {
				return ERR_DECODING_ARGS;
			}
			socket = (char *) malloc(vsize + 1);
			if (!socket) return ERR_DECODING_ARGS;
			if (ei_decode_string(buf, &index, socket)) {
				free(socket);
				return ERR_DECODING_ARGS;
			}

			// encode result
			rs = sqlrcon_resumeSession(con, port, socket);
			free(socket);
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, rs)) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("identify", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_string(&result, sqlrcon_identify(con) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("dbVersion", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_string(&result, sqlrcon_dbVersion(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("dbHostName", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_string(&result, sqlrcon_dbHostName(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("dbIpAddress", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_string(&result, sqlrcon_dbIpAddress(con))) {
				return ERR_ENCODING_ARGS;
			}
		}
		
		if (strcmp("serverVersion", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_string(&result, sqlrcon_serverVersion(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("clientVersion", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_string(&result, sqlrcon_clientVersion(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("bindFormat", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_string(&result, sqlrcon_bindFormat(con))) {
				return ERR_ENCODING_ARGS;
			}
		}
		
		if (strcmp("nextvalFormat", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_string(&result, sqlrcon_nextvalFormat(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("selectDatabase", command) == TRUE) {
			char database[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &database[0])) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			sqlrcon_selectDatabase(con, database); 	
			ENCODE_VOID;   
		}
		
		if (strcmp("getCurrentDatabase", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_string(&result, sqlrcon_getCurrentDatabase(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("selectCatalog", command) == TRUE) {
			char catalog[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &catalog[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			sqlrcon_selectCatalog(con, catalog);
			ENCODE_VOID;
		}

		if (strcmp("getCurrentCatalog", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_string(&result, sqlrcon_getCurrentCatalog(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("selectSchema", command) == TRUE) {
			char schema[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &schema[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			sqlrcon_selectSchema(con, schema);
			ENCODE_VOID;
		}

		if (strcmp("getCurrentSchema", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_string(&result, sqlrcon_getCurrentSchema(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getDatabaseIsSchema", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcon_getDatabaseIsSchema(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getCurrentUser", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_string(&result, sqlrcon_getCurrentUser(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getLastInsertId", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcon_getLastInsertId(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("autoCommitOn", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcon_autoCommitOn(con))) {
				return ERR_ENCODING_ARGS;
			}
		}
		
		if (strcmp("autoCommitOff", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcon_autoCommitOff(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getAutoCommit", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcon_getAutoCommit(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("beginTransaction", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcon_begin(con))) {
				return ERR_ENCODING_ARGS;
			}
		}
		
		if (strcmp("commit", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcon_commit(con))) {
				return ERR_ENCODING_ARGS;
			}
		}
		
		if (strcmp("rollback", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcon_rollback(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getDefaultTransactionModel", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_string(&result, sqlrcon_getDefaultTransactionModel(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("setTransactionModel", command) == TRUE) {
			char txmodel[256];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &txmodel[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcon_setTransactionModel(con, txmodel))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getTransactionModel", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_string(&result, sqlrcon_getTransactionModel(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("setIsolationLevel", command) == TRUE) {
			char isolationlevel[256];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &isolationlevel[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcon_setIsolationLevel(con, isolationlevel))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getIsolationLevel", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_string(&result, sqlrcon_getIsolationLevel(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getDatabaseFeature", command) == TRUE) {
			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get the arguments
			char feature[256];
			if (ei_decode_string(buf, &index, feature)) return ERR_DECODING_ARGS;

			// encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_string(&result, sqlrcon_getDatabaseFeature(con, feature))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("connectionErrorMessage", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result
			const char *emsg = sqlrcon_errorMessage(con);
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_string(&result, emsg ? emsg : "")) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("connectionErrorNumber", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcon_errorNumber(con) )) {
				return ERR_ENCODING_ARGS;
			}
		}
		
		if (strcmp("debugOn", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			sqlrcon_debugOn(con); 	
			ENCODE_VOID;   
		}
		
		if (strcmp("debugOff", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			sqlrcon_debugOff(con); 	
			ENCODE_VOID;   
		}
		
		if (strcmp("getDebug", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcon_getDebug(con))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("setDebugFile", command) == TRUE) {
			char debugfile[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &debugfile[0])) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			sqlrcon_setDebugFile(con, debugfile); 	
			ENCODE_VOID;   
		}

		if (strcmp("setClientInfo", command) == TRUE) {
			char clientinfo[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &clientinfo[0])) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			sqlrcon_setClientInfo(con, clientinfo); 	
			ENCODE_VOID;   
		}
		
		if (strcmp("getClientInfo", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_string(&result, sqlrcon_getClientInfo(con))) {
				return ERR_ENCODING_ARGS;
			}
		}
		
		if (strcmp("setResultSetBufferSize", command) == TRUE) {
                	unsigned long rows; 			

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &rows)) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			sqlrcur_setResultSetBufferSize(cur, rows); 	
			ENCODE_VOID;   
		}
		
		if (strcmp("getResultSetBufferSize", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_getResultSetBufferSize(cur))) {
				return ERR_ENCODING_ARGS;
			}
		}
		
		if (strcmp("dontGetColumnInfo", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			sqlrcur_dontGetColumnInfo(cur); 	
			ENCODE_VOID;   
		}
		
		if (strcmp("getColumnInfo", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			sqlrcur_getColumnInfo(cur); 	
			ENCODE_VOID;   
		}

		if (strcmp("mixedCaseColumnNames", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			sqlrcur_mixedCaseColumnNames(cur); 	
			ENCODE_VOID;   
		}

		if (strcmp("upperCaseColumnNames", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			sqlrcur_upperCaseColumnNames(cur); 	
			ENCODE_VOID;   
		}
		
		if (strcmp("lowerCaseColumnNames", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			sqlrcur_lowerCaseColumnNames(cur); 	
			ENCODE_VOID;   
		}
		
		if (strcmp("cacheToFile", command) == TRUE) {
			char filename[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &filename[0])) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			sqlrcur_cacheToFile(cur, filename); 	
			ENCODE_VOID;   
		}
		
		if (strcmp("setCacheTtl", command) == TRUE) {
                	unsigned long ttl; 			

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &ttl)) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			sqlrcur_setCacheTtl(cur, ttl); 	
			ENCODE_VOID;   
		}

		if (strcmp("getCacheFileName", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ENCODE_STRING_OR_UNDEFINED(&result, sqlrcur_getCacheFileName(cur))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getDatabaseList", command) == TRUE) {
			char databases[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &databases[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			sqlrcur_getDatabaseList(cur, databases);
			ENCODE_VOID;
		}

		if (strcmp("getCatalogList", command) == TRUE) {
			char catalogs[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &catalogs[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result,
					sqlrcur_getCatalogList(cur, catalogs))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getSchemaList", command) == TRUE) {
			char schemas[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &schemas[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result,
					sqlrcur_getSchemaList(cur, schemas))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getTableTypeList", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result,
					sqlrcur_getTableTypeList(cur))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getTableList", command) == TRUE) {
			char tables[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &tables[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result,
					sqlrcur_getTableList(cur, tables))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getTypeInfoList", command) == TRUE) {
			char type[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &type[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result,
					sqlrcur_getTypeInfoList(cur, type))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getColumnList", command) == TRUE) {
			char table[2000];
			char columns[2000];

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &table[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &columns[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result,
					sqlrcur_getColumnList(cur, table, columns))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getPrimaryKeysList", command) == TRUE) {
			char table[2000];
			char columns[2000];

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &table[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &columns[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result,
					sqlrcur_getPrimaryKeysList(cur, table, columns))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getKeyAndIndexList", command) == TRUE) {
			char table[2000];
			char qualifier[2000];

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &table[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &qualifier[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result,
					sqlrcur_getKeyAndIndexList(cur, table, qualifier))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getProcedureList", command) == TRUE) {
			char procedures[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &procedures[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result,
					sqlrcur_getProcedureList(cur, procedures))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getProcedureParameterList", command) == TRUE) {
			char procedure[2000];
			char parameters[2000];

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &procedure[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &parameters[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result,
					sqlrcur_getProcedureParameterList(cur, procedure, parameters))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("cacheOff", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			sqlrcur_cacheOff(cur); 	
			ENCODE_VOID;   
		}


		if (strcmp("sendQuery", command) == TRUE) {
			int sqltype, sqlsize;

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// size the sql buffer from the incoming term so that
			// queries longer than a fixed 2000-byte stack buffer
			// don't overflow.
			if (ei_get_type(buf, &index, &sqltype, &sqlsize)) {
				return ERR_DECODING_ARGS;
			}
			{
				char *sql = (char *)malloc(sqlsize + 1);
				long rc;
				if (!sql) return ERR_DECODING_ARGS;
				if (ei_decode_string(buf, &index, sql)) {
					free(sql);
					return ERR_DECODING_ARGS;
				}
				rc = sendQuery(sql);
				free(sql);
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, rc)) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("sendQueryWithLength", command) == TRUE) {
			int sqltype, sqlsize;
			long length;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			if (ei_get_type(buf, &index, &sqltype, &sqlsize)) {
				return ERR_DECODING_ARGS;
			}
			{
				char *sql = (char *)malloc(sqlsize + 1);
				long rc;
				if (!sql) return ERR_DECODING_ARGS;
				if (ei_decode_string(buf, &index, sql)) {
					free(sql);
					return ERR_DECODING_ARGS;
				}
				if (ei_decode_long(buf, &index, &length)) {
					free(sql);
					return ERR_DECODING_ARGS;
				}
				rc = sendQueryWithLength(sql, length);
				free(sql);
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, rc)) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("sendFileQuery", command) == TRUE) {
			char path[2000];
			char filename[2000];

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &path[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &filename[0])) { 
				return ERR_DECODING_ARGS;
			}

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sendFileQuery(path, filename))) {
				return ERR_ENCODING_ARGS;
			}
		}
		
		if (strcmp("prepareQuery", command) == TRUE) {
			int sqltype, sqlsize;

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			if (ei_get_type(buf, &index, &sqltype, &sqlsize)) {
				return ERR_DECODING_ARGS;
			}
			{
				char *sql = (char *)malloc(sqlsize + 1);
				long rc;
				if (!sql) return ERR_DECODING_ARGS;
				if (ei_decode_string(buf, &index, sql)) {
					free(sql);
					return ERR_DECODING_ARGS;
				}
				rc = prepareQuery(sql);
				free(sql);
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, rc)) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("prepareQueryWithLength", command) == TRUE) {
			int sqltype, sqlsize;
			long length;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			if (ei_get_type(buf, &index, &sqltype, &sqlsize)) {
				return ERR_DECODING_ARGS;
			}
			{
				char *sql = (char *)malloc(sqlsize + 1);
				long rc;
				if (!sql) return ERR_DECODING_ARGS;
				if (ei_decode_string(buf, &index, sql)) {
					free(sql);
					return ERR_DECODING_ARGS;
				}
				if (ei_decode_long(buf, &index, &length)) {
					free(sql);
					return ERR_DECODING_ARGS;
				}
				rc = prepareQueryWithLength(sql, length);
				free(sql);
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, rc)) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("prepareFileQuery", command) == TRUE) {
			char path[2000];
			char filename[2000];

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &path[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &filename[0])) { 
				return ERR_DECODING_ARGS;
			}

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, prepareFileQuery(path, filename) )) {
				return ERR_ENCODING_ARGS;
			}
		}


		if (strcmp("subString", command) == TRUE) {
			char variable[2000];
			char value[2000];

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &value[0])) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			sqlrcur_subString(cur, variable, value); 	
			ENCODE_VOID;
		}


		if (strcmp("subLong", command) == TRUE) {
			char variable[2000];
			long value;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &value)) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			sqlrcur_subLong(cur, variable, value); 	
			ENCODE_VOID;
		}

		if (strcmp("subDouble", command) == TRUE) {
			char variable[2000];
			double value;
			long precision;
			long scale;

			// check number of arguments
		    	if (arity != 4) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_double(buf, &index, &value)) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &precision)) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &scale)) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			sqlrcur_subDouble(cur, variable, value, precision, scale); 	
			ENCODE_VOID;
		}

		if (strcmp("clearBinds", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			sqlrcur_clearBinds(cur); 	
			ENCODE_VOID;
		}

		if (strcmp("countBindVariables", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_countBindVariables(cur) )) {
				return ERR_ENCODING_ARGS;
			}
		}


		if (strcmp("inputBindString", command) == TRUE) {
			char variable[2000];
			char value[2000];

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &value[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			sqlrcur_inputBindString(cur, variable, value);
			ENCODE_VOID;
		}

		if (strcmp("inputBindNull", command) == TRUE) {
			char variable[2000];

			// check number of arguments
			if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) {
				return ERR_DECODING_ARGS;
			}

			// pass NULL value to bind a NULL
			sqlrcur_inputBindString(cur, variable, NULL);
			ENCODE_VOID;
		}

		if (strcmp("inputBindStringWithLength", command) == TRUE) {
			char variable[2000];
			char value[2000];
			long length;

			// check number of arguments
		    	if (arity != 3) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &value[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &length)) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			sqlrcur_inputBindStringWithLength(cur, variable, value, length); 	
			ENCODE_VOID;
		}


		if (strcmp("inputBindLong", command) == TRUE) {
			char variable[2000];
			long value;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &value)) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			sqlrcur_inputBindLong(cur, variable, value); 	
			ENCODE_VOID;
		}

		if (strcmp("inputBindDouble", command) == TRUE) {
			char variable[2000];
			double value;
			long precision;
			long scale;

			// check number of arguments
		    	if (arity != 4) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_double(buf, &index, &value)) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &precision)) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &scale)) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			sqlrcur_inputBindDouble(cur, variable, value, precision, scale); 	
			ENCODE_VOID;
		}

		if (strcmp("inputBindBlob", command) == TRUE) {
			char variable[2000];
			int vtype, vsize;
			long size;

			// check number of arguments
		    	if (arity != 3) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) {
				return ERR_DECODING_ARGS;
			}
			// size the value buffer from the incoming term so
			// large blobs (e.g. 8 KB+) don't overflow a fixed
			// 2000-byte stack array.
			if (ei_get_type(buf, &index, &vtype, &vsize)) {
				return ERR_DECODING_ARGS;
			}
			{
				char *value = (char *)malloc(vsize + 1);
				if (!value) return ERR_DECODING_ARGS;
				if (ei_decode_string(buf, &index, value)) {
					free(value);
					return ERR_DECODING_ARGS;
				}
				if (ei_decode_long(buf, &index, &size)) {
					free(value);
					return ERR_DECODING_ARGS;
				}
				sqlrcur_inputBindBlob(cur, variable, value, size);
				free(value);
			}
			ENCODE_VOID;
		}

		if (strcmp("inputBindClob", command) == TRUE) {
			char variable[2000];
			int vtype, vsize;
			long size;

			// check number of arguments
		    	if (arity != 3) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_get_type(buf, &index, &vtype, &vsize)) {
				return ERR_DECODING_ARGS;
			}
			{
				char *value = (char *)malloc(vsize + 1);
				if (!value) return ERR_DECODING_ARGS;
				if (ei_decode_string(buf, &index, value)) {
					free(value);
					return ERR_DECODING_ARGS;
				}
				if (ei_decode_long(buf, &index, &size)) {
					free(value);
					return ERR_DECODING_ARGS;
				}
				sqlrcur_inputBindClob(cur, variable, value, size);
				free(value);
			}
			ENCODE_VOID;
		}

		if (strcmp("inputBindDate", command) == TRUE) {
			char variable[2000];
			long year;
			long month;
			long day;
			long hour;
			long minute;
			long second;
			long microsecond;
			char tz[2000];
			long isnegative;

			// check number of arguments
		    	if (arity != 10) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &year)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &month)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &day)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &hour)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &minute)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &second)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &microsecond)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &tz[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &isnegative)) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			sqlrcur_inputBindDate(cur, variable,
				year, month, day,
				hour, minute, second,
				microsecond, tz, isnegative);
			ENCODE_VOID;
		}

		if (strcmp("defineOutputBindString", command) == TRUE) {
			char variable[2000];
			long length;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &length)) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			sqlrcur_defineOutputBindString(cur, variable, length); 	
			ENCODE_VOID;
		}

		if (strcmp("defineOutputBindInteger", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			sqlrcur_defineOutputBindInteger(cur, variable); 	
			ENCODE_VOID;
		}

		if (strcmp("defineOutputBindDouble", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			sqlrcur_defineOutputBindDouble(cur, variable); 	
			ENCODE_VOID;
		}

		if (strcmp("defineOutputBindDate", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			sqlrcur_defineOutputBindDate(cur, variable);
			ENCODE_VOID;
		}

		if (strcmp("defineOutputBindBlob", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			sqlrcur_defineOutputBindBlob(cur, variable);
			ENCODE_VOID;
		}

		if (strcmp("defineOutputBindClob", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			sqlrcur_defineOutputBindClob(cur, variable); 	
			ENCODE_VOID;
		}
		
		if (strcmp("defineOutputBindCursor", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			sqlrcur_defineOutputBindCursor(cur, variable); 	
			ENCODE_VOID;
		}

		if (strcmp("validateBinds", command) == TRUE) {

			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			sqlrcur_validateBinds(cur); 	
			ENCODE_VOID;
		}

		if (strcmp("validBind", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_validBind(cur, variable) )) {
				return ERR_ENCODING_ARGS;
			}
		}


		if (strcmp("executeQuery", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_executeQuery(cur) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("fetchFromBindCursor", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_fetchFromBindCursor(cur) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getOutputBindString", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ENCODE_STRING_OR_UNDEFINED(&result, sqlrcur_getOutputBindString(cur, variable))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getOutputBindBlob", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			{
				const char *bval = sqlrcur_getOutputBindBlob(cur, variable);
				uint32_t blen = sqlrcur_getOutputBindLength(cur, variable);
				if (ei_x_encode_atom(&result, "ok") ||
					ENCODE_BYTES_OR_UNDEFINED(&result, bval, blen)) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getOutputBindClob", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			{
				const char *cval = sqlrcur_getOutputBindClob(cur, variable);
				uint32_t clen = sqlrcur_getOutputBindLength(cur, variable);
				if (ei_x_encode_atom(&result, "ok") ||
					ENCODE_BYTES_OR_UNDEFINED(&result, cval, clen)) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getOutputBindInteger", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_getOutputBindInteger(cur, variable) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getOutputBindDouble", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_double(&result, sqlrcur_getOutputBindDouble(cur, variable) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getOutputBindLength", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcur_getOutputBindLength(cur, variable) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getOutputBindDateYear", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcur_getOutputBindDateYear(cur, variable) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getOutputBindDateMonth", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcur_getOutputBindDateMonth(cur, variable) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getOutputBindDateDay", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcur_getOutputBindDateDay(cur, variable) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getOutputBindDateHour", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcur_getOutputBindDateHour(cur, variable) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getOutputBindDateMinute", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcur_getOutputBindDateMinute(cur, variable) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getOutputBindDateSecond", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcur_getOutputBindDateSecond(cur, variable) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getOutputBindDateMicrosecond", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcur_getOutputBindDateMicrosecond(cur, variable) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getOutputBindDateTz", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ENCODE_STRING_OR_UNDEFINED(&result, sqlrcur_getOutputBindDateTz(cur, variable))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getOutputBindDateIsNegative", command) == TRUE) {
			char variable[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &variable[0])) {
				return ERR_DECODING_ARGS;
			}

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcur_getOutputBindDateIsNegative(cur, variable) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("openCachedResultSet", command) == TRUE) {
			char filename[2000];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &filename[0])) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_openCachedResultSet(cur, filename) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("colCount", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, colCount() )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("rowCount", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, rowCount() )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("totalRows", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_totalRows(cur) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("affectedRows", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_affectedRows(cur) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("firstRowIndex", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_firstRowIndex(cur) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("endOfResultSet", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcur_endOfResultSet(cur) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("nextResultSet", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcur_nextResultSet(cur) )) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("errorMessage", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result
			const char *emsg = sqlrcur_errorMessage(cur);
			if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_string(&result, emsg ? emsg : "")) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("errorNumber", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_errorNumber(cur) )) {
				return ERR_ENCODING_ARGS;
			}
		}
		
		if (strcmp("getNullsAsEmptyStrings", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			sqlrcur_getNullsAsEmptyStrings(cur); 	
			ENCODE_VOID;
		}

		if (strcmp("getNullsAsNulls", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			sqlrcur_getNullsAsNulls(cur); 	
			ENCODE_VOID;
		}

		if (strcmp("getFieldByIndex", command) == TRUE) {
			long row;
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of row and col values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE; 
			}
			if (checkColLimitsOK(col) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE; 
			}
	
			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				const char *field = sqlrcur_getFieldByIndex(cur, row, col);
				uint32_t flen = sqlrcur_getFieldLengthByIndex(cur, row, col);
				if (ei_x_encode_atom(&result, "ok") ||
					ENCODE_BYTES_OR_UNDEFINED(&result, field, flen)) {
						return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldByName", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE; 
			}
	
			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				const char *field = sqlrcur_getFieldByName(cur, row, col);
				uint32_t flen = sqlrcur_getFieldLengthByName(cur, row, col);
				if (ei_x_encode_atom(&result, "ok") ||
					ENCODE_BYTES_OR_UNDEFINED(&result, field, flen)) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldByNameIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			// Note: no case-insensitive getFieldLength in the C wrapper,
			// so fall back to the NUL-terminated encoder here.  Binary
			// data with embedded NULs will be truncated via this path.
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ENCODE_STRING_OR_UNDEFINED(&result, sqlrcur_getFieldByNameIgnoringCase(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsIntegerByIndex", command) == TRUE) {
			long row;
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of row and col values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE; 
			}
			if (checkColLimitsOK(col) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_getFieldAsIntegerByIndex(cur, row, col) )) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsIntegerByName", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
					ei_x_encode_long(&result, sqlrcur_getFieldAsIntegerByName(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsIntegerByNameIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsIntegerByNameIgnoringCase(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDoubleByIndex", command) == TRUE) {
			long row;
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of row and col values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE; 
			}
			if (checkColLimitsOK(col) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_double(&result, sqlrcur_getFieldAsDoubleByIndex(cur, row, col) )) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDoubleByName", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
					ei_x_encode_double(&result, sqlrcur_getFieldAsDoubleByName(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDoubleByNameIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_double(&result, sqlrcur_getFieldAsDoubleByNameIgnoringCase(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsBooleanByIndex", command) == TRUE) {
			long row;
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row and col values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}
			if (checkColLimitsOK(col) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
				ei_x_encode_long(&result, sqlrcur_getFieldAsBooleanByIndex(cur, row, col) )) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsBooleanByName", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsBooleanByName(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsBooleanByNameIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsBooleanByNameIgnoringCase(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}


		if (strcmp("getFieldAsDateYearByIndex", command) == TRUE) {
			long row;
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateYearByIndex(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateYearByName", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateYearByName(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateYearByNameIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateYearByNameIgnoringCase(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateYearByIndexWithDdMm", command) == TRUE) {
			long row;
			long col;
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateYearByIndexWithDdMm(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateYearByNameWithDdMm", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateYearByNameWithDdMm(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateYearByNameWithDdMmIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateYearByNameWithDdMmIgnoringCase(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateMonthByIndex", command) == TRUE) {
			long row;
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateMonthByIndex(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateMonthByName", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateMonthByName(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateMonthByNameIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateMonthByNameIgnoringCase(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateMonthByIndexWithDdMm", command) == TRUE) {
			long row;
			long col;
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateMonthByIndexWithDdMm(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateMonthByNameWithDdMm", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateMonthByNameWithDdMm(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateMonthByNameWithDdMmIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateMonthByNameWithDdMmIgnoringCase(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateDayByIndex", command) == TRUE) {
			long row;
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateDayByIndex(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateDayByName", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateDayByName(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateDayByNameIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateDayByNameIgnoringCase(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateDayByIndexWithDdMm", command) == TRUE) {
			long row;
			long col;
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateDayByIndexWithDdMm(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateDayByNameWithDdMm", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateDayByNameWithDdMm(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateDayByNameWithDdMmIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateDayByNameWithDdMmIgnoringCase(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateHourByIndex", command) == TRUE) {
			long row;
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateHourByIndex(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateHourByName", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateHourByName(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateHourByNameIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateHourByNameIgnoringCase(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateHourByIndexWithDdMm", command) == TRUE) {
			long row;
			long col;
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateHourByIndexWithDdMm(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateHourByNameWithDdMm", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateHourByNameWithDdMm(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateHourByNameWithDdMmIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateHourByNameWithDdMmIgnoringCase(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateMinuteByIndex", command) == TRUE) {
			long row;
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateMinuteByIndex(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateMinuteByName", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateMinuteByName(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateMinuteByNameIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateMinuteByNameIgnoringCase(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateMinuteByIndexWithDdMm", command) == TRUE) {
			long row;
			long col;
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateMinuteByIndexWithDdMm(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateMinuteByNameWithDdMm", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateMinuteByNameWithDdMm(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateMinuteByNameWithDdMmIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateMinuteByNameWithDdMmIgnoringCase(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateSecondByIndex", command) == TRUE) {
			long row;
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateSecondByIndex(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateSecondByName", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateSecondByName(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateSecondByNameIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateSecondByNameIgnoringCase(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateSecondByIndexWithDdMm", command) == TRUE) {
			long row;
			long col;
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateSecondByIndexWithDdMm(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateSecondByNameWithDdMm", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateSecondByNameWithDdMm(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateSecondByNameWithDdMmIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateSecondByNameWithDdMmIgnoringCase(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateMicrosecondByIndex", command) == TRUE) {
			long row;
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateMicrosecondByIndex(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateMicrosecondByName", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateMicrosecondByName(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateMicrosecondByNameIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateMicrosecondByNameIgnoringCase(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateMicrosecondByIndexWithDdMm", command) == TRUE) {
			long row;
			long col;
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateMicrosecondByIndexWithDdMm(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateMicrosecondByNameWithDdMm", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateMicrosecondByNameWithDdMm(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateMicrosecondByNameWithDdMmIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateMicrosecondByNameWithDdMmIgnoringCase(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateIsNegativeByIndex", command) == TRUE) {
			long row;
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateIsNegativeByIndex(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateIsNegativeByName", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateIsNegativeByName(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateIsNegativeByNameIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateIsNegativeByNameIgnoringCase(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateIsNegativeByIndexWithDdMm", command) == TRUE) {
			long row;
			long col;
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateIsNegativeByIndexWithDdMm(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateIsNegativeByNameWithDdMm", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateIsNegativeByNameWithDdMm(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldAsDateIsNegativeByNameWithDdMmIgnoringCase", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			long ddmm;
			long yyyyddmm;
			char datedelimiters[256];
			err = 0;

			// check number of arguments
		    	if (arity != 5) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &ddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &yyyyddmm)) {
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &datedelimiters[0])) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				if (ei_x_encode_atom(&result, "ok") ||
					ei_x_encode_long(&result, sqlrcur_getFieldAsDateIsNegativeByNameWithDdMmIgnoringCase(cur, row, col, ddmm, yyyyddmm, datedelimiters))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}


		if (strcmp("getFieldLengthByIndex", command) == TRUE) {
			long row;
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_long(buf, &index, &col)) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of row and col values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE; 
			}
			if (checkColLimitsOK(col) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_getFieldLengthByIndex(cur, row, col) )) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getFieldLengthByName", command) == TRUE) {
			long row;
			char col[COL_NAME_SIZE];
			err = 0;

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &col[0])) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
					ei_x_encode_long(&result, sqlrcur_getFieldLengthByName(cur, row, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getRow", command) == TRUE) {
			long row;
			err = 0;

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				const char * const *fields =
					sqlrcur_getRow(cur, row);
				uint32_t colcount = sqlrcur_colCount(cur);
				if (ei_x_encode_atom(&result, "ok")) {
					return ERR_ENCODING_ARGS;
				}
				if (!fields) {
					if (ei_x_encode_atom(&result,
								"undefined")) {
						return ERR_ENCODING_ARGS;
					}
				} else {
					if (colcount > 0 &&
						ei_x_encode_list_header(
							&result, colcount)) {
						return ERR_ENCODING_ARGS;
					}
					for (uint32_t i = 0; i < colcount;
									i++) {
						if (ei_x_encode_string(
							&result,
							fields[i] ?
							fields[i] : "")) {
							return
							ERR_ENCODING_ARGS;
						}
					}
					if (ei_x_encode_empty_list(
								&result)) {
						return ERR_ENCODING_ARGS;
					}
				}
			}
		}

		if (strcmp("getRowLengths", command) == TRUE) {
			long row;
			err = 0;

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &row)) {
				return ERR_DECODING_ARGS;
			}

			// check sanity of row values
			if (checkRowLimitsOK(row) == FALSE) {
				err = ERR_ROW_OUT_OF_RANGE;
			}

			// encode result
			if (err) {
				signalError(&result, err);
			} else {
				uint32_t *lengths =
					sqlrcur_getRowLengths(cur, row);
				uint32_t colcount = sqlrcur_colCount(cur);
				if (ei_x_encode_atom(&result, "ok")) {
					return ERR_ENCODING_ARGS;
				}
				if (!lengths) {
					if (ei_x_encode_atom(&result,
								"undefined")) {
						return ERR_ENCODING_ARGS;
					}
				} else {
					if (colcount > 0 &&
						ei_x_encode_list_header(
							&result, colcount)) {
						return ERR_ENCODING_ARGS;
					}
					for (uint32_t i = 0; i < colcount;
									i++) {
						if (ei_x_encode_long(
							&result,
							(long)lengths[i])) {
							return
							ERR_ENCODING_ARGS;
						}
					}
					if (ei_x_encode_empty_list(
								&result)) {
						return ERR_ENCODING_ARGS;
					}
				}
			}
		}

		if (strcmp("getColumnNames", command) == TRUE) {
			const char * const *names =
				sqlrcur_getColumnNames(cur);
			uint32_t colcount = sqlrcur_colCount(cur);
			if (ei_x_encode_atom(&result, "ok")) {
				return ERR_ENCODING_ARGS;
			}
			if (!names) {
				if (ei_x_encode_atom(&result,
								"undefined")) {
					return ERR_ENCODING_ARGS;
				}
			} else {
				if (colcount > 0 &&
					ei_x_encode_list_header(
						&result, colcount)) {
					return ERR_ENCODING_ARGS;
				}
				for (uint32_t i = 0; i < colcount; i++) {
					if (ei_x_encode_string(&result,
							names[i] ?
							names[i] : "")) {
						return ERR_ENCODING_ARGS;
					}
				}
				if (ei_x_encode_empty_list(&result)) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getColumnName", command) == TRUE) {
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &col)) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of column value
			if (checkColLimitsOK(col) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
					ENCODE_STRING_OR_UNDEFINED(&result, sqlrcur_getColumnName(cur, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

/***/
		if (strcmp("getColumnTypeByIndex", command) == TRUE) {
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &col)) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of column value
			if (checkColLimitsOK(col) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
					ENCODE_STRING_OR_UNDEFINED(&result, sqlrcur_getColumnTypeByIndex(cur, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getColumnTypeByName", command) == TRUE) {
			char col[COL_NAME_SIZE];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &col[0])) { 
				return ERR_DECODING_ARGS;
			}

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ENCODE_STRING_OR_UNDEFINED(&result, sqlrcur_getColumnTypeByName(cur, col))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getColumnLengthByIndex", command) == TRUE) {
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &col)) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of column value
			if (checkColLimitsOK(col) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
					ei_x_encode_long(&result, sqlrcur_getColumnLengthByIndex(cur, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getColumnLengthByName", command) == TRUE) {
			char col[COL_NAME_SIZE];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &col[0])) { 
				return ERR_DECODING_ARGS;
			}

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_getColumnLengthByName(cur, col))) {
				return ERR_ENCODING_ARGS;
			}
		}


		if (strcmp("getColumnPrecisionByIndex", command) == TRUE) {
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &col)) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of column value
			if (checkColLimitsOK(col) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
					ei_x_encode_long(&result, sqlrcur_getColumnPrecisionByIndex(cur, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getColumnPrecisionByName", command) == TRUE) {
			char col[COL_NAME_SIZE];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &col[0])) { 
				return ERR_DECODING_ARGS;
			}

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_getColumnPrecisionByName(cur, col))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getColumnScaleByIndex", command) == TRUE) {
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &col)) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of column value
			if (checkColLimitsOK(col) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
					ei_x_encode_long(&result, sqlrcur_getColumnScaleByIndex(cur, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getColumnScaleByName", command) == TRUE) {
			char col[COL_NAME_SIZE];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &col[0])) { 
				return ERR_DECODING_ARGS;
			}

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_getColumnScaleByName(cur, col))) {
				return ERR_ENCODING_ARGS;
			}
		}


		if (strcmp("getColumnIsNullableByIndex", command) == TRUE) {
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &col)) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of column value
			if (checkColLimitsOK(col) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
					ei_x_encode_long(&result, sqlrcur_getColumnIsNullableByIndex(cur, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getColumnIsNullableByName", command) == TRUE) {
			char col[COL_NAME_SIZE];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &col[0])) { 
				return ERR_DECODING_ARGS;
			}

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_getColumnIsNullableByName(cur, col))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getColumnIsPrimaryKeyByIndex", command) == TRUE) {
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &col)) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of column value
			if (checkColLimitsOK(col) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
					ei_x_encode_long(&result, sqlrcur_getColumnIsPrimaryKeyByIndex(cur, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getColumnIsPrimaryKeyByName", command) == TRUE) {
			char col[COL_NAME_SIZE];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &col[0])) { 
				return ERR_DECODING_ARGS;
			}

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_getColumnIsPrimaryKeyByName(cur, col))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getColumnIsUniqueByIndex", command) == TRUE) {
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &col)) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of column value
			if (checkColLimitsOK(col) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
					ei_x_encode_long(&result, sqlrcur_getColumnIsUniqueByIndex(cur, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getColumnIsUniqueByName", command) == TRUE) {
			char col[COL_NAME_SIZE];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &col[0])) { 
				return ERR_DECODING_ARGS;
			}

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_getColumnIsUniqueByName(cur, col))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getColumnIsPartOfKeyByIndex", command) == TRUE) {
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &col)) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of column value
			if (checkColLimitsOK(col) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
					ei_x_encode_long(&result, sqlrcur_getColumnIsPartOfKeyByIndex(cur, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getColumnIsPartOfKeyByName", command) == TRUE) {
			char col[COL_NAME_SIZE];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &col[0])) { 
				return ERR_DECODING_ARGS;
			}

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_getColumnIsPartOfKeyByName(cur, col))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("getColumnIsUnsignedByIndex", command) == TRUE) {
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &col)) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of column value
			if (checkColLimitsOK(col) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
					ei_x_encode_long(&result, sqlrcur_getColumnIsUnsignedByIndex(cur, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getColumnIsUnsignedByName", command) == TRUE) {
			char col[COL_NAME_SIZE];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &col[0])) { 
				return ERR_DECODING_ARGS;
			}

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_getColumnIsUnsignedByName(cur, col))) {
				return ERR_ENCODING_ARGS;
			}
		}


		if (strcmp("getColumnIsZeroFilledByIndex", command) == TRUE) {
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &col)) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of column value
			if (checkColLimitsOK(col) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
					ei_x_encode_long(&result, sqlrcur_getColumnIsZeroFilledByIndex(cur, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getColumnIsZeroFilledByName", command) == TRUE) {
			char col[COL_NAME_SIZE];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &col[0])) { 
				return ERR_DECODING_ARGS;
			}

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_getColumnIsZeroFilledByName(cur, col))) {
				return ERR_ENCODING_ARGS;
			}
		}



		if (strcmp("getColumnIsBinaryByIndex", command) == TRUE) {
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &col)) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of column value
			if (checkColLimitsOK(col) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
					ei_x_encode_long(&result, sqlrcur_getColumnIsBinaryByIndex(cur, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getColumnIsBinaryByName", command) == TRUE) {
			char col[COL_NAME_SIZE];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &col[0])) { 
				return ERR_DECODING_ARGS;
			}

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_getColumnIsBinaryByName(cur, col))) {
				return ERR_ENCODING_ARGS;
			}
		}


		if (strcmp("getColumnIsAutoIncrementByIndex", command) == TRUE) {
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &col)) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of column value
			if (checkColLimitsOK(col) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
					ei_x_encode_long(&result, sqlrcur_getColumnIsAutoIncrementByIndex(cur, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getColumnIsAutoIncrementByName", command) == TRUE) {
			char col[COL_NAME_SIZE];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &col[0])) { 
				return ERR_DECODING_ARGS;
			}

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_getColumnIsAutoIncrementByName(cur, col))) {
				return ERR_ENCODING_ARGS;
			}
		}


		if (strcmp("getLongestByIndex", command) == TRUE) {
			long col;
			err = 0;

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &col)) { 
				return ERR_DECODING_ARGS;
			}

			// check sanity of column value
			if (checkColLimitsOK(col) == FALSE) {
				err = ERR_COL_OUT_OF_RANGE; 
			}
	
			// encode result 
			if (err) {
				signalError(&result, err); 
			} else { 
				if (ei_x_encode_atom(&result, "ok") || 
					ei_x_encode_long(&result, sqlrcur_getLongestByIndex(cur, col))) {
					return ERR_ENCODING_ARGS;
				}
			}
		}

		if (strcmp("getLongestByName", command) == TRUE) {
			char col[COL_NAME_SIZE];

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_string(buf, &index, &col[0])) { 
				return ERR_DECODING_ARGS;
			}

			// encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_getLongestByName(cur, col))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("suspendResultSet", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			sqlrcur_suspendResultSet(cur); 	
			ENCODE_VOID;   
		}

		if (strcmp("getResultSetId", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_getResultSetId(cur))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("resumeResultSet", command) == TRUE) {
			long id;

			// check number of arguments
		    	if (arity != 1) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &id)) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_resumeResultSet(cur, id))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("resumeCachedResultSet", command) == TRUE) {
			long id;
			char filename[FILE_NAME_SIZE];

			// check number of arguments
		    	if (arity != 2) return ERR_NUMBER_OF_ARGS;

			// get input parameters
			if (ei_decode_long(buf, &index, &id)) { 
				return ERR_DECODING_ARGS;
			}
			if (ei_decode_string(buf, &index, &filename[0])) { 
				return ERR_DECODING_ARGS;
			}

			// call function and encode result 
			if (ei_x_encode_atom(&result, "ok") || 
				ei_x_encode_long(&result, sqlrcur_resumeCachedResultSet(cur, id, filename))) {
				return ERR_ENCODING_ARGS;
			}
		}

		if (strcmp("closeResultSet", command) == TRUE) {
			// check number of arguments
		    	if (arity != 0) return ERR_NUMBER_OF_ARGS;

			// call function and encode result 
			sqlrcur_closeResultSet(cur); 	
			ENCODE_VOID;   
		}



		// write the result buffer back to the 
		// calling Erlang program    	
		write_cmd(&result);

		// free memory structure
    		ei_x_free(&result);
  	} // end of while statement

  	return 0;
}
