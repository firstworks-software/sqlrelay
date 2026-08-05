# SQL Relay

SQL Relay is a database proxy. It sits between your application and your
database, and gives you connection pooling, throttling, load balancing, query
routing, query filtering, and result set caching.

It has been in active development since 2000, and is used in production to
front Oracle, DB2, Informix, and other databases that are slow to connect to or
expensive to license per connection.

- Project site: http://sqlrelay.sourceforge.net
- Documentation: http://sqlrelay.sourceforge.net/documentation.html
- Requires: [Rudiments](https://github.com/firstworks-software/rudiments)

## What it does

- **Persistent connection pooling.** The proxy holds a pool of open database
  sessions. Your application connects to the proxy instead, so it never pays
  the cost of opening a database connection.
- **Throttling.** Cap how many sessions reach the database, so a traffic spike
  queues instead of overwhelming the server.
- **Load balancing and failover.** Spread sessions across replicas, and route
  around a database that is down.
- **Query routing.** Send queries to different databases based on rules - for
  example, writes to the primary and reads to a replica.
- **Query filtering and translation.** Block queries that match a pattern, or
  rewrite them on the way through.
- **Result set caching.** Serve repeated queries from a cache instead of the
  database.
- **Protocol emulation.** SQL Relay can speak the MySQL and PostgreSQL wire
  protocols. Native clients for those databases can point at SQL Relay
  without any code change.

## Databases

Oracle, MySQL, PostgreSQL, SAP/Sybase, Microsoft SQL Server, DB2, Informix,
Firebird, SQLite, and anything with an ODBC driver.

## Client APIs

C, C++, C#, Erlang, Java, JDBC, Node.js, ODBC, ADO.NET, Perl, Perl DBI, PHP,
PHP PDO, Python, Python DB-API, Ruby, and TCL.

There are also command line tools: sqlrsh for an interactive shell, plus
sqlr-export, sqlr-import, and sqlr-pwdenc.

## Building

Build and install Rudiments first, then:

```
./configure
make
sudo make install
```

The default install prefix is /usr/local/firstworks. Run the following for the
full list of options, including how to disable APIs or backends you do not
need.

```
./configure --help
```

See http://sqlrelay.sourceforge.net/admin/installing.html for the details,
including binary packages and Windows builds.

## How it runs

An SQL Relay instance is three kinds of process:

- sqlr-listener accepts client connections and hands each session to a free
  database connection.
- sqlr-connection holds one pooled database session and runs queries for
  whichever client it is given.
- sqlr-scaler starts and stops sqlr-connection processes as load changes,
  between the minimum and maximum you configure.

## License

Terms differ per component. The server and the command line tools are under the
GNU General Public License version 2, with a special linking exception. The
client libraries and language APIs are under the GNU Library General Public
License version 2, so linking them into a commercial application is fine.

Read [COPYING](COPYING) for the exact terms, and check it before copying code
from one component into another.

## Contributing

Issues, discussions, and pull requests are welcome here on GitHub.

One thing to know before you push: this repository is a mirror. The canonical
repository is the firstworks git server, and both this one and the SourceForge
one are refreshed from it every hour. A pull request is applied on the
canonical repository and then shows up here on the next refresh, so it lands as
an ordinary commit rather than a merge commit.

Please do not push directly to this repository. A direct push makes the mirror
diverge from the canonical repository, and the next refresh fails.
