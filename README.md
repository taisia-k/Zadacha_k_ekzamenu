Описание задачи

Цель: реализовать систему интернет-магазина на C++ с использованием ООП, умных указателей, STL-алгоритмов и PostgreSQL (libpqxx).
Система поддерживает роли: Admin / Manager / Customer, работу с заказами и аудит действий.

Технологии: C++17, PostgreSQL, libpqxx.



Архитектура проекта

Классы:
- DatabaseConnection<T> — шаблонный класс подключения к PostgreSQL, поддержка транзакций.
- User (abstract) + наследники Admin / Manager / Customer.
- Order (композиция OrderItem, композиция Payment через std::unique_ptr).
- PaymentStrategy (abstract) + стратегии CardPayment / WalletPayment / SBPPayment.
- User агрегирует заказы: std::vector<std::shared_ptr<Order>>.

ООП:
- наследование и полиморфизм: User/PaymentStrategy.
- чисто виртуальные функции: User::createOrder и PaymentStrategy::pay.

Запрещено new/delete: используются std::unique_ptr / std::shared_ptr.



Работа с базой данных

Таблицы:
- users, products, orders, order_items, order_status_history, audit_log
Ограничения:
- price > 0, stock_quantity >= 0, UNIQUE email.

Процедуры/функции/триггеры:
- createOrder(user_id, items_jsonb, out order_id) — создание заказа + проверка склада + расчёт total.
- update_order_status(order_id, new_status, changed_by) — изменение статуса через процедуру.
- return_order(order_id, user_id) — возврат (canReturnOrder).
- getOrderStatus, getUserOrderCount, getTotalSpentByUser, canReturnOrder.
- getOrderStatusHistory(order_id), getAuditLogByUser(user_id).
- report_orders_history_audit() — отчётная функция для CSV.
Триггеры:
- обновление order_date при смене статуса,
- запись истории статусов,
- пересчёт total_price при смене цены продукта,
- аудит операций по товарам.

При ошибках: откат транзакции и запись error в audit_log.



Умные указатели и STL

- std::unique_ptr: соединение с БД, транзакция, Payment в Order, стратегии оплаты.
- std::shared_ptr: заказы в User (агрегация).
STL:
- std::copy_if: фильтрация (демо в main).
- std::accumulate: подсчёт сумм (Order::total + демо).



Логика ролей и прав доступа

- Admin: управление товарами, статусами заказов, просмотр аудита, формирование CSV.
- Manager: просмотр pending, approve/update status, обновление склада.
- Customer: создание заказа (через createOrder), просмотр статуса, возврат (return_order), оплата стратегиями (в памяти).



Аудит и история ищменений

- order_status_history заполняется триггером при изменении status.
- audit_log заполняется через log_audit() (в процедурах и триггерах), ошибки пишутся как entity_type='system', operation='error'.



Отчёт в формате CSV

Отчёт формируется на основе таблиц orders, order_status_history, audit_log и показывает по каждому заказу текущий статус и сумму, дату последнего изменения статуса, количество изменений статуса и количество записей аудита

SQL-функция формирования отчёта:

SELECT
  o.order_id,
  o.user_id,
  o.status,
  o.total_price,
  MAX(h.changed_at) AS last_status_change,
  COUNT(h.history_id) AS status_changes_count,
  (SELECT COUNT(*) FROM audit_log a WHERE a.entity_type='order' AND a.entity_id=o.order_id) AS audit_rows_count
FROM orders o
LEFT JOIN order_status_history h ON h.order_id = o.order_id
GROUP BY o.order_id, o.user_id, o.status, o.total_price
ORDER BY o.order_id;

Пример содержимого CSV-файла:
order_id,user_id,status,total_price,last_status_change,status_changes_count,audit_rows_count
1,3,completed,350.00,2026-01-10 14:22:05,2,5
2,3,pending,100.00,2026-01-12 09:11:37,0,1
3,2,canceled,250.00,2026-01-13 18:40:10,1,3



Сборка и запуск проекта

Создать БД online_store, затем выполнить:
- sql/tables.sql
- sql/functions.sql
- sql/procedures.sql
- sql/triggers.sql
- sql/sample_data.sql

mkdir build
cd build
cmake ..
cmake --build .
