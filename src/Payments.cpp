#include "Payments.h"

bool CardPayment::pay(double) { return true; }
std::string CardPayment::name() const { return "card"; }

bool WalletPayment::pay(double) { return true; }
std::string WalletPayment::name() const { return "wallet"; }

bool SBPPayment::pay(double) { return true; }
std::string SBPPayment::name() const { return "sbp"; }

Payment::Payment(std::unique_ptr<PaymentStrategy> strategy)
    : strategy_(std::move(strategy)) {}

bool Payment::process(double amount) {
    return strategy_ ? strategy_->pay(amount) : false;
}

std::string Payment::method() const {
    return strategy_ ? strategy_->name() : "none";
}
