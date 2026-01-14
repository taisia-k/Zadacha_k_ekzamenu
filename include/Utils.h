#pragma once
#include <string>
#include <vector>
#include "DatabaseConnection.h"
#include "Users.h"

std::string quote(DatabaseConnection<std::string>& db, const std::string& s);

bool canAccessAudit(const User& u);
bool canAccessOrderHistory(const User& u, int orderOwnerId);

void callProcUpdateStatus(DatabaseConnection<std::string>& db, int orderId, const std::string& status, int changedBy);
void logError(DatabaseConnection<std::string>& db, int userId, const std::string& operation, const std::string& details);

void writeCsv(const std::string& path,
              const std::vector<std::string>& header,
              const std::vector<std::vector<std::string>>& rows);

int readInt();
