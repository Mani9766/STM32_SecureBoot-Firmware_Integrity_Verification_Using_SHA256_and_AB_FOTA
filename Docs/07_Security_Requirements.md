
# Secure Boot – Security Requirements

## 1. Purpose

This document defines the security requirements for the STM32F407 Secure Boot and A/B FOTA implementation.

The requirements describe the expected behavior of the Bootloader, firmware metadata handling, SHA-256 integrity verification, application handover, candidate-image processing, firmware update states, and failure handling.

The requirements provide a basis for implementation, verification, and traceability to the project's test records.

The current project focuses on firmware integrity verification and update recovery. Its verification claims are limited to the implemented functionality and documented test scope.

---

## 2. Bootloader Requirements

### SEC-BOOT-001 – Bootloader Startup

The Bootloader shall execute before the application after MCU reset.

### SEC-BOOT-002 – Application Validation

The Bootloader shall validate the selected application firmware before transferring execution to it.

### SEC-BOOT-003 – Invalid Firmware

The Bootloader shall not execute an application image that fails any mandatory validation check.

### SEC-BOOT-004 – Memory Boundary

The Bootloader shall verify that an application image lies within its permitted Flash memory region.

### SEC-BOOT-005 – Firmware Size

The Bootloader shall reject an image whose declared size exceeds the configured capacity of its permitted image region.

### SEC-BOOT-006 – Boot Decision

The Bootloader shall select the firmware image according to the validated metadata state and the implemented boot policy.

---

## 3. Metadata Requirements

### SEC-META-001 – Metadata Availability

The Bootloader shall read the firmware metadata associated with an image before using that metadata to validate or select the image.

### SEC-META-002 – Magic Number

The Bootloader shall validate the firmware metadata magic value.

### SEC-META-003 – Image Size

The Bootloader shall validate the firmware image size against the configured image region.

### SEC-META-004 – Firmware Version

The firmware metadata shall contain the firmware version required by the update-selection policy.

### SEC-META-005 – Reference Digest

The firmware metadata shall contain the expected SHA-256 digest associated with the firmware image.

### SEC-META-006 – Metadata Record Validity

The Bootloader shall validate applicable metadata record fields before accepting a record for boot or update decisions.

### SEC-META-007 – Record Commit Marker

The metadata journal shall use a commit marker to distinguish committed records from incomplete or invalid records. Records without the expected commit marker shall be rejected.

### SEC-META-008 – Record Sequence and CRC

The metadata journal shall use record sequence information and a CRC to support record selection and detection of accidental corruption.

The CRC and commit marker provide corruption-detection and record-validity mechanisms; they do not establish cryptographic protection against an attacker who can modify Flash contents.

---

## 4. Firmware Integrity Requirements

### SEC-INT-001 – Firmware Hash

The Bootloader shall calculate a cryptographic hash over the configured firmware image data.

### SEC-INT-002 – SHA-256

The current implementation shall use SHA-256 for firmware integrity verification.

### SEC-INT-003 – Hash Verification

The Bootloader shall compare the calculated SHA-256 digest against the expected digest stored in the associated firmware metadata.

### SEC-INT-004 – Hash Failure

If SHA-256 verification fails, the Bootloader shall reject the corresponding firmware image.

### SEC-INT-005 – Independent Verification

The Bootloader shall independently calculate the firmware digest from the firmware image stored in STM32 internal Flash.

### SEC-INT-006 – Staged-Image Verification

The Bootloader shall verify the integrity of staged firmware before accepting it for programming into an executable slot.

### SEC-INT-007 – Destination-Image Verification

After programming the candidate into the selected inactive slot, the Bootloader shall independently verify the programmed destination image using SHA-256 before proceeding to candidate trial boot.

---

## 5. Candidate and Active Image Requirements

### SEC-FOTA-001 – Image Separation

The incoming firmware staging region shall be separate from the two executable firmware slots.

### SEC-FOTA-002 – Candidate Metadata

The Bootloader shall read and validate the metadata associated with a candidate image before processing that image.

### SEC-FOTA-003 – Candidate Version Check

The Bootloader shall compare the candidate firmware version with the selected active firmware version.

A candidate whose version is equal to or lower than the active firmware version shall not be processed as a new update candidate.

### SEC-FOTA-004 – Candidate Integrity

A candidate image shall pass the applicable SHA-256 integrity checks before being selected for trial boot.

### SEC-FOTA-005 – Candidate Rejection

The Bootloader shall reject a candidate image when its metadata, memory boundaries, version criteria, or SHA-256 verification fails.

### SEC-FOTA-006 – Active Image Preservation

The update workflow shall preserve the previously confirmed firmware in its executable slot while the candidate is programmed into the inactive executable slot.

### SEC-FOTA-007 – Active-Image Fallback After Candidate Rejection

When a candidate is rejected, the Bootloader shall continue with verification of the previously confirmed firmware and boot it when its metadata, integrity, and applicable handover checks succeed.

### SEC-FOTA-008 – Inactive-Slot Selection

The Bootloader shall select the inactive executable slot according to the validated firmware state and the implemented update policy.

### SEC-FOTA-009 – Destination Programming Verification

The Bootloader shall verify the programmed destination image before accepting it for trial boot.

The staging image's successful validation alone shall not be treated as proof that the destination image was programmed correctly.

---

## 6. Application Handover Requirements

### SEC-JMP-001 – Vector Table

The Bootloader shall configure the application vector table before transferring execution.

### SEC-JMP-002 – Stack Pointer Validation

The Bootloader shall validate the application's initial stack pointer against the configured SRAM region.

### SEC-JMP-003 – Reset Handler Validation

The Bootloader shall validate the application's reset-handler address against the permitted executable application region.

### SEC-JMP-004 – Reset Handler Handover

The Bootloader shall transfer execution to the application reset handler only after the required image and handover validation checks succeed.

### SEC-JMP-005 – Slot-Specific Execution Address

The Bootloader shall use the selected executable slot's address when preparing application handover.

The staging address shall not be treated as the executable slot address after the candidate has been programmed into Slot A or Slot B.

---

## 7. Failure Handling Requirements

### SEC-FAIL-001 – Verification Failure

The Bootloader shall not execute firmware when a mandatory validation or integrity check fails.

### SEC-FAIL-002 – Invalid Candidate

If candidate metadata, version policy, image boundaries, or SHA-256 verification fails, the Bootloader shall reject the candidate and continue according to the applicable active-image verification path.

### SEC-FAIL-003 – Invalid Active Image

If the selected active firmware fails mandatory validation or SHA-256 verification, the Bootloader shall enter the configured BootSafe/fail-safe path.

### SEC-FAIL-004 – Invalid Application Handover

The Bootloader shall not transfer execution when the application vector information, initial stack pointer, or reset-handler address fails the applicable validation.

### SEC-FAIL-005 – Flash Operation Failure

Flash erase, programming, and metadata commit operations shall report failure when an operation fails. The update workflow shall not report a failed operation as successful or advance the update state incorrectly.

### SEC-FAIL-006 – Distinct Failure Responses

The Bootloader shall apply the implemented response appropriate to the failure condition.

Candidate rejection, rollback after an unconfirmed candidate, and BootSafe/fail-safe handling for selected active-image integrity failure shall be treated as distinct paths.

---

## 8. Firmware Image and Update-State Requirements

### SEC-IMG-001 – Image Identification

The Bootloader shall use validated firmware metadata to identify the image associated with a physical executable slot.

### SEC-IMG-002 – Image Size

The firmware image size shall be available to the Bootloader through the associated metadata and validated against the configured image region.

### SEC-IMG-003 – Image Version

The firmware version shall be available through the associated metadata for update-selection decisions.

### SEC-IMG-004 – Image Digest

The expected SHA-256 digest shall be available through the associated metadata for firmware integrity verification.

### SEC-STATE-001 – Persistent Update State

The update workflow shall record firmware state information in the metadata journal stored in non-volatile Flash.

### SEC-STATE-002 – Pending Candidate

After the programmed destination image passes verification, the Bootloader shall record the candidate as `BOOT_PENDING` before proceeding with its trial boot.

### SEC-STATE-003 – Application Confirmation

The candidate application shall provide an explicit confirmation mechanism after successful startup. A successful confirmation shall cause a `CONFIRMED` metadata record to be appended.

### SEC-STATE-004 – Unconfirmed Candidate Recovery

If a candidate remains unconfirmed following reset, the Bootloader shall detect the unconfirmed state and return to the previously confirmed firmware according to the implemented recovery policy.

### SEC-STATE-005 – Confirmation Persistence

The latest valid confirmed firmware state shall remain available across subsequent resets, subject to the implemented metadata and boot-selection rules.

### SEC-STATE-006 – Update Retry After Recovery

After recovering from a failed candidate update, the update workflow shall permit a subsequent valid update using the previously failed slot when that slot can be safely reused.

### SEC-STATE-007 – Interrupted Update Handling

The update workflow shall use metadata validity, commit information, image verification, and the confirmed-image state to handle the documented interrupted-programming and interrupted-update scenarios.

The behavior demonstrated by debugger-induced reset tests shall not be interpreted as proof of recovery from every possible physical power interruption.

---

## 9. Requirement Traceability

Requirements are traced through the project development and verification records.

```text
Security Requirement
        |
        v
Architecture / Design
        |
        v
Implementation
        |
        v
Test Case
        |
        v
Actual Result and Status
        |
        v
Supporting Evidence
```

The project test records contain the test identifier, feature, requirement, objective, preconditions, test procedure, expected result, actual result, status, evidence, and remarks.

The following test suites provide the primary verification records:

- `SHA256_Testing`
- `Memory_Linked_Config_Test`
- `Dual_Image_FOTA_Test`
- `FOTA Boot Flow Validation`
- `FOTA A/B Update and Recovery`

Detailed test procedures and outcomes remain in their corresponding test records. A requirement should be reported as verified only to the extent supported by the recorded test results.

---

## 10. Verification Coverage

The defined verification scope includes:

- SHA-256 standard test vectors.
- Empty-message, padding-boundary, and multi-block hashing.
- Firmware hashing from STM32 internal Flash.
- Metadata reading and validation.
- Magic-number validation.
- Metadata sequence, CRC, and commit-marker handling.
- Image-size and memory-boundary validation.
- Candidate version comparison.
- Staged-image SHA-256 verification.
- Destination-image SHA-256 verification.
- Active-image SHA-256 verification.
- Candidate rejection and active-image verification.
- Application vector-table configuration.
- Initial stack pointer and reset-handler validation.
- Application handover.
- `BOOT_PENDING` and `CONFIRMED` state handling.
- A/B update cycling and slot reuse.
- Candidate confirmation failure and watchdog-triggered recovery.
- Interrupted-update scenarios represented in the test records.
- Flash-operation failure handling.
- BootSafe/fail-safe behavior when selected active-image integrity verification fails.

The verification status is based on the defined test procedures and recorded results. It does not claim exhaustive fault coverage or a separate full end-to-end regression run across every possible failure combination.

---

## 11. Known Scope Boundaries and Future Enhancements

The current project includes firmware integrity verification, metadata-driven update-state management, candidate trial boot, application confirmation, and rollback for the documented candidate-failure scenarios.

The following capabilities are not claimed as fully implemented or established by the current verification evidence:

- **Actual power-loss resilience:** Recovery from physical power interruption at every stage of Flash programming and metadata updates.
- **Automatic alternate-slot fallback after active-image corruption:** The current selected-active-image SHA-256 mismatch path enters BootSafe/fail-safe behavior.
- **Production firmware download:** The current demonstration uses a button-based trigger and a staged firmware image rather than a complete production communication/download mechanism.
- **Cryptographically protected downgrade prevention:** The update workflow rejects candidates whose versions are equal to or lower than the active version. A protected monotonic anti-rollback mechanism is not claimed.
- **Persistent protected diagnostics:** A production-grade protected security-event logging mechanism is not claimed.
- **Exhaustive fault coverage:** The test evidence covers the recorded scenarios, not every possible input, hardware fault, or sequence of interruptions.

These are documented boundaries of the current project scope, not pending implementation or testing tasks for this portfolio release.

---

## 12. Implementation Status

**Status: Core Requirements Implemented and Verified for the Documented Test Scope**

The current implementation covers:

- Bootloader startup and boot decisions.
- Firmware metadata validation and record handling.
- Image-size and memory-boundary validation.
- SHA-256 integrity verification.
- Staging and executable-slot separation.
- Candidate version checking.
- Candidate and destination-image verification.
- Candidate rejection and active-image verification.
- Application vector-table, stack pointer, and reset-handler validation.
- Controlled application handover.
- `BOOT_PENDING` trial boot and `CONFIRMED` state handling.
- Rollback after the documented unconfirmed-candidate reset and watchdog scenarios.
- Recovery followed by a subsequent valid update.
- BootSafe/fail-safe behavior when the selected active image fails integrity verification.
- Recorded handling of relevant metadata and Flash-operation failures.

The status reflects the implementation and recorded verification results for the defined test scope. It does not establish exhaustive fault coverage, formal standards compliance, or guaranteed recovery from every possible power interruption.
