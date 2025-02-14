// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "oauth_manager.h"

#include "data/oauth_data.h"
#include "cdb_request_path.h"

#include <nx/network/http/rest/http_rest_client.h>

namespace nx::cloud::db::client {

OauthManager::OauthManager(AsyncRequestsExecutor* requestsExecutor):
    m_requestsExecutor(requestsExecutor)
{
}

void OauthManager::issueToken(
    const api::IssueTokenRequest& request,
    nx::utils::MoveOnlyFunc<void(api::ResultCode, api::IssueTokenResponse)> completionHandler)
{
    m_requestsExecutor->executeRequest<api::IssueTokenResponse>(
        nx::network::http::Method::post,
        kOauthTokenPath,
        request,
        std::move(completionHandler));
}

void OauthManager::issueAuthorizationCode(
    const api::IssueTokenRequest& request,
    nx::utils::MoveOnlyFunc<void(api::ResultCode, api::IssueCodeResponse)> completionHandler)
{
    m_requestsExecutor->executeRequest<api::IssueCodeResponse>(
        nx::network::http::Method::post,
        kOauthTokenPath,
        request,
        std::move(completionHandler));
}

void OauthManager::validateToken(
    const std::string& token,
    nx::utils::MoveOnlyFunc<void(api::ResultCode, api::ValidateTokenResponse)> completionHandler)
{
    auto requestPath =
        nx::network::http::rest::substituteParameters(kOauthTokenValidatePath, {token});

    m_requestsExecutor->executeRequest<api::ValidateTokenResponse>(
        nx::network::http::Method::get,
        requestPath,
        std::move(completionHandler));
}

void OauthManager::deleteToken(
    const std::string& token,
    nx::utils::MoveOnlyFunc<void(api::ResultCode)> completionHandler)
{
    auto requestPath =
        nx::network::http::rest::substituteParameters(kOauthTokenValidatePath, {token});

    m_requestsExecutor->executeRequest</*Output*/ void>(
        nx::network::http::Method::delete_,
        requestPath,
        std::move(completionHandler));
}

void OauthManager::deleteTokens(
    const std::string& clientId,
    nx::utils::MoveOnlyFunc<void(api::ResultCode)> completionHandler)
{
    auto requestPath =
        nx::network::http::rest::substituteParameters(kOauthTokensDeletePath, {clientId});

    m_requestsExecutor->executeRequest</*Output*/ void>(
        nx::network::http::Method::delete_,
        requestPath,
        std::move(completionHandler));
}

void OauthManager::logout(nx::utils::MoveOnlyFunc<void(api::ResultCode)> completionHandler)
{
    m_requestsExecutor->executeRequest</*Output*/ void>(
        nx::network::http::Method::delete_,
        kOauthLogoutPath,
        std::move(completionHandler));
}

void OauthManager::issueStunToken(
    const api::IssueStunTokenRequest& request,
    nx::utils::MoveOnlyFunc<void(api::ResultCode, api::IssueStunTokenResponse)> completionHandler)
{
    m_requestsExecutor->executeRequest<api::IssueStunTokenResponse>(
        nx::network::http::Method::post,
        kOauthStunTokenPath,
        request,
        std::move(completionHandler));
}

} // namespace nx::cloud::db::client
