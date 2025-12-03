#include "AccountManager.h"

#include <algorithm>

#include "StorageManager.h"
#include "cocos2d.h"

using namespace cocos2d;

AccountManager& AccountManager::getInstance()
{
    static AccountManager instance;

    return instance;
}

bool AccountManager::initialize()
{
    loadFromStorage();

    return _activeIndex >= 0 && _activeIndex < (int)_accounts.size();
}

const AccountInfo* AccountManager::getCurrentAccount() const
{
    if (_activeIndex >= 0 && _activeIndex < (int)_accounts.size())
    {
        return &_accounts[_activeIndex];
    }

    return nullptr;
}

bool AccountManager::switchAccount(const std::string& userId)
{
    for (size_t i = 0; i < _accounts.size(); ++i)
    {
        if (_accounts[i].userId == userId)
        {
            _activeIndex = (int)i;

            save();

            return true;
        }
    }

    return false;
}

// 验证账号密码是否匹配
bool AccountManager::verifyPassword(const std::string& userId, const std::string& password) const
{
    for (const auto& acc : _accounts)
    {
        if (acc.userId == userId)
        {
            return acc.password == password;
        }
    }
    return false;
}

void AccountManager::upsertAccount(const AccountInfo& acc)
{
    auto it =
        std::find_if(_accounts.begin(), _accounts.end(), [&](const AccountInfo& a) { return a.userId == acc.userId; });

    if (it != _accounts.end())
    {
        *it = acc;

        _activeIndex = (int)std::distance(_accounts.begin(), it);
    }
    else
    {
        _accounts.push_back(acc);

        _activeIndex = (int)_accounts.size() - 1;
    }

    save();
}

const std::vector<AccountInfo>& AccountManager::listAccounts() const
{
    return _accounts;
}

void AccountManager::signOut()
{
    _activeIndex = -1;

    save();
}

void AccountManager::save()
{
    ValueMap root;

    root["activeIndex"] = Value(_activeIndex);

    ValueVector arr;

    arr.reserve(_accounts.size());

    for (const auto& a : _accounts)
    {
        ValueMap vm;

        vm["userId"] = Value(a.userId);

        vm["username"] = Value(a.username);

        vm["password"] = Value(a.password); // 保存密码

        vm["token"] = Value(a.token);

        arr.push_back(Value(vm));
    }

    root["accounts"] = Value(arr);

    StorageManager::getInstance().setValueMap("accounts", root);

    StorageManager::getInstance().flush();
}

void AccountManager::loadFromStorage()
{
    auto root = StorageManager::getInstance().getValueMap("accounts");

    _accounts.clear();

    _activeIndex = -1;

    if (root.empty())
        return;

    auto itIdx = root.find("activeIndex");

    if (itIdx != root.end())
    {
        _activeIndex = itIdx->second.asInt();
    }

    auto itArr = root.find("accounts");

    if (itArr != root.end())
    {
        const ValueVector& arr = itArr->second.asValueVector();

        for (const auto& v : arr)
        {
            const ValueMap& vm = v.asValueMap();

            AccountInfo a;

            auto it = vm.find("userId");
            if (it != vm.end())
                a.userId = it->second.asString();

            it = vm.find("username");
            if (it != vm.end())
                a.username = it->second.asString();

            it = vm.find("password");
            if (it != vm.end())
                a.password = it->second.asString();

            it = vm.find("token");
            if (it != vm.end())
                a.token = it->second.asString();

            _accounts.push_back(a);
        }
    }

    if (_activeIndex < 0 || _activeIndex >= (int)_accounts.size())
    {
        _activeIndex = _accounts.empty() ? -1 : 0;
    }
}
