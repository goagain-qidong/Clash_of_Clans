#pragma once

#include <string>

#include <vector>

#include <functional>



// Simple account model

struct AccountInfo {

    std::string userId;   // unique id

    std::string username; // display name

    std::string password; // account password (plaintext,建议加密存储)

    std::string token;    // auth token (optional)

};



class AccountManager {

public:

    static AccountManager& getInstance();



    // Initialize from storage. Returns true if an account was restored.

    bool initialize();



    // Get current account.

    const AccountInfo* getCurrentAccount() const;



    // Switch active account by userId. Returns true if success.

    bool switchAccount(const std::string& userId);



    // Create or update an account and set it active.

    void upsertAccount(const AccountInfo& acc);



    // List all accounts stored locally.

    const std::vector<AccountInfo>& listAccounts() const;



    // Sign out current account.

    void signOut();



    // Verify account password.

    bool verifyPassword(const std::string& userId, const std::string& password) const;



    // Persist current state to storage.

    void save();



private:

    AccountManager() = default;

    AccountManager(const AccountManager&) = delete;

    AccountManager& operator=(const AccountManager&) = delete;



    std::vector<AccountInfo> _accounts;

    int _activeIndex = -1;



    // Internal helpers

    void loadFromStorage();

};

