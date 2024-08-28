#include "Reencryptor.h"
#include "Trace.h"

#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#include <cstring>
#include <csignal>
#include <cstdlib>
#include <cerrno>
#include <cstdlib>
#include <iostream>

void terminate(int signal);
bool startSignalHandler();

void terminate(int signal)
{
	//terminate in a cool way
	Reencryptor::Instance().stop();
	Trace::Instance().stop();

	syslog(LOG_USER*8 + LOG_INFO, "%s", "terminate(): com-reencryptor Exited gracefully.");

	std::exit(signal);
}

bool startSignalHandler()
{
	bool retVal = true;

	if (SIG_ERR == std::signal(SIGTERM, terminate)) {
		retVal = false;
	}

	return retVal;
}

bool dropPrivilege()
{
	char *uname = std::getenv("COM_USER");
	bool returnValue = true;

	if (uname != NULL) {

		struct passwd pwd,*result;
		char *buf;
		size_t bufSize;
		long bufLength;

		bufLength = sysconf(_SC_GETPW_R_SIZE_MAX);
		if (bufLength == -1) {      /* Value was indeterminate */
			bufSize = 16384;        /* Should be more than enough */
		} else {
			bufSize =size_t(bufLength);
		}

		buf = (char*)malloc(bufSize);
		if(buf == NULL) {
			syslog(LOG_USER*8 + LOG_ERR, "%s%s", "Unable to allocate memory: ", strerror(errno));
			return false;
		}

		getpwnam_r(uname, &pwd, buf, bufSize, &result);

		if (result) {
			uid_t euid = ::geteuid();
			if((euid == 0) && ::initgroups(pwd.pw_name, pwd.pw_gid) == -1) {
				syslog(LOG_USER*8 + LOG_ERR, "%s%s%s%s%s", "Unable to authenticate user : ", uname, " , reason : could not initiate the groups database for the user (",strerror(errno), ")");
				returnValue = false;
			}

			if ((returnValue == true) && (setgid(pwd.pw_gid) < 0)) {
				syslog(LOG_USER*8 + LOG_ERR, "%s%d%s%s%s", "setgid failed, gid=", pwd.pw_gid, " (", strerror(errno), ")");
				returnValue = false;
			}

			if ((returnValue == true) && (setuid(pwd.pw_uid) < 0)) {
				syslog(LOG_USER*8 + LOG_ERR, "%s%d%s%s%s", "setuid failed, uid=", pwd.pw_uid, " (", strerror(errno), ")");
				returnValue = false;
			}
		} else {
			syslog(LOG_USER*8 + LOG_ERR, "%s%s%s", "Invalid user name ", uname," in the environment variable COM_USER");
			returnValue = false;
		}
		free(buf);
	} else {
		syslog(LOG_USER*8 + LOG_ERR, "%s", "Environment variable COM_USER is not defined.");
		returnValue = false;
	}
	return returnValue;
}

int main(int argc, char **argv)
{
	syslog(LOG_USER*8 + LOG_INFO, "%s", "main(): Starting com-reencryptor.");

	char *userMode = std::getenv("COM_USER_MODE");

	if ((userMode != NULL) && (strcmp(userMode,"1") == 0)) {
		//Dropping user privileges
		if (getuid() == 0 || geteuid() == 0) {
			if (dropPrivilege() == false) {
				syslog(LOG_USER*8 + LOG_ERR, "%s", "Failed to drop privileges.");
			}
		}
	}

	if (false == startSignalHandler()) {
		syslog(LOG_USER*8 + LOG_ERR, "%s", "main(): Unable to start signal handler. Exiting.");
		exit(-1);
	}

	if (false == Trace::Instance().start()) {
		syslog(LOG_USER*8 + LOG_WARNING, "%s", "main(): Unable to start trace service. Further logs might not be logged.");
	}

	std::string flatFile;
	if (argc == 2) {
		flatFile = std::string(argv[1]);
	}
	else {
		ERR_COM_RP("main(): Invalid Arguments to com-reencryptor");
	}

	if (false == Reencryptor::Instance().start(flatFile.c_str())) {
		ERR_COM_RP("main(): Unable to start Rencryptor service!");
	}
	else {
		INFO_COM_RP("main(): com-reencryptor Started Successfully.");
	}

	while(1) {
		// Wait indefinitely.
		sleep(-1);
	}

	return 0;
}
