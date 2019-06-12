#ifndef _KEY_PROVISIONING_H_
#define _KEY_PROVISIONING_H_

#include <stdint.h>
#include <stdlib.h>
#include "aws_pkcs11.h"

typedef struct ProvisioningParams_t
{
    uint32_t ulClientPrivateKeyType;
    uint8_t * pcClientPrivateKey;
    uint32_t ulClientPrivateKeyLength;
    uint8_t * pcClientCertificate;
    uint32_t ulClientCertificateLength;
} ProvisioningParams_t;

void key_prov_load_keys( void );

void key_prov_vDevModeKeyProvisioning( void );

void key_prov_vAlternateKeyProvisioning( ProvisioningParams_t * xParams );

CK_RV key_prov_xInitializePkcsSession( CK_FUNCTION_LIST_PTR * ppxFunctionList,
                              CK_SLOT_ID * pxSlotId,
                              CK_SESSION_HANDLE * pxSession );

CK_RV key_prov_xProvisionCertificate( CK_SESSION_HANDLE xSession,
                             uint8_t * pucCertificate,
                             size_t xCertificateLength,
                             uint8_t * pucLabel,
                             CK_OBJECT_HANDLE_PTR xObjectHandle );

#endif