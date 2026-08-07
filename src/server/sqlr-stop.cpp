// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrutil.h>
#include <rudiments/sys.h>
#include <rudiments/process.h>
#include <rudiments/snooze.h>
#include <rudiments/linkedlist.h>
#include <rudiments/stringbuffer.h>
#include <rudiments/directory.h>
#include <rudiments/file.h>
#include <rudiments/error.h>
#include <rudiments/stdio.h>
#include <config.h>
#include <defaults.h>
#include <version.h>

// how long to wait for processes to exit, in polls,
// and how long to wait between polls, in microseconds
#define TERM_POLLS	30
#define KILL_POLLS	10
#define POLL_INTERVAL	100000

// The scaler is stopped by itself, before anything else, because it spawns
// connections.  See the comment in main().  Each suffixes array is
// index-coupled to the programs array above it.
const char *scalerprograms[]={
	"sqlr-scaler-",
	NULL
};

const char *scalersuffixes[]={".pid",NULL};

const char *otherprograms[]={
	"sqlr-listener-",
	"sqlr-connection-",
	"sqlr-cachemanager",
	NULL
};

const char *othersuffixes[]={".pid",".",".pid",NULL};

struct targetprocess {
	uint64_t	pid;
	char		*pidfile;
	bool		dead;
};

static bool stillRunning(targetprocess *tp) {

	// On platforms with kill(), signal 0 tests whether the process still
	// exists without disturbing it.  Windows has no kill() and rudiments
	// emulates signals there by injecting a thread into the target, which
	// is far too invasive to do over and over, so watch for the pid file
	// instead.  Each process removes its own pid file as it exits.
	#ifndef _WIN32
	if (process::sendSignal(tp->pid,0)) {
		return true;
	}

	// Signal 0 also fails if the process belongs to someone else, which
	// means that it's still running, we just can't stop it.
	return (error::getErrorNumber()==EPERM);
	#else
	return file::exists(tp->pidfile);
	#endif
}

static bool waitForExit(linkedlist< targetprocess * > *targets,
							uint16_t polls) {

	for (uint16_t i=0; i<polls; i++) {

		// wait a bit between polls
		if (i) {
			snooze::microsnooze(0,POLL_INTERVAL);
		}

		// see which processes are still running
		bool	anyleft=false;
		for (listnode< targetprocess * > *node=targets->getFirst();
						node; node=node->getNext()) {

			targetprocess	*tp=node->getValue();
			if (tp->dead) {
				continue;
			}

			if (stillRunning(tp)) {
				anyleft=true;
			} else {
				tp->dead=true;
			}
		}

		if (!anyleft) {
			return true;
		}
	}
	return false;
}

static void collectTargets(directory *dir,
				const char *piddir,
				const char *id,
				size_t idlen,
				const char **programs,
				const char **suffixes,
				linkedlist< uint64_t > *seen,
				linkedlist< targetprocess * > *targets) {

	// some useful string buffers
	stringbuffer	match;
	stringbuffer	fqp;

	// run through the programs that we want to kill
	const char **prog=NULL;
	const char **suffix=NULL;
	for (prog=programs,suffix=suffixes; *prog; prog++, suffix++) {

		// don't kill the cachemanager if an id was given
		if (idlen && !charstring::compare(*prog,"sqlr-cachemanager")) {
			break;
		}

		// rewind the directory
		dir->rewind();

		// reset the file to match
		match.clear();
		match.append(*prog);
		if (idlen) {
			match.append(id);
			match.append(*suffix);
		}

		// look through the files in the directory...
		for (;;) {

			// get a file
			const char	*file=dir->read();
			if (!file) {
				break;
			}

			// skip the file if it doesn't match what
			// we're looking for
			if ((idlen &&
				(((*suffix)[1]=='\0' &&
					charstring::compare(file,
						match.getString(),
						match.getStringLength())) ||
				((*suffix)[1]=='p' &&
					charstring::compare(file,
						match.getString())))) ||
				charstring::compare(file,
						match.getString(),
						match.getStringLength())) {
				continue;
			}

			// build the fully qualified path name of the pid file
			fqp.clear();
			fqp.append(piddir);
			fqp.append(file);

			// skip the pid file if it's not readable
			if (!file::isReadable(fqp.getString())) {
				continue;
			}

			// get the pid from the file
			char		*pidstr=
					file::getContents(fqp.getString());
			uint64_t	pid=
					charstring::convertToInteger(pidstr);

			if (pid) {

				// Skip the process if an earlier sweep already
				// dealt with it.  A process that couldn't be
				// killed still has its pid file, and must not
				// be signalled and reported a second time.
				if (seen->find(pid)) {
					delete[] pidstr;
					continue;
				}
				seen->append(pid);

				// remember the process for later
				targetprocess	*tp=new targetprocess;
				tp->pid=pid;
				tp->pidfile=charstring::duplicate(
							fqp.getString());
				tp->dead=false;
				targets->append(tp);

			} else {

				// Sometimes the pid file gets removed or
				// truncated while it's being read and pid is
				// 0.  In that case, just attempt to remove the
				// pid file.  This might also fail becase it
				// might already be in the process of being
				// removed, so we don't need to check whether
				// it succeeded or not.
				file::remove(fqp.getString());
			}

			delete[] pidstr;
		}
	}
}

static void stopTargets(linkedlist< targetprocess * > *targets) {

	// Signal all of the processes, then wait for all of them at once.
	// Signalling and waiting for each one in turn would multiply the
	// timeout by the number of processes.
	for (listnode< targetprocess * > *node=targets->getFirst();
						node; node=node->getNext()) {
		process::sendSignal(node->getValue()->pid,SIGTERM);
	}

	// wait for them to exit
	if (waitForExit(targets,TERM_POLLS)) {
		return;
	}

	// kill whatever ignored the signal
	for (listnode< targetprocess * > *node=targets->getFirst();
						node; node=node->getNext()) {

		targetprocess	*tp=node->getValue();
		if (tp->dead) {
			continue;
		}

		#ifdef SIGKILL
		process::sendSignal(tp->pid,SIGKILL);
		#else
		process::sendSignal(tp->pid,SIGTERM);
		#endif
	}

	// wait for them to exit
	waitForExit(targets,KILL_POLLS);
}

static bool reportAndCleanUp(linkedlist< targetprocess * > *targets) {

	bool	allstopped=true;
	for (listnode< targetprocess * > *node=targets->getFirst();
						node; node=node->getNext()) {

		targetprocess	*tp=node->getValue();

		stdoutput.printf("killing process %lld\n",tp->pid);
		if (tp->dead) {
			stdoutput.printf("   success\n");
			file::remove(tp->pidfile);
		} else {
			stdoutput.printf("   failed\n");
			allstopped=false;
		}

		delete[] tp->pidfile;
		delete tp;
	}
	return allstopped;
}

static void helpmessage(const char *progname) {
	stdoutput.printf(
		"%s is the shutdown program for the %s server processes.\n"
		"\n"
		"The %s program stops %s-listener, %s-connection, and %s-scaler processes.\n"
		"\n"
		"When run with the -id argument, %s stops processes for the specified instance.  When run with no -id argument, %s stops all running %s-listener, %s-connection, and %s-scaler processes.\n"
		"\n"
		"Usage: %s [OPTIONS]\n"
		"\n"
		"Options:\n"
		CONFIG
		LOCALSTATEDIR
		"\n"
		"Examples:\n"
		"\n"
		"Stop instance \"myinst\" who's pid files are found in the default location.\n"
		"	%s -id myinst\n"
		"\n"
		"Stop instance \"myinst\" who's pid files are found under /opt/myapp/var\n"
		"	%s -localstatedir /opt/myapp/var -id myinst\n"
		"\n"
		"Stop all running instances who's pid files are found in the default location.\n"
		"	%s\n"
		"\n"
		"Stop all running instances who's pid files are found under /opt/myapp/var\n"
		"	%s -localstatedir /opt/myapp/var\n"
		"\n",
		progname,SQL_RELAY,progname,SQLR,SQLR,SQLR,
		progname,progname,SQLR,SQLR,SQLR,
		progname,progname,progname,progname,progname);
}

int main(int argc, const char **argv) {

	version(argc,argv);
	help(argc,argv);

	// process the command line
	sqlrcmdline	cmdl(argc,argv);

	// get the id
	const char	*id=cmdl.getValue("-id");
	size_t		idlen=charstring::getLength(id);

	// get the pid directory
	sqlrpaths	sqlrpth(&cmdl);

	// open the pid directory
	directory	dir;
	if (!dir.open(sqlrpth.getPidDir())) {
		stdoutput.printf("failed to open pid directory %s\n",
							sqlrpth.getPidDir());
		process::exit(1);
	}

	// The scaler gets stopped, and confirmed dead, before anything else is
	// even looked for.  The scaler exists to spawn connections, and it
	// spawns one whenever the connected client count in the shared memory
	// segment exceeds the current connection count, so killing connections
	// while it is still running just makes it replace them.  A connection
	// spawned after the pid directory had been scanned would never be
	// signalled, and would be orphaned onto init when the scaler exited.
	linkedlist< uint64_t >		seen;
	linkedlist< targetprocess * >	scaler;
	collectTargets(&dir,sqlrpth.getPidDir(),id,idlen,
				scalerprograms,scalersuffixes,&seen,&scaler);
	stopTargets(&scaler);
	bool	allstopped=reportAndCleanUp(&scaler);

	// Now stop everything else.  Sweep twice, because a connection that
	// the scaler spawned just before it exited might not have created its
	// pid file yet when the first sweep read the pid directory.  The seen
	// list keeps the second sweep from signalling and reporting anything
	// that the first sweep already dealt with.
	for (uint16_t sweep=0; sweep<2; sweep++) {

		linkedlist< targetprocess * >	targets;
		collectTargets(&dir,sqlrpth.getPidDir(),id,idlen,
				otherprograms,othersuffixes,&seen,&targets);
		stopTargets(&targets);
		if (!reportAndCleanUp(&targets)) {
			allstopped=false;
		}
	}

	process::exit((allstopped)?0:1);
}
