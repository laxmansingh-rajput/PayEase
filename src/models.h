#pragma once

#include <string>
#include <vector>
#include <map>

using namespace std;

// ─── Enums ───────────────────────────────────────────────────────────────────

enum class TxnType { SALE, REFUND, CHARGEBACK };

enum class CommissionPlanType { FLAT_PERCENTAGE, TIERED_BY_VOLUME, FLAT_FEE_PLUS_PERCENTAGE };

// ─── Transaction ─────────────────────────────────────────────────────────────

struct Transaction {
    string id;
    string merchantId;
    TxnType type;
    double amount;
    string date;       // "YYYY-MM-DD"
    string refTxnId;   // for REFUND/CHARGEBACK: the original sale txn id
};

// ─── Tier ────────────────────────────────────────────────────────────────────

struct Tier {
    double upperLimit;   // exclusive upper bound; use -1 for infinity
    double rate;         // percentage as decimal, e.g. 0.02 = 2%
};

// ─── Merchant ────────────────────────────────────────────────────────────────

struct Merchant {
    string id;
    string name;
    CommissionPlanType planType;

    // Flat Percentage params
    double flatRate = 0.0;

    // Flat Fee + Percentage params
    double flatFee = 0.0;
    double feeRate = 0.0;

    // Tiered-by-Volume params
    vector<Tier> tiers;

    // Running state
    double carryForwardBalance = 0.0;          // negative balance from clawbacks
    map<string, double> monthlyVolume; // "YYYY-MM" -> cumulative sales volume
};

// ─── Batch ───────────────────────────────────────────────────────────────────

struct Batch {
    string id;
    string merchantId;
    string date;
    double grossSales = 0.0;
    double totalRefunds = 0.0;
    double totalChargebacks = 0.0;
    double commission = 0.0;
    double clawbackApplied = 0.0;
    double netPayable = 0.0;
    bool settled = false;
    bool disputed = false;
};

// ─── AuditEntry ──────────────────────────────────────────────────────────────

struct AuditEntry {
    string timestamp;   // logical timestamp or date
    string entityType;  // "TRANSACTION", "BATCH", "MERCHANT"
    string entityId;
    string event;       // "RECORDED", "SETTLED", "CLAWBACK", "DISPUTED", etc.
    string details;
};
