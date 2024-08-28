//============================================================================
// Name        : TheLittleTestPgm.cpp
// Author      : Jonas Jonsson
// Version     :
// Copyright   : (C) 2011 Ericsson AB
// Description : Hello World in C++, Ansi-style
//============================================================================

#include "MyStubs.hxx"
#include "PerfMgmtTransferSA.hxx"
#include "PmEventHandler.hxx"
#include "PmRunnable.hxx"
#include "PmThreadTemplate.hxx"

#include <ComMwSpiServiceIdentities_1.h>
#include <ComOamSpiServiceIdentities_1.h>
#include <ComOamSpiPm_1.h>
#include <ComMwSpiLog_1.h>

#include <iostream>
#include <vector>
#include <string>
#include <string.h>
#include <cassert>

using namespace std;

vector<ComMgmtSpiComponent_1T*> daComps;

class COMmock {
public:
	COMmock();
	virtual ~COMmock();

	static COMmock* instance() { return s_instance; };

	bool run();

	void stop() { m_running = false; static_cast<void>(m_myThread.join());};

	void addGp(ComOamSpiPmGpId_1T gp) { m_gpidVector.push_back(gp); }
private:
	void myLoop();
	static COMmock* s_instance;
	volatile bool m_running;
	std::vector<ComOamSpiPmGpId_1T> m_gpidVector; // Only set from outside, only cleared from inside
	PmtSa::PmThreadType<COMmock> m_myThread;
};

COMmock* COMmock::s_instance = NULL;

COMmock::COMmock() :
	m_myThread(&COMmock::myLoop)
{
	s_instance = this;
	m_running = false;
}

COMmock::~COMmock()
{
	m_gpidVector.clear();
	s_instance = NULL;
}

bool COMmock::run()
{
	m_running = true;
	return m_myThread.run(this);
}

void COMmock::myLoop()
{
	cerr << "COMmock-thread starts running now!" << endl;
	while(m_running)
	{
		if (!m_gpidVector.empty())
		{
			// We have PM-data to fetch and print!
			ComOamSpiPmGpId_1T gpId = m_gpidVector.front();
			// Now fetch the data associated with the GP
			cerr << "COMmock tries to read & store data from ID# " << gpId << endl;
			ComOamSpiPmGpData_1T* gpdata;
			if (pmtsa_getGpData(gpId, &gpdata) == ComOk)
			{
			    // Now we have it all in gpdata, lets create a file and save the data!
			    FILE* fd;
			    char fileName[255];
			    memset(fileName, 0x00, sizeof(fileName));
			    sprintf(fileName, "/var/log/opensaf/PM-GP%ld.txt", gpId);
			    fd = fopen(fileName, "w+");
			    if (fd != NULL)
			    {
			        fprintf(fd, "<jobId=\"%ld\" size=%d\n", gpId, gpdata->size);
			        for(uint32_t i = 0; i < gpdata->size; ++i)
			        {
			            ComOamSpiPmMeasObjClass_1T moc = gpdata->measObjClasses[i];
			            fprintf(fd, "  <measObjClass=\"%s\" size=%d\n", moc.measObjClass, moc.size);
			            for(uint32_t j = 0; j < moc.size; ++j)
			            {
			                ComOamSpiPmInstance_1T coi = moc.instances[j];
			                fprintf(fd, "    <measObjLDN=\"%s\" size=%d\n", coi.measObjLDN, coi.size);
			                for(uint32_t k = 0; k < coi.size; ++k)
			                {
			                    ComOamSpiPmAggregatedValue_1T vvv = coi.values[k];
			                    fprintf(fd, "      <type=\"%s\" value=", (vvv.measType != NULL ? vvv.measType : "NO_TYPE"));
			                    if (vvv.isFloat)
			                        fprintf(fd, "%f",vvv.value.floatVal);
			                    else
			                        fprintf(fd, "%ld",vvv.value.intVal);
			                    if (vvv.isSuspect)
			                        fprintf(fd, " suspect");
			                    fprintf(fd, ">\n");
			                }
	                        fprintf(fd, "    >\n");
			            }
			            fprintf(fd, "  >\n");
			        }
                    fprintf(fd, ">\n");
			        fclose(fd);
			    }
			}
			else
			{
			    cerr << "COMmock failed fetching data from PM for ID# " << gpId << endl;
			}
			// Finally, remember to "kill" it
			if (pmtsa_releaseGpData(gpId) != ComOk)
			{
				cerr << "Failed releasing GP-data with ID# " << gpId << endl;
			}
			m_gpidVector.erase(m_gpidVector.begin());
		}
		sleep(2);
	}
}

ComReturnT mockRegComp(ComMgmtSpiComponent_1T *c)
{
	cerr << "Registering component " << c->base.componentName << endl;
	daComps.push_back(c);
	return ComOk;
}

ComReturnT mockUnregComp(ComMgmtSpiComponent_1T *c)
{
	cerr << "Un-registering component " << c->base.componentName << endl;
	daComps.clear();
	return c->stop(ComDeactivating);
}

ComReturnT mockRegisterProducer(ComOamSpiEventProducer_1T* interface, ComOamSpiEventProducerHandleT* handle)
{
	cerr << "New producer of events registered! " << endl;
	return ComOk;
}

ComReturnT mockUnregisterProducer(ComOamSpiEventProducerHandleT handle, ComOamSpiEventProducer_1T* interface)
{
	cerr << "Producer of events unregistered" << endl;
	return ComOk;
}

ComReturnT mockAddEvProd(ComOamSpiEventProducerHandleT h, const char* et)
{
	cerr << "Add producer event: Somebody is producing " << et << endl;
	return ComOk;
}

ComReturnT mockDelEvProd(ComOamSpiEventProducerHandleT h, const char* et)
{
	cerr << "Del producer event: Somebody stopped " << et << endl;
	return ComOk;
}

ComReturnT mockNotify(ComOamSpiEventProducerHandleT producerId,
        			  ComOamSpiEventConsumerHandleT consumerId,
        			  const char * eventType,
        			  ComNameValuePairT **filter,
        			  void * value)
{
    ComOamSpiPmEventValue_1T* ev = reinterpret_cast<ComOamSpiPmEventValue_1T*>(value);
	cerr << "Here came a notification about (" << eventType << "), Name <" << ev->jobId << "> GP-ID " << ev->gpId << endl;
	COMmock::instance()->addGp(ev->gpId);
	return ComOk;
}

ComReturnT mockLogWrite(uint32_t foo, MwSpiSeverityT bar, MwSpiFacilityT faz, const char* msg)
{
    cerr << "MW-LOG: " << msg << endl;
    return ComOk;
}

// Currently we only have to return a pointer to an EventRouter and a LogWriter
ComReturnT mockGetIf(ComMgmtSpiInterface_1T ifName, ComMgmtSpiInterface_1T** res)
{
    if (strcmp(ifName.interfaceName, ComMwSpiLog_1Id.interfaceName) == 0)
    {
        ComMwSpiLog_1T* lif = new ComMwSpiLog_1T();
        assert(lif != NULL);
        lif->logWrite = mockLogWrite;
        *res = reinterpret_cast<ComMgmtSpiInterface_1T*>(lif);
        return ComOk;
    }
    if (strcmp(ifName.interfaceName, ComOamSpiEventService_1Id.interfaceName) == 0)
    {
        ComOamSpiEventRouter_1T* erif = new ComOamSpiEventRouter_1T();
        assert(erif != NULL);
        erif->addProducerEvent = mockAddEvProd;
        erif->removeProducerEvent = mockDelEvProd;
        erif->registerProducer = mockRegisterProducer;
        erif->unregisterProducer = mockUnregisterProducer;
        erif->notify = mockNotify;
        *res = reinterpret_cast<ComMgmtSpiInterface_1T*>(erif);
        return ComOk;
    }
    assert(!"Crash and bang, we've passed the interesting stuff!");
    return ComFailure;
}

void* mockGetPortal(const char *s)
{
	cerr << "getPortal called with argument " << (s != NULL ? s : "NULL") << endl;
	ComMgmtSpiInterfacePortal_1T* foo = new ComMgmtSpiInterfacePortal_1T();
	assert(foo != NULL);
	foo->getInterface = mockGetIf;
	foo->registerComponent = mockRegComp;
	foo->unregisterComponent = mockUnregComp;
	return reinterpret_cast<void*>(foo);
}

int main() {
	PmtSa::PerfMgmtTransferSA& pmtSa = PmtSa::PerfMgmtTransferSA::instance();
	cout << "Testing the pmtSa-component ...." << endl;
	if (pmtSa.comLCMinit(NULL, NULL) != ComFailure)
		cerr << "FAIL" << endl;
	else
		cerr << "OK" << endl;
	if (pmtSa.comLCMinit(NULL, "Fail") != ComFailure)
		cerr << "FAIL" << endl;
	else
		cerr << "OK" << endl;
	// Let's fake COM!
	ComMgmtSpiInterfacePortalAccessorT* ac = new ComMgmtSpiInterfacePortalAccessorT();
	assert(ac != NULL);
	ac->getPortal = mockGetPortal;
	if (pmtSa.comLCMinit(ac, NULL) == ComOk)
	{
		vector<ComMgmtSpiComponent_1T*>::iterator it;
		for (it = daComps.begin(); it != daComps.end(); ++it)
		{
			ComMgmtSpiComponent_1T* comp = *it;
			cerr << "Calling start on component " << comp->base.componentName << " ";
			if (comp->start(ComActivating) == ComOk)
				cerr << "OK" << endl;
			else
				cerr << "FAIL" << endl;
		}
	}
	else
	{
		cerr << "FAIL" << endl;
	}
	// So now the component is running, so should the thread be as well
	PmtSa::PmRunnable* theListener = PmtSa::PmRunnable::instance();
	assert(theListener != NULL);
	if (theListener->running() == true)
		cerr << "OK" << endl;
	else
		cerr << "FAIL" << endl;
	cerr << "Lets fake a COM-consumer too" << endl;
	COMmock* foo = new COMmock();
	assert(foo->run() == true);
	cerr << "Now we wait a while ..." << endl;
	sleep(10);
	cerr << "Ok, lets add a subscriber to consumer data: ";
	// Now we should be able to register a consumer in the EventHandler,
	// it should then send a message to the thread telling it to start
	// listening to events from the PM-service
	string eventType(ComOamSpiPmEventTypeGpReady_1);
	PmtSa::PmEventHandler* evHdlr = PmtSa::PmEventHandler::instance();
	if (evHdlr->addFilter(1, eventType.c_str(), NULL) == ComOk)
		cerr << "OK" << endl;
	else
		cerr << "FAIL" << endl;
	// Now the thread should not only be reading the pipes for internal messages,
	// it should also have connected to the PM-service, and stuff should start
	// coming on that pipe.
	cout << "Running 10 minutes and 10 seconds, waiting for events ..." << endl;
	sleep(10*61);
	// Stop the COMmock object
	foo->stop();
	delete foo;
	cerr << "Let's terminate the component ";
	// Finally, lets try to shut down grace-fully!
	if (pmtSa.comLCMterminate() == ComOk)
		cerr << "OK" << endl;
	else
		cerr << "FAIL" << endl;

	// See if it works again ;)
	cerr << "Starting up again!" << endl;
	if (pmtSa.comLCMinit(ac, NULL) == ComOk)
	{
		vector<ComMgmtSpiComponent_1T*>::iterator it;
		for (it = daComps.begin(); it != daComps.end(); ++it)
		{
			ComMgmtSpiComponent_1T* comp = *it;
			cerr << "Calling start on component " << comp->base.componentName << " ";
			if (comp->start(ComActivating) == ComOk)
				cerr << "OK" << endl;
			else
				cerr << "FAIL" << endl;
		}
	}
	else
	{
		cerr << "FAIL" << endl;
	}
	// So now the component is running, so should the thread be as well
	theListener = PmtSa::PmRunnable::instance();
	assert(theListener != NULL);
	if (theListener->running() == true)
		cerr << "OK" << endl;
	else
		cerr << "FAIL" << endl;
//    evHdlr = PmtSa::PmEventHandler::instance();
//    if (evHdlr->addFilter(1, eventType.c_str(), NULL) == ComOk)
//        cerr << "OK" << endl;
//    else
//        cerr << "FAIL" << endl;
	// Finally, lets try to shut down grace-fully!
	if (pmtSa.comLCMterminate() == ComOk)
		cerr << "OK" << endl;
	else
		cerr << "FAIL" << endl;
	cout << "Stopping the component once more ...." << endl;
    if (pmtSa.comLCMterminate() == ComOk)
        cerr << "OK" << endl;
    else
        cerr << "FAIL" << endl;
	delete ac;
	return 0;
}
