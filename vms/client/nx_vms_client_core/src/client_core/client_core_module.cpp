// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "client_core_module.h"

#include <QtCore/QStandardPaths>
#include <QtQml/QQmlEngine>

#include <api/common_message_processor.h>
#include <nx/utils/app_info.h>
#include <nx/utils/log/log.h>
#include <nx/utils/timer_manager.h>
#include <nx/vms/client/core/application_context.h>
#include <nx/vms/client/core/ini.h>
#include <nx/vms/client/core/network/network_module.h>
#include <nx/vms/client/core/network/remote_connection.h>
#include <nx/vms/client/core/network/session_token_terminator.h>
#include <nx/vms/client/core/settings/client_core_settings.h>
#include <nx/vms/client/core/skin/skin_image_provider.h>
#include <nx/vms/client/core/system_context.h>
#include <nx/vms/client/core/thumbnails/remote_async_image_provider.h>

using namespace nx::vms::client::core;

static QnClientCoreModule* s_instance = nullptr;

struct QnClientCoreModule::Private
{
    std::unique_ptr<NetworkModule> networkModule;
    std::unique_ptr<SessionTokenTerminator> sessionTokenTerminator;
};

//-------------------------------------------------------------------------------------------------
// QnClientCoreModule

QnClientCoreModule::QnClientCoreModule(
    SystemContext* systemContext)
    :
    base_type(),
    SystemContextAware(systemContext),
    d(new Private())
{
    if (s_instance)
        NX_ERROR(this, "Singleton is created more than once.");
    else
        s_instance = this;

    d->sessionTokenTerminator = std::make_unique<SessionTokenTerminator>();

    if (const auto qmlEngine = appContext()->qmlEngine())
    {
        systemContext->storeToQmlContext(qmlEngine->rootContext());
        qmlEngine->addImageProvider("skin", new nx::vms::client::core::SkinImageProvider());
        qmlEngine->addImageProvider("remote", new nx::vms::client::core::RemoteAsyncImageProvider(
            systemContext));
    }
}

QnClientCoreModule::~QnClientCoreModule()
{
    if (const auto qmlEngine = appContext()->qmlEngine())
    {
        qmlEngine->removeImageProvider("remote");
        qmlEngine->removeImageProvider("skin");
    }

    if (s_instance == this)
        s_instance = nullptr;
}

QnClientCoreModule* QnClientCoreModule::instance()
{
    return s_instance;
}

void QnClientCoreModule::initializeNetworking(
    nx::vms::api::PeerType peerType,
    Qn::SerializationFormat serializationFormat)
{
    d->networkModule = std::make_unique<NetworkModule>(
        systemContext(),
        peerType,
        serializationFormat);

    connect(&appContext()->coreSettings()->certificateValidationLevel,
        &Settings::BaseProperty::changed,
        this,
        [this](nx::utils::property_storage::BaseProperty* /*property*/)
        {
            d->networkModule->reinitializeCertificateStorage();
        });
}

NetworkModule* QnClientCoreModule::networkModule() const
{
    return d->networkModule.get();
}

SessionTokenTerminator* QnClientCoreModule::sessionTokenTerminator() const
{
    return d->sessionTokenTerminator.get();
}
