
#ifndef MafMwSpiCrypto_1_h_
#define MafMwSpiCrypto_1_h_

#include <MafMgmtSpiInterface_1.h>
#include <MafMgmtSpiCommon.h>

/**
 * Encryption interface.
 *
 * @file MafMwSpiCrypto_1.h
 * @ingroup MafMwSpi
 *
 * The SPI provides an interface for encryption and decryption of @a null terminated ASCII strings
 */

/**
 * Encryption/decryption interface.
 * The interface supports encryption and decryption of @a null terminated ASCII strings.
 */
typedef struct MafMwSpiCrypto_1 {
    /**
     * Common interface description.
     * The "base class" for this interface. Contains component name, interface name and interface version.
     */
    MafMgmtSpiInterface_1T base;

    /**
     * Encrypt a string.
     *
     * @param[in] string A @a null terminated ASCII string that should be encrypted.
     *
     * @param[out] result A pointer to a @a null terminated ASCII string that contains the encrypted string.
     * Note that the memory for the result string is allocated by the component implementing the MafMwSpiCrypto interface
     * but the caller is responsible for freeing the memory.
     *
     * @return MafOk or MafFailure.
     */
    MafReturnT (*encrypt) (const char* string, char** result);

    /**
     * Decrypt an encrypted string.
     *
     * @param[in] encryptedString A @a null terminated ASCII string that should be decrypted
     *
     * @param[out] result A pointer to a @a null terminated ASCII string that contains the decrypted string.
     * Note that the memory for the result string is allocated by the component implementing the MafMwSpiCrypto interface
     * but the caller is responsible for freeing the memory.
     *
     * @return MafOk or MafFailure.
     */
    MafReturnT (*decrypt) (const char* encryptedString, char** result);

} MafMwSpiCrypto_1T;

#endif /* MafMwSpiCrypto_1_h_ */
