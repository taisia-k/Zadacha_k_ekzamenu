#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Models.h"
#include "DatabaseConnection.h"

enum class Role { Admin, Manager, Customer };

class User {
public:
    User(int id, std::string name, Role role);
    virtual ~User() = default;

    int id() const;
    Role role() const;

    virtual std::shared_ptr<Order> createOrder() = 0;
    virtual std::string viewOrderStatus(DatabaseConnection<std::string>& db, int orderId) = 0;
    virtual void cancelOrder(DatabaseConnection<std::string>& db, int orderId) = 0;

    const std::vector<std::shared_ptr<Order>>& orders() const;
    void addLocalOrder(const std::shared_ptr<Order>& o);

protected:
    int userId_{};
    std::string name_;
    Role role_{Role::Customer};
    std::vector<std::shared_ptr<Order>> orders_;
};

class Admin : public User {
public:
    Admin(int id, std::string name);

    std::shared_ptr<Order> createOrder() override;
    std::string viewOrderStatus(DatabaseConnection<std::string>& db, int orderId) override;
    void cancelOrder(DatabaseConnection<std::string>& db, int orderId) override;

    void addProduct(DatabaseConnection<std::string>& db, const std::string& name, double price, int stock);
    void updateProduct(DatabaseConnection<std::string>& db, int productId, double price, int stock);
    void deleteProduct(DatabaseConnection<std::string>& db, int productId);

    void viewAllOrders(DatabaseConnection<std::string>& db);
    void updateOrderStatus(DatabaseConnection<std::string>& db, int orderId, const std::string& status);

    void viewOrderHistory(DatabaseConnection<std::string>& db, int orderId);
    void viewAuditLog(DatabaseConnection<std::string>& db);

    void exportCsvReport(DatabaseConnection<std::string>& db, const std::string& path);
};

class Manager : public User {
public:
    Manager(int id, std::string name);

    std::shared_ptr<Order> createOrder() override;
    std::string viewOrderStatus(DatabaseConnection<std::string>& db, int orderId) override;
    void cancelOrder(DatabaseConnection<std::string>& db, int orderId) override;

    void approveOrder(DatabaseConnection<std::string>& db, int orderId);
    void updateStock(DatabaseConnection<std::string>& db, int productId, int stock);
    void updateOrderStatus(DatabaseConnection<std::string>& db, int orderId, const std::string& status);

    void viewPending(DatabaseConnection<std::string>& db);
};

class Customer : public User {
public:
    Customer(int id, std::string name);

    std::shared_ptr<Order> createOrder() override;
    std::string viewOrderStatus(DatabaseConnection<std::string>& db, int orderId) override;
    void cancelOrder(DatabaseConnection<std::string>& db, int orderId) override;

    void addToOrder(const std::shared_ptr<Order>& order, int productId, int qty, double price);
    void removeFromOrder(const std::shared_ptr<Order>& order, int productId);

    void makePayment(const std::shared_ptr<Order>& order, int strategyType);
    void returnProduct(DatabaseConnection<std::string>& db, int orderId);
};
