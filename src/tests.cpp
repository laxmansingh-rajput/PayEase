#include <bits/stdc++.h>
#include "engine.h"
#include "tests.h"

using namespace std;

static int testsPassed = 0, testsFailed = 0;

static bool approxEq(double a, double b, double eps = 0.01) {
    return fabs(a - b) < eps;
}

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { cout << "  FAIL: " << msg << "\n"; testsFailed++; } \
        else { cout << "  PASS: " << msg << "\n"; testsPassed++; } \
    } while(0)

#define ASSERT_APPROX(a, b, msg) \
    ASSERT_TRUE(approxEq(a, b), msg)

static Merchant merchant(string id, CommissionPlanType plan) {
    Merchant m;
    m.id = id;
    m.name = "Shop";
    m.planType = plan;
    return m;
}

// 1. Flat percentage
static void test1() {
    cout << "\n=== Test 1: Flat Percentage ===\n";
    SettlementEngine e;
    auto m = merchant("M1", CommissionPlanType::FLAT_PERCENTAGE);
    m.flatRate = 0.02;
    e.registerMerchant(m);

    e.recordTransaction({"T1","M1",TxnType::SALE,1000,"2025-01-15",""});
    e.settleBatch("M1","2025-01-15");

    Batch* b = e.getBatch("B1");
    ASSERT_TRUE(b != nullptr, "Batch created");
    ASSERT_APPROX(b->commission, 20, "2% commission");
    ASSERT_APPROX(b->netPayable, 980, "Net payable");
}

// 2. Tiered volume
static void test2() {
    cout << "\n=== Test 2: Tiered Commission ===\n";
    SettlementEngine e;
    auto m = merchant("M2", CommissionPlanType::TIERED_BY_VOLUME);
    m.tiers = {{10000,.01},{50000,.015},{-1,.02}};
    e.registerMerchant(m);

    e.recordTransaction({"T1","M2",TxnType::SALE,8000,"2025-01-10",""});
    e.settleBatch("M2","2025-01-10");

    e.recordTransaction({"T2","M2",TxnType::SALE,5000,"2025-01-12",""});
    e.settleBatch("M2","2025-01-12");

    ASSERT_APPROX(e.getBatch("B2")->commission,65,
                  "Marginal commission across tier");
}

// 3. Flat fee + percentage
static void test3() {
    cout << "\n=== Test 3: Flat Fee + Percentage ===\n";
    SettlementEngine e;
    auto m = merchant("M3", CommissionPlanType::FLAT_FEE_PLUS_PERCENTAGE);
    m.flatFee = 50;
    m.feeRate = .01;
    e.registerMerchant(m);

    e.recordTransaction({"T1","M3",TxnType::SALE,3000,"2025-01-15",""});
    e.settleBatch("M3","2025-01-15");

    Batch* b = e.getBatch("B1");
    ASSERT_APPROX(b->commission,80,"$50 + 1% commission");
    ASSERT_APPROX(b->netPayable,2920,"Net payable");
}

// 4. Same-batch refund
static void test4() {
    cout << "\n=== Test 4: Same-Batch Refund ===\n";
    SettlementEngine e;
    auto m = merchant("M4", CommissionPlanType::FLAT_PERCENTAGE);
    m.flatRate = .02;
    e.registerMerchant(m);

    e.recordTransaction({"T1","M4",TxnType::SALE,1000,"2025-01-15",""});
    e.recordTransaction({"T2","M4",TxnType::REFUND,200,"2025-01-15","T1"});
    e.settleBatch("M4","2025-01-15");

    Batch* b = e.getBatch("B1");
    ASSERT_APPROX(b->grossSales,1000,"Gross sales");
    ASSERT_APPROX(b->totalRefunds,200,"Refund deducted");
    ASSERT_APPROX(b->commission,16,"Commission on net sales");
}

// 5. Clawback
static void test5() {
    cout << "\n=== Test 5: Clawback ===\n";
    SettlementEngine e;
    auto m = merchant("M5", CommissionPlanType::FLAT_PERCENTAGE);
    m.flatRate = .02;
    e.registerMerchant(m);

    e.recordTransaction({"T1","M5",TxnType::SALE,1000,"2025-01-10",""});
    e.settleBatch("M5","2025-01-10");

    e.recordTransaction({"T2","M5",TxnType::CHARGEBACK,300,"2025-01-11","T1"});
    e.recordTransaction({"T3","M5",TxnType::SALE,500,"2025-01-11",""});
    e.settleBatch("M5","2025-01-11");

    Batch* b = e.getBatch("B2");
    ASSERT_APPROX(b->clawbackApplied,300,"Clawback applied");
    ASSERT_APPROX(b->netPayable,190,"Net after clawback");
}

// 6. Carry-forward
static void test6() {
    cout << "\n=== Test 6: Carry-Forward ===\n";
    SettlementEngine e;
    auto m = merchant("M6", CommissionPlanType::FLAT_PERCENTAGE);
    m.flatRate = .02;
    e.registerMerchant(m);

    e.recordTransaction({"T1","M6",TxnType::SALE,500,"2025-01-10",""});
    e.settleBatch("M6","2025-01-10");

    e.recordTransaction({"T2","M6",TxnType::CHARGEBACK,400,"2025-01-11","T1"});
    e.recordTransaction({"T3","M6",TxnType::SALE,100,"2025-01-11",""});
    e.settleBatch("M6","2025-01-11");

    ASSERT_APPROX(e.getMerchant("M6")->carryForwardBalance,302,
                  "Carry-forward created");

    e.recordTransaction({"T4","M6",TxnType::SALE,1000,"2025-01-12",""});
    e.settleBatch("M6","2025-01-12");

    ASSERT_APPROX(e.getBatch("B3")->netPayable,678,
                  "Carry-forward deducted");
}

// 7. Invalid transactions
static void test7() {
    cout << "\n=== Test 7: Invalid Transactions ===\n";
    SettlementEngine e;
    auto m = merchant("M7", CommissionPlanType::FLAT_PERCENTAGE);
    e.registerMerchant(m);

    ASSERT_TRUE(!e.recordTransaction(
        {"T1","UNKNOWN",TxnType::SALE,100,"2025-01-15",""}),
        "Unknown merchant rejected");

    ASSERT_TRUE(!e.recordTransaction(
        {"T2","M7",TxnType::REFUND,50,"2025-01-15",""}),
        "Refund without reference rejected");

    ASSERT_TRUE(!e.recordTransaction(
        {"T3","M7",TxnType::CHARGEBACK,50,"2025-01-15","GHOST"}),
        "Invalid reference rejected");
}

// 8. Merchant validation
static void test8() {
    cout << "\n=== Test 8: Merchant Validation ===\n";
    SettlementEngine e;
    auto m = merchant("M8", CommissionPlanType::FLAT_PERCENTAGE);

    ASSERT_TRUE(e.registerMerchant(m),"Merchant registered");
    ASSERT_TRUE(!e.registerMerchant(m),"Duplicate rejected");
    ASSERT_TRUE(!e.settleBatch("GHOST","2025-01-15"),
                "Unknown merchant settlement rejected");
}

// 9. Dispute
static void test9() {
    cout << "\n=== Test 9: Dispute ===\n";
    SettlementEngine e;
    auto m = merchant("M9", CommissionPlanType::FLAT_PERCENTAGE);
    m.flatRate = .02;
    e.registerMerchant(m);

    e.recordTransaction({"T1","M9",TxnType::SALE,1000,"2025-01-15",""});
    e.settleBatch("M9","2025-01-15");

    Batch* b = e.getBatch("B1");
    double net = b->netPayable;

    e.flagDispute("B1");

    ASSERT_TRUE(b->disputed,"Batch disputed");
    ASSERT_APPROX(b->netPayable,net,"Financial data unchanged");
}

// 10. Date range query
static void test10() {
    cout << "\n=== Test 10: Date Range Query ===\n";
    SettlementEngine e;
    auto m = merchant("M10", CommissionPlanType::FLAT_PERCENTAGE);
    m.flatRate = .02;
    e.registerMerchant(m);

    e.recordTransaction({"T1","M10",TxnType::SALE,1000,"2025-01-10",""});
    e.settleBatch("M10","2025-01-10");

    e.recordTransaction({"T2","M10",TxnType::SALE,2000,"2025-01-12",""});
    e.settleBatch("M10","2025-01-12");

    ASSERT_APPROX(
        e.queryNetPayable("M10","2025-01-10","2025-01-12"),
        2940,
        "Date range total"
    );
}

// 11. Merchant isolation
static void test11() {
    cout << "\n=== Test 11: Merchant Isolation ===\n";
    SettlementEngine e;

    auto a = merchant("MA", CommissionPlanType::FLAT_PERCENTAGE);
    a.flatRate = .02;
    e.registerMerchant(a);

    auto b = merchant("MB", CommissionPlanType::FLAT_PERCENTAGE);
    b.flatRate = .05;
    e.registerMerchant(b);

    e.recordTransaction({"T1","MA",TxnType::SALE,1000,"2025-01-15",""});
    e.recordTransaction({"T2","MB",TxnType::SALE,1000,"2025-01-15",""});
    e.settleBatch("MA","2025-01-15");
    e.settleBatch("MB","2025-01-15");

    ASSERT_APPROX(e.getBatch("B1")->netPayable,980,"Merchant A");
    ASSERT_APPROX(e.getBatch("B2")->netPayable,950,"Merchant B");
}

// 12. Idempotent settlement
static void test12() {
    cout << "\n=== Test 12: Idempotent Settlement ===\n";
    SettlementEngine e;
    auto m = merchant("M12", CommissionPlanType::FLAT_PERCENTAGE);
    m.flatRate = .02;
    e.registerMerchant(m);

    e.recordTransaction({"T1","M12",TxnType::SALE,500,"2025-01-15",""});
    e.settleBatch("M12","2025-01-15");
    e.settleBatch("M12","2025-01-15");

    ASSERT_TRUE(e.getBatch("B2") == nullptr,
                "No duplicate batch created");
    ASSERT_APPROX(e.getBatch("B1")->netPayable,490,
                  "Original batch unchanged");
}

// ─── Run Tests ───────────────────────────────────────────────────────────────

int runAllTests() {
    testsPassed = testsFailed = 0;

    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    test9();
    test10();
    test11();
    test12();

    cout << "\n════════════════════════════════════\n";
    cout << "Results: " << testsPassed << " passed, "
         << testsFailed << " failed, "
         << testsPassed + testsFailed << " total\n";
    cout << "════════════════════════════════════\n";

    return testsFailed;
}
