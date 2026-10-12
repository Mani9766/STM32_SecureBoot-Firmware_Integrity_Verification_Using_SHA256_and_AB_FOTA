# Threat Modeling & Security Hardening

## 1. Scope Definition

### Project Overview
- **Target:** STM32F407 microcontroller.
- **Architecture:** Secure Boot with SHA-256 firmware integrity verification and A/B FOTA.
- **Primary objective:** Detect invalid or corrupted firmware and prevent its execution through bootloader validation and recovery logic.
- **Bootloader responsibilities:**
  - Validate firmware metadata.
  - Calculate and compare SHA-256 digests.
  - Check firmware version and image parameters.
  - Select the appropriate firmware image.
  - Handle validation failures through fallback or fail-safe logic.
  - Validate application handover before transferring control.

### Security Objectives
- Detect firmware modifications through SHA-256 verification.
- Validate metadata before using it for firmware verification or selection.
- Prevent invalid image sizes and addresses from bypassing validation.
- Handle corrupted metadata and invalid firmware safely.
- Review interrupted updates and flash programming failures.
- Evaluate version checks and rollback-related behavior.
- Validate the application vector table, stack pointer, and reset-handler address.
- Maintain consistency between host-side metadata generation and bootloader verification.

### In Scope
- Bootloader validation and boot decision logic.
- Slot A, Slot B, and staging-area firmware.
- Active and candidate firmware metadata.
- SHA-256 calculation and digest comparison.
- Firmware image sizes, addresses, and flash boundaries.
- Metadata corruption and manipulation.
- Firmware version checks and rollback-related scenarios.
- Flash programming, erase operations, and interrupted updates.
- Candidate rejection, active-image fallback, and fail-safe handling.
- Application handover and vector-table configuration.
- Host-side firmware binary and metadata preparation.
- Update-protocol inputs and error handling as the protocol is integrated.

### Out of Scope
- Claims that SHA-256 verification proves the firmware source is trusted.
- Formal security certification or compliance claims.

### Key Limitation
SHA-256 comparison verifies that the calculated digest matches the expected digest stored in metadata. If both firmware and its expected digest can be modified, SHA-256 comparison alone cannot establish firmware authenticity. This project focuses on firmware integrity verification; authenticity is outside its implementation scope.

## 2. Security Asset Identification

### Asset Inventory

| Asset ID&nbsp;&nbsp;&nbsp; | Asset | Location | Protection Objective |
|---|---|---|---|
| AST-01 | Bootloader firmware | Sectors 0–1 | Preserve validation, boot selection, and recovery logic. |
| AST-02 | Active firmware metadata | Sector 2: `0x08008000` | Detect invalid or corrupted metadata before use. |
| AST-03 | Candidate firmware metadata | Sector 3: `0x0800C000` | Validate candidate metadata before accepting an image. |
| AST-04 | Staging firmware | Sector 4: `0x08010000` | Prevent incomplete or invalid staged firmware from being accepted. |
| AST-05 | Slot A firmware | Sector 5: `0x08020000` | Verify image integrity before execution when selected. |
| AST-06 | Slot B firmware | Sector 6: `0x08040000` | Verify image integrity before execution when selected. |
| AST-07 | SHA-256 verification logic | Bootloader | Calculate the digest over the intended image region and compare it correctly. |
| AST-08 | Firmware metadata fields | Metadata structures | Validate magic value, image size, version, digest, and applicable sequence fields. |
| AST-09 | Flash layout and boundaries | Bootloader constants and linker scripts | Prevent invalid memory ranges and incorrect image-region selection. |
| AST-10 | Flash programming and erase logic | Firmware update implementation | Prevent unintended flash modification and unsafe acceptance after interrupted writes. |
| AST-11 | Boot selection and recovery logic | Bootloader | Reject invalid candidates and follow the defined fallback or fail-safe path. |
| AST-12 | Application handover data | Application vector table and bootloader | Validate stack pointer, reset-handler address, and vector-table configuration. |
| AST-13 | Host-side metadata generator | `Tools/prepare_slot_metadata.py` | Generate metadata consistent with the firmware binary and verification format. |
| AST-14 | Firmware image representation | Build output and programmed flash | Ensure host-side hashing and device-side verification use the intended bytes and image size. |
| AST-15 | Update inputs | Staging and update mechanism | Handle corrupted, malformed, or incomplete firmware inputs safely. |

### Asset Identification Notes
- The bootloader, firmware images, metadata, and boot decision logic are critical
  assets because they directly affect whether firmware is accepted and executed.
- Firmware metadata and image contents must be evaluated together.
- Image size, slot boundaries, and digest calculation range must remain consistent
  across firmware packaging and bootloader verification.
- Application handover requires appropriate address and vector-table validation
  in addition to firmware digest verification.
- Asset protection effectiveness will be evaluated through threat scenarios,
  source-code review, security hardening, and relevant test evidence.

## 3. Attack Surface Identification

### Attack Surface Inventory

| Attack Surface ID&nbsp;&nbsp; | Attack Surface | Potential Security Concern | Related Assets |
|---|---|---|---|
| AS-01 | Firmware images in staging, Slot A, and Slot B | Firmware corruption or modification may go undetected if validation is missing, incorrect, or performed on the wrong image region. | AST-02, AST-03, AST-04 |
| AS-02 | Firmware metadata | Corrupted or manipulated magic values, image sizes, versions, digests, sequence numbers, or state fields may affect validation and boot decisions. | AST-05, AST-06, AST-08 |
| AS-03 | Image address and size validation | Invalid addresses, oversized images, or arithmetic overflow may cause incorrect memory-range checks or verification of unintended flash contents. | AST-07, AST-08, AST-09 |
| AS-04 | SHA-256 calculation and comparison | Incorrect digest length, image boundaries, or error handling may cause corrupted firmware to be accepted or valid firmware to be rejected. | AST-07, AST-14 |
| AS-05 | Staging-to-slot firmware transfer | An interrupted or incomplete copy from staging to the destination slot may leave an invalid candidate image. | AST-03, AST-04, AST-10 |
| AS-06 | Flash erase and programming operations | Incorrect sector selection, programming failures, or interrupted operations may corrupt firmware or metadata and leave inconsistent update state. | AST-05, AST-06, AST-10 |
| AS-07 | Metadata state transitions and boot selection | Invalid or inconsistent state transitions may cause the bootloader to select an unvalidated image or mishandle recovery. | AST-05, AST-06, AST-11 |
| AS-08 | Application handover | Invalid stack-pointer or reset-handler values, incorrect vector-table configuration, or address mismatches may cause an unsafe jump to an application. | AST-02, AST-03, AST-11, AST-12 |
| AS-09 | Host-side firmware and metadata preparation | Mismatches between binary size, padding, digest calculation, and device-side verification may cause incorrect verification results. | AST-13, AST-14 |
| AS-10 | Firmware update inputs and protocol | Malformed, oversized, corrupted, or incomplete update data may reach the staging or flash-programming logic without adequate validation. | AST-04, AST-10, AST-15 |
| AS-11 | Debug and programming interface | Programming access may permit unintended firmware or metadata modification if access is available and not appropriately restricted. | AST-01, AST-02, AST-03, AST-05, AST-06 |

### Attack Surface Assessment Notes

- **Flash contents:** Firmware images and metadata can be affected by corruption, unintended writes, or unauthorized modification.
- **Metadata processing:** Fields used for image validation and boot selection must be validated before use.
- **Image verification:** The image address, size, and exact bytes used for SHA-256 calculation must be consistent across host-side and bootloader implementations.
- **Update operations:** Firmware copying, flash erase/program operations, and metadata state transitions must handle failures and interruptions safely.
- **Boot decision and handover:** Only an image that satisfies the implemented validation requirements should be selected for execution. Its execution addresses must also be checked.
- **External inputs:** Protocol-specific attack surfaces will be assessed against the implemented update protocol once integration is complete.
- **Debug access:** Security restrictions on SWD/ST-LINK access depend on the device configuration and deployment assumptions; they are not presumed to be enabled.

The attack surfaces listed above are potential points of influence, not confirmed vulnerabilities. Their actual exploitability and the effectiveness of existing controls will be evaluated through threat scenario analysis, source-code review, and testing.

## 4. Threat Scenario Identification

### Threat Scenarios

| Threat ID&nbsp;&nbsp; | Threat Scenario | Potential Impact | Related Attack Surfaces |
|---|---|---|---|
| TH-01 | **Firmware image tampering:** Firmware bytes are modified after the expected SHA-256 digest is generated. | Corrupted firmware may be executed if verification is bypassed, incorrect, or performed over the wrong image region. | AS-01, AS-04 |
| TH-02 | **Metadata corruption:** Metadata fields such as magic value, digest, image size, version, or state become corrupted or invalid. | Incorrect validation results, rejection of valid firmware, or an unsafe boot decision. | AS-02, AS-07 |
| TH-03 | **Invalid image size or address:** Metadata provides an image size or address outside the permitted firmware region. | Out-of-bounds memory access, verification of unintended data, or unsafe application execution. | AS-03 |
| TH-04 | **SHA-256 verification failure mishandled:** A digest mismatch or verification error is not handled as a validation failure. | Corrupted firmware may be accepted despite the integrity check. | AS-04 |
| TH-05 | **Staging-to-slot image mismatch:** The staged image is validated, but the destination image differs after copying or programming. | An invalid destination image may be selected if its integrity is not adequately verified. | AS-01, AS-04, AS-05, AS-08 |
| TH-06 | **Interrupted firmware download:** Power loss, communication failure, or incomplete input leaves a partially written staging image. | An incomplete image may be accepted if update completion and image validity are not established before promotion. | AS-01, AS-05, AS-10 |
| TH-07 | **Interrupted flash programming or erase:** Power loss or a flash-operation failure interrupts an update or metadata write. | Firmware or metadata may become inconsistent, potentially affecting candidate selection or recovery. | AS-05, AS-06, AS-07 |
| TH-08 | **Invalid application handover:** The selected image has an invalid initial stack pointer, reset-handler address, or vector-table configuration. | The processor may jump to an invalid address, enter a fault, or fail to start the application correctly. | AS-08 |
| TH-09 | **Incorrect boot-state transition:** Metadata states or boot-selection logic become inconsistent during candidate validation, trial boot, confirmation, or rollback. | The bootloader may select the wrong image or fail to follow the intended recovery behavior. | AS-02, AS-07 |
| TH-10 | **Unexpected firmware downgrade:** An older firmware version is selected or accepted contrary to the intended version policy. | The device may run an older image containing known defects or previously corrected integrity-handling problems. | AS-02, AS-07 |
| TH-11 | **Host/device image representation mismatch:** The host-side tool and bootloader use different image sizes, byte ranges, or padding assumptions when generating and checking the digest. | Valid firmware may fail verification, or the intended image contents may not be fully covered by the verification process. | AS-04, AS-09 |
| TH-12 | **Incorrect flash-region modification:** An erase or programming operation targets the wrong sector or exceeds its intended boundary. | Firmware, metadata, or other protected flash contents may be corrupted. | AS-03, AS-06 |
| TH-13 | **Malformed update-protocol input:** Update data contains invalid lengths, unexpected fields, or incomplete payloads. | Invalid data may reach flash programming or influence update state without appropriate validation. | AS-10 |
| TH-14 | **Firmware modification through programming access:** Firmware or metadata is modified through an available debug or programming interface. | Changes to firmware should be detected when they cause a mismatch with the expected digest; protection depends on the implemented verification and access configuration. | AS-01, AS-02, AS-11 |

### Threat Assessment Notes

- These scenarios describe potential threats; they are not confirmed vulnerabilities.
- Firmware integrity depends on correct SHA-256 calculation, digest comparison, metadata validation, and image-boundary checks.
- The firmware image must be validated at the address and over the byte range relevant to the image that will actually execute.
- Interrupted updates must not cause incomplete firmware to be treated as a valid candidate.
- Boot-state transitions and fallback behavior must remain consistent when flash operations or validation fail.
- Version checks and rollback behavior will be assessed against the implemented version policy. Strong anti-rollback protection is not assumed.
- Firmware authenticity is outside the project scope. SHA-256 comparison alone cannot establish authenticity if both an image and its expected digest can be modified.
- Protocol-specific scenarios will be refined after update-protocol integration.
- Threat severity and control effectiveness will be assessed during security control mapping, source-code review, hardening, and testing.

## 5. Threats and Security Controls Mapping

### Threat-to-Control Mapping

| Threat ID | Security Controls | Control Status |
|---|---|---|
| TH-01 | Calculate the firmware SHA-256 digest and compare it with the expected digest in metadata before execution. | Implemented and verified. |
| TH-02 | Validate metadata magic value, image size, digest, version, state, and applicable sequence fields before use. | Implemented and verified. |
| TH-03 | Validate image start address, image size, end address, and permitted slot boundaries before accessing image contents. | Implemented; boundary-test coverage pending. |
| TH-04 | Treat digest mismatches and SHA-256 calculation errors as validation failures; reject the image and follow the defined fallback or fail-safe path. | Implemented and verified. |
| TH-05 | Verify the image at its destination slot after copying from staging. Use the destination address and intended image size for validation. | Implemented and verified. |
| TH-06 | Keep staged firmware in the pending-validation state until the complete image has been received and validated. Reject incomplete or invalid images. | Implemented and verified. |
| TH-07 | Check flash erase/program operation results. Use valid metadata commit/state handling so interrupted operations do not produce an accepted, inconsistent record. | Commit-marker mechanism implemented; flash-operation failure and interrupted metadata-write recovery need verification. |
| TH-08 | Validate the initial stack pointer, reset-handler address, Thumb-state bit, and application vector-table configuration before handover. | Implemented and verified. |
| TH-09 | Validate metadata states and enforce the permitted transitions for candidate validation, trial boot, confirmation, invalidation, and rollback. | Implemented and verified. |
| TH-10 |	Check the staged firmware version against the project's update-version policy before accepting the candidate. Handle rollback separately through metadata/state and selection of the previously active slot. |	Version check implemented and verified; rollback selection implemented and verified under TH-09. |
| TH-11 | Ensure host-side metadata generation and bootloader verification use consistent firmware image bytes, image size, and padding. Generate the binary with --gap-fill=0xFF to match the flash image representation. | Implemented and verified. |
| TH-12 |Use the defined flash-sector mapping for erase and programming operations. Check operation return statuses and ensure writes or erases cannot affect unintended regions. | Flash-sector selection and programming implemented; invalid-target protection and flash-operation error handling require confirmation. |
| TH-13 | Validate protocol message fields, payload lengths, transfer completion, and update state before programming or accepting firmware. | Protocol-specific validation planned; existing incomplete-transfer recovery is implemented and verified under TH-06. |
| TH-14 | Verify firmware integrity before execution using SHA-256 digest comparison. Consider the possibility of firmware and its expected digest being modified together. Assess exposure through SWD/ST-LINK programming access. | Firmware integrity verification implemented and verified; debug/programming access configuration requires assessment. Authenticity is out of scope. |

### Security Control Principles

- **Firmware integrity:** SHA-256 digest comparison detects image modifications that result in a digest mismatch.
- **Metadata validation:** Validate metadata fields before using them for image verification, slot selection, or execution.
- **Memory safety:** Validate image addresses and sizes against permitted flash boundaries and detect address arithmetic overflow.
- **Update recovery:** Handle incomplete transfers and failed flash operations without accepting invalid firmware or inconsistent metadata.
- **Destination validation:** Verify the firmware at its destination address after copying from the staging area.
- **Application handover:** Validate the initial stack pointer, reset-handler address, and applicable vector-table configuration before transferring control.
- **Version policy:** Enforce the implemented firmware version checks. Version validation and rollback slot selection are separate mechanisms.
- **Host/device consistency:** Ensure firmware packaging and bootloader verification use the same image bytes, image size, and padding convention.
- **Protocol validation:** Validate update-protocol inputs, payload lengths, transfer completion, and update state before programming or accepting firmware.

These principles define the expected security behavior. The implementation and verification status of each control is recorded in the threat-to-control mapping.

### Control Verification Status

- **Implemented:** The control exists in the current implementation.
- **Implemented and verified:** The control has been tested successfully for the relevant scenarios.
- **Implemented; verification pending:** The control exists, but required verification or test coverage remains incomplete.
- **Requires review or hardening:** The implementation needs further inspection or defensive improvements.
- **Planned:** The control has not yet been implemented.
- **Out of scope:** The control is outside the objectives of this project.

A control is marked verified only when supported by test evidence. Verification applies to the scenarios tested and does not automatically establish coverage of every possible failure or attack scenario.

## 6. Security Hardening Review

### 6.1 Review Findings

| Review ID | Area | Existing Implementation / Finding | Action / Status |
|---|---|---|---|
| HR-01 | SHA-256 verification | Firmware digest comparison, mismatch rejection, and fallback behavior are implemented and tested. | **Implemented and verified. No additional action identified.** |
| HR-02 | Metadata validation | `Metadata_Validate()` checks the metadata pointer, magic value, non-zero image size, address arithmetic overflow, and image-region boundary. Digest, version, and state checks are handled by the relevant validation logic. | **Implemented.** Add or confirm maximum-size and one-byte-over-boundary test coverage in the Unit Testing task. |
| HR-03 | Staging-area boundary | The staging area is 64 KB, while Slot A and Slot B each have 128 KB capacity. The metadata size check rejects images beyond the supplied region boundary. | **Hardening implemented; verification pending.** |
| HR-04 | Metadata sequence handling | The append mechanism derives the next sequence number from the previous record and increments it. Record-selection logic ignores duplicate or lower sequence numbers as described by the implementation. | **Implemented; edge-case review required:** Confirm handling of sequence-number overflow and incomplete or invalid records. |
| HR-05 | Flash erase/program failure handling | The bootloader checks erase and programming results, preserves VALIDATED state after a programming failure, and propagates metadata-write and readback failures. | **Caller-side error handling implemented; low-level flash unlock, erase-error reporting, and metadata commit handling require final review.** |
| HR-06 | Application handover | Stack-pointer and reset-handler validation are implemented. Slot-specific addresses are important because the staging and destination addresses differ. | **Implemented and verified. No additional action identified.** |
| HR-07 | Update recovery and rollback | Incomplete-transfer recovery, metadata state transitions, candidate validation, and rollback behavior have been implemented and tested. | **Implemented and verified. No additional action identified.** |
| HR-08 | Host/device image consistency | Host-side image preparation uses `--gap-fill=0xFF` to match the expected flash representation. Staging and destination digest matching have been observed. | **Implemented and verified. No additional action identified.** |
| HR-09 | Update-protocol input validation | Protocol-specific validation is not yet complete. | **Planned:** Review message fields, payload lengths, transfer completion, and update-state handling after protocol integration. |
| HR-10 | Debug/programming access | Firmware integrity verification is implemented, but the current SWD/ST-LINK access configuration has not been established in this review. | **Assessment complete; development access retained. Residual risk documented.** |

### 6.2 Security Hardening Actions

- **Image boundaries:** Destination-capacity and staging-source size checks have been added to `FlashStorage_ProgramImage()`. Boundary-test coverage will be addressed in the Unit Testing task.
- **Metadata sequences:** Append logic increments the sequence number based on previous records. Review sequence-number overflow and incomplete-record handling.
- **Flash operations:** Caller-side erase/program failure handling is implemented. Final review remains for flash-unlock return values, erase-sector error reporting, and metadata commit handling.
- **Application handover:** SRAM range and stack alignment, reset-handler range, and Thumb-state checks are implemented and tested. The vector table is configured for the selected application through `SCB->VTOR`.
- **Protocol inputs:** Review message fields, payload lengths, transfer completion, and update-state handling after protocol integration.
- **Debug access:** Default SWD/ST-LINK debug and programming access is retained for development. The assessment is complete; access restrictions may be reconsidered if the design is prepared for deployment.

### 6.3 Review Status Definitions

- **Implemented:** The control exists in the current implementation.
- **Implemented and verified:** The control has been implemented and successfully tested for the relevant scenarios.
- **Implemented; verification pending:** The control exists, but required verification or test coverage remains incomplete.
- **Requires review or hardening:** Further source inspection or defensive changes are needed to establish adequate protection.
- **Planned:** The required functionality has not yet been implemented or completed.
- **Assessment complete; residual risk documented:** The relevant configuration or limitation has been assessed, and the remaining exposure has been recorded.

A review-required item is not automatically a confirmed vulnerability. It remains open until source inspection or available evidence establishes whether the control is adequate.

### 6.4 Review Outcome

- **Firmware integrity:** SHA-256 verification, digest-mismatch rejection, and fallback behavior are implemented and verified.
- **Metadata validation:** Magic value, image size, arithmetic-overflow, and image-region boundary checks are implemented. Boundary-test coverage will be addressed in the Unit Testing task.
- **Image boundaries:** Destination-capacity and staging-source size checks have been added to `FlashStorage_ProgramImage()`. Verification remains pending.
- **Metadata sequences:** Append-based sequence handling is implemented. Sequence-number overflow and incomplete-record handling require further review.
- **Flash operations:** The bootloader checks erase and programming results and propagates failures. Low-level unlock/erase reporting and metadata commit handling require final review.
- **Application handover:** Stack-pointer range and alignment, reset-handler range, Thumb-state, and vector-table configuration are implemented and tested.
- **Update recovery and rollback:** Incomplete-transfer recovery, metadata state transitions, candidate validation, and rollback behavior are implemented and tested.
- **Host/device consistency:** Firmware image preparation and SHA-256 verification use the corrected image representation, including `0xFF` padding. The relevant verification results have been confirmed.
- **Update protocol:** Protocol-specific input validation remains planned until integration and review are complete.
- **Debug/programming access:** Default SWD/ST-LINK access is retained for development. The associated exposure has been assessed and will remain a documented limitation.
- **Project scope:** Firmware authenticity and digital-signature verification are outside the scope of this project.

### 6.5 Integrity-Only Security Limitation

SHA-256 verification compares the calculated firmware digest against the expected digest stored in metadata. If an attacker can modify both the firmware and its expected digest, this comparison alone does not establish firmware authenticity.

Digital signatures and signing-key management are outside the scope of this project. This limitation is documented to clarify the security properties of the integrity-only design; it does not require implementing a firmware-authentication mechanism.

