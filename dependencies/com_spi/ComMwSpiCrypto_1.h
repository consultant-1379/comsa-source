
#ifndef ComMwSpiCrypto_1_h_
#define ComMwSpiCrypto_1_h_

#include <ComMgmtSpiInterface_1.h>
#include <ComMgmtSpiCommon.h>

/**
 * Encryption interface.
 *
 * @file ComMwSpiCrypto_1.h
 * The SPI provides an interface for encryption and decryption of '\\0' terminated ASCII strings
 *
 */

/**
 * Encryption/decryption interface.
 * The interface supports encryption and decryption of '\\0' terminated ASCII strings.
 */
typedef struct ComMwSpiCrypto_1 {
    /**
     * Common interface description.
     * The "base class" for this interface. Contains component name, interface name and interface version.
     */
    ComMgmtSpiInterface_1T base;

    /**
     * Encrypt a string.
     *
     * @param[in] string A '\\0' terminated ASCII string that should be encrypted.
     *
     * @param[out] result A pointer to a '\\0' terminated ASCII string that contains the encrypted string.
     * Note that the memory for the result string is allocated by the component implementing the ComMwSpiCrypto interface
     * but the caller is responsible for freeing the memory.
     *
     * @return ComOk or ComFailure.
     */
    ComReturnT (*encrypt) (const char* string, char** result);

    /**
     * Decrypt an encrypted string.
     *
     * @param[in] encryptedString A '\\0' terminated ASCII string that should be decrypted
     *
     * @param[out] result A pointer to a '\\0' terminated ASCII string that contains the decrypted string.
     * Note that the memory for the result string is allocated by the component implementing the ComMwSpiCrypto interface
     * but the caller is responsible for freeing the memory.
     *
     * @return ComOk or ComFailure.
     */
    ComReturnT (*decrypt) (const char* encryptedString, char** result);

} ComMwSpiCrypto_1T;

#endif /* ComMwSpiCrypto_1_h_ */
