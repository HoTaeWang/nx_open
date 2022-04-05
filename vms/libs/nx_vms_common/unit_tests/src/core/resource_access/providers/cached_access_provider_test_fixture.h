// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <gtest/gtest.h>

#include <common/common_module.h>
#include <common/static_common_module.h>
#include <core/resource_access/providers/abstract_resource_access_provider.h>
#include <core/resource_access/resource_access_subject.h>
#include <nx/vms/common/test_support/resource/resource_pool_test_helper.h>

namespace nx::core::access {

class AbstractResourceAccessProvider;

namespace test {

class CachedAccessProviderTestFixture: public testing::Test,
    protected QnResourcePoolTestHelper
{
protected:
    virtual void SetUp();
    virtual void TearDown();

    void setupAwaitAccess();
    virtual AbstractResourceAccessProvider* accessProvider() const = 0;

    using Source = nx::core::access::Source;

    void expectAccess(const QnResourceAccessSubject& subject, const QnResourcePtr& resource,
        Source value);

private:
    struct AccessKey
    {
        QnUuid subject;
        QnUuid resource;

        bool operator<(const AccessKey& rhs) const;
        bool operator==(const AccessKey& rhs) const;
    };

private:
    std::unique_ptr<QnStaticCommonModule> m_staticCommon;
    std::unique_ptr<QnCommonModule> m_module;

    nx::Mutex m_mutex;
    nx::WaitCondition m_condition;
    std::map<AccessKey, Source> m_notifiedAccess;
};

} // namespace test
} // namespace nx::core::access
