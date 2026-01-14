BEGIN;

CREATE OR REPLACE PROCEDURE createOrder(
  p_user_id INTEGER,
  p_items_json JSONB,
  p_created_order_id OUT INTEGER
)
LANGUAGE plpgsql
AS $$
DECLARE
  v_order_id INTEGER;
  v_total NUMERIC := 0;
  v_rec JSONB;
  v_pid INTEGER;
  v_qty INTEGER;
  v_price NUMERIC;
  v_stock INTEGER;
BEGIN
  v_order_id := nextval('seq_orders');

  INSERT INTO orders(order_id, user_id, status, total_price, order_date)
  VALUES (v_order_id, p_user_id, 'pending', 0, NOW());

  FOR v_rec IN SELECT * FROM jsonb_array_elements(p_items_json)
  LOOP
    v_pid := (v_rec->>'product_id')::INTEGER;
    v_qty := (v_rec->>'quantity')::INTEGER;

    SELECT price, stock_quantity INTO v_price, v_stock
    FROM products
    WHERE product_id = v_pid
    FOR UPDATE;

    IF v_price IS NULL THEN
      RAISE EXCEPTION 'Product % not found', v_pid;
    END IF;

    IF v_stock < v_qty THEN
      RAISE EXCEPTION 'Not enough stock for product %', v_pid;
    END IF;

    UPDATE products
    SET stock_quantity = stock_quantity - v_qty
    WHERE product_id = v_pid;

    INSERT INTO order_items(order_item_id, order_id, product_id, quantity, price)
    VALUES (nextval('seq_order_items'), v_order_id, v_pid, v_qty, v_price);

    v_total := v_total + (v_price * v_qty);
  END LOOP;

  UPDATE orders SET total_price = v_total WHERE order_id = v_order_id;

  PERFORM log_audit('order', v_order_id, 'insert', p_user_id, 'createOrder');

  p_created_order_id := v_order_id;
END;
$$;

CREATE OR REPLACE PROCEDURE update_order_status(
  p_order_id INTEGER,
  p_new_status VARCHAR,
  p_changed_by INTEGER
)
LANGUAGE plpgsql
AS $$
DECLARE v_old VARCHAR;
BEGIN
  SELECT status INTO v_old FROM orders WHERE order_id = p_order_id FOR UPDATE;
  IF v_old IS NULL THEN
    RAISE EXCEPTION 'Order % not found', p_order_id;
  END IF;

  UPDATE orders SET status = p_new_status WHERE order_id = p_order_id;

  PERFORM log_audit('order', p_order_id, 'update', p_changed_by, 'status: '||v_old||' -> '||p_new_status);
END;
$$;

CREATE OR REPLACE PROCEDURE return_order(
  p_order_id INTEGER,
  p_user_id INTEGER
)
LANGUAGE plpgsql
AS $$
DECLARE ok BOOLEAN;
BEGIN
  ok := canReturnOrder(p_order_id);
  IF NOT ok THEN
    RAISE EXCEPTION 'Return not allowed for order %', p_order_id;
  END IF;

  CALL update_order_status(p_order_id, 'returned', p_user_id);
END;
$$;

COMMIT;
