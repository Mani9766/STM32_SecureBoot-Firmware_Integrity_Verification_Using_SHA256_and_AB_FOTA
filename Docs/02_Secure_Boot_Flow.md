# Secure Boot Flow

## 1. Overview

The Secure Boot process verifies the application firmware before allowing execution.

The Bootloader is the first software component executed after MCU reset. It validates the application metadata, verifies the firmware image integrity using SHA-256, and transfers control to the application only when the defined validation checks succeed.


## 2. Boot Flow

```text
                         MCU RESET
                             |
                             v
                      +--------------+
                      |  Bootloader  |
                      +--------------+
                             |
                             v
                Read Valid Metadata Records
                  for Firmware Slots/Update
                             |
                             v
                  Determine Boot/Update State
                             |
                 +-----------+-----------------+
                 |                             |
          Update Candidate              Normal Boot /
              Pending                  Trial-Boot State
                 |                            |
                 v                            v
         Validate Staged Image         Select Image According
          Metadata, Size,              to Metadata and State
          Version and SHA-256                 |
                 |                            |
           +-----+-----+                      |
           |           |                      |
        INVALID       VALID                   |
           |           |                      |
           v           v                      |
      Reject Update  Program Inactive         |
                      Firmware Slot           |
                           |                  | 
                           v                  |
                  Verify Destination          |
                       SHA-256                |
                         |                    |
                 +-------+---------+          |
                 |                 |          | 
             MISMATCH            MATCH        | 
                 |                 |          |
                 v                 v          |
         Reject Candidate        Append BOOT_PENDING
         Do Not Trial Boot               |
                                         v
                                  Select Candidate
                                   for Trial Boot
                                         |
                                         v
                              Validate Selected Image
                               Metadata, Boundaries,
                               SHA-256 and Vector Data
                                         |
                                   +-----+-----+
                                   |           |
                                INVALID       VALID
                                   |           |
                                   v           v
                           Failure Handling  Prepare
                           / Reject Candidate Handover
                                               |
                                               v
                                        Jump to Application

                  Candidate remains BOOT_PENDING
                   after an unconfirmed trial boot
                               and reset
                                  |
                                  v
                        Roll Back to Previously
                           Confirmed Firmware

      Selected active image SHA-256 mismatch:
                  Enter BootSafe / Fail-Safe
          (no automatic alternate-slot fallback)
```

The diagram summarizes the main boot and update paths. The bootloader selects the appropriate image based on the committed metadata state, validates the image before execution, and uses the implemented recovery policy for an unconfirmed candidate or an integrity failure.


## 3. Detailed Steps

### Step 1 – MCU Reset

After reset, the MCU starts execution from the configured boot memory region.

The Bootloader is located at the beginning of the boot memory region and executes first.

### Step 2 – Bootloader Initialization

The Bootloader performs the minimum required initialization for firmware validation and application handover.

This includes the required hardware and system initialization used by the bootloader.

### Step 3 – Application Metadata Read

The Bootloader reads the firmware metadata associated with the active application image.

The metadata contains information used to validate the firmware image, including:

* Metadata magic value
* Firmware image size
* Firmware version
* Reference SHA-256 digest


### Step 5 – Firmware Integrity Verification

The Bootloader independently calculates the SHA-256 digest of the application firmware stored in STM32 internal Flash.

The calculated digest is compared with the SHA-256 digest stored in the firmware metadata.

```text
Application Firmware
        |
        v
   SHA-256 Calculation
        |
        v
Calculated Digest
        |
        +---------- Compare ----------+
                                     |
                              Stored Metadata
                              SHA-256 Digest
```


## 6. Current Verification Coverage

Functional verification has been performed for the defined test scope. Supporting evidence includes ITM logs, debugger observations, firmware version and metadata-state output, and LED indications where applicable.

The documented verification includes:

- SHA-256 test vectors, including padding boundaries and single-block and multi-block inputs.
- Metadata validation and journal operations, including sequence handling, CRC and commit-marker checks.
- Application image-size, memory-boundary and reset-handler validation.
- Firmware version checks and staged-image validation.
- SHA-256 verification of firmware stored in STM32 Flash.
- Detection of matching and mismatched firmware digests.
- Destination flash programming and post-program SHA-256 verification.
- Consecutive A/B firmware update cycling (`A → B → A → B`).
- Application confirmation and persistence of the confirmed state.
- Retry handling following the tested interrupted destination-programming scenario.
- Candidate trial boot, failure to confirm, watchdog reset and rollback to the previously confirmed image.
- Active-image SHA-256 mismatch and BootSafe/fail-safe entry.
- Recovery followed by a subsequent valid firmware update.
- Successful application handover following validation.

The verification claims are limited to the executed test cases and their recorded evidence. The debugger-induced reset scenario does not establish resilience to an actual power cut during flash programming or metadata writes.

### Step 7 – Application Handover

After successful validation, the Bootloader prepares the processor for application execution.

The Bootloader configures the application vector table, obtains the application's initial stack pointer and reset-handler address, and validates the applicable address constraints before transferring control to the application.

### Step 8 – Application Execution

When all required validation checks succeed, the Bootloader transfers execution to the application reset handler.

The validated application firmware then starts execution.

## 4. Failure Handling

The Bootloader prevents execution of firmware that fails the applicable validation and integrity checks.

The current implementation handles the following scenarios:

- **Invalid staged firmware:** Reject the candidate update when its metadata, image boundaries, version policy, or integrity check fails.
- **Destination integrity mismatch:** Do not select the programmed candidate for trial boot if its SHA-256 verification fails.
- **Candidate fails to confirm:** If the candidate remains unconfirmed after a reset or the tested watchdog-reset scenario, return to the previously confirmed firmware according to the implemented recovery policy.
- **Selected active-image integrity failure:** If the selected active image fails SHA-256 verification, enter the configured BootSafe/fail-safe behavior. The current implementation does not automatically fall back to the alternate slot in this path.
- **Invalid boot metadata or application handover information:** Do not transfer control to an image that fails the applicable metadata, boundary, or reset-handler validation.

These behaviors are supported by the documented functional and recovery test cases. The exact recovery action depends on the failure scenario and the state of the available firmware images.

## 5. Security Principle

The fundamental security rule of the current implementation is:

**Firmware that fails the defined validation and integrity checks must never be executed.**

The Bootloader provides the initial validation boundary between MCU reset and application execution.

The current security mechanism provides firmware **integrity verification using SHA-256**. Firmware authenticity verification using digital signatures is planned as a future enhancement.

## 6. Current Verification Coverage

The Secure Boot implementation has been validated for:

* Firmware metadata validation
* Application image boundary validation
* SHA-256 test vectors
* SHA-256 calculation on firmware stored in STM32 Flash
* Matching SHA-256 digest
* SHA-256 mismatch detection
* Invalid firmware handling
* Application reset-handler validation
* Successful application handover

## 7. Out-of-Scope Enhancements

The current project release is complete for its intended portfolio demonstration scope. The following capabilities are documented as possible future enhancements, not pending work for this release:

- **Digital-signature authentication:** Verify firmware authenticity using digital signatures and trusted public-key management.
- **Authenticated metadata:** Protect security-relevant firmware metadata and the expected integrity digest against unauthorized modification.
- **Cryptographically enforced anti-rollback protection:** Strengthen the existing firmware version checks with protected version state and enforcement against unauthorized downgrade attempts.
- **Production firmware download:** Replace the demonstration trigger and pre-staged firmware workflow with a suitable communication and download mechanism.
- **Enhanced active-slot recovery:** Evaluate alternate-slot recovery when the selected active firmware fails integrity verification.
- **Power-loss resilience:** Evaluate and validate behavior under actual power interruption during flash programming and metadata updates.
- **Production security hardening:** Evaluate additional protections according to the intended deployment environment and threat model.

The project documents firmware integrity and recovery mechanisms but does not claim firmware authenticity, cryptographically enforced anti-rollback protection, production deployment readiness, or formal standards compliance.

