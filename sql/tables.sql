BEGIN;

CREATE TABLE IF NOT EXISTS users (
  user_id        INTEGER PRIMARY KEY,
  name           VARCHAR(255) NOT NULL,
  email          VARCHAR(255) NOT NULL UNIQUE,
  role           VARCHAR(20)  NOT NULL CHECK (role IN ('admin','manager','customer')),
  password_hash  VARCHAR(255) NOT NULL,
  loyalty_level  INTEGER      NOT NULL DEFAULT 0 CHECK (loyalty_level IN (0,1))
);

CREATE TABLE IF NOT EXISTS products (
  product_id      INTEGER PRIMARY KEY,
  name            VARCHAR(255) NOT NULL,
  price           NUMERIC(12,2) NOT NULL CHECK (price > 0),
  stock_quantity  INTEGER       NOT NULL CHECK (stock_quantity >= 0)
);

CREATE TABLE IF NOT EXISTS orders (
  order_id     INTEGER PRIMARY KEY,
  user_id      INTEGER NOT NULL REFERENCES users(user_id),
  status       VARCHAR(20) NOT NULL CHECK (status IN ('pending','completed','canceled','returned')),
  total_price  NUMERIC(12,2) NOT NULL DEFAULT 0,
  order_date   TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS order_items (
  order_item_id INTEGER PRIMARY KEY,
  order_id      INTEGER NOT NULL REFERENCES orders(order_id) ON DELETE CASCADE,
  product_id    INTEGER NOT NULL REFERENCES products(product_id),
  quantity      INTEGER NOT NULL CHECK (quantity > 0),
  price         NUMERIC(12,2) NOT NULL CHECK (price > 0)
);

CREATE TABLE IF NOT EXISTS order_status_history (
  history_id  INTEGER PRIMARY KEY,
  order_id    INTEGER NOT NULL REFERENCES orders(order_id) ON DELETE CASCADE,
  old_status  VARCHAR(20) NOT NULL,
  new_status  VARCHAR(20) NOT NULL,
  changed_at  TIMESTAMP NOT NULL DEFAULT NOW(),
  changed_by  INTEGER NOT NULL REFERENCES users(user_id)
);

CREATE TABLE IF NOT EXISTS audit_log (
  log_id        INTEGER PRIMARY KEY,
  entity_type   VARCHAR(20) NOT NULL CHECK (entity_type IN ('order','product','user','system')),
  entity_id     INTEGER NOT NULL DEFAULT 0,
  operation     VARCHAR(20) NOT NULL CHECK (operation IN ('insert','update','delete','error')),
  performed_by  INTEGER NOT NULL DEFAULT 0,
  performed_at  TIMESTAMP NOT NULL DEFAULT NOW(),
  details       TEXT NOT NULL DEFAULT ''
);

CREATE SEQUENCE IF NOT EXISTS seq_users START 1;
CREATE SEQUENCE IF NOT EXISTS seq_products START 1;
CREATE SEQUENCE IF NOT EXISTS seq_orders START 1;
CREATE SEQUENCE IF NOT EXISTS seq_order_items START 1;
CREATE SEQUENCE IF NOT EXISTS seq_status_history START 1;
CREATE SEQUENCE IF NOT EXISTS seq_audit_log START 1;

COMMIT;
