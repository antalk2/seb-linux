#include "configuration/cryptography/crypto_key_generator.h"

#include <QCryptographicHash>
#include <QMessageAuthenticationCode>

namespace seb::configuration::cryptography {

CryptoKeyGenerator::CryptoKeyGenerator(const contracts::AppConfig &appConfig)
    : appConfig_(appConfig)
{
}

QString CryptoKeyGenerator::calculateAppSignatureKey(const QString &connectionToken, const QString &salt)
{
    return hashBytes((connectionToken + salt).toUtf8());
}

QString CryptoKeyGenerator::calculateBrowserExamKeyHash(const QString &configurationKey, const QByteArray &salt, const QString &url)
{
    const QString urlWithoutFragment = url.section('#', 0, 0);
    const QString key = browserExamKey_.isEmpty() ? computeBrowserExamKey(configurationKey, salt) : browserExamKey_;
    return hashBytes((urlWithoutFragment + key).toUtf8());
}

QString CryptoKeyGenerator::calculateConfigurationKeyHash(const QString &configurationKey, const QString &url)
{
    const QString urlWithoutFragment = url.section('#', 0, 0);
    return hashBytes((urlWithoutFragment + configurationKey).toUtf8());
}

void CryptoKeyGenerator::useCustomBrowserExamKey(const QString &browserExamKey)
{
    if (!browserExamKey.isEmpty()) {
        browserExamKey_ = browserExamKey;
    }
}

QString CryptoKeyGenerator::computeBrowserExamKey(const QString &configurationKey, const QByteArray &salt)
{
    if (browserExamKey_.isEmpty()) {
        const QByteArray input = (appConfig_.codeSignatureHash + appConfig_.programBuildVersion + configurationKey).toUtf8();
        browserExamKey_ = QString::fromLatin1(QMessageAuthenticationCode::hash(input, salt, QCryptographicHash::Sha256).toHex());
    }
    return browserExamKey_;
}

QString CryptoKeyGenerator::hashBytes(const QByteArray &data)
{
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

}  // namespace seb::configuration::cryptography
