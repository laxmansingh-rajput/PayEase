#include <bits/stdc++.h>
#include "engine.h"
#include "tests.h"

using namespace std;

static void printHelp() {
    cout << "\nPayEase Settlement Engine — Commands:\n"
         << "  run_tests                         Run built-in test suite\n"
         << "  register <id> <name> <planType> [params]  Register a merchant\n"
         << "    planType: flat <rate> | tiered <n> <limit1 rate1> ... | flatfee <fee> <rate>\n"
         << "  sale <txnId> <merchantId> <amount> <date>           Record a sale\n"
         << "  refund <txnId> <merchantId> <amount> <date> <refTxnId>   Record a refund\n"
         << "  chargeback <txnId> <merchantId> <amount> <date> <refTxnId>  Record a chargeback\n"
         << "  settle <merchantId> <date>        Settle daily batch\n"
         << "  dispute <batchId>                 Flag batch as disputed\n"
         << "  query <merchantId> <startDate> <endDate>  Query net payable\n"
         << "  audit <entityId>                  Show audit trail\n"
         << "  help                              Show this help\n"
         << "  quit                              Exit\n\n";
}

int main(int argc, char* argv[]) {
    if (argc > 1 && string(argv[1]) == "--test") {
        return runAllTests() > 0 ? 1 : 0;
    }

    SettlementEngine engine;

    cout << "╔══════════════════════════════════════════╗\n"
         << "║   PayEase Settlement Engine v1.0         ║\n"
         << "║   Type 'help' for commands               ║\n"
         << "╚══════════════════════════════════════════╝\n";

    string line;
    while (true) {
        cout << "\npayease> ";
        if (!getline(cin, line)) break;

        istringstream iss(line);
        string cmd;
        iss >> cmd;

        if (cmd.empty()) continue;

        if (cmd == "quit" || cmd == "exit") {
            break;
        } else if (cmd == "help") {
            printHelp();
        } else if (cmd == "run_tests") {
            runAllTests();
        } else if (cmd == "register") {
            string id, name, planStr;
            iss >> id >> name >> planStr;

            Merchant m;
            m.id = id;
            m.name = name;

            if (planStr == "flat") {
                m.planType = CommissionPlanType::FLAT_PERCENTAGE;
                iss >> m.flatRate;
            } else if (planStr == "tiered") {
                m.planType = CommissionPlanType::TIERED_BY_VOLUME;
                int n;
                iss >> n;
                for (int i = 0; i < n; i++) {
                    Tier t;
                    iss >> t.upperLimit >> t.rate;
                    m.tiers.push_back(t);
                }
            } else if (planStr == "flatfee") {
                m.planType = CommissionPlanType::FLAT_FEE_PLUS_PERCENTAGE;
                iss >> m.flatFee >> m.feeRate;
            } else {
                cout << "Unknown plan type: " << planStr << "\n";
                continue;
            }

            if (engine.registerMerchant(m))
                cout << "Merchant " << id << " registered.\n";
        } else if (cmd == "sale") {
            string txnId, merchantId, date;
            double amount;
            iss >> txnId >> merchantId >> amount >> date;
            Transaction t{txnId, merchantId, TxnType::SALE, amount, date, ""};
            if (engine.recordTransaction(t))
                cout << "Sale " << txnId << " recorded.\n";
        } else if (cmd == "refund") {
            string txnId, merchantId, date, refId;
            double amount;
            iss >> txnId >> merchantId >> amount >> date >> refId;
            Transaction t{txnId, merchantId, TxnType::REFUND, amount, date, refId};
            if (engine.recordTransaction(t))
                cout << "Refund " << txnId << " recorded.\n";
        } else if (cmd == "chargeback") {
            string txnId, merchantId, date, refId;
            double amount;
            iss >> txnId >> merchantId >> amount >> date >> refId;
            Transaction t{txnId, merchantId, TxnType::CHARGEBACK, amount, date, refId};
            if (engine.recordTransaction(t))
                cout << "Chargeback " << txnId << " recorded.\n";
        } else if (cmd == "settle") {
            string merchantId, date;
            iss >> merchantId >> date;
            if (engine.settleBatch(merchantId, date))
                cout << "Batch settled for " << merchantId << " on " << date << ".\n";
        } else if (cmd == "dispute") {
            string batchId;
            iss >> batchId;
            if (engine.flagDispute(batchId))
                cout << "Batch " << batchId << " flagged as disputed.\n";
        } else if (cmd == "query") {
            string merchantId, start, end;
            iss >> merchantId >> start >> end;
            double net = engine.queryNetPayable(merchantId, start, end);
            cout << "Net payable for " << merchantId << " [" << start << ", " << end << "]: $"
                 << fixed << net << "\n";
        } else if (cmd == "audit") {
            string entityId;
            iss >> entityId;
            engine.printAuditTrail(entityId);
        } else {
            cout << "Unknown command. Type 'help' for usage.\n";
        }
    }

    cout << "Goodbye.\n";
    return 0;
}
