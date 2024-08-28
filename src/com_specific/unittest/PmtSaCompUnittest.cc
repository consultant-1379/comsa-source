/*
 * PmtSaCompUnittest.cxx
 *
 *  Created on: Nov 1, 2011
 *      Author: ejonajo
 */


// Google test framework
#include "gtest/gtest.h"
#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <stdint.h>
#include <list>
// COM
#include <ComOamSpiPm_2.h>
#include <MafMgmtSpiComponent_1.h>
#include <MafMwSpiLog_1.h>
#include <MafMwSpiServiceIdentities_1.h>
#include <MafOamSpiServiceIdentities_1.h>
#include <MafMgmtSpiInterfacePortal_1.h>
#include <MafOamSpiEvent_1.h>

// COMSA
#include "PerfMgmtTransferSA.hxx"
#include "PmEventHandler.hxx"
#include "PmRunnable.hxx"
#include "PmtSaTrace.hxx"

using namespace std;

typedef struct _auto_clean
{
    std::list<void*> listItem;
    ~_auto_clean()
    {
        std::list<void*>::iterator it = listItem.begin();
        for(; it != listItem.end(); it++)
        {
            delete *it;
        }
    }
} auto_clean;

vector<MafMgmtSpiComponent_1T*> daComps;

class COMmock {
public:
    COMmock();
    virtual ~COMmock();

    static COMmock* instance() { return s_instance; };

    bool run();

    void stop() { m_running = false; static_cast<void>(m_myThread.join());};

    void addGp(ComOamSpiPmGpId_2T gp) { m_gpidVector.push_back(gp); }
private:
    void myLoop();
    static COMmock* s_instance;
    volatile bool m_running;
    std::vector<ComOamSpiPmGpId_2T> m_gpidVector; // Only set from outside, only cleared from inside
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
            ComOamSpiPmGpId_2T gpId = m_gpidVector.front();
            // Now fetch the data associated with the GP
            cerr << "COMmock tries to read & store data from ID# " << gpId << endl;
            ComOamSpiPmGpData_2T* gpdata;
            if (pmtsa_getGpData(gpId, &gpdata) == MafOk)
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
                        ComOamSpiPmMeasObjClass_2T moc = gpdata->measObjClasses[i];
                        fprintf(fd, "  <measObjClass=\"%s\" size=%d\n", moc.measObjClass, moc.size);
                        for(uint32_t j = 0; j < moc.size; ++j)
                        {
                            ComOamSpiPmInstance_2T coi = moc.instances[j];
                            fprintf(fd, "    <measObjLDN=\"%s\" size=%d\n", coi.measObjLDN, coi.size);
                            for(uint32_t k = 0; k < coi.size; ++k)
                            {
                                ComOamSpiPmAggregatedValue_2T vvv = coi.values[k];
                                fprintf(fd, "      <type=\"%s\" value=", (vvv.measType != NULL ? vvv.measType : "NO_TYPE"));
// xnikvap: Add tests for all types, arrays, etc.
//                              if (vvv.isFloat)
                                if (ComOamSpiPmValueType_2_FLOAT == vvv.valueType)
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
            if (pmtsa_releaseGpData(gpId) != MafOk)
            {
                cerr << "Failed releasing GP-data with ID# " << gpId << endl;
            }
            m_gpidVector.erase(m_gpidVector.begin());
        }
        sleep(2);
    }
}


MafReturnT mockRegComp(MafMgmtSpiComponent_1T *c)
{
    daComps.push_back(c);
    return MafOk;
}

MafReturnT mockUnregComp(MafMgmtSpiComponent_1T *c)
{
    MafReturnT ret = MafOk;

    ret = c->stop(MafDeactivating);
    daComps.clear();
    return ret;
}

MafReturnT mockRegisterProducer(MafOamSpiEventProducer_1T* interface, MafOamSpiEventProducerHandleT* handle)
{
    return MafOk;
}

MafReturnT mockUnregisterProducer(MafOamSpiEventProducerHandleT handle, MafOamSpiEventProducer_1T* interface)
{
    return MafOk;
}

MafReturnT mockAddEvProd(MafOamSpiEventProducerHandleT h, const char* et)
{
    return MafOk;
}

MafReturnT mockDelEvProd(MafOamSpiEventProducerHandleT h, const char* et)
{
    return MafOk;
}

MafReturnT mockNotify(MafOamSpiEventProducerHandleT producerId,
                      MafOamSpiEventConsumerHandleT consumerId,
                      const char * eventType,
                      MafNameValuePairT **filter,
                      void * value)
{
    ComOamSpiPmGpId_2T* gp = reinterpret_cast<ComOamSpiPmGpId_2T*>(value);
    return MafOk;
}

MafReturnT mockLogWrite(uint32_t foo, MwSpiSeverityT bar, MwSpiFacilityT faz, const char* msg)
{
    return MafOk;
}

auto_clean auto_clean_MafMwSpiLog;
auto_clean auto_clean_MafOamSpiEventRouter;
auto_clean auto_clean_InterfacePortal;

// Currently we only have to return a pointer to an EventRouter and a LogWriter
MafReturnT mockGetIf(MafMgmtSpiInterface_1T ifName, MafMgmtSpiInterface_1T** res)
{
    if (strcmp(ifName.interfaceName, MafMwSpiLog_1Id.interfaceName) == 0)
    {
        MafMwSpiLog_1T* lif = new MafMwSpiLog_1T();
        assert(lif != NULL);
        auto_clean_MafMwSpiLog.listItem.push_back(lif);
        lif->logWrite = mockLogWrite;
        *res = reinterpret_cast<MafMgmtSpiInterface_1T*>(lif);
        return MafOk;
    }
    if (strcmp(ifName.interfaceName, MafOamSpiEventService_1Id.interfaceName) == 0)
    {
        MafOamSpiEventRouter_1T* erif = new MafOamSpiEventRouter_1T();
        assert(erif != NULL);
        auto_clean_MafOamSpiEventRouter.listItem.push_back(erif);
        erif->addProducerEvent = mockAddEvProd;
        erif->removeProducerEvent = mockDelEvProd;
        erif->registerProducer = mockRegisterProducer;
        erif->unregisterProducer = mockUnregisterProducer;
        erif->notify = mockNotify;
        *res = reinterpret_cast<MafMgmtSpiInterface_1T*>(erif);
        return MafOk;
    }
    return MafFailure;
}

void* mockGetPortal(const char *s)
{
    MafMgmtSpiInterfacePortal_1T* foo = new MafMgmtSpiInterfacePortal_1T();
    assert(foo != NULL);
    auto_clean_InterfacePortal.listItem.push_back(foo);
    foo->getInterface = mockGetIf;
    foo->registerComponent = mockRegComp;
    foo->unregisterComponent = mockUnregComp;
    return reinterpret_cast<void*>(foo);
}


TEST(TestEnvironment, SixtyFourBitEnvironment)
{
    ASSERT_EQ(sizeof(void*), sizeof(int64_t));
}

TEST(PmtSaStartup, BadInputParameters)
{
    PmtSa::PerfMgmtTransferSA& pmtSa = PmtSa::PerfMgmtTransferSA::instance();
    ASSERT_EQ(pmtSa.comLCMinit(NULL, NULL), ComFailure);
    ASSERT_EQ(pmtSa.comLCMinit(NULL, "FakedNonExistingConfiguration"), ComFailure);
}

TEST(PmtSaStartup, GoodParameters)
{
    daComps.clear();
    ComMgmtSpiInterfacePortalAccessorT* ac = new ComMgmtSpiInterfacePortalAccessorT();
    ac->getPortal = mockGetPortal;
    PmtSa::PerfMgmtTransferSA& pmtSa = PmtSa::PerfMgmtTransferSA::instance();
    ASSERT_EQ(pmtSa.comLCMinit(ac, NULL), MafOk);
    ASSERT_EQ(daComps.size(), 1);
    ASSERT_EQ(pmtSa.comLCMterminate(), MafOk);
    delete ac;
}

TEST(PmtSaStartup, DoubleLoad)
{
    daComps.clear();
    ComMgmtSpiInterfacePortalAccessorT* ac = new ComMgmtSpiInterfacePortalAccessorT();
    ac->getPortal = mockGetPortal;
    PmtSa::PerfMgmtTransferSA& pmtSa = PmtSa::PerfMgmtTransferSA::instance();
    ASSERT_EQ(pmtSa.comLCMinit(ac, NULL), MafOk);
    ASSERT_EQ(daComps.size(), 1);
    ASSERT_EQ(pmtSa.comLCMinit(ac, NULL), MafOk);
    ASSERT_EQ(pmtSa.comLCMterminate(), MafOk);
    delete ac;
}

TEST(PmtSaStartup, LoadUnloadLoadUnload)
{
    daComps.clear();
    ComMgmtSpiInterfacePortalAccessorT* ac = new ComMgmtSpiInterfacePortalAccessorT();
    ac->getPortal = mockGetPortal;
    PmtSa::PerfMgmtTransferSA& pmtSa = PmtSa::PerfMgmtTransferSA::instance();
    ASSERT_EQ(pmtSa.comLCMinit(ac, NULL), MafOk);
    ASSERT_EQ(daComps.size(), 1);
    ASSERT_EQ(pmtSa.comLCMterminate(), MafOk);
    ASSERT_EQ(daComps.size(), 0);
    ASSERT_EQ(pmtSa.comLCMinit(ac, NULL), MafOk);
    ASSERT_EQ(daComps.size(), 1);
    ASSERT_EQ(pmtSa.comLCMterminate(), MafOk);
    ASSERT_EQ(daComps.size(), 0);
    delete ac;
}

TEST(PmtSaStartup, StartStopThread100Times)
{
    daComps.clear();
    ComMgmtSpiInterfacePortalAccessorT* ac = new ComMgmtSpiInterfacePortalAccessorT();
    ac->getPortal = mockGetPortal;
    PmtSa::PerfMgmtTransferSA& pmtSa = PmtSa::PerfMgmtTransferSA::instance();
    int i = 0;
    PmtSa::PmRunnable* theThread;
    cout << "Starting and stopping component 100 times\n"; cout.flush();
    for (i = 0; i < 100; ++i)
    {
        ASSERT_EQ(pmtSa.comLCMinit(ac, NULL), MafOk);
        ASSERT_EQ(daComps.size(), 1);
        // Now start the component
        {
            vector<MafMgmtSpiComponent_1T*>::iterator it;
            for (it = daComps.begin(); it != daComps.end(); ++it)
            {
                MafMgmtSpiComponent_1T* comp = *it;
                ASSERT_EQ(comp->start(MafActivating), MafOk);
            }
        }
        theThread = PmtSa::PmRunnable::instance();
        ASSERT_EQ(NULL != theThread, true);
        ASSERT_EQ(theThread->running(), true);
        ASSERT_EQ(daComps.size(), 1);
        // Now tear down
        {
            vector<MafMgmtSpiComponent_1T*>::iterator it;
            for (it = daComps.begin(); it != daComps.end(); ++it)
            {
                MafMgmtSpiComponent_1T* comp = *it;
                ASSERT_EQ(comp->stop(MafDeactivating), MafOk);
            }
        }
        ASSERT_EQ(pmtSa.comLCMterminate(), MafOk);
        ASSERT_EQ(daComps.size(), 0);
        theThread = PmtSa::PmRunnable::instance();
        ASSERT_EQ(NULL == theThread, true);
        cout << "."; cout.flush();
    }
    cout << endl << "Now sleeping 15 seconds" << endl;
    sleep(15);
    delete ac;
}

TEST(PmtSaStartup, StopTwice)
{
    daComps.clear();
    ComMgmtSpiInterfacePortalAccessorT* ac = new ComMgmtSpiInterfacePortalAccessorT();
    ac->getPortal = mockGetPortal;
    PmtSa::PerfMgmtTransferSA& pmtSa = PmtSa::PerfMgmtTransferSA::instance();
    ASSERT_EQ(pmtSa.comLCMinit(ac, NULL), MafOk);
    ASSERT_EQ(daComps.size(), 1);
    // Now start the component
    {
        vector<MafMgmtSpiComponent_1T*>::iterator it;
        for (it = daComps.begin(); it != daComps.end(); ++it)
        {
            MafMgmtSpiComponent_1T* comp = *it;
            ASSERT_EQ(comp->start(MafActivating), MafOk);
        }
    }
    PmtSa::PmRunnable* theThread = PmtSa::PmRunnable::instance();
    ASSERT_EQ(NULL != theThread, true);
    ASSERT_EQ(theThread->running(), true);
    // Now tear down
    {
        vector<MafMgmtSpiComponent_1T*>::iterator it;
        for (it = daComps.begin(); it != daComps.end(); ++it)
        {
            MafMgmtSpiComponent_1T* comp = *it;
            ASSERT_EQ(comp->stop(MafDeactivating), MafOk);
        }
    }
    ASSERT_EQ(pmtSa.comLCMterminate(), MafOk);
    theThread = PmtSa::PmRunnable::instance();
    ASSERT_EQ(NULL == theThread, true);
    // Call terminate once again
    ASSERT_EQ(pmtSa.comLCMterminate(), MafOk);
    delete ac;
}

TEST (logService, initializeFinalizePmtSaLogStream)
{
	bool myRet;
	myRet = PmtSa::PmtSaTrace::Instance().initLogStream();
	EXPECT_EQ(true, myRet);
	sleep(1);
	PmtSa::PmtSaTrace::Instance().finalizeLogStream();
}
