#pragma once

#include "communication/contracts/interlocutor.h"
#include "communication/contracts/proxies/i_client_proxy.h"

#include <memory>

namespace seb::communication::contracts::proxies {

class IProxyFactory
{
public:
    virtual ~IProxyFactory() = default;
    virtual std::unique_ptr<IClientProxy> createClientProxy(const QString &address, contracts::Interlocutor owner) = 0;
};

}  // namespace seb::communication::contracts::proxies
