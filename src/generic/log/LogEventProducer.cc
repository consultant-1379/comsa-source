#include "LogEventProducer.h"
#include "EventProducer.h"


class LogEventProducer;

static LogEventProducer *logEventProducer = NULL;

class LogEventProducer
{
public:

	LogEventProducer();
	virtual ~LogEventProducer();

	virtual MafReturnT start(MafMgmtSpiInterfacePortal_3& portal);
	virtual MafReturnT stop(void);
	virtual MafReturnT sendEvent(MafStreamLoggingLevelChangeValue_2T &mafCmNot);


	class LogNotification :  public MafStreamLoggingLevelChangeValue_2T, public Com::RefCounter
	{
	public:

		LogNotification(MafStreamLoggingLevelChangeValue_2T &mafLogNot);

        static const char* eventName()
        {
            return MafStreamLoggingLevelChangeEventType_2;
        }

		virtual ~LogNotification();

		std::string toString() const;

	};

	Com::EventProducer<LogNotification, MafStreamLoggingLevelChangeValue_2T> _logNotificationProducer;

};

LogEventProducer::LogEventProducer()
{
	ENTER_OAMSA_CMEVENT();
	LEAVE_OAMSA_CMEVENT();
}
LogEventProducer::~LogEventProducer( )
{
	ENTER_OAMSA_CMEVENT();
	LEAVE_OAMSA_CMEVENT();
}

MafReturnT LogEventProducer::start(MafMgmtSpiInterfacePortal_3& portal)
{
	ENTER_OAMSA_CMEVENT();

	_logNotificationProducer.start(portal, LogNotification::eventName());

	LEAVE_OAMSA_CMEVENT();
	return MafOk;
}

MafReturnT LogEventProducer::sendEvent(MafStreamLoggingLevelChangeValue_2T &mafCmNot)
{
	ENTER_OAMSA_CMEVENT();

	MafReturnT ret = MafOk;
	LogNotification* eventValue = new LogNotification(mafCmNot);

	_logNotificationProducer.push(*eventValue);
	eventValue->releaseRef();
	DEBUG_OAMSA_CMEVENT("LogEventProducer::sendEvent(): LEAVE with %d",ret);

	return ret;

}

MafReturnT LogEventProducer::stop()
{
	ENTER_OAMSA_CMEVENT();

	_logNotificationProducer.stop();

	LEAVE_OAMSA_CMEVENT();
	return MafOk;
}


// Copy the content of the MAF Log notification struct ("mafCmNot") to local data
LogEventProducer::LogNotification::LogNotification(MafStreamLoggingLevelChangeValue_2T &mafCmNot)
{
	ENTER_OAMSA_CMEVENT();

	newLevel = mafCmNot.newLevel;
	streamType = mafCmNot.streamType;

	LEAVE_OAMSA_CMEVENT();
}

LogEventProducer::LogNotification::~LogNotification()
{
	ENTER_OAMSA_CMEVENT();

	LEAVE_OAMSA_CMEVENT();
}


/*******************************************************
 *  Interface functions of the Log event handler:
 *
 *           -start_LogEventProducer
 *           -push_LogEventProducer
 *           -stop_LogEventProducer
 *
 *******************************************************/

/* This function starts the LogEventProducer.
 * Only one instance of LogEventProducer can exists at a time. This function takes care of this.
 *
 * return values:
 *
 *     MafOK     : if LogEventProducer successfully started
 *     MafFailure: if LogEventProducer not started
 */
MafReturnT start_LogEventProducer(MafMgmtSpiInterfacePortal_3T *portal_MAF)
{
	ENTER_OAMSA_CMEVENT();

	DEBUG_OAMSA_CMEVENT("start_LogEventProducer(): create new LogEventProducer");
	logEventProducer = new LogEventProducer();

	// before calling start, check if the allocation went well
	if(logEventProducer != NULL)
	{
		DEBUG_OAMSA_CMEVENT("start_LogEventProducer(): starting LogEventProducer");
		if(logEventProducer->start(*portal_MAF) == MafOk)
		{
			DEBUG_OAMSA_CMEVENT("start_LogEventProducer(): LogEventProducer successfully started, return MafOk");
			LEAVE_OAMSA_CMEVENT();
			return MafOk;
		}
		else
		{
			DEBUG_OAMSA_CMEVENT("start_LogEventProducer(): LogEventProducer not started, return MafFailure");
			LEAVE_OAMSA_CMEVENT();
			return MafFailure;
		}
	}
	else
	{
		DEBUG_OAMSA_CMEVENT("start_LogEventProducer(): failed to allocate memory for LogEventProducer, return MafFailure");
		LEAVE_OAMSA_CMEVENT();
		return MafFailure;
	}
}

/* This function pushes one Log event to LogEventProducer
 * LogEventProducer will send the event to the consumers. If that fails then this function will also fail.
 *
 * return values:
 *
 *     MafOK     : if the whole event sending chain went well
 *     MafFailure: if there was a problem in the event sending
 */
MafReturnT push_LogEventProducer(MafStreamLoggingLevelChangeValue_2T *mafCmNot)
{
	ENTER_OAMSA_CMEVENT();
	if(mafCmNot == NULL)
	{
		DEBUG_OAMSA_CMEVENT("push_LogEventProducer(): mafCmNot is NULL, returning MafFailure");
		LEAVE_OAMSA_CMEVENT();
		return MafFailure;
	}

	DEBUG_OAMSA_CMEVENT("push_LogEventProducer(): push mafCmNot(%p)",mafCmNot);
	if(logEventProducer != NULL)
	{

		if(logEventProducer->sendEvent(*mafCmNot) == MafOk)
		{
			DEBUG_OAMSA_CMEVENT("push_LogEventProducer(): pushEvent ok, returning MafOk");
			LEAVE_OAMSA_CMEVENT();
			return MafOk;
		}
		else
		{
			DEBUG_OAMSA_CMEVENT("push_LogEventProducer(): pushEvent failed, returning MafFailure");
			LEAVE_OAMSA_CMEVENT();
			return MafFailure;
		}
	}
	else
	{
		// this error case should not happen (logEventProducer not exists): it must be started before pushing the events.
		// if it happens then it means that the logEventProducer stopped
		// Not pushing the event
		DEBUG_OAMSA_CMEVENT("push_LogEventProducer(): ERROR case: logEventProducer not exists, returning MafFailure");
		LEAVE_OAMSA_CMEVENT();
		return MafFailure;
	}
}

/* This function stops the LogEventProducer
 *
 * return values:
 *
 *     MafOK     : if LogEventProducer successfully stopped
 *     MafFailure: if the proper stopping of the LogEventProducer failed
 */
MafReturnT stop_LogEventProducer(void)
{
	ENTER_OAMSA_CMEVENT();

	DEBUG_OAMSA_CMEVENT("stop_LogEventProducer(): stop Log Notification Cache");
	MafReturnT rc = MafOk;

	if(logEventProducer)
	{
		if(logEventProducer->stop() == MafOk)
		{
			DEBUG_OAMSA_CMEVENT("stop_LogEventProducer(): successfully stopped, returning MafOk");
			delete logEventProducer;
			logEventProducer = NULL;
			LEAVE_OAMSA_CMEVENT();
			rc = MafOk;
		}
		else
		{
			DEBUG_OAMSA_CMEVENT("stop_LogEventProducer(): stopping failed, returning MafFailure");
			LEAVE_OAMSA_CMEVENT();
			rc = MafFailure;
		}
	}
	LEAVE_OAMSA_CMEVENT();
	return rc;
}
