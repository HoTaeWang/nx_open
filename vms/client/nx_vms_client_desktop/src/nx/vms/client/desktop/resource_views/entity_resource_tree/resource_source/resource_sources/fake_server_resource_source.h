// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <core/resource/resource_fwd.h>
#include <nx/vms/client/desktop/resource_views/entity_resource_tree/resource_source/abstract_resource_source.h>

class QnResourcePool;

namespace nx::vms::client::desktop {
namespace entity_resource_tree {

class FakeServerResourceSource: public AbstractResourceSource
{
    Q_OBJECT
    using base_type = AbstractResourceSource;

public:
    FakeServerResourceSource(const QnResourcePool* resourcePool);

    virtual QVector<QnResourcePtr> getResources() override;

private:
    const QnResourcePool* m_resourcePool;
};

} // namespace entity_resource_tree
} // namespace nx::vms::client::desktop
