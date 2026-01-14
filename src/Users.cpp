#include "Users.h"
#include "Utils.h"
#include <iostream>
#include <algorithm>
#include <numeric>

User::User(int id, std::string name, Role role)
    : userId_(id), name_(std::move(name)), role_(role) {}

int User::id() const { return userId_; }
Role User::role() const { return role_; }

const std::vector<std::shared_ptr<Order>>& User::orders() const { return orders_; }
void User::addLocalOrder(const std::shared_ptr<Order>& o) { orders_.push_back(o); }


Admin::Admin(int id, std::string name) : User(id, std::move(name), Role::Admin) {}

std::shared_ptr<Order> Admin::createOrder() {
    auto o = std::make_shared<Order>(0, userId_);
    addLocalOrder(o);
    return o;
}

std::string Admin::viewOrderStatus(DatabaseConnection<std::string>& db, int orderId) {
    auto r = db.executeQuery("SELECT getOrderStatus(" + std::to_string(orderId) + ")");
    return r.empty() || r[0].empty() ? "" : r[0][0];
}

void Admin::cancelOrder(DatabaseConnection<std::string>& db, int orderId) {
    callProcUpdateStatus(db, orderId, "canceled", userId_);
}

void Admin::addProduct(DatabaseConnection<std::string>& db, const std::string& name, double price, int stock) {
    db.beginTransaction();
    try {
        std::string sql =
            "INSERT INTO products(product_id,name,price,stock_quantity) VALUES ("
            "nextval('seq_products'), " + quote(db, name) + ", " + std::to_string(price) + ", " + std::to_string(stock) + ")";
        db.executeNonQuery(sql);
        db.commitTransaction();
    } catch (const std::exception& e) {
        db.rollbackTransaction();
        logError(db, userId_, "update", std::string("addProduct: ") + e.what());
        throw;
    }
}

void Admin::updateProduct(DatabaseConnection<std::string>& db, int productId, double price, int stock) {
    db.beginTransaction();
    try {
        db.executeNonQuery(
            "UPDATE products SET price=" + std::to_string(price) + ", stock_quantity=" + std::to_string(stock) +
            " WHERE product_id=" + std::to_string(productId)
        );
        db.commitTransaction();
    } catch (const std::exception& e) {
        db.rollbackTransaction();
        logError(db, userId_, "update", std::string("updateProduct: ") + e.what());
        throw;
    }
}

void Admin::deleteProduct(DatabaseConnection<std::string>& db, int productId) {
    db.beginTransaction();
    try {
        db.executeNonQuery("DELETE FROM products WHERE product_id=" + std::to_string(productId));
        db.commitTransaction();
    } catch (const std::exception& e) {
        db.rollbackTransaction();
        logError(db, userId_, "delete", std::string("deleteProduct: ") + e.what());
        throw;
    }
}

void Admin::viewAllOrders(DatabaseConnection<std::string>& db) {
    auto r = db.executeQuery("SELECT order_id,user_id,status,total_price,order_date FROM orders ORDER BY order_id");
    for (const auto& row : r) {
        std::cout << "order_id=" << row[0] << " user=" << row[1] << " status=" << row[2]
                  << " total=" << row[3] << " date=" << row[4] << "\n";
    }
}

void Admin::updateOrderStatus(DatabaseConnection<std::string>& db, int orderId, const std::string& status) {
    callProcUpdateStatus(db, orderId, status, userId_);
}

void Admin::viewOrderHistory(DatabaseConnection<std::string>& db, int orderId) {
    auto r = db.executeQuery("SELECT old_status,new_status,changed_at,changed_by FROM getOrderStatusHistory(" + std::to_string(orderId) + ")");
    for (const auto& row : r) {
        std::cout << row[0] << " -> " << row[1] << " at " << row[2] << " by " << row[3] << "\n";
    }
}

void Admin::viewAuditLog(DatabaseConnection<std::string>& db) {
    auto r = db.executeQuery("SELECT entity_type,entity_id,operation,performed_by,performed_at,details FROM audit_log ORDER BY performed_at DESC LIMIT 50");
    for (const auto& row : r) {
        std::cout << row[0] << " #" << row[1] << " op=" << row[2] << " by=" << row[3] << " at=" << row[4] << " " << row[5] << "\n";
    }
}

void Admin::exportCsvReport(DatabaseConnection<std::string>& db, const std::string& path) {
    auto r = db.executeQuery("SELECT order_id,order_user_id,order_status,order_total,last_status_change,status_changes_count,audit_rows_count FROM report_orders_history_audit()");
    writeCsv(path, {"order_id","user_id","status","total","last_status_change","status_changes","audit_rows"}, r);
}


Manager::Manager(int id, std::string name) : User(id, std::move(name), Role::Manager) {}

std::shared_ptr<Order> Manager::createOrder() {
    auto o = std::make_shared<Order>(0, userId_);
    addLocalOrder(o);
    return o;
}

std::string Manager::viewOrderStatus(DatabaseConnection<std::string>& db, int orderId) {
    auto r = db.executeQuery("SELECT getOrderStatus(" + std::to_string(orderId) + ")");
    return r.empty() || r[0].empty() ? "" : r[0][0];
}

void Manager::cancelOrder(DatabaseConnection<std::string>& db, int orderId) {
    callProcUpdateStatus(db, orderId, "canceled", userId_);
}

void Manager::approveOrder(DatabaseConnection<std::string>& db, int orderId) {
    callProcUpdateStatus(db, orderId, "completed", userId_);
}

void Manager::updateStock(DatabaseConnection<std::string>& db, int productId, int stock) {
    db.beginTransaction();
    try {
        db.executeNonQuery("UPDATE products SET stock_quantity=" + std::to_string(stock) + " WHERE product_id=" + std::to_string(productId));
        db.commitTransaction();
    } catch (const std::exception& e) {
        db.rollbackTransaction();
        logError(db, userId_, "update", std::string("updateStock: ") + e.what());
        throw;
    }
}

void Manager::updateOrderStatus(DatabaseConnection<std::string>& db, int orderId, const std::string& status) {
    callProcUpdateStatus(db, orderId, status, userId_);
}

void Manager::viewPending(DatabaseConnection<std::string>& db) {
    auto r = db.executeQuery("SELECT order_id,user_id,total_price FROM orders WHERE status='pending' ORDER BY order_id");
    for (const auto& row : r) {
        std::cout << "pending order " << row[0] << " user=" << row[1] << " total=" << row[2] << "\n";
    }
}

Customer::Customer(int id, std::string name) : User(id, std::move(name), Role::Customer) {}

std::shared_ptr<Order> Customer::createOrder() {
    auto orderId = 0;
    auto o = std::make_shared<Order>(orderId, userId_);
    addLocalOrder(o);
    return o;
}

std::string Customer::viewOrderStatus(DatabaseConnection<std::string>& db, int orderId) {
    auto r = db.executeQuery("SELECT getOrderStatus(" + std::to_string(orderId) + ")");
    return r.empty() || r[0].empty() ? "" : r[0][0];
}

void Customer::cancelOrder(DatabaseConnection<std::string>& db, int orderId) {
    callProcUpdateStatus(db, orderId, "canceled", userId_);
}

void Customer::addToOrder(const std::shared_ptr<Order>& order, int productId, int qty, double price) {
    order->addItem(productId, qty, price);
}

void Customer::removeFromOrder(const std::shared_ptr<Order>& order, int productId) {
    order->removeItem(productId);
}

void Customer::makePayment(const std::shared_ptr<Order>& order, int strategyType) {
    std::unique_ptr<PaymentStrategy> s;
    if (strategyType == 1) s = std::make_unique<CardPayment>();
    else if (strategyType == 2) s = std::make_unique<WalletPayment>();
    else s = std::make_unique<SBPPayment>();

    order->attachPayment(std::make_unique<Payment>(std::move(s)));
    (void)order->pay();
}

void Customer::returnProduct(DatabaseConnection<std::string>& db, int orderId) {
    db.beginTransaction();
    try {
        db.executeNonQuery("CALL return_order(" + std::to_string(orderId) + ", " + std::to_string(userId_) + ")");
        db.commitTransaction();
    } catch (const std::exception& e) {
        db.rollbackTransaction();
        logError(db, userId_, "update", std::string("returnProduct: ") + e.what());
        throw;
    }
}
