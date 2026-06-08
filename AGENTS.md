# SQL Relay - agent notes

## What this is

SQL Relay: a database proxy providing connection pooling, throttling, load balancing, query routing/filtering, and result-set caching. Server is C++; clients/APIs exist for many languages (C, C++, C#, Java, JDBC, Perl, Perl-DBI, Python, Python-DB, PHP, PHP PDO, Ruby, TCL, Erlang, Node.js, ODBC, ADO.NET). Backends include Oracle, MySQL, PostgreSQL, SAP/Sybase, MS SQL Server, DB2, Informix, Firebird, SQLite, and generic ODBC.

Depends on **rudiments** (>= 2.0.2) - a sibling firstworks library. Autoconf macros (`acfirstworks.m4`, `acrudiments.m4`) are pulled in by `autogen.sh` from `../rudiments/autoconf/` or `/usr/local/firstworks/share/rudiments/autoconf/`.

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

To reinstall just one component: `cd src/api/odbc && make && sudo make install` (every component directory has an `install` target).

Common `./configure` toggles (see `configure.in`): `--disable-server`, `--disable-client`, `--disable-cmdline`, `--enable-debug`, `--enable-fortify`, per-API `--disable-{c,c++,perl,python,php,java,ruby,tcl,erlang,cs,nodejs,odbc}`, per-backend detection is automatic via probes.

Top-level `Makefile` delegates to `src/Makefile`, which delegates to per-module directories. Each module directory builds a libtool `.la` plugin (e.g. `src/connections/sqlrconnection_mysql.la`, `src/auths/sqlrauth_mysql_userlist.la`).

### Module registry generation

Plugin "registries" are generated at `configure` time from `*declarations.cpp.in` + `*assignments.cpp.in` templates (see files under `src/server/` and `src/util/`). Configure substitutes in only the modules actually being built. If you add a new auth/filter/logger/router/trigger/translation/protocol/parser/connection/directive/etc. module, you must update `configure.in` so it gets included in the generated registry - editing only the `.cpp.in` template is not enough unless the module is unconditionally built.

## Runtime architecture

Three long-running server processes per instance (all under `src/server/`):

- **sqlr-listener** (`sqlr-listener.cpp` / `sqlrlistener.cpp`) - accepts client connections on a TCP port and/or Unix socket, authenticates, then hands the session off to an available sqlr-connection (either by proxying or by passing the socket fd - see `handoff=` in config).
- **sqlr-connection** (`sqlr-connection.cpp` + `sqlrserverconnection.cpp` + `sqlrservercontroller.cpp`) - one per pooled database session. Holds a live DB connection via a `src/connections/*.cpp` backend module and services queries from whichever client the listener hands it.
- **sqlr-scaler** (`sqlr-scaler.cpp`) - spawns/reaps sqlr-connection processes between `connections=` (min) and `maxconnections=` (max) based on load.

Supporting command-line tools (`src/server/`): `sqlr-start`, `sqlr-stop`, `sqlr-status`, `sqlr-cachemanager`. Client-side command-line tools (`src/cmdline/`): `sqlrsh`, `sqlr-export`, `sqlr-import`, `sqlr-pwdenc`.

## Code layout

Most of `src/` is discoverable by listing it (one directory per module type: auths, filters, loggers, routers, the various translations, etc.); the non-obvious parts:

- `src/api/` - per-language client libraries; `c/` and `c++/` are the reference implementations (`libsqlrclient`), the rest mostly wrap the C++ API or speak the sqlrclient wire protocol directly.
- `src/server/` - listener/connection/scaler plus the server-side module base classes. Public headers under `src/server/sqlrelay/`; private under `src/server/sqlrelay/private/`.
- `src/connections/` - one `.cpp` per database backend, extending `sqlrserverconnection`; `router` is a pseudo-backend.
- `src/protocols/` - wire-protocol frontends; `sqlrclient.cpp` is the native protocol, the others (mysql, postgresql, tds, oracle, teradata, firebird) let those DBs' native clients talk to sqlr-listener directly.
- `src/util/` - `libsqlrutil` (shared config parsing, paths, password encryption framework).
- `doc/` - the project website; not generated from source comments (see `doc/AGENTS.md`).
- `init/`, `bin/` - configure-generated (systemd/rc/launchd files; pkg-config-style helper scripts).

## Tests

`make tests` runs everything enabled by configure. Tests live under `test/` and run against real databases via a live sqlr instance the harness starts and stops (no mocking) - see `test/AGENTS.md` for running subsets, fixtures, and test conventions.

## Conventions worth knowing

- **C++ standard is C++98.** Don't use C++11+ features in `.cpp` / `.h` files - no `auto` type deduction, no lambdas, no range-based `for`, no `nullptr`, no `std::move`, no `=delete`/`=default`, no strongly-typed enums, no brace-init-list constructors, no variadic templates. Use plain functions (or pass state via structs) instead of lambdas; `NULL` instead of `nullptr`.
- **Copyright header**: every `.cpp` / `.h` starts with `// Copyright (c) David Muse` / `// See the file COPYING for more information`. Preserve it on edits; add it to new files.
- **Comment style.** Match the terse style already in the source: comments sit on the line(s) above the block they describe (never to the right of code), inside the function rather than summarizing it from outside, one comment per logical block. Two registers:
    - **Step labels (the common case):** a 2-6 word *lowercase* fragment naming what the next block does - e.g. `// log in`, `// clean up`, `// run the query...`. Not a sentence: no capital, no period, no filler; don't restate the code.
    - **Why-comments (rare):** full punctuated prose, *only* for what the next reader can't infer from the code - a bug workaround, a version/compatibility quirk, why an obvious approach was avoided.
  Default to no comment, then a short label (`// initialize database connection structure` over a three-line explanation); reach for prose only when there's a genuine non-obvious *why*. If a label would just echo the code (`// increment i`), omit it.
- **`config.mk` is generated** - never edit it; edit `config.mk.in` and rerun configure. Same for `config.h` (from `config.h.in`) and `src/common/defines.h` (from `defines.h.in`).
- **Most `.in` files are processed by `configure`**, but a few are processed by `make` in their directory instead: `test/tcl/*.tcl.in` (-> `*.tcl`), `src/api/python/PySQLRDB.py.in` (-> `PySQLRDB.py`), and `src/api/erlang/sqlrelay.erl.in` (-> `sqlrelay.erl`). After editing any of those, run `make` in that directory to regenerate the output - rerunning `./configure` won't do it.
- **Windows build** is separate: `config_windows.mk`, `config_windows.h`, `configure.vbs`, `msvc/`. Changes that touch build variables usually need to be mirrored there.
- **License**: GPLv2 with linking exceptions per-component - see `COPYING`. The server, command-line clients, and each language API have distinct license terms; check before copying code across component boundaries.
- **`ChangeLog` entries**: append new entries to the end of the first (current-version) section, not in the middle next to topically-related entries. The first section is whatever appears before the first blank line at the top of the file. Update the ChangeLog whenever something outside of `test/` changes, unless the change is already covered by an existing entry in that release (e.g. if an entry says "added some feature to c++ api", don't log further changes to that feature). Never log changes to tests.
