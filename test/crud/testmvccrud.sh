#!/bin/sh

# mvccrud.cpp connects to oracle, so target that instance here too
CONFIG=../sqlrelay.conf.d/oracle.conf
ID=oracle

sqlrsh -config $CONFIG -id $ID -user testuser -password testpassword -command "drop table testtable"
sqlrsh -config $CONFIG -id $ID -user testuser -password testpassword -command "create table testtable (colstr varchar(128), colint int, colnull varchar(128))"



# create test
echo "======================================================================"
echo "create:"
unset PATH_INFO
unset REQUEST_METHOD
unset CONTENT_TYPE
PATH_INFO="/create"
export PATH_INFO
REQUEST_METHOD="POST"
export REQUEST_METHOD
CONTENT_TYPE="application/json"
export CONTENT_TYPE
./mvccrud.cgi << EOF
{
	"data": {
		"colstr": "val1",
		"colint": 1,
		"colnull": null
	}
}
EOF
echo


# read test
echo "======================================================================"
echo "read:"
unset PATH_INFO
unset REQUEST_METHOD
unset CONTENT_TYPE
PATH_INFO="/read"
export PATH_INFO
REQUEST_METHOD="POST"
export REQUEST_METHOD
CONTENT_TYPE="application/json"
export CONTENT_TYPE
./mvccrud.cgi << EOF
{
	"criteria" : {
		"and" : [
			{ "=" : [
				{ "var": "colstr" },
				"val1"
			] },
			{ "=" : [
				{ "var": "colint" },
				1
			] },
			{ "isnull" : [
				{ "var": "colnull" }
			] }
		]
	},
	"sort": {
		"colstr" : "asc",
		"colint" : "asc",
		"colnull" : "asc"
	}
}
EOF
echo


# update test
echo "======================================================================"
echo "update:"
unset PATH_INFO
unset REQUEST_METHOD
unset CONTENT_TYPE
PATH_INFO="/update"
export PATH_INFO
REQUEST_METHOD="POST"
export REQUEST_METHOD
CONTENT_TYPE="application/json"
export CONTENT_TYPE
./mvccrud.cgi << EOF
{
	"criteria" : {
		"=" : [
			{ "var": "colstr" },
			"val1"
		]
	},
	"data": {
		"colstr": "val2",
		"colint": 2,
		"colnull": "not-null"
	}
}
EOF
echo


# read-after-update test
echo "======================================================================"
echo "read-after-update:"
unset PATH_INFO
unset REQUEST_METHOD
unset CONTENT_TYPE
PATH_INFO="/read"
export PATH_INFO
./mvccrud.cgi << EOF
EOF
echo



# delete test
echo "======================================================================"
echo "delete:"
unset PATH_INFO
unset REQUEST_METHOD
unset CONTENT_TYPE
PATH_INFO="/delete"
export PATH_INFO
REQUEST_METHOD="POST"
export REQUEST_METHOD
CONTENT_TYPE="application/json"
export CONTENT_TYPE
./mvccrud.cgi << EOF
{
	"criteria" : {
		"=" : [
			{ "var": "colstr" },
			"val2"
		]
	},
}
EOF
echo


# read-after-delete test
echo "======================================================================"
echo "read-after-delete:"
unset PATH_INFO
unset REQUEST_METHOD
unset CONTENT_TYPE
PATH_INFO="/read"
export PATH_INFO
./mvccrud.cgi << EOF
EOF
echo

echo "======================================================================"
sqlrsh -config $CONFIG -id $ID -user testuser -password testpassword -command "drop table testtable"
