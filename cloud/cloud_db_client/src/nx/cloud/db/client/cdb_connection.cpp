// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "cdb_connection.h"

#include <nx/network/socket_common.h>

#include "cdb_request_path.h"
#include "data/module_info.h"

namespace nx::cloud::db::client {

Connection::Connection(
    network::cloud::CloudModuleUrlFetcher* const endPointFetcher)
    :
    m_requestExecutor(endPointFetcher),
    m_accountManager(&m_requestExecutor),
    m_systemManager(&m_requestExecutor),
    m_organizationManager(&m_requestExecutor),
    m_authProvider(&m_requestExecutor),
    m_maintenanceManager(&m_requestExecutor),
    m_oauthManager(&m_requestExecutor),
    m_twoFactorAuthManager(&m_requestExecutor),
    m_batchUserProcessingManager(&m_requestExecutor)
{
    bindToAioThread(m_requestExecutor.getAioThread());
}

api::AccountManager* Connection::accountManager()
{
    return &m_accountManager;
}

api::SystemManager* Connection::systemManager()
{
    return &m_systemManager;
}

api::OrganizationManager* Connection::organizationManager()
{
    return &m_organizationManager;
}

api::AuthProvider* Connection::authProvider()
{
    return &m_authProvider;
}

api::MaintenanceManager* Connection::maintenanceManager()
{
    return &m_maintenanceManager;
}

api::OauthManager* Connection::oauthManager()
{
    return &m_oauthManager;
}

api::TwoFactorAuthManager* Connection::twoFactorAuthManager()
{
    return &m_twoFactorAuthManager;
}

api::BatchUserProcessingManager* Connection::batchUserProcessingManager()
{
    return &m_batchUserProcessingManager;
}

void Connection::bindToAioThread(
    nx::network::aio::AbstractAioThread* aioThread)
{
    m_requestExecutor.bindToAioThread(aioThread);
}

void Connection::setCredentials(nx::network::http::Credentials credentials)
{
    m_requestExecutor.setCredentials(credentials);
}

void Connection::setProxyVia(
    const std::string& proxyHost,
    std::uint16_t proxyPort,
    nx::network::http::Credentials credentials,
    nx::network::ssl::AdapterFunc adapterFunc)
{
    m_requestExecutor.setProxyCredentials(credentials);
    m_requestExecutor.setProxyVia({proxyHost, proxyPort}, adapterFunc, /*isSecure*/ true);
}

void Connection::setRequestTimeout(std::chrono::milliseconds timeout)
{
    m_requestExecutor.setRequestTimeout(timeout);
}

std::chrono::milliseconds Connection::requestTimeout() const
{
    return m_requestExecutor.requestTimeout();
}

void Connection::setAdditionalHeaders(nx::network::http::HttpHeaders headers)
{
    m_requestExecutor.setAdditionalHeaders(std::move(headers));
}

void Connection::ping(
    std::function<void(api::ResultCode, api::ModuleInfo)> completionHandler)
{
    m_requestExecutor.executeRequest<api::ModuleInfo>(
        kPingPath,
        std::move(completionHandler));
}

} // namespace nx::cloud::db::client
