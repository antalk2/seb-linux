#pragma once

#include "configuration/contracts/app_config.h"
#include "configuration/contracts/cryptography/i_key_generator.h"

namespace seb::configuration::cryptography {

// was: KeyGenerator
class CryptoKeyGenerator : public contracts::cryptography::ICryptoKeyGenerator {
 public:
  explicit CryptoKeyGenerator(const contracts::AppConfig& appConfig);

  QString calculateAppSignatureKey(const QString& connectionToken,
                                   const QString& salt) override;
  QString calculateBrowserExamKeyHash(const QString& configurationKey,
                                      const QByteArray& salt,
                                      const QString& url) override;
  QString calculateConfigurationKeyHash(const QString& configurationKey,
                                        const QString& url) override;
  void useCustomBrowserExamKey(const QString& browserExamKey) override;

 private:
  QString computeBrowserExamKey(const QString& configurationKey,
                                const QByteArray& salt);
  static QString hashBytes(const QByteArray& data);

  contracts::AppConfig appConfig_;
  QString browserExamKey_;
};

}  // namespace seb::configuration::cryptography
