#include <iostream>
#include <memory>
#include <algorithm>
#include <numeric>
#include "DatabaseConnection.h"
#include "Users.h"
#include "Utils.h"


static void demoStlOnOrders(const User& u) {
    std::vector<std::shared_ptr<Order>> pending;
    std::copy_if(u.orders().begin(), u.orders().end(), std::back_inserter(pending),
        [](const std::shared_ptr<Order>& o){ return o && o->total() >= 0; });

    double sum = std::accumulate(u.orders().begin(), u.orders().end(), 0.0,
        [](double acc, const std::shared_ptr<Order>& o){ return acc + (o ? o->total() : 0.0); });

    (void)pending;
    (void)sum;
}

static std::unique_ptr<User> loginMenu() {
    std::cout << "Пожалуйста, выберите свою роль:\n"
              << "1. Войти как Администратор\n"
              << "2. Войти как Менеджер\n"
              << "3. Войти как Покупатель\n"
              << "4. Выход\n";

    int c = readInt();
    if (c == 1) return std::make_unique<Admin>(1, "Admin A");
    if (c == 2) return std::make_unique<Manager>(2, "Manager M");
    if (c == 3) return std::make_unique<Customer>(3, "Customer C");
    return nullptr;
}

int main() {
    try {
        DatabaseConnection<std::string> db("host=localhost port=5432 dbname=online_store user=postgres password=postgres");

        auto user = loginMenu();
        if (!user) return 0;

        demoStlOnOrders(*user);

        if (user->role() == Role::Admin) {
            auto& a = dynamic_cast<Admin&>(*user);
            while (true) {
                std::cout << "\nМеню администратора:\n"
                          << "1. Добавить новый продукт\n"
                          << "2. Обновить информацию о продукте\n"
                          << "3. Удалить продукт\n"
                          << "4. Просмотр всех заказов\n"
                          << "5. Изменить статус заказа\n"
                          << "6. Просмотр истории статусов заказа\n"
                          << "7. Просмотр журнала аудита\n"
                          << "8. Сформировать отчёт (CSV)\n"
                          << "9. Выход\n";
                int c = readInt();
                if (c == 9) break;

                try {
                    if (c == 1) {
                        std::string name; double price; int stock;
                        std::cout << "name price stock:\n";
                        std::cin >> name >> price >> stock;
                        a.addProduct(db, name, price, stock);
                    } else if (c == 2) {
                        int id; double price; int stock;
                        std::cout << "product_id price stock:\n";
                        std::cin >> id >> price >> stock;
                        a.updateProduct(db, id, price, stock);
                    } else if (c == 3) {
                        int id; std::cout << "product_id:\n"; std::cin >> id;
                        a.deleteProduct(db, id);
                    } else if (c == 4) {
                        a.viewAllOrders(db);
                    } else if (c == 5) {
                        int oid; std::string st;
                        std::cout << "order_id status(pending/completed/canceled/returned):\n";
                        std::cin >> oid >> st;
                        a.updateOrderStatus(db, oid, st);
                    } else if (c == 6) {
                        int oid; std::cout << "order_id:\n"; std::cin >> oid;
                        a.viewOrderHistory(db, oid);
                    } else if (c == 7) {
                        if (!canAccessAudit(*user)) { std::cout << "Нет доступа\n"; continue; }
                        a.viewAuditLog(db);
                    } else if (c == 8) {
                        if (!canAccessAudit(*user)) { std::cout << "Нет доступа\n"; continue; }
                        a.exportCsvReport(db, "reports/audit_report.csv");
                        std::cout << "OK: reports/audit_report.csv\n";
                    }
                } catch (const std::exception& e) {
                    std::cout << "Ошибка: " << e.what() << "\n";
                }
            }
        } else if (user->role() == Role::Manager) {
            auto& m = dynamic_cast<Manager&>(*user);
            while (true) {
                std::cout << "\nМеню менеджера:\n"
                          << "1. Просмотр заказов в ожидании утверждения\n"
                          << "2. Утвердить заказ\n"
                          << "3. Обновить количество товара на складе\n"
                          << "4. Изменить статус заказа\n"
                          << "5. Выход\n";
                int c = readInt();
                if (c == 5) break;

                try {
                    if (c == 1) m.viewPending(db);
                    else if (c == 2) { int oid; std::cout << "order_id:\n"; std::cin >> oid; m.approveOrder(db, oid); }
                    else if (c == 3) { int pid, st; std::cout << "product_id stock:\n"; std::cin >> pid >> st; m.updateStock(db, pid, st); }
                    else if (c == 4) { int oid; std::string st; std::cout << "order_id status:\n"; std::cin >> oid >> st; m.updateOrderStatus(db, oid, st); }
                } catch (const std::exception& e) {
                    std::cout << "Ошибка: " << e.what() << "\n";
                }
            }
        } else {
            auto& cst = dynamic_cast<Customer&>(*user);

            while (true) {
                std::cout << "\nМеню покупателя:\n"
                          << "1. Создать новый заказ (1 товар)\n"
                          << "2. Просмотр статуса заказа\n"
                          << "3. Оплатить заказ (в памяти)\n"
                          << "4. Оформить возврат заказа\n"
                          << "5. Выход\n";
                int c = readInt();
                if (c == 5) break;

                try {
                    if (c == 1) {
                        int pid, qty;
                        std::cout << "product_id quantity:\n";
                        std::cin >> pid >> qty;

                        db.beginTransaction();
                        try {
                            std::string json = "[{\"product_id\":" + std::to_string(pid) + ",\"quantity\":" + std::to_string(qty) + "}]";
                            db.executeNonQuery(
                                "DO $$ DECLARE oid INTEGER; BEGIN "
                                "CALL createOrder(" + std::to_string(user->id()) + ", " + quote(db, json) + "::jsonb, oid); "
                                "RAISE NOTICE 'order_id=%', oid; END $$;"
                            );
                            db.commitTransaction();
                            std::cout << "OK: заказ создан\n";
                        } catch (...) {
                            db.rollbackTransaction();
                            throw;
                        }

                    } else if (c == 2) {
                        int oid; std::cout << "order_id:\n"; std::cin >> oid;
                        std::cout << "status=" << cst.viewOrderStatus(db, oid) << "\n";
                    } else if (c == 3) {
                        auto o = cst.createOrder();
                        cst.addToOrder(o, 1, 1, 100.0);
                        std::cout << "strategy 1-card 2-wallet 3-sbp:\n";
                        int st = readInt();
                        cst.makePayment(o, st);
                        std::cout << "paid (in-memory), total=" << o->total() << "\n";
                    } else if (c == 4) {
                        int oid; std::cout << "order_id:\n"; std::cin >> oid;
                        cst.returnProduct(db, oid);
                        std::cout << "OK: return requested\n";
                    }
                } catch (const std::exception& e) {
                    std::cout << "Ошибка: " << e.what() << "\n";
                }
            }
        }

    } catch (const std::exception& e) {
        std::cout << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
