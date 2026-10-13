# Secure Boot – Test Strategy

## 1. Purpose

This document defines the verification strategy and summarizes the test coverage for the STM32F407 Secure Boot and A/B FOTA implementation.

The objective is to verify that the implemented Bootloader and firmware-update flow:

- Validates firmware metadata and metadata records.
- Checks image sizes and permitted Flash boundaries.
- Verifies firmware integrity using SHA-256.
- Rejects invalid or corrupted candidate images.
- Verifies the programmed destination image before trial boot.
- Applies the configured firmware-version policy.
- Selects firmware according to metadata state and the boot policy.
- Transfers control only after the required image and vector information checks pass.
- Confirms successful candidate startup and recovers to previously confirmed firmware when a candidate fails to confirm.
- Enters the configured BootSafe/fail-safe path when the selected active image fails integrity verification.
- Handles documented Flash-operation and metadata-commit failures without reporting a failed update as successful.

This strategy describes the implemented project scope and its recorded verification evidence. Detailed test procedures, actual results, status, evidence filenames, and remarks are maintained in the individual test records.

---

## 2. Verification Approach

Verification was performed incrementally using test techniques appropriate to the implemented firmware functionality.

The documented verification approach includes:

- SHA-256 algorithm and test-vector testing.
- Firmware metadata and metadata-journal testing.
- Linker configuration and image-boundary checks.
- Positive and negative functional testing.
- Image-size and memory-boundary testing.
- Firmware corruption and digest-mismatch testing.
- Boot-flow and application-handover testing.
- Candidate and active image validation.
- Firmware-version eligibility checks.
- A/B update cycling and slot reuse.
- Candidate trial-boot and confirmation testing.
- Rollback after candidate confirmation failure or watchdog reset.
- Controlled debugger-based fault injection.
- Flash-operation and metadata-commit failure handling.
- Interrupted-update and recovery scenarios as recorded in the test cases.

Testing was performed primarily on the STM32F407 target using STM32CubeIDE, ST-LINK/SWD, GDB, ITM/debug output, LED status indications, and screenshots as evidence.

---

## 3. Test Suites and Traceability

Detailed cases are maintained in separate test sheets. The following suites provide the primary traceability for the project.

| Test suite / record | Test ID range | Main verification areas |
|---|---|---|
| `SHA256_Testing` | `SHA-001`–`SHA-005` | Standard SHA-256 vectors, empty input, padding boundaries, and multi-block input. |
| `Memory_Linked_Config_Test` | `MEM-001`–`MEM-014` | Linker/image-boundary information, image-size generation, Flash metadata operations, integrity checks, invalid metadata/size handling, and staging/destination programming boundaries. |
| `Dual_Image_FOTA_Test` | `DIF-001`–`DIF-009` | Slot address configuration, candidate image setup, metadata, normal active-image verification, and metadata commit-marker handling. |
| `FOTA Boot Flow Validation` | `FOTA-VAL-001`–`FOTA-VAL-013` | Active/candidate metadata decisions, version eligibility, SHA-256 validation, fallback/fail-safe decisions, and controlled Flash-operation failure handling. |
| `FOTA A/B Update and Recovery` | `FOTA-AB-001`–`FOTA-AB-016` | A/B updates, repeated cycling, interrupted-update scenarios, confirmation persistence, candidate failure, watchdog recovery, and active-image integrity failure. |

The test sheets remain the authoritative records for individual test status and evidence. A test status should reflect the recorded actual result. If an actual result says that a check is pending or a configuration correction is required, its status and remarks should be reconciled before it is reported as passed.

Some `MEM` records document earlier linker/image-boundary development stages. The final A/B memory map and slot-specific execution behavior are represented by the relevant slot-layout and FOTA test records. The individual test records should be read with their stated configuration and scope in mind.

---

## 4. SHA-256 Verification

The SHA-256 unit-test suite verifies the implementation against reference digests.

The documented cases include:

- Empty input.
- The standard `"abc"` test vector.
- A 55-byte message near the padding boundary.
- A 56-byte message that requires an additional padded block.
- Multi-block input.

Integration tests also verify SHA-256 calculations over firmware stored in STM32 Flash, comparison against the digest in metadata, detection of a digest mismatch, and rejection of the affected image.

**Recorded unit-test suite:** `SHA-001`–`SHA-005`.

Active and candidate firmware integrity behavior is covered by `MEM`, `FOTA-VAL`, and `FOTA-AB` records as applicable.

---

## 5. Metadata, Memory Boundaries, and Application Handover

The verification suite covers the metadata and address checks used by the Bootloader before application execution or firmware programming.

Coverage includes:

- Metadata read success and failure.
- Metadata magic and content validation.
- Metadata record sequence, CRC, and commit-marker handling where covered by the test record.
- Image-size validation against configured regions.
- Staging and destination programming capacity checks.
- Linker-generated application image boundaries and image-size information.
- Application vector table and VTOR configuration.
- Initial stack pointer validation.
- Reset-handler address validation.
- Successful handover to a validated executable image.

When a validation check fails, the associated image or operation is rejected according to the implemented decision path. The exact failure response differs for candidate metadata, active metadata, candidate integrity failure, and selected active-image integrity failure.

---

## 6. Candidate and Active Firmware Validation

The FOTA boot-flow tests cover both candidate and active firmware paths.

### Candidate path

- Read and validate candidate metadata.
- Reject invalid candidate metadata.
- Reject a candidate that does not satisfy the configured version policy.
- Verify the staged candidate using SHA-256.
- Program the candidate into the selected inactive executable slot.
- Verify the SHA-256 digest of the programmed destination image.
- Prevent a candidate that fails applicable validation from proceeding to trial boot.

### Active-image path

- Read and validate active metadata.
- Verify the selected active image using SHA-256.
- Continue boot when the active image passes the required checks.
- Enter `Bootloader_FailSafe()` / BootSafe behavior when selected active-image integrity verification fails.

A candidate SHA-256 mismatch follows the tested candidate-rejection path and continues to active-image verification. A selected active-image SHA-256 mismatch follows the configured fail-safe path; automatic alternate-slot fallback is not implemented in that path.

**Recorded test suite:** `FOTA-VAL-001`–`FOTA-VAL-013`.

---

## 7. A/B Update, Confirmation, and Recovery

The A/B update tests verify the implemented firmware lifecycle across the staging area and executable Slot A / Slot B locations.

Coverage includes:

- Booting a known-good confirmed baseline image.
- Updating from Slot A to Slot B.
- Updating from Slot B to Slot A.
- Reusing a previously used slot for a later update.
- Completing the `A → B → A → B` update cycle.
- Verifying successful candidate programming and destination integrity.
- Recording `BOOT_PENDING` and handling candidate trial boot.
- Confirming the candidate and retaining the confirmed state across resets.
- Recovering when a candidate does not confirm.
- Recovering after the tested candidate watchdog-reset scenario.
- Handling repeated candidate watchdog failures according to the configured recovery behavior.
- Reusing a previously failed candidate slot for a subsequent valid update.
- Processing the documented interrupted-programming, interrupted-activation, and metadata/commit interruption scenarios.
- Revalidating staged firmware and retrying destination programming in the documented interrupted-update scenario.
- Entering BootSafe/fail-safe behavior when the selected active image fails SHA-256 verification.

**Recorded test suite:** `FOTA-AB-001`–`FOTA-AB-016`.

The test records describe the precise preconditions, procedures, and outcomes for each scenario. The strategy does not broaden an individual test result beyond what its procedure and evidence demonstrate.

---

## 8. Negative Testing and Fault Injection

Negative tests verify that invalid metadata, image sizes, firmware versions, firmware digests, and application vector information are not accepted by the relevant validation paths.

Debugger-based fault injection is used for selected branches by modifying values or return statuses in GDB and observing the Bootloader decision. This includes inspecting metadata fields, image addresses, calculated values, validation results, and application-handover state.

Controlled Flash-operation failure handling is covered by the corresponding FOTA validation record. These tests demonstrate the specific injected failure conditions; they do not establish coverage of every possible hardware failure.

---

## 9. Test Evidence and Reporting

Evidence is maintained in the individual test records and may include:

- Test procedure and preconditions.
- Expected and actual results.
- Test status.
- GDB checkpoints and variable inspection.
- Flash-memory and metadata inspection.
- ITM/debug output.
- LED status indications.
- Screenshots.
- Build, linker-map, and firmware execution results.

The test records provide traceability between the feature or requirement, objective, procedure, expected result, actual result, status, evidence filename, and remarks.

A test should be reported as passed only when its actual result supports the expected result. Where a test record contains contradictory information—for example, a `PASS` status alongside a note that a correction or final verification is pending—the record should be reconciled rather than treated as unambiguously passed.

---

## 10. Regression Testing

If the implementation is changed in a future revision, relevant previously verified cases should be repeated. Areas that can affect existing behavior include:

- Bootloader decision logic.
- Application or candidate firmware code.
- Metadata structure or journal handling.
- SHA-256 implementation or image range calculation.
- Linker configuration and vector-table locations.
- Flash memory layout.
- Candidate version policy.
- Flash erase/program and metadata commit handling.
- Trial-boot, confirmation, and recovery logic.

This section describes the regression approach for future changes; it does not assert that additional testing is currently planned for this project release.

---

## 11. Verification Scope and Boundaries

The completed test records demonstrate the specified functional, integrity, memory-boundary, boot-flow, A/B update, and recovery scenarios within their recorded scope.

The following boundaries apply when interpreting the results:

- Results apply to the documented test procedures and preconditions, not every possible input or execution sequence.
- Debugger-induced reset testing does not, by itself, establish resilience to physical power removal at every point during Flash programming or metadata writes.
- The project does not claim a separate full end-to-end regression run across every possible scenario combination.
- A successful test of one failure path does not imply equivalent behavior for a different failure path. In particular, candidate rejection/rollback and selected active-image BootSafe behavior are distinct.

These boundaries describe the scope of the evidence and do not invalidate the test cases that passed under their recorded conditions.

---

## 12. Implementation and Verification Status

**Status: Core functionality and defined test scenarios verified.**

The project includes recorded tests for SHA-256 calculation, metadata handling, image-size and memory-boundary validation, firmware integrity checks, application handover, candidate eligibility, A/B update cycling, candidate confirmation, rollback after the tested candidate failure scenarios, and fail-safe handling for selected active-image integrity failure.

The individual test sheets contain the detailed procedures, actual results, statuses, and supporting evidence. This strategy summarizes the verification approach and coverage; it does not replace those records or claim exhaustive fault coverage.
