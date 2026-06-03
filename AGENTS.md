# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

SQL Relay: a database proxy providing connection pooling, throttling, load balancing, query routing/filtering, and result-set caching. Server is C++; clients/APIs exist for many languages (C, C++, C#, Java, JDBC, Perl, Perl-DBI, Python, Python-DB, PHP, PHP PDO, Ruby, TCL, Erlang, Node.js, ODBC, ADO.NET). Backends include Oracle, MySQL, PostgreSQL, SAP/Sybase, MS SQL Server, DB2, Informix, Firebird, SQLite, and generic ODBC.

Depends on **rudiments** (>= 2.0.2) — a sibling firstworks library. Autoconf macros (`acfirstworks.m4`, `acrudiments.m4`) are pulled in by `autogen.sh` from `../rudiments/autoconf/` or `/usr/local/firstworks/share/rudiments/autoconf/`.

## Build

Autoconf/libtool build, not autotools-generated-by-make. Install prefix defaults to `/usr/local/firstworks` (see `config.mk:prefix`).

```
./autogen.sh            # regenerate configure from configure.in (rarely needed)
./configure [opts]      # produces config.mk, config.h, libtool, etc.
make                    # top-level; descends into src/
make install            # installs to $prefix
make clean
make distclean          # also removes configure-generated files
make cppcheck           # run cppcheck across src/
```

To install just one component after rebuilding it, run `sudo make install` in
that component's directory (e.g. `cd src/api/odbc && make && sudo make install`)
— per-component Makefiles all expose an `install` target that drops the freshly
built artifacts into `$prefix`.

Common `./configure` toggles (see `configure.in`): `--disable-server`, `--disable-client`, `--disable-cmdline`, `--enable-debug`, `--enable-fortify`, per-API `--disable-{c,c++,perl,python,php,java,ruby,tcl,erlang,cs,nodejs,odbc}`, per-backend detection is automatic via probes.

Top-level `Makefile` delegates to `src/Makefile`, which delegates to per-module directories. Each module directory builds a libtool `.la` plugin (e.g. `src/connections/sqlrconnection_mysql.la`, `src/auths/sqlrauth_mysql_userlist.la`).

### Module registry generation

Plugin "registries" are generated at `configure` time from `*declarations.cpp.in` + `*assignments.cpp.in` templates (see files under `src/server/` and `src/util/`). Configure substitutes in only the modules actually being built. If you add a new auth/filter/logger/router/trigger/translation/protocol/parser/connection/directive/etc. module, you must update `configure.in` so it gets included in the generated registry — editing only the `.cpp.in` template is not enough unless the module is unconditionally built.

### `modulerename.sh`

Used at install time (primarily on OS X) to rename `.so` built modules to `.bundle`. Not relevant on Linux.

## Runtime architecture

Three long-running server processes per instance (all under `src/server/`):

- **sqlr-listener** (`sqlr-listener.cpp` / `sqlrlistener.cpp`) — accepts client connections on a TCP port and/or Unix socket, authenticates, then hands the session off to an available sqlr-connection (either by proxying or by passing the socket fd — see `handoff=` in config).
- **sqlr-connection** (`sqlr-connection.cpp` + `sqlrserverconnection.cpp` + `sqlrservercontroller.cpp`) — one per pooled database session. Holds a live DB connection via a `src/connections/*.cpp` backend module and services queries from whichever client the listener hands it.
- **sqlr-scaler** (`sqlr-scaler.cpp`) — spawns/reaps sqlr-connection processes between `connections=` (min) and `maxconnections=` (max) based on load.

Supporting command-line tools (`src/server/`): `sqlr-start`, `sqlr-stop`, `sqlr-status`, `sqlr-cachemanager`. Client-side command-line tools (`src/cmdline/`): `sqlrsh`, `sqlr-export`, `sqlr-import`, `sqlr-pwdenc`.

## Code layout

- `src/api/` — per-language client libraries. `c/` and `c++/` are the reference implementations (`libsqlrclient`); others (`cs`, `java`, `jdbc`, `nodejs`, `odbc`, `perl`, `php`, `phppdo`, `python`, `ruby`, `tcl`, `erlang`) mostly wrap the C++ API or speak the sqlrclient wire protocol directly.
- `src/server/` — listener/connection/scaler plus the server-side module base classes (`sqlrauth`, `sqlrfilter`, `sqlrlogger`, `sqlrparser`, `sqlrrouter`, `sqlrtrigger`, various `sqlr*translation`, `sqlrdirective`, `sqlrnotification`, `sqlrschedule`, `sqlrquery`, `sqlrmoduledata`, `sqlrprotocol`). Public headers live under `src/server/sqlrelay/`; private headers under `src/server/sqlrelay/private/`.
- `src/connections/` — one `.cpp` per database backend (mysql, postgresql, oracle, db2, firebird, freetds, informix, odbc, sap, sqlite, plus the `router` pseudo-backend). These extend `sqlrserverconnection`.
- `src/protocols/` — wire-protocol frontends. `sqlrclient.cpp` is the native protocol; `mysql`, `postgresql`, `tds`, `oracle`, `teradata`, `firebird` let native clients for those DBs talk to sqlr-listener directly.
- `src/{auths,filters,loggers,notifications,parsers,routers,schedules,triggers,pwdencs,directives,queries,moduledatas}/` — pluggable modules loaded by name from config.
- `src/{querytranslations,bindvariabletranslations,resultsettranslations,resultsetrowtranslations,resultsetrowblocktranslations,resultsetheadertranslations,errortranslations}/` — SQL/result/error rewriting modules applied in the per-session pipeline.
- `src/util/` — `libsqlrutil` (shared config parsing, paths, password encryption framework).
- `src/common/` — headers only (`defines.h` is configure-generated from `defines.h.in`; `version.h`, `datatypes.h`, `defaults.h`, `bindvariables.h`).
- `src/cmdline/` — client-facing command-line tools.
- `etc/sqlrelay.conf`, `etc/sqlrelay.xsd` — example config + XSD.
- `init/` — systemd units / rc scripts / launchd plists (all configure-generated).
- `doc/` — the project website (HTML + `.wt` templates); not generated from source comments. `.wt` files compile to `.html` via `make` in the directory containing them (e.g. `cd doc/programming && make`) — edit the `.wt` and regenerate; don't hand-edit the `.html` only.
- `bin/` — pkg-config-style helper scripts (`sqlr{client,clientwrapper,server}-config`), configure-generated.

## Tests

Tests live under `test/` and run against a **real database** via a live sqlr-listener/sqlr-connection instance that the harness starts and stops. There is no mocking.

```
make tests                        # equivalent to: cd test && make tests
cd test && ./testall.sh           # run everything enabled by configure
cd test && ./test.sh <db>         # run one DB through every enabled API
```

`TESTDBS` (set by `configure.in` around line 1005) is the list of DB/config fixtures to exercise — each corresponds to `test/sqlrelay.conf.d/<name>.conf`. `TESTAPIS` (configure.in ~940) is the list of client-API directories. `testall.sh.in` loops over `$TESTDBS`, starts an sqlr instance per DB with that fixture, pings it, then calls `test.sh <db>` which loops over `$TESTAPIS` and runs `./<db>` (or `./<db>.py`, `./<db>.pl`, etc.) in each.

Per-test-type directories: `c/`, `c++/`, `extensions/` (trigger/router-specific C++ tests — recently split out of `c++/`), `cs/`, `ado.net/`, `erlang/`, `java/`, `jdbc/`, `nodejs/`, `odbc/`, `perl/`, `perldbi/`, `php/`, `phppdo/`, `python/`, `pythondb/`, `ruby/`, `tcl/`, plus `crud/`, `protocol/` (native-protocol emulation tests), `legacy/`, `stress/`, `bench/`.

### Running a single test

C/C++ tests are plain executables built by each per-language Makefile:

```
cd test/c++ && make mysql         # build just the mysql test binary
# start an sqlr instance pointed at test/sqlrelay.conf.d/mysql.conf first
/usr/local/firstworks/bin/sqlr-start -config \
    $(pwd)/../sqlrelay.conf.d/mysql.conf -id mysqltest
./mysql                           # run it
/usr/local/firstworks/bin/sqlr-stop -config \
    $(pwd)/../sqlrelay.conf.d/mysql.conf -id mysqltest
```

For other languages the pattern is the same — the per-API Makefile produces `<db>.{exe,class,jar,py,pl,php,rb,tcl,beam,js}` and `test.sh` decides which runtime invokes it.

**SQL Relay vs. native-driver mode**: the `test/odbc/` and `test/jdbc/` programs can run either through SQL Relay or directly against the backend database — useful for confirming a test reflects spec behavior rather than a SQL Relay quirk.
- ODBC: `./oracle` (through SQL Relay) vs. `./oracle native` (directly against Oracle's ODBC driver). Same pattern for `mysql`, `mssql`, etc.
- JDBC: `./run oracle` vs. `./run oracle native`.

All c++ tests share `test/c++/asserts.cpp` (and `test/crud/asserts.cpp` / `test/extensions/asserts.cpp`) for `assert*()` helpers — prefer those over rolling ad-hoc checks in new tests.

The `freetds` fixture runs the connection module against an **SAP ASE** database (not MS SQL Server), so any SAP-specific test workaround — log management, `sysobjects.user_name(uid)` quirks, Transact-SQL dialect choices, etc. — generally needs to be mirrored between `test/<api>/freetds.*` and `test/<api>/sap.*`.

Result/detail logs land at `test/testresults.log` and `test/testdetails.log` after `testall.sh` runs.

**Write tests to spec, not to the current implementation.** Tests should assert what _ought_ to happen per the relevant API/protocol spec (ODBC, JDBC, wire protocol, etc.), not what the current SQL Relay driver or native backend driver happens to do today. If a test fails, investigate whether the driver is buggy or incomplete before relaxing the assertion — the failing test may be the signal that drove the fix. `issqlrelay` branching is fine when SQL Relay and the native driver genuinely differ _in both being spec-legal_; don't use it to paper over gaps in either implementation.

## Conventions worth knowing

- **C++ standard is C++98.** Don't use C++11+ features in `.cpp` / `.h` files — no `auto` type deduction, no lambdas, no range-based `for`, no `nullptr`, no `std::move`, no `=delete`/`=default`, no strongly-typed enums, no brace-init-list constructors, no variadic templates. Use plain functions (or pass state via structs) instead of lambdas; `NULL` instead of `nullptr`.
- **Copyright header**: every `.cpp` / `.h` starts with `// Copyright (c) David Muse` / `// See the file COPYING for more information`. Preserve it on edits; add it to new files.
- **Comment style.** Match the terse style already in the source. Comments sit on the line(s) *above* the block they describe, never to the right of code, and one comment covers a logical block of several lines — don't annotate every statement. Put comments *inside* the function/method, above the specific block they describe, rather than as one block summarizing the whole function/method from outside; a method-header comment, if any, stays to one short line. There are two registers, and they are different:
    - **Step labels (the common case):** a 2-6 word *lowercase* fragment naming what the next block does, at a higher level than the code itself — e.g. `// log in`, `// detach`, `// clean up`, `// init some variables`, `// run the query...`, `// initialize notification modules`, `// re-init error data`. Not a sentence: no capital, no trailing period, no articles/filler. Don't restate the code.
    - **Why-comments (rare, reserved):** full prose, capitalized and punctuated, used *only* to record something the next reader could not infer from the code — a bug workaround, a version/compatibility quirk, a race condition, or why an obvious approach was avoided. These earn their length; routine code does not get them.
  Default to no comment, then a short label; reach for prose only when there is a genuine non-obvious *why*. If a label would just echo the code (`// increment i` over `i++`), omit it. This applies to new functions/methods and to adding comments to existing ones.

  Avoid the verbose default:
  ```
  // Initialize the database connection structure. This must be done before
  // any other MySQL call, since mysql_init allocates the connection handle
  // that all subsequent calls operate on.
  mysqlptr=mysql_init(NULL);
  ```
  Prefer:
  ```
  // initialize database connection structure
  mysqlptr=mysql_init(NULL);
  ```
- **ASCII only in code comments and docs you write.** Don't introduce non-ASCII characters into source comments, documentation (`doc/**`, `.wt`, `.html`), or other text you author. Use a regular hyphen-minus (U+002D) instead of em-dash (U+2014) or en-dash (U+2013); straight ASCII quotes instead of smart/curly quotes; three ASCII dots instead of a horizontal ellipsis (U+2026); a regular space instead of NBSP (U+00A0); and so on for accented letters, math symbols, arrows, etc. Existing non-ASCII content in files you didn't write is fine, don't churn it.
- **`config.mk` is generated** — never edit it; edit `config.mk.in` and rerun configure. Same for `config.h` (from `config.h.in`) and `src/common/defines.h` (from `defines.h.in`).
- **Most `.in` files are processed by `configure`**, but a few are processed by `make` in their directory instead: `test/tcl/*.tcl.in` (→ `*.tcl`), `src/api/python/PySQLRDB.py.in` (→ `PySQLRDB.py`), and `src/api/erlang/sqlrelay.erl.in` (→ `sqlrelay.erl`). After editing any of those, run `make` in that directory to regenerate the output — rerunning `./configure` won't do it.
- **Windows build** is separate: `config_windows.mk`, `config_windows.h`, `configure.vbs`, `msvc/`. Changes that touch build variables usually need to be mirrored there.
- **License**: GPLv2 with linking exceptions per-component — see `COPYING`. The server, command-line clients, and each language API have distinct license terms; check before copying code across component boundaries.
- **`ChangeLog` entries**: append new entries to the end of the first (current-version) section, not in the middle next to topically-related entries. The first section is whatever appears before the first blank line at the top of the file.

## Commits

**Never commit on your own.** The maintainer references Trac ticket IDs in commit messages, which you don't have access to — let them write the commit. Stage changes if you like, but don't run `git commit` unless explicitly told to in the current turn (a prior "yes, commit" doesn't carry forward to later changes).

**Never revert/discard working-tree changes without asking.** That means no `git checkout -- <file>`, `git restore <file>`, `git reset --hard`, `git stash` of unstaged work, or anything else that overwrites uncommitted changes. The maintainer often has substantial uncommitted work in the tree (sometimes from prior Claude sessions that didn't end with a commit) — silently reverting it loses that work and `git reflog` won't help recover it. If you need a clean tree to narrow down a problem, use `#if 0` blocks, a temporary copy, or ask first.
