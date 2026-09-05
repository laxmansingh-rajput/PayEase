#pragma once

#include "models.h"
#include <vector>
#include <string>

using namespace std;

// ─── Append-only Audit Manager ──────────────────────────────────────────────

class AuditManager {
public:
    void log(const string& entityType, const string& entityId,
             const string& event, const string& details) {
        entries_.push_back({nextTimestamp(), entityType, entityId, event, details});
    }

    vector<AuditEntry> getTrail(const string& entityId) const {
        vector<AuditEntry> result;

        for (const auto& e : entries_) {
            if (e.entityId == entityId) {
                result.push_back(e);
            }
        }

        return result;
    }

    const vector<AuditEntry>& allEntries() const {
        return entries_;
    }

private:
    vector<AuditEntry> entries_;
    int counter_ = 0;

    string nextTimestamp() {
        return "T" + to_string(++counter_);
    }
};
