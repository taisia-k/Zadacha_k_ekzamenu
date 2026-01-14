BEGIN;

INSERT INTO users(user_id, name, email, role, password_hash, loyalty_level) VALUES
(1,'Admin A','admin@example.com','admin','hash',1),
(2,'Manager M','manager@example.com','manager','hash',0),
(3,'Customer C','customer@example.com','customer','hash',0)
ON CONFLICT (user_id) DO NOTHING;

INSERT INTO products(product_id, name, price, stock_quantity) VALUES
(1,'Product 1', 100.00, 50),
(2,'Product 2', 250.00, 20),
(3,'Product 3',  75.50, 100)
ON CONFLICT (product_id) DO NOTHING;

COMMIT;
