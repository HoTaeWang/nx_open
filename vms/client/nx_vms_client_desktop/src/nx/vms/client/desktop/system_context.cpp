// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "system_context.h"

#include <QtQml/QQmlEngine> //< For registering types.

#include <api/media_server_statistics_manager.h>
#include <api/runtime_info_manager.h>
#include <camera/camera_bookmarks_manager.h>
#include <camera/camera_data_manager.h>
#include <client/client_message_processor.h>
#include <client/client_runtime_settings.h>
#include <core/resource/resource.h>
#include <core/resource_management/incompatible_server_watcher.h>
#include <nx/branding.h>
#include <nx/vms/client/core/network/remote_connection.h>
#include <nx/vms/client/desktop/access/access_controller.h>
#include <nx/vms/client/desktop/access/caching_access_controller.h>
#include <nx/vms/client/desktop/analytics/analytics_entities_tree.h>
#include <nx/vms/client/desktop/analytics/analytics_taxonomy_manager.h>
#include <nx/vms/client/desktop/ini.h>
#include <nx/vms/client/desktop/intercom/intercom_manager.h>
#include <nx/vms/client/desktop/resource/layout_snapshot_manager.h>
#include <nx/vms/client/desktop/resource/local_resources_initializer.h>
#include <nx/vms/client/desktop/resource/rest_api_helper.h>
#include <nx/vms/client/desktop/server_runtime_events/server_runtime_event_connector.h>
#include <nx/vms/client/desktop/settings/system_specific_local_settings.h>
#include <nx/vms/client/desktop/showreel/showreel_state_manager.h>
#include <nx/vms/client/desktop/statistics/statistics_sender.h>
#include <nx/vms/client/desktop/system_administration/watchers/logs_management_watcher.h>
#include <nx/vms/client/desktop/system_administration/watchers/non_editable_users_and_groups.h>
#include <nx/vms/client/desktop/system_health/default_password_cameras_watcher.h>
#include <nx/vms/client/desktop/system_health/system_health_state.h>
#include <nx/vms/client/desktop/system_logon/logic/delayed_data_loader.h>
#include <nx/vms/client/desktop/system_logon/logic/remote_session.h>
#include <nx/vms/client/desktop/utils/ldap_status_watcher.h>
#include <nx/vms/client/desktop/utils/video_cache.h>
#include <nx/vms/client/desktop/videowall/desktop_camera_initializer.h>
#include <nx/vms/client/desktop/videowall/videowall_online_screens_watcher.h>
#include <nx/vms/client/desktop/virtual_camera/virtual_camera_manager.h>
#include <nx/vms/common/system_settings.h>
#include <server/server_storage_manager.h>

#include "application_context.h"

namespace nx::vms::client::desktop {

namespace {

Qn::SerializationFormat serializationFormat()
{
    return ini().forceJsonConnection ? Qn::SerializationFormat::json : Qn::SerializationFormat::ubjson;
}

} // namespace

struct SystemContext::Private
{
    SystemContext* const q;
    std::unique_ptr<VideoWallOnlineScreensWatcher> videoWallOnlineScreensWatcher;
    std::unique_ptr<LdapStatusWatcher> ldapStatusWatcher;
    std::unique_ptr<QnIncompatibleServerWatcher> incompatibleServerWatcher;
    std::unique_ptr<ServerRuntimeEventConnector> serverRuntimeEventConnector;
    std::unique_ptr<QnServerStorageManager> serverStorageManager;
    std::unique_ptr<QnCameraBookmarksManager> cameraBookmarksManager;
    std::unique_ptr<QnCameraDataManager> cameraDataManager;
    std::unique_ptr<StatisticsSender> statisticsSender;
    std::unique_ptr<VirtualCameraManager> virtualCameraManager;
    std::unique_ptr<VideoCache> videoCache;
    std::unique_ptr<LocalResourcesInitializer> localResourcesInitializer;
    std::unique_ptr<LayoutSnapshotManager> layoutSnapshotManager;
    std::unique_ptr<ShowreelStateManager> showreelStateManager;
    std::unique_ptr<LogsManagementWatcher> logsManagementWatcher;
    std::unique_ptr<QnMediaServerStatisticsManager> mediaServerStatisticsManager;
    std::unique_ptr<SystemSpecificLocalSettings> localSettings;
    std::unique_ptr<RestApiHelper> restApiHelper;
    std::unique_ptr<DelayedDataLoader> delayedDataLoader;
    std::unique_ptr<analytics::TaxonomyManager> taxonomyManager;
    std::unique_ptr<NonEditableUsersAndGroups> nonEditableUsersAndGroups;
    std::unique_ptr<DefaultPasswordCamerasWatcher> defaultPasswordCamerasWatcher;
    std::unique_ptr<DesktopCameraInitializer> desktopCameraInitializer;
    std::unique_ptr<IntercomManager> intercomManager;
    std::unique_ptr<AnalyticsEventsSearchTreeBuilder> analyticsEventsSearchTreeBuilder;
    std::unique_ptr<SystemHealthState> systemHealthState;

    void initLocalRuntimeInfo()
    {
        nx::vms::api::RuntimeData runtimeData;
        runtimeData.peer.id = q->peerId();
        runtimeData.peer.instanceId = q->sessionId();
        runtimeData.peer.peerType = appContext()->localPeerType();
        runtimeData.peer.dataFormat = serializationFormat();
        runtimeData.brand = ini().developerMode ? QString() : nx::branding::brand();
        runtimeData.customization = ini().developerMode ? QString() : nx::branding::customization();
        runtimeData.videoWallInstanceGuid = appContext()->videoWallInstanceId();
        q->runtimeInfoManager()->updateLocalItem(runtimeData);
    }
};

SystemContext::SystemContext(
    Mode mode,
    QnUuid peerId,
    nx::core::access::Mode resourceAccessMode,
    QObject* parent)
    :
    base_type(mode, std::move(peerId), resourceAccessMode, parent),
    d(new Private{.q = this})
{
    resetAccessController(mode == Mode::client || mode == Mode::unitTests
        ? new CachingAccessController(this)
        : new AccessController(this));

    switch (mode)
    {
        case Mode::client:
            d->initLocalRuntimeInfo();
            d->videoWallOnlineScreensWatcher = std::make_unique<VideoWallOnlineScreensWatcher>(
                this);
            d->incompatibleServerWatcher = std::make_unique<QnIncompatibleServerWatcher>(this);
            d->serverRuntimeEventConnector = std::make_unique<ServerRuntimeEventConnector>();
            // Depends on ServerRuntimeEventConnector.
            d->serverStorageManager = std::make_unique<QnServerStorageManager>(this);
            d->cameraBookmarksManager = std::make_unique<QnCameraBookmarksManager>(this);
            d->cameraDataManager = std::make_unique<QnCameraDataManager>(this);
            d->statisticsSender = std::make_unique<StatisticsSender>(this);
            d->virtualCameraManager = std::make_unique<VirtualCameraManager>(this);
            d->videoCache = std::make_unique<VideoCache>(this);
            // LocalResourcesInitializer must be created before LayoutSnapshotManager to avoid
            // modifying layouts after they are opened.
            d->localResourcesInitializer = std::make_unique<LocalResourcesInitializer>(this);
            d->layoutSnapshotManager = std::make_unique<LayoutSnapshotManager>(this);
            d->showreelStateManager = std::make_unique<ShowreelStateManager>(this);
            d->logsManagementWatcher = std::make_unique<LogsManagementWatcher>(this);
            d->mediaServerStatisticsManager = std::make_unique<QnMediaServerStatisticsManager>(
                this);
            d->localSettings = std::make_unique<SystemSpecificLocalSettings>(this);
            d->restApiHelper = std::make_unique<RestApiHelper>(this);
            d->delayedDataLoader = std::make_unique<DelayedDataLoader>(this);
            d->taxonomyManager = std::make_unique<analytics::TaxonomyManager>(this);
            d->ldapStatusWatcher = std::make_unique<LdapStatusWatcher>(this);
            d->nonEditableUsersAndGroups = std::make_unique<NonEditableUsersAndGroups>(this);
            d->defaultPasswordCamerasWatcher = std::make_unique<DefaultPasswordCamerasWatcher>(
                this);
            break;

        case Mode::crossSystem:
            d->cameraBookmarksManager = std::make_unique<QnCameraBookmarksManager>(this);
            d->cameraDataManager = std::make_unique<QnCameraDataManager>(this);
            d->videoCache = std::make_unique<VideoCache>(this);
            d->mediaServerStatisticsManager = std::make_unique<QnMediaServerStatisticsManager>(
                this);
            break;

        case Mode::cloudLayouts:
            d->layoutSnapshotManager = std::make_unique<LayoutSnapshotManager>(this);
            break;

        case Mode::unitTests:
            d->nonEditableUsersAndGroups = std::make_unique<NonEditableUsersAndGroups>(this);
            break;
    }

    connect(this, &SystemContext::userChanged, this,
        [this](const QnUserResourcePtr& user)
        {
            if (d->virtualCameraManager)
                d->virtualCameraManager->setCurrentUser(user);
        });
}

SystemContext::~SystemContext()
{
}

void SystemContext::registerQmlType()
{
    qmlRegisterUncreatableType<SystemContext>("nx.vms.client.desktop", 1, 0, "SystemContext",
        "Cannot create instance of SystemContext.");
}

SystemContext* SystemContext::fromResource(const QnResourcePtr& resource)
{
    if (!resource)
        return {};

    return dynamic_cast<SystemContext*>(resource->systemContext());
}

void SystemContext::initializeDesktopCamera()
{
    d->desktopCameraInitializer = std::make_unique<DesktopCameraInitializer>(this);
}

std::shared_ptr<RemoteSession> SystemContext::session() const
{
    return std::dynamic_pointer_cast<RemoteSession>(base_type::session());
}

AnalyticsEventsSearchTreeBuilder* SystemContext::analyticsEventsSearchTreeBuilder() const
{
    return d->analyticsEventsSearchTreeBuilder.get();
}

VideoWallOnlineScreensWatcher* SystemContext::videoWallOnlineScreensWatcher() const
{
    return d->videoWallOnlineScreensWatcher.get();
}

LdapStatusWatcher* SystemContext::ldapStatusWatcher() const
{
    return d->ldapStatusWatcher.get();
}

NonEditableUsersAndGroups* SystemContext::nonEditableUsersAndGroups() const
{
    return d->nonEditableUsersAndGroups.get();
}

ServerRuntimeEventConnector* SystemContext::serverRuntimeEventConnector() const
{
    return d->serverRuntimeEventConnector.get();
}

QnServerStorageManager* SystemContext::serverStorageManager() const
{
    return d->serverStorageManager.get();
}

QnCameraBookmarksManager* SystemContext::cameraBookmarksManager() const
{
    return d->cameraBookmarksManager.get();
}

QnCameraDataManager* SystemContext::cameraDataManager() const
{
    return d->cameraDataManager.get();
}

VirtualCameraManager* SystemContext::virtualCameraManager() const
{
    return d->virtualCameraManager.get();
}

VideoCache* SystemContext::videoCache() const
{
    return d->videoCache.get();
}

LayoutSnapshotManager* SystemContext::layoutSnapshotManager() const
{
    return d->layoutSnapshotManager.get();
}

ShowreelStateManager* SystemContext::showreelStateManager() const
{
    return d->showreelStateManager.get();
}

LogsManagementWatcher* SystemContext::logsManagementWatcher() const
{
    return d->logsManagementWatcher.get();
}

QnMediaServerStatisticsManager* SystemContext::mediaServerStatisticsManager() const
{
    return d->mediaServerStatisticsManager.get();
}

SystemSpecificLocalSettings* SystemContext::localSettings() const
{
    return d->localSettings.get();
}

RestApiHelper* SystemContext::restApiHelper() const
{
    return d->restApiHelper.get();
}

QnUuid SystemContext::localSystemId() const
{
    const auto& currentConnection = connection();
    return currentConnection ? currentConnection->moduleInformation().localSystemId : QnUuid();
}

analytics::TaxonomyManager* SystemContext::taxonomyManager() const
{
    QQmlEngine::setObjectOwnership(d->taxonomyManager.get(), QQmlEngine::CppOwnership);
    return d->taxonomyManager.get();
}

common::SessionTokenHelperPtr SystemContext::getSessionTokenHelper() const
{
    return d->restApiHelper->getSessionTokenHelper();
}

DefaultPasswordCamerasWatcher* SystemContext::defaultPasswordCamerasWatcher() const
{
    return d->defaultPasswordCamerasWatcher.get();
}

SystemHealthState* SystemContext::systemHealthState() const
{
    return d->systemHealthState.get();
}

void SystemContext::setMessageProcessor(QnCommonMessageProcessor* messageProcessor)
{
    base_type::setMessageProcessor(messageProcessor);
    if (mode() != Mode::client)
        return;

    auto clientMessageProcessor = qobject_cast<QnClientMessageProcessor*>(messageProcessor);
    if (!NX_ASSERT(clientMessageProcessor, "Invalid message processor type"))
        return;

    d->incompatibleServerWatcher->setMessageProcessor(clientMessageProcessor);
    d->serverRuntimeEventConnector->setMessageProcessor(clientMessageProcessor);
    d->logsManagementWatcher->setMessageProcessor(clientMessageProcessor);
    d->intercomManager = std::make_unique<IntercomManager>(this);
    d->analyticsEventsSearchTreeBuilder = std::make_unique<AnalyticsEventsSearchTreeBuilder>(this);
    d->systemHealthState = std::make_unique<SystemHealthState>(this);

    // Desktop camera must work in the normal mode only.
    if (appContext()->runtimeSettings()->isDesktopMode())
        initializeDesktopCamera();
}

} // namespace nx::vms::client::desktop
