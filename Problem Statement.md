# Problem Statement: PayEase Fintech Settlement Engine

**Company Background**
PayEase is a payment-processing fintech that settles daily transaction proceeds to its merchant partners after deducting a commission[span_0](start_span)[span_0](end_span). The finance team currently relies on manual spreadsheets, leading to incorrect commission-tier calculations and inconsistent handling of refunds and chargebacks that arrive after a batch has settled[span_1](start_span)[span_1](end_span).

**Objective**
Design a command-line Low-Level Design (LLD) prototype in C++ or Java to automate daily merchant settlements[span_2](start_span)[span_2](end_span). The system must batch transactions, apply accurate commission plans, handle complex clawbacks, and generate auditable reports without using external databases or frameworks[span_3](start_span)[span_3](end_span).

**Core Requirements**
*   **Commission Plans:** Support Flat Percentage, Tiered-by-Volume, and Flat Fee + Percentage plans[span_4](start_span)[span_4](end_span).
*   **Transaction Management:** Record Sales, Refunds, and Chargebacks with amounts, timestamps, and merchant associations[span_5](start_span)[span_5](end_span).
*   **Daily Settlement:** Sum daily sales, deduct appropriate commissions, and compute the net payable amount[span_6](start_span)[span_6](end_span).
*   **Clawback Handling:** Deduct refunds/chargebacks referencing settled batches from the merchant's next unsettled batch without altering finalized records[span_7](start_span)[span_7](end_span).
*   **Dynamic Tiering:** Calculate Tiered-by-Volume rates based on cumulative monthly volume as of the batch date[span_8](start_span)[span_8](end_span).
*   **Audit & Disputes:** Provide a transaction-to-settlement audit trail, support net payable queries over date ranges, and allow flagging settled batches as disputed without altering financial data[span_9](start_span)[span_9](end_span).
