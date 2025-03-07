// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/process.h>
#include <rudiments/sys.h>
#include <rudiments/signalclasses.h>
#include <rudiments/error.h>

class sqlrserverbaseprivate {
	friend class sqlrserverbase;
	private:
		bool	_debug;
};

static signalhandler		alarmhandler;
static volatile sig_atomic_t	alarmrang=0;

sqlrserverbase::sqlrserverbase() {
	pvt=new sqlrserverbaseprivate;
	pvt->_debug=false;
}

sqlrserverbase::~sqlrserverbase() {
	delete pvt;
}

void sqlrserverbase::init() {

	// set a handler for SIGALRMs
	#ifdef SIGALRM
	alarmhandler.setHandler(alarmHandler);
	alarmhandler.handleSignal(SIGALRM);
	#endif
}

void sqlrserverbase::setDebug(bool debug) {
	pvt->_debug=debug;
}

bool sqlrserverbase::getDebug() {
	return pvt->_debug;
}

bool sqlrserverbase::semWait(semaphoreset *semset,
					int32_t index,
					thread *thr,
					bool withundo,
					int32_t	timeout,
					bool *timedout) {

	bool	result=true;
	if (timedout) {
		*timedout=false;
	}
	if (timeout>0 && semset->supportsTimedSemaphoreOperations()) {
		if (withundo) {
			result=semset->waitWithUndo(index,timeout,0);
		} else {
			result=semset->wait(index,timeout,0);
		}
		if (timedout) {
			*timedout=(!result && error::getErrorNumber()==EAGAIN);
		}
	} else if (timeout>0 && !thr && sys::getSignalsInterruptSystemCalls()) {
		// We can't use this when using threads because alarmrang isn't
		// thread-local and there's no way to make it be.  Also, the
		// alarm doesn't reliably interrupt the wait() when it's called
		// from a thread, at least not on Linux.  Hopefully platforms
		// that supports threads also supports timed semaphore ops.
		semset->setRetryInterruptedOperations(false);
		alarmrang=0;
		signalmanager::alarm(timeout);
		do {
			if (withundo) {
				result=semset->waitWithUndo(index);
			} else {
				result=semset->wait(index);
			}
		} while (!result && error::getErrorNumber()==EINTR &&
				!process::getShutDownFlag() &&
				alarmrang!=1);
		if (timedout) {
			*timedout=(alarmrang==1);
		}
		signalmanager::alarm(0);
		semset->setRetryInterruptedOperations(true);
	} else {
		if (withundo) {
			result=semset->waitWithUndo(index);
		} else {
			result=semset->wait(index);
		}
	}

	return result;
}

void sqlrserverbase::alarmHandler(int32_t signum) {
	alarmrang=1;
}

static const char *eventtypes[]={
	"CLIENT_CONNECTED",
	"CLIENT_CONNECTION_REFUSED",
	"CLIENT_DISCONNECTED",
	"CLIENT_PROTOCOL_ERROR",
	"DB_LOGIN",
	"DB_LOGOUT",
	"DB_ERROR",
	"DB_WARNING",
	"QUERY_RECEIVED",
	"QUERY_PREPARED",
	"QUERY_EXECUTED",
	"FILTER_VIOLATION",
	"INTERNAL_ERROR",
	"INTERNAL_WARNING",
	"DEBUG_MESSAGE",
	"SCHEDULE_VIOLATION",
	"INTEGRITY_VIOLATION",
	"TRANSLATION_FAILURE",
	"PARSE_FAILURE",
	NULL
};

const char *sqlrserverbase::getEventType(sqlrevent_t event) {
	return eventtypes[(uint16_t)event];
}

sqlrevent_t sqlrserverbase::getEventType(const char *event) {
	uint16_t	retval=SQLREVENT_CLIENT_CONNECTED;
	for (const char * const *ev=eventtypes; *ev; ev++) {
		if (!charstring::compareIgnoringCase(event,*ev)) {
			break;
		}
		retval++;
	}
	return (sqlrevent_t)retval;
}
