// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "batch_user_processing_manager.h"

#include <nx/network/http/rest/http_rest_client.h>

#include "cdb_request_path.h"
#include "data/system_data.h"

namespace nx::cloud::db::client {

BatchUserProcessingManager::BatchUserProcessingManager(
    AsyncRequestsExecutor* requestsExecutor):
    m_requestsExecutor(requestsExecutor)
{
}

void BatchUserProcessingManager::createUpdateBatch(
    const api::CreateBatchRequest& request,
    std::function<void(api::ResultCode, api::CreateBatchResponse)> completionHandler)
{
    m_requestsExecutor->executeRequest<api::CreateBatchResponse>(
        nx::network::http::Method::post,
        kSystemUsersBatchPath,
        request,
        std::move(completionHandler));
}

void BatchUserProcessingManager::getBatchState(
    const std::string & batchId,
    std::function<void(api::ResultCode, api::BatchState)> completionHandler)
{
    m_requestsExecutor->executeRequest<api::BatchState>(
        nx::network::http::Method::get,
        nx::network::http::rest::substituteParameters(kSystemUsersBatchStatePath, {batchId}),
        std::move(completionHandler));
}

void BatchUserProcessingManager::getBatchErrorInfo(
    const std::string& batchId,
    std::function<void(api::ResultCode, api::BatchErrorInfo)> completionHandler)
{
    m_requestsExecutor->executeRequest<api::BatchErrorInfo>(
        nx::network::http::Method::get,
        nx::network::http::rest::substituteParameters(kSystemUsersBatchErrorInfoPath, {batchId}),
        std::move(completionHandler));
}

} // namespace nx::cloud::db::client
