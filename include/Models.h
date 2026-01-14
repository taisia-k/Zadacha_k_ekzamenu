#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Payments.h"

struct Product {
    int id{};
    std::string name;
    double price{};
    int stock{};
};

class OrderItem {
public:
    OrderItem(int productId, int quantity, double price);
    int productId() const;
    int quantity() const;
    double price() const;

private:
    int productId_{};
    int quantity_{};
    double price_{};
};

class Order {
public:
    explicit Order(int orderId, int userId);

    int id() const;
    int userId() const;

    void addItem(int productId, int quantity, double price);
    void removeItem(int productId);

    double total() const;
    const std::vector<OrderItem>& items() const;

    void attachPayment(std::unique_ptr<Payment> payment); 
    bool pay();

private:
    int orderId_{};
    int userId_{};
    std::vector<OrderItem> items_;              
    std::unique_ptr<Payment> payment_;          
};
