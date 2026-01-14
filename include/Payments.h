#pragma once
#include <memory>
#include <string>

class PaymentStrategy {
public:
    virtual ~PaymentStrategy() = default;
    virtual bool pay(double amount) = 0;
    virtual std::string name() const = 0;
};

class CardPayment : public PaymentStrategy {
public:
    bool pay(double amount) override;
    std::string name() const override;
};

class WalletPayment : public PaymentStrategy {
public:
    bool pay(double amount) override;
    std::string name() const override;
};

class SBPPayment : public PaymentStrategy {
public:
    bool pay(double amount) override;
    std::string name() const override;
};

class Payment {
public:
    explicit Payment(std::unique_ptr<PaymentStrategy> strategy);
    bool process(double amount);
    std::string method() const;

private:
    std::unique_ptr<PaymentStrategy> strategy_;
};
