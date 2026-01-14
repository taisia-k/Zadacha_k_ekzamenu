#pragma once
#include <pqxx/pqxx>
#include <memory>
#include <string>
#include <vector>

template <typename T>
class DatabaseConnection {
public:
    explicit DatabaseConnection(const T& connectionString);
    ~DatabaseConnection();

    std::vector<std::vector<std::string>> executeQuery(const std::string& sql);
    void executeNonQuery(const std::string& sql);

    void beginTransaction();
    void commitTransaction();
    void rollbackTransaction();

    void createFunction(const std::string& sql);
    void createTrigger(const std::string& sql);

    bool getTransactionStatus() const;

    pqxx::connection& raw();

private:
    std::unique_ptr<pqxx::connection> conn_;
    std::unique_ptr<pqxx::work> tx_;
    bool txActive_{false};
};

#include "DatabaseConnection.tpp"
