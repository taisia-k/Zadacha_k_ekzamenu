BEGIN;

CREATE OR REPLACE FUNCTION log_audit(
  p_entity_type VARCHAR,
  p_entity_id   INTEGER,
  p_operation   VARCHAR,
  p_user_id     INTEGER,
  p_details     TEXT DEFAULT ''
) RETURNS VOID AS $$
BEGIN
  INSERT INTO audit_log(log_id, entity_type, entity_id, operation, performed_by, performed_at, details)
  VALUES (nextval('seq_audit_log'), p_entity_type, COALESCE(p_entity_id,0), p_operation, COALESCE(p_user_id,0), NOW(), COALESCE(p_details,''));
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION getOrderStatus(p_order_id INTEGER)
RETURNS VARCHAR AS $$
DECLARE s VARCHAR;
BEGIN
  SELECT status INTO s FROM orders WHERE order_id = p_order_id;
  RETURN s;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION getUserOrderCount()
RETURNS TABLE(user_id INTEGER, order_count BIGINT) AS $$
BEGIN
  RETURN QUERY
  SELECT o.user_id, COUNT(*)::BIGINT
  FROM orders o
  GROUP BY o.user_id
  ORDER BY o.user_id;
END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION getTotalSpentByUser(p_user_id INTEGER)
RETURNS NUMERIC AS $$
DECLARE total NUMERIC;
BEGIN
  SELECT COALESCE(SUM(total_price),0) INTO total
  FROM orders
  WHERE user_id = p_user_id AND status IN ('completed','returned'); -- минимально: учитываем покупки/возвраты
  RETURN total;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION canReturnOrder(p_order_id INTEGER)
RETURNS BOOLEAN AS $$
DECLARE s VARCHAR;
DECLARE d TIMESTAMP;
BEGIN
  SELECT status, order_date INTO s, d FROM orders WHERE order_id = p_order_id;
  IF s IS NULL THEN RETURN FALSE; END IF;
  RETURN (s = 'completed') AND (NOW() <= d + INTERVAL '30 days');
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION getOrderStatusHistory(p_order_id INTEGER)
RETURNS TABLE(old_status VARCHAR, new_status VARCHAR, changed_at TIMESTAMP, changed_by INTEGER) AS $$
BEGIN
  RETURN QUERY
  SELECT h.old_status, h.new_status, h.changed_at, h.changed_by
  FROM order_status_history h
  WHERE h.order_id = p_order_id
  ORDER BY h.changed_at;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION getAuditLogByUser(p_user_id INTEGER)
RETURNS TABLE(entity_type VARCHAR, entity_id INTEGER, operation VARCHAR, performed_at TIMESTAMP, details TEXT) AS $$
BEGIN
  RETURN QUERY
  SELECT a.entity_type, a.entity_id, a.operation, a.performed_at, a.details
  FROM audit_log a
  WHERE a.performed_by = p_user_id
  ORDER BY a.performed_at;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION report_orders_history_audit()
RETURNS TABLE(
  order_id INTEGER,
  order_user_id INTEGER,
  order_status VARCHAR,
  order_total NUMERIC,
  last_status_change TIMESTAMP,
  status_changes_count BIGINT,
  audit_rows_count BIGINT
) AS $$
BEGIN
  RETURN QUERY
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
END;
$$ LANGUAGE plpgsql;

COMMIT;
