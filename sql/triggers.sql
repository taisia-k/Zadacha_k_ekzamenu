BEGIN;


CREATE OR REPLACE FUNCTION trg_orders_status_set_date()
RETURNS TRIGGER AS $$
BEGIN
  IF NEW.status IS DISTINCT FROM OLD.status THEN
    NEW.order_date := NOW();
  END IF;
  RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS orders_status_set_date ON orders;
CREATE TRIGGER orders_status_set_date
BEFORE UPDATE OF status ON orders
FOR EACH ROW
EXECUTE FUNCTION trg_orders_status_set_date();


CREATE OR REPLACE FUNCTION trg_orders_status_history()
RETURNS TRIGGER AS $$
DECLARE v_user INTEGER := 0;
BEGIN
  SELECT performed_by INTO v_user
  FROM audit_log
  WHERE entity_type='order' AND entity_id=NEW.order_id AND operation='update'
  ORDER BY performed_at DESC
  LIMIT 1;

  INSERT INTO order_status_history(history_id, order_id, old_status, new_status, changed_at, changed_by)
  VALUES (nextval('seq_status_history'), NEW.order_id, OLD.status, NEW.status, NOW(), COALESCE(v_user,0));

  RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS orders_status_history ON orders;
CREATE TRIGGER orders_status_history
AFTER UPDATE OF status ON orders
FOR EACH ROW
WHEN (NEW.status IS DISTINCT FROM OLD.status)
EXECUTE FUNCTION trg_orders_status_history();


CREATE OR REPLACE FUNCTION trg_products_price_recalc_orders()
RETURNS TRIGGER AS $$
BEGIN
  IF NEW.price IS DISTINCT FROM OLD.price THEN
    UPDATE orders o
    SET total_price = (
      SELECT COALESCE(SUM(oi.price * oi.quantity),0)
      FROM order_items oi
      WHERE oi.order_id = o.order_id
    )
    WHERE o.order_id IN (
      SELECT DISTINCT order_id FROM order_items WHERE product_id = NEW.product_id
    );

    PERFORM log_audit('product', NEW.product_id, 'update', 0, 'price changed, orders recalculated');
  END IF;
  RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS products_price_recalc_orders ON products;
CREATE TRIGGER products_price_recalc_orders
AFTER UPDATE OF price ON products
FOR EACH ROW
EXECUTE FUNCTION trg_products_price_recalc_orders();


CREATE OR REPLACE FUNCTION trg_products_audit()
RETURNS TRIGGER AS $$
BEGIN
  IF TG_OP = 'INSERT' THEN
    PERFORM log_audit('product', NEW.product_id, 'insert', 0, 'product insert');
    RETURN NEW;
  ELSIF TG_OP = 'UPDATE' THEN
    PERFORM log_audit('product', NEW.product_id, 'update', 0, 'product update');
    RETURN NEW;
  ELSIF TG_OP = 'DELETE' THEN
    PERFORM log_audit('product', OLD.product_id, 'delete', 0, 'product delete');
    RETURN OLD;
  END IF;
  RETURN NULL;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS products_audit_ins ON products;
DROP TRIGGER IF EXISTS products_audit_upd ON products;
DROP TRIGGER IF EXISTS products_audit_del ON products;

CREATE TRIGGER products_audit_ins AFTER INSERT ON products FOR EACH ROW EXECUTE FUNCTION trg_products_audit();
CREATE TRIGGER products_audit_upd AFTER UPDATE ON products FOR EACH ROW EXECUTE FUNCTION trg_products_audit();
CREATE TRIGGER products_audit_del AFTER DELETE ON products FOR EACH ROW EXECUTE FUNCTION trg_products_audit();

COMMIT;
