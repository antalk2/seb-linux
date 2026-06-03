#pragma once

#include "configuration/contracts/cryptography/encryption_parameters.h"
#include "configuration/contracts/cryptography/password_parameters.h"

#include <QSslCertificate>

namespace seb::configuration::contracts::cryptography {

struct PublicKeyParameters : EncryptionParameters
{
    QSslCertificate certificate;
    PasswordParameters innerEncryption;
    bool symmetricEncryption = false;
};

}  // namespace seb::configuration::contracts::cryptography
