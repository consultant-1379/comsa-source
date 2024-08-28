/*
 * Ac_unittest.cc
 *
 *  Created on: Sep 6, 2011
 *      Author: uabjoy
 *      This is the unit test file for test of code in the scr/ac directory.
 *      First test added is for the SDP 815 registration code.
 *
 *   Modified: xnikvap 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 */

#include <list>
#include <string>
#include <iostream>
#include <memory>
#include <assert.h>
#include <gtest/gtest.h>
// COM
#include "MafMgmtSpiCommon.h"
#include "MafMgmtSpiComponent_1.h"
#include "MafMgmtSpiInterfacePortal_1.h"
#include "MafOamSpiModelRepository_4.h"
#include "MafOamSpiManagedObject_3.h"
// COM SA
#include "OamSATransactionRepository.h"
#include "OamSACache.h"
#include "OamSATranslator.h"
#include "ProcessUtil.h"
#include "ComSA.h"
/*
 *
 * Mock ProcessUtil
 *
 */

#include "MockProcessUtil.h"

aaservice::MockProcessUtil *mymockProcessUtil;

// External prototype definitions

extern "C" void setUpFileMConsumer();
extern "C" void removeFileMConsumer();
extern "C" void setUpInstanceModelConsumers();

/**
 *  External references
 */
extern OamSATranslator theTranslator;

// Methods to create our model repository with correct data in it.

extern struct MafOamSpiMoc* maf_makeMoc(const char* name, struct MafOamSpiMoAttribute* attrList);

extern struct MafOamSpiMom* maf_makeMom(const char* name, struct MafOamSpiMoc* rootMoc);

extern struct MafOamSpiMoAttribute* maf_makeAttribute(char* name, MafOamSpiMoAttributeType_3T type, struct MafOamSpiMoAttribute* next);

extern void maf_setMom(struct MafOamSpiMom* theMom);

extern struct MafOamSpiContainment* makeContainment(struct MafOamSpiMoc* parent, struct MafOamSpiMoc* child);

extern void maf_makeParentContainmentToTop(struct MafOamSpiMoc* childClass);

extern MafOamSpiMrMocHandle_4T Ghandle;
/*
 * Stubs for externals calls used.
 */

static std::list<std::string>	ReceivedCommands;

// Global variable controlling behavior of the system function.

static bool TestCase3 = false;
static bool TestCase4 = false;


// Mock function for logger
void auditlogger(char* tag, int priority, int add_pid,
        char* msgid, const char *format_msg)
{
}

/*
 *
 * UNIT TESTS
 *
 */


TEST (AcTest, removeFileMDFTest1)
{
	// This test case must come first because it test a internal state of Com-SA
	// that is changed by later test cases.
	// Check to NOT register and then check that it not try to unregister

	ReceivedCommands.clear();

	bool TestStatus = true;

	removeFileMConsumer();

	if (!ReceivedCommands.empty()) {
		TestStatus = false;
		std::cout << "Received in system : " << std::endl ;
	}

	EXPECT_EQ ( true, TestStatus );
}



TEST(AcTest, setUpFileMDFTest1)
{
	// Test that registration works if FileM model exists in repository.

	// Clear the Model Repository
	theTranslator.ResetMOMRoot();
	// setup ModelRepository
	// Add the key attribute for the root class
	struct MafOamSpiMoAttribute* attrs = maf_makeAttribute((char*)"pets", MafOamSpiMoAttributeType_3_STRING, NULL);
	attrs->isKey = true;
	struct MafOamSpiMoc* FileMMoc = maf_makeMoc("FileM", attrs);
	struct MafOamSpiMom* model = maf_makeMom("FileM",FileMMoc);
	maf_setMom(model);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(FileMMoc);

	// setUp the MOCK for the ProcessUtil class.

	std::string Output = "";
	mymockProcessUtil = new aaservice::MockProcessUtil(Output,0);
	std::auto_ptr<aaservice::MockProcessUtil> x(mymockProcessUtil);

	setUpFileMConsumer();

	// Check that the code has called system two times with expected content
	bool TestStatus = true;

	using namespace std;

	ReceivedCommands.clear();

	//std::list<int> int_list;

	std::string myLocalResult(mymockProcessUtil->getExecutedCommand());

	int Index = myLocalResult.find("cmw-modeltype-link --help");

	if (Index == string::npos) {
		TestStatus = false;
		std::cout << "Received Command1 : " << myLocalResult << std::endl ;
	}
	Index = myLocalResult.find("cmw-modeltype-link IMM-I-FileM_R1 IMM_R1");
	//Command1 = *list_iter;
	if (Index == string::npos) {
		TestStatus = false;
		std::cout << "Received Command1 : " << myLocalResult << std::endl ;
	}

	EXPECT_EQ ( true, TestStatus );

	//delete FileMMoc;
	//delete model;
	//Remove linked model from ManagedElement
	//delete ((struct MafOamSpiMom*)Ghandle.handle);
}


TEST(AcTest, setUpFileMDFTest2)
{
	// Test that registration not is performed if FileM model not exists in repository.


	// Clear the Model Repository
	theTranslator.ResetMOMRoot();
	// setup ModelRepository, in this case nothing that is FileM
	// setup ModelRepository
	// Add the key attribute for the root class
	struct MafOamSpiMoAttribute* attrs = maf_makeAttribute((char*)"pets", MafOamSpiMoAttributeType_3_STRING, NULL);
	attrs->isKey = true;
	struct MafOamSpiMoc* FileMMoc = maf_makeMoc("FileMNOT", attrs);
	struct MafOamSpiMom* model = maf_makeMom("FileMNOT",FileMMoc);
	maf_setMom(model);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(FileMMoc);

	// Define the test mock return result to 0.
	std::string Output = "";
	mymockProcessUtil = new aaservice::MockProcessUtil(Output,0);
	std::auto_ptr<aaservice::MockProcessUtil> x(mymockProcessUtil);

	bool TestStatus = true;


	setUpFileMConsumer();

	// Check that the code has called system one time with expected content

	std::string myLocalResult(mymockProcessUtil->getExecutedCommand());

	int Index = myLocalResult.find("cmw-modeltype-link --help");

	if (Index == string::npos) {
		TestStatus = false;
		std::cout << "Received Command1 : " << myLocalResult << std::endl ;
	}

	EXPECT_EQ ( true, TestStatus );

	//delete FileMMoc;
	//delete model;
	//Remove linked model from ManagedElement
	//delete ((struct MafOamSpiMom*)Ghandle.handle);
}


TEST(AcTest, setUpFileMDFTest3)
{
	// Test that the code do nothing if the MDF function not exists on target.

	// Clear the Model Repository
	theTranslator.ResetMOMRoot();
	// setup ModelRepository,
	// Add the key attribute for the root class
	struct MafOamSpiMoAttribute* attrs = maf_makeAttribute((char*)"pets", MafOamSpiMoAttributeType_3_STRING, NULL);
	attrs->isKey = true;
	struct MafOamSpiMoc* FileMMoc = maf_makeMoc("FileM", attrs);
	struct MafOamSpiMom* model = maf_makeMom("FileM",FileMMoc);
	maf_setMom(model);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(FileMMoc);

	// Define the test mock return result to 127.
	std::string Output = "";
	mymockProcessUtil = new aaservice::MockProcessUtil(Output,127);
	std::auto_ptr<aaservice::MockProcessUtil> x(mymockProcessUtil);


	setUpFileMConsumer();

	// Check that the code has called system one time with expected content
	bool TestStatus = true;

	std::string myLocalResult(mymockProcessUtil->getExecutedCommand());

	if (myLocalResult.find("cmw-modeltype-link --help") == string::npos) {
		TestStatus = false;
		std::cout << "Received Command1 : " << myLocalResult << std::endl ;
	}

	if (myLocalResult.find("cmw-modeltype-link --help IMM-I-FileM_R1 IMM_R1") != string::npos) {
		TestStatus = false;
		std::cout << "Received Command1 : " << myLocalResult << std::endl ;
	}

	EXPECT_EQ ( true, TestStatus );

	//delete FileMMoc;
	//delete model;
	//Remove linked model from ManagedElement
	//delete ((struct MafOamSpiMom*)Ghandle.handle);
}

TEST(AcTest, removeFileMDFTest2)
{
	// Test to register first and then check that it unregister correctly
	// Test that registration works if FileM model exists in repository.

	// Clear the Model Repository
	theTranslator.ResetMOMRoot();
	// setup ModelRepository
	// Add the key attribute for the root class
	struct MafOamSpiMoAttribute* attrs = maf_makeAttribute((char*)"pets", MafOamSpiMoAttributeType_3_STRING, NULL);
	attrs->isKey = true;

	struct MafOamSpiMoc* FileMMoc = maf_makeMoc("FileM", attrs);
	struct MafOamSpiMom* model = maf_makeMom("FileM",FileMMoc);
	maf_setMom(model);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(FileMMoc);


	// Define the test mock return result to 0.
	std::string Output = "";
	mymockProcessUtil = new aaservice::MockProcessUtil(Output,0);
	std::auto_ptr<aaservice::MockProcessUtil> x(mymockProcessUtil);


	setUpFileMConsumer();

	// Check that the code has called system two times with expected content
	bool TestStatus = true;

	std::string myLocalResult(mymockProcessUtil->getExecutedCommand());

	if (myLocalResult.find("cmw-modeltype-link --help") == string::npos) {
		TestStatus = false;
		std::cout << "Received Command1 : " << myLocalResult << std::endl ;
	}

	if (myLocalResult.find("cmw-modeltype-link IMM-I-FileM_R1 IMM_R1") == string::npos) {
		TestStatus = false;
		std::cout << "Received Command1 : " << myLocalResult << std::endl ;
	}

	// Clear The test Mock
	mymockProcessUtil->executedCommand.clear();

	removeFileMConsumer();

	std::string myLocalResultNext(mymockProcessUtil->getExecutedCommand());

	if (myLocalResultNext.find("cmw-modeltype-unlink IMM-I-FileM_R1") == string::npos) {
		TestStatus = false;
		std::cout << "Received Command1 : " << myLocalResultNext << std::endl ;
	}

	EXPECT_EQ ( true, TestStatus );

	//delete FileMMoc;
	//delete model;
	//Remove linked model from ManagedElement
	//delete ((struct MafOamSpiMom*)Ghandle.handle);
}

TEST(AcTest, setUpInstanceModelConsumersWithMDF)
{
/*
 * 	Test to register via MDF when MDF is present on the system.
 */
	// Define the test mock return result to 0.
	std::string Output = "";
	mymockProcessUtil = new aaservice::MockProcessUtil(Output,0);
	std::auto_ptr<aaservice::MockProcessUtil> x(mymockProcessUtil);

	setUpInstanceModelConsumers();

	// Check that the code has called system two times with expected content
	bool TestStatus = true;

	std::string myLocalResult(mymockProcessUtil->getExecutedCommand());

	if (myLocalResult.find("cmw-modeltype-link --help") == string::npos) {
		TestStatus = false;
		std::cout << "Received Command : " << myLocalResult << std::endl ;
	}

	if (myLocalResult.find("cmw-modeltype-link IMM-I-FM_R1 IMM_R1") == string::npos) {
		TestStatus = false;
		std::cout << "Received Command : " << myLocalResult << std::endl ;
	}

	if (myLocalResult.find("cmw-modeltype-link IMM-I-Local_Authorization_R1 IMM_R1") == string::npos) {
		TestStatus = false;
		std::cout << "Received Command : " << myLocalResult << std::endl ;
	}

	// Clear The test Mock
	mymockProcessUtil->executedCommand.clear();

	EXPECT_EQ ( true, TestStatus );
}

TEST(AcTest, setUpInstanceModelConsumersWithNOMDF)
{
/*
 * 	Test to register via MDF when MDF is present on the system.
 */
	// Define the test mock return result to 0.
	std::string Output = "";
	mymockProcessUtil = new aaservice::MockProcessUtil(Output,127);
	std::auto_ptr<aaservice::MockProcessUtil> x(mymockProcessUtil);

	setUpInstanceModelConsumers();

	// Check that the code has called system two times with expected content
	bool TestStatus = true;

	std::string myLocalResult(mymockProcessUtil->getExecutedCommand());

	if (myLocalResult.find("cmw-modeltype-link --help") == string::npos) {
		TestStatus = false;
		std::cout << "Received Command : " << myLocalResult << std::endl ;
	}

	if (myLocalResult.find("cmw-modeltype-link IMM-I-FM_R1 IMM_R1") != string::npos) {
		TestStatus = false;
		std::cout << "Received Command : " << myLocalResult << std::endl ;
	}

	if (myLocalResult.find("cmw-modeltype-link IMM-I-Local-Authorization_R1 IMM_R1") != string::npos) {
		TestStatus = false;
		std::cout << "Received Command : " << myLocalResult << std::endl ;
	}

	// Clear The test Mock
	mymockProcessUtil->executedCommand.clear();

	EXPECT_EQ ( true, TestStatus );
}

TEST(AcTest, emptyTestCase)
{
    // Empty test case to be able to build
}
