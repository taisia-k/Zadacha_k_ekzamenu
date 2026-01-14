#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>

std::string quote(DatabaseConnection<std::string>& db, const std::string& s) {
    return db.raw().quote(s);
}

bool canAccessAudit(const User& u) {
    auto access = [](Role r){ return r == Role::Admin; };
    return access(u.role());
}

bool canAccessOrderHistory(const User& u, int orderOwnerId) {
    auto access = [&](Role r){
        if (r == Role::Admin) return true;
        if (r == Role::Manager) return true;
        return (r == Role::Customer) && (u.id() == orderOwnerId);
    };
    return access(u.role());
}

void callProcUpdateStatus(DatabaseConnection<std::string>& db, int orderId, const std::string& status, int changedBy) {
    db.beginTransaction();
    try {
        db.executeNonQuery("CALL update_order_status(" + std::to_string(orderId) + ", " + quote(db, status) + ", " + std::to_string(changedBy) + ")");
        db.commitTransaction();
    } catch (const std::exception& e) {
        db.rollbackTransaction();
        logError(db, changedBy, "update", std::string("updateOrderStatus: ") + e.what());
        throw;
    }
}

void logError(DatabaseConnection<std::string>& db, int userId, const std::string& operation, const std::string& details) {
    try {
        db.executeNonQuery(
            "SELECT log_audit('system', 0, 'error', " + std::to_string(userId) + ", " + quote(db, operation + ": " + details) + ")"
        );
    } catch (...) {}
}

void writeCsv(const std::string& path,
              const std::vector<std::string>& header,
              const std::vector<std::vector<std::string>>& rows) {
    std::ofstream f(path, std::ios::out | std::ios::trunc);
    for (size_t i = 0; i < header.size(); ++i) {
        if (i) f << ",";
        f << header[i];
    }
    f << "\n";

    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i) f << ",";
            std::string cell = row[i];
            for (char& c : cell) if (c == '\n' || c == '\r') c = ' ';
            f << "\"" << cell << "\"";
        }
        f << "\n";
    }
}

int readInt() {
    int x;
    std::cin >> x;
    return x;
}
