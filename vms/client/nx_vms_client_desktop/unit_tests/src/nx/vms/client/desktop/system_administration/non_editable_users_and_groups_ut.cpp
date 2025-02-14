// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include <gtest/gtest.h>

#include <core/resource/user_resource.h>
#include <core/resource_management/resource_pool.h>
#include <nx/utils/qt_helpers.h>
#include <nx/utils/std/algorithm.h>
#include <nx/vms/client/core/access/access_controller.h>
#include <nx/vms/client/desktop/system_administration/watchers/non_editable_users_and_groups.h>
#include <nx/vms/client/desktop/system_context.h>
#include <nx/vms/client/desktop/test_support/test_context.h>
#include <nx/vms/common/user_management/predefined_user_groups.h>
#include <nx/vms/common/user_management/user_group_manager.h>

namespace nx::vms::client::desktop {

namespace test {

class NonEditableUsersAndGroupsTest: public nx::vms::client::desktop::test::ContextBasedTest
{
public:
    virtual void SetUp() override
    {
    }

    virtual void TearDown() override
    {
    }

    void loginAs(const QnUuid& userId)
    {
        auto resourcePool = systemContext()->resourcePool();
        auto user = resourcePool->getResourceById<QnUserResource>(userId);
        ASSERT_TRUE(!user.isNull());
        systemContext()->accessController()->setUser(user);

        QObject::connect(systemContext()->nonEditableUsersAndGroups(),
            &NonEditableUsersAndGroups::userModified,
            [this](const QnUserResourcePtr& user)
            {
                const auto action = systemContext()->nonEditableUsersAndGroups()->containsUser(user)
                    ? "added"
                    : "removed";

                signalLog << nx::format("%1 %2", action, user->getName());
            });

        QObject::connect(systemContext()->nonEditableUsersAndGroups(),
            &NonEditableUsersAndGroups::groupModified,
            [this](const QnUuid& groupId)
            {
                const auto action =
                    systemContext()->nonEditableUsersAndGroups()->containsGroup(groupId)
                        ? "added"
                        : "removed";

                const auto group = systemContext()->userGroupManager()->find(groupId);
                ASSERT_TRUE(group);

                signalLog << nx::format("%1 %2", action, group->name);
            });
    }

    QnUuid addUser(
        const QString& name,
        const std::vector<QnUuid>& parents,
        const api::UserType& userType = api::UserType::local)
    {
        QnUserResourcePtr user(
            new QnUserResource(userType, /*externalId*/ {}));
        user->setIdUnsafe(QnUuid::createUuid());
        user->setName(name);
        user->addFlags(Qn::remote);
        user->setGroupIds(parents);
        systemContext()->resourcePool()->addResource(user);
        return user->getId();
    }

    QnUuid addGroup(const QString& name, const std::vector<QnUuid>& parents)
    {
        api::UserGroupData group;
        group.setId(QnUuid::createUuid());
        group.name = name;
        group.parentGroupIds = parents;
        systemContext()->userGroupManager()->addOrUpdate(group);
        return group.id;
    }

    void removeUser(const QnUuid& id)
    {
        auto resourcePool = systemContext()->resourcePool();
        auto resource = resourcePool->getResourceById<QnUserResource>(id);
        ASSERT_TRUE(!resource.isNull());
        resourcePool->removeResource(resource);
    }

    void removeGroup(const QnUuid& id)
    {
        const auto allUsers = systemContext()->resourcePool()->getResources<QnUserResource>();
        for (const auto& user: allUsers)
        {
            std::vector<QnUuid> ids = user->groupIds();
            if (nx::utils::erase_if(ids, [&user](auto id){ return id == user->getId(); }))
                user->setGroupIds(ids);
        }

        for (auto group: systemContext()->userGroupManager()->groups())
        {
            const auto isTargetGroup = [&group](auto id){ return id == group.id; };
            if (nx::utils::erase_if(group.parentGroupIds, isTargetGroup))
                systemContext()->userGroupManager()->addOrUpdate(group);
        }

        systemContext()->userGroupManager()->remove(id);
    }

    void updateUser(const QnUuid& id, const std::vector<QnUuid>& parents)
    {
        auto resourcePool = systemContext()->resourcePool();
        auto user = resourcePool->getResourceById<QnUserResource>(id);
        ASSERT_TRUE(!user.isNull());
        user->setGroupIds(parents);
    }

    void updateGroup(const QnUuid& id, const std::vector<QnUuid>& parents)
    {
        auto group = systemContext()->userGroupManager()->find(id).value_or(api::UserGroupData{});
        ASSERT_EQ(id, group.id);
        group.parentGroupIds = parents;
        systemContext()->userGroupManager()->addOrUpdate(group);
    }

    bool isUserEditable(const QnUuid& id)
    {
        return !systemContext()->nonEditableUsersAndGroups()->containsUser(
            systemContext()->resourcePool()->getResourceById<QnUserResource>(id));
    }

    bool isGroupEditable(const QnUuid& id)
    {
        return !systemContext()->nonEditableUsersAndGroups()->containsGroup(id);
    }

    bool checkSignalLog(QList<QString> value)
    {
        signalLog.sort();
        value.sort();
        const bool result = value == signalLog;
        signalLog.clear();
        return result;
    }

    const QSet<QnUuid> kPredefinedGroups = nx::utils::toQSet(api::kPredefinedGroupIds);
    QList<QString> signalLog;
};

TEST_F(NonEditableUsersAndGroupsTest, adminIsNonEditableByPowerUser)
{
    auto power = addUser("power", {api::kPowerUsersGroupId});
    loginAs(power);

    auto admin = addUser("admin", {api::kAdministratorsGroupId});

    ASSERT_EQ(kPredefinedGroups, systemContext()->nonEditableUsersAndGroups()->groups());

    ASSERT_FALSE(isUserEditable(admin));
}

TEST_F(NonEditableUsersAndGroupsTest, adminIsNonEditableByAdmin)
{
    auto admin = addUser("admin", {api::kAdministratorsGroupId});

    loginAs(admin);

    ASSERT_EQ(kPredefinedGroups, systemContext()->nonEditableUsersAndGroups()->groups());

    ASSERT_FALSE(isUserEditable(admin));
}

TEST_F(NonEditableUsersAndGroupsTest, powerUserNotEditableByPowerUser)
{
    auto poweruser = addUser("poweruser", {api::kPowerUsersGroupId});
    loginAs(poweruser);

    auto group = addGroup("group", {});
    auto user = addUser("user", {group});

    ASSERT_TRUE(isUserEditable(user));
    ASSERT_TRUE(isGroupEditable(group));

    updateGroup(group, {api::kPowerUsersGroupId});

    ASSERT_FALSE(isUserEditable(user));
    ASSERT_FALSE(isGroupEditable(group));
}

TEST_F(NonEditableUsersAndGroupsTest, usersPreventParentGroupFromDelete)
{
    auto poweruser = addUser("poweruser", {api::kPowerUsersGroupId});

    loginAs(poweruser);

    auto group1 = addGroup("group1", {});
    auto group2 = addGroup("group2", {});

    auto power1 = addUser("power1", {api::kPowerUsersGroupId});
    auto power2 = addUser("power2", {api::kPowerUsersGroupId});

    ASSERT_TRUE(isGroupEditable(group1));
    ASSERT_TRUE(isGroupEditable(group2));

    // Add non-editable user to both groups so they become non-editable.
    updateUser(power1, {api::kPowerUsersGroupId, group1, group2});

    ASSERT_FALSE(isGroupEditable(group1));
    ASSERT_FALSE(isGroupEditable(group2));
    ASSERT_TRUE(checkSignalLog({
        "added power1",
        "added power2",
        "added group1",
        "added group2"
        }));

    // Adding another non-editable groups changes nothing.
    updateUser(power2, {api::kPowerUsersGroupId, group1, group2});

    ASSERT_FALSE(isGroupEditable(group1));
    ASSERT_FALSE(isGroupEditable(group2));
    ASSERT_TRUE(checkSignalLog({}));

    // Removing the first non-editable groups changes nothing.
    updateUser(power1, {api::kPowerUsersGroupId});

    ASSERT_FALSE(isGroupEditable(group1));
    ASSERT_FALSE(isGroupEditable(group2));
    ASSERT_TRUE(checkSignalLog({}));

    // Removing last non-editable user makes the groups editable.
    updateUser(power2, {group1, group2});

    ASSERT_TRUE(isGroupEditable(group1));
    ASSERT_TRUE(isGroupEditable(group2));

    ASSERT_TRUE(checkSignalLog({
        "removed power2",
        "removed group1",
        "removed group2"
        }));
}

TEST_F(NonEditableUsersAndGroupsTest, groupPreventsParentGroupFromDelete)
{
    auto poweruser = addUser("poweruser", {api::kPowerUsersGroupId});
    loginAs(poweruser);

    auto group = addGroup("group", {});
    auto subGroup = addGroup("subGroup", {group});

    ASSERT_TRUE(isGroupEditable(group));

    updateGroup(subGroup, {group, api::kPowerUsersGroupId});

    ASSERT_FALSE(isGroupEditable(group));
}

TEST_F(NonEditableUsersAndGroupsTest, cycleGroup)
{
    auto poweruser = addUser("poweruser", {api::kPowerUsersGroupId});

    loginAs(poweruser);

    auto group = addGroup("group", {});
    auto subGroup = addGroup("subGroup", {group});
    auto user = addUser("user", {subGroup});

    ASSERT_TRUE(isUserEditable(user));
    ASSERT_TRUE(isGroupEditable(group));
    ASSERT_TRUE(isGroupEditable(subGroup));

    // Introduce group <-> subGroup cycle and make PowerUsers a parent, so all become non-editable.
    updateGroup(group, {api::kPowerUsersGroupId, subGroup});

    ASSERT_FALSE(isUserEditable(user));
    ASSERT_FALSE(isGroupEditable(group));
    ASSERT_FALSE(isGroupEditable(subGroup));
    ASSERT_TRUE(checkSignalLog({
        "added user",
        "added group",
        "added subGroup"
        }));

    // Remove a cycle and PowerUsers, all becomes editable.
    updateGroup(group, {});

    ASSERT_TRUE(isUserEditable(user));
    ASSERT_TRUE(isGroupEditable(group));
    ASSERT_TRUE(isGroupEditable(subGroup));

    ASSERT_TRUE(checkSignalLog({
        "removed user",
        "removed group",
        "removed subGroup"
        }));
}

TEST_F(NonEditableUsersAndGroupsTest, nonEditablePropagatesDown)
{
    auto poweruser = addUser("poweruser", {api::kPowerUsersGroupId});
    loginAs(poweruser);

    /* When 'group' becomes non-editable, only 'sideGroup1' should remain editable.

      PowerUsers
          |
        group       sideGroup1
          |             |
        subGroup    sideGroup2
                \   /
                user
    */

    auto sideGroup1 = addGroup("sideGroup1", {});
    auto sideGroup2 = addGroup("sideGroup2", {sideGroup1});

    auto group = addGroup("group", {});
    auto subGroup = addGroup("subGroup", {group});
    auto user = addUser("user", {subGroup, sideGroup2});

    ASSERT_TRUE(isUserEditable(user));
    ASSERT_TRUE(isGroupEditable(group));
    ASSERT_TRUE(isGroupEditable(subGroup));
    ASSERT_TRUE(isGroupEditable(sideGroup1));
    ASSERT_TRUE(isGroupEditable(sideGroup2));

    // Add PowerUsers parent.
    updateGroup(group, {api::kPowerUsersGroupId});

    ASSERT_FALSE(isUserEditable(user));
    ASSERT_FALSE(isGroupEditable(group));
    ASSERT_FALSE(isGroupEditable(subGroup));
    ASSERT_TRUE(isGroupEditable(sideGroup1)); //< Remains editable.
    ASSERT_FALSE(isGroupEditable(sideGroup2));
    ASSERT_EQ(
        (kPredefinedGroups + QSet{group, subGroup, sideGroup2}),
        systemContext()->nonEditableUsersAndGroups()->groups());

    // User and poweruser are non-editable.
    ASSERT_EQ(2, systemContext()->nonEditableUsersAndGroups()->userCount());

    ASSERT_TRUE(checkSignalLog({
        "added group",
        "added subGroup",
        "added sideGroup2",
        "added user"
        }));

    // Remove PowerUsers parent.
    updateGroup(group, {});

    ASSERT_TRUE(isUserEditable(user));
    ASSERT_TRUE(isGroupEditable(group));
    ASSERT_TRUE(isGroupEditable(subGroup));
    ASSERT_TRUE(isGroupEditable(sideGroup1));
    ASSERT_TRUE(isGroupEditable(sideGroup2));
    ASSERT_TRUE(checkSignalLog({
        "removed group",
        "removed subGroup",
        "removed sideGroup2",
        "removed user"
        }));
}

TEST_F(NonEditableUsersAndGroupsTest, newUserPermissionsAreMonitored)
{
    auto poweruser = addUser("poweruser", {api::kPowerUsersGroupId});

    loginAs(poweruser);

    auto user = addUser("user", {});

    ASSERT_TRUE(isUserEditable(user));
    updateUser(user, {api::kPowerUsersGroupId});
    ASSERT_FALSE(isUserEditable(user));
}

TEST_F(NonEditableUsersAndGroupsTest, newGroupPermissionsAreMonitored)
{
    auto poweruser = addUser("poweruser", {api::kPowerUsersGroupId});

    loginAs(poweruser);

    auto group = addGroup("group", {});

    ASSERT_TRUE(isGroupEditable(group));
    updateGroup(group, {api::kPowerUsersGroupId});
    ASSERT_FALSE(isGroupEditable(group));
}

} // namespace test

} // namespace nx::vms::client::desktop
