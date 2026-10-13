
# Failure Handling

## 1. Overview

The Bootloader validates firmware metadata, image boundaries, firmware integrity, and application handover information before transferring control to an application.

The fundamental rule of the current implementation is:

> **Firmware that fails the required validation or integrity checks must not be executed.**

The Bootloader uses metadata-driven boot decisions, SHA-256 verification, candidate trial-boot handling, application confirmation, rollback, and BootSafe/fail-safe behavior to handle the documented failure scenarios.

Different failures have different recovery paths. Rejecting an update candidate, rolling back an unconfirmed candidate, and entering BootSafe after active-image corruption are distinct behaviors.

---

## 2. Current Failure Conditions

The implemented validation and failure-handling paths cover conditions including:

- Metadata read or validation failure
- Invalid metadata magic value
- Invalid firmware image size
- Image boundaries outside the permitted region
- Candidate firmware rejected by the version policy
- Staged-image SHA-256 mismatch
- Programmed destination-image SHA-256 mismatch
- Selected active-image SHA-256 mismatch
- Invalid application initial stack pointer or reset-handler address
- Candidate firmware that fails to confirm after trial boot

The Bootloader rejects firmware that fails the applicable validation checks.

Digital-signature verification and trusted signing-key validation are not implemented in the current project scope.

---

## 3. Failure Decision and Recovery Paths

The response depends on which image fails and the current metadata state.

### 3.1 Candidate Rejected Before Trial Boot

If staged-image validation or destination-image verification fails, the candidate is rejected and must not proceed to trial boot.

The previously confirmed firmware remains the intended boot target, provided its metadata and firmware independently pass the applicable validation checks.

### 3.2 Candidate Fails to Confirm

When a candidate is marked `BOOT_PENDING` but remains unconfirmed after a reset, the Bootloader detects the unconfirmed state and returns to the previously confirmed firmware according to the implemented recovery policy.

This behavior has been tested using the documented candidate trial-boot and watchdog-reset scenario.

### 3.3 Selected Active Image Fails Integrity Verification

If the selected active firmware fails SHA-256 verification, the current implementation enters the configured BootSafe/fail-safe behavior.

It does not automatically try the alternate slot in this failure path.

These paths must not be represented as one generic fallback mechanism.

---

## 4. Metadata Validation Failure

The Bootloader reads and validates firmware metadata before using it to make boot or update decisions.

Relevant validation checks include:

- Metadata magic value
- Image size
- Permitted image boundaries
- Metadata record validity
- Update state and associated record information

### Candidate Metadata Failure

If candidate metadata is invalid, the candidate update is rejected. The candidate must not be accepted for trial boot.

The Bootloader may continue with the previously confirmed firmware according to the implemented boot policy, provided that the relevant metadata and firmware pass validation.

### Selected Active Metadata Failure

If the metadata required to identify and validate the selected active firmware cannot be used, the Bootloader follows the applicable failure-handling path rather than executing an image whose validity cannot be established.

Metadata read and validation failures are covered by the documented test scenarios.

---

## 5. Firmware Integrity Verification Failure

The Bootloader independently calculates the SHA-256 digest of a firmware image stored in STM32 internal Flash and compares it with the expected digest stored in the associated metadata.

```text
Firmware Image in Flash
          |
          v
   Calculate SHA-256
          |
          v
 Compare with Metadata Digest
          |
      +---+---+
      |       |
    MATCH   MISMATCH
      |       |
      v       v
Continue    Reject Image
Required    Do Not Execute
Validation
```

A mismatch may indicate:

- Flash corruption
- Incomplete or invalid firmware data
- Accidental modification
- Unauthorized modification

SHA-256 mismatch detection identifies a difference between the calculated digest and the expected reference digest. It does not independently identify the cause of the mismatch.

The behavior after rejection depends on the image's role and the current update state.

---

## 6. Candidate Rejection and Previously Confirmed Firmware

The dual-slot architecture allows the Bootloader to program and evaluate a candidate without erasing the previously confirmed image in the other executable slot.

When staged firmware or a programmed destination image fails validation:

```text
Candidate Validation
        |
        v
      FAILED
        |
        v
Reject Candidate
Do Not Trial Boot
        |
        v
Retain Previously Confirmed
Firmware as Intended Boot Target
        |
        v
Validate Previously Confirmed Image
        |
   +----+----+
   |         |
  VALID    INVALID
   |         |
   v         v
Continue   BootSafe /
Normal     Fail-Safe
Boot
```

The previously confirmed image must still pass its own validation and integrity checks before execution.

This behavior is distinct from the rollback performed when a candidate has already entered `BOOT_PENDING` and subsequently fails to confirm.

---

## 7. Invalid Application Handover

Before transferring control to an application, the Bootloader validates the application's vector information.

The applicable checks include the initial stack pointer and reset-handler address.

```text
Application Vector Information
             |
             v
    Validate Stack Pointer
             |
             v
    Validate Reset Handler
             |
        +----+----+
        |         |
      VALID     INVALID
        |         |
        v         v
Continue       Reject Handover
Handover        Do Not Jump
```

An invalid reset-handler address prevents application handover.

The candidate or active image must satisfy the applicable validation requirements before control is transferred to it.

---

## 8. Candidate Trial-Boot Failure and Rollback

After the staged image has passed validation and has been programmed into the selected inactive slot, the Bootloader verifies the programmed destination image.

If destination verification succeeds, the update workflow records `BOOT_PENDING` and selects the candidate for trial boot.

The candidate application is expected to call `Firmware_Confirm()` after successful startup.

If the candidate remains unconfirmed following a reset, the Bootloader uses the recorded metadata state to return to the previously confirmed firmware.

```text
Candidate Destination Verified
              |
              v
       Append BOOT_PENDING
              |
              v
         Trial Boot
              |
              v
     Application Startup
              |
       +------+------+
       |             |
   Confirms       Does Not Confirm
       |             |
       v             v
Append CONFIRMED   Reset / Watchdog
                     |
                     v
              Detect Unconfirmed
                Candidate State
                     |
                     v
              Roll Back to the
             Previously Confirmed
                  Firmware
```

The project has verified this recovery scenario using a candidate that does not confirm and the documented watchdog-reset behavior.

Rollback recovery does not imply protection against every possible flash failure or actual power interruption.

---

## 9. Selected Active Firmware Failure

If the selected active firmware fails SHA-256 verification, the Bootloader cannot accept that image for execution.

The current implementation enters the configured BootSafe/fail-safe behavior.

```text
Selected Active Firmware
           |
           v
   Calculate SHA-256
           |
           v
    Digest Mismatch
           |
           v
 Do Not Execute Image
           |
           v
 BootSafe / Fail-Safe
```

The current implementation does not automatically switch to the alternate executable slot following this active-image integrity failure.

This behavior is different from the tested rollback scenario in which a candidate remains unconfirmed after trial boot.

---

## 10. BootSafe and Diagnostics

The Bootloader provides a defined BootSafe/fail-safe path for the documented unrecoverable validation failures.

During development and verification, status indications and diagnostic output help identify the path taken by the Bootloader.

Evidence includes debugger observations, ITM output, and LED indications where applicable.

The fail-safe behavior prevents execution of an image that fails the required checks. It is not a complete communication-based firmware recovery service.

---

## 11. Digital Signature Verification

Digital-signature-based firmware authentication is outside the current project scope.

The existing SHA-256 mechanism compares the calculated firmware digest against the expected digest in metadata. It does not verify a digital signature or establish that the firmware came from an authorized publisher.

If signature verification is introduced in a future project phase, signature-validation failures must result in rejection of the unauthenticated candidate before execution.

The signature scheme, trusted public-key storage, signed-data format, and related key-management policy have not been implemented in this project.

---

## 12. Recovery and Update Limitations

The current implementation includes candidate trial boot, application confirmation, metadata-driven recovery, and rollback following the tested unconfirmed-candidate reset scenario.

The following capabilities are not claimed as implemented:

- Communication-based production firmware download and recovery
- Automatic alternate-slot fallback when the selected active image fails SHA-256 verification
- Digital-signature verification and trusted-key management
- Cryptographically enforced anti-rollback protection
- Resilience to actual power interruption during Flash programming and metadata writes

The update demonstration uses a button-based trigger and a staged firmware image rather than a complete production communication and download mechanism.

These are documented boundaries of the current project scope, not pending implementation or testing tasks for this release.

---

## 13. Logging and Diagnostics

Development and verification use diagnostic output, debugger observations, and status indications to identify firmware validation and recovery behavior.

Documented diagnostic scenarios include:

- Metadata read or validation failure
- Invalid firmware size or memory boundaries
- Candidate version rejection
- Candidate SHA-256 mismatch
- Destination-image SHA-256 mismatch
- Active-image SHA-256 mismatch
- Invalid application reset-handler address
- Candidate remaining unconfirmed after trial boot
- BootSafe/fail-safe entry
- Rollback to previously confirmed firmware

The current project uses development-oriented diagnostics. Persistent, protected security-event logging is not claimed as an implemented production feature.

---

## 14. Current Verification Coverage

Failure-handling verification has been performed for the defined test cases, with supporting evidence maintained separately from this overview.

The documented verification includes:

- Active metadata read failure
- Invalid active metadata
- Invalid candidate metadata
- Candidate version rejection
- Candidate SHA-256 mismatch
- Destination-image SHA-256 mismatch
- Active-image SHA-256 mismatch
- Invalid image-size and memory-boundary conditions
- Invalid application reset-handler address
- Candidate rejection and use of the previously confirmed firmware where covered by the executed test case
- Candidate trial boot without confirmation
- Watchdog reset and rollback to the previously confirmed firmware
- BootSafe/fail-safe entry following selected active-image integrity failure
- Recovery followed by a subsequent valid update

The claims are limited to the scenarios actually executed and their recorded outcomes. Debugger-induced reset testing does not establish resilience to actual power interruption during a Flash or metadata operation.

---

## 15. Implementation Status

**Status: Implemented and Verified for the Documented Test Scope**

The current implementation provides firmware metadata validation, memory-boundary checks, SHA-256 integrity verification, application handover validation, candidate rejection, candidate trial boot, application confirmation, rollback after the tested unconfirmed-candidate reset, and BootSafe/fail-safe behavior for selected active-image integrity failure.

The documented test evidence supports the behaviors within the executed test scope.

Digital-signature verification, production communication-based recovery, cryptographically enforced anti-rollback protection, automatic alternate-slot fallback after selected active-image corruption, and actual power-loss resilience are not claimed as implemented capabilities.

This document describes the implemented failure-handling behavior and its verified scenarios. It does not claim exhaustive fault coverage or formal standards compliance.
