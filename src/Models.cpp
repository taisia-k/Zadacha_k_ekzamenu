#include "Models.h"
#include <algorithm>
#include <numeric>

OrderItem::OrderItem(int productId, int quantity, double price)
    : productId_(productId), quantity_(quantity), price_(price) {}

int OrderItem::productId() const { return productId_; }
int OrderItem::quantity() const { return quantity_; }
double OrderItem::price() const { return price_; }

Order::Order(int orderId, int userId) : orderId_(orderId), userId_(userId) {}

int Order::id() const { return orderId_; }
int Order::userId() const { return userId_; }

void Order::addItem(int productId, int quantity, double price) {
    items_.emplace_back(productId, quantity, price);
}

void Order::removeItem(int productId) {
    items_.erase(
        std::remove_if(items_.begin(), items_.end(),
            [productId](const OrderItem& it){ return it.productId() == productId; }),
        items_.end()
    );
}

double Order::total() const {
    return std::accumulate(items_.begin(), items_.end(), 0.0,
        [](double sum, const OrderItem& it){ return sum + it.price() * it.quantity(); });
}

const std::vector<OrderItem>& Order::items() const { return items_; }

void Order::attachPayment(std::unique_ptr<Payment> payment) {
    payment_ = std::move(payment);
}

bool Order::pay() {
    return payment_ ? payment_->process(total()) : false;
}
