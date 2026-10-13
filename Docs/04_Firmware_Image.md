
# Firmware Image Layout and Validation

## 1. Purpose

This document describes the firmware images used by the STM32F407 Secure Boot and A/B FOTA project, including their build addresses, storage locations, associated metadata, and integrity-validation process.

The project uses three firmware-image locations:

- **Download / staging region:** Stores the incoming firmware image before promotion.
- **Slot A:** Stores one executable firmware image.
- **Slot B:** Stores the other executable firmware image.

Slot A and Slot B have fixed physical addresses. Their active, previously confirmed, and candidate roles depend on the current firmware update state.

Each executable firmware image must be built for its intended execution address. The Bootloader validates the staged image, programs the selected inactive slot, verifies the programmed destination image, and manages candidate trial boot and confirmation through metadata records.

The implementation uses SHA-256 for firmware integrity verification. Digital-signature-based firmware authenticity verification is outside the current project scope.

---

## 2. Firmware Image Locations

| Image location | Flash sector | Start address | Purpose |
|---|---|---|---|
| Download / staging | Sector 4 | `0x08010000` | Holds incoming firmware for validation and promotion. |
| Firmware Slot A | Sector 5 | `0x08020000` | Executable firmware image A. |
| Firmware Slot B | Sector 6 | `0x08040000` | Executable firmware image B. |

The configured regions are:

- **Staging:** `0x08010000 – 0x0801FFFF`
- **Slot A:** `0x08020000 – 0x0803FFFF`
- **Slot B:** `0x08040000 – 0x0805FFFF`

The usable image size is constrained by the configured linker layout and the Bootloader's image-size and memory-boundary checks.

---

## 3. Download / Staging Firmware Region

The staging region holds incoming firmware before promotion into an executable firmware slot.

- **Start address:** `0x08010000`
- **End address:** `0x0801FFFF`
- **Flash sector:** Sector 4
- **Purpose:** Stores firmware for validation and subsequent programming into the selected inactive slot.

The update workflow uses staging metadata and image information to validate the candidate before destination programming.

The Bootloader performs the applicable checks, including:

- Metadata validity
- Firmware image size and memory boundaries
- Firmware version policy
- SHA-256 integrity verification

When the staged image passes validation, the Bootloader selects the inactive executable slot, erases the relevant destination region, and programs the staged image into that slot.

The SHA-256 digest of the programmed destination image is then independently calculated and compared with the expected digest.

A matching destination digest allows the update workflow to proceed to the `BOOT_PENDING` state. The candidate is then selected for trial boot from its executable slot, not directly from the staging region.

An invalid staged image is rejected and is not selected for trial boot.

---

## 4. Executable Firmware Slots

The architecture provides two executable firmware slots.

### Slot A

- **Start address:** `0x08020000`
- **End address:** `0x0803FFFF`
- **Flash sector:** Sector 5
- **Purpose:** Stores executable firmware Slot A.

### Slot B

- **Start address:** `0x08040000`
- **End address:** `0x0805FFFF`
- **Flash sector:** Sector 6
- **Purpose:** Stores executable firmware Slot B.

Slot A and Slot B are physical locations. The currently confirmed and candidate roles are determined by the metadata state and update policy.

The Bootloader validates the selected image metadata, image boundaries, SHA-256 digest, and application handover information before transferring control to the application.

### Trial Boot and Confirmation

After the inactive slot has been programmed and its destination SHA-256 digest verified, the Bootloader records `BOOT_PENDING` in the applicable metadata journal and selects the candidate for trial boot.

The candidate application is expected to confirm successful startup by calling `Firmware_Confirm()`. A successful confirmation appends a `CONFIRMED` metadata record.

If the candidate remains unconfirmed after a reset, the Bootloader uses the recorded state to return to the previously confirmed firmware according to the implemented recovery policy.

The current implementation has been tested for the documented candidate confirmation and watchdog-reset rollback scenarios.

---

## 5. Vector Table and Application Handover

The Bootloader and executable applications use their respective vector tables.

The Bootloader begins execution from the boot memory region. Before transferring control to an application, the Bootloader validates the application image and configures the Vector Table Offset Register (VTOR) for the selected executable slot.

### Executable Vector Table Locations

```text
Bootloader Vector Table  -> 0x08000000
Slot A Vector Table      -> 0x08020000
Slot B Vector Table      -> 0x08040000
```

The staging region starts at `0x08010000`, but that is the storage address of the incoming image, not the executable vector-table address used for Slot A or Slot B trial boot.

The Bootloader obtains the application's initial stack pointer and reset-handler address from the selected executable image. It validates the applicable address constraints before transferring control to the application's reset handler.

The executable firmware build and its vector-table configuration must correspond to the slot address at which that firmware will run.

---

## 6. Linker Script Configuration

The project uses linker configurations for the Bootloader and executable firmware images.

The relevant configurations define:

- Flash origin and length
- RAM origin and length
- Code and read-only data sections
- Initialized and uninitialized data sections
- Stack and heap settings, where configured
- Application vector-table location

Executable firmware must be built for its intended execution address. Slot A firmware uses the Slot A address, and Slot B firmware uses the Slot B address.

The staging region holds the incoming image before it is programmed into an executable slot. Staging storage must not be confused with the destination address for application execution.

The linker configuration and Bootloader validation rules must consistently enforce the intended Flash and RAM boundaries.

---

## 7. Memory Validation and Integrity Protection

The current memory architecture and Bootloader validation logic provide the following mechanisms:

- Separation of Bootloader, metadata, staging, and executable firmware regions.
- Fixed physical locations for Slot A and Slot B.
- Separate metadata sectors for the two executable slots.
- Known image start addresses and configured image-size limits.
- Firmware image boundary validation.
- Firmware version checks in the update workflow.
- Metadata validation before firmware selection.
- SHA-256 verification of the staged image.
- SHA-256 verification of the programmed destination image.
- Reset-handler validation before application handover.
- Candidate trial boot, explicit confirmation, and the implemented rollback policy.

Additional hardware memory-protection mechanisms may be considered separately if required by a future deployment context.

The SHA-256 mechanism verifies that an image matches the expected digest stored in metadata. It does not, by itself, establish firmware authenticity or protect the expected digest against unauthorized modification.

---

## 8. Known Design Limitations and Out-of-Scope Enhancements

The current project release includes metadata-driven update state management, candidate-to-slot programming, trial boot, application confirmation, and the documented recovery behavior.

The following capabilities are not claimed as implemented security features:

- **Firmware authenticity:** Digital-signature verification and trusted-key management.
- **Authenticated metadata:** Cryptographic protection of security-relevant metadata and reference digests against unauthorized modification.
- **Cryptographically enforced anti-rollback protection:** The update workflow includes firmware version checks, but a protected monotonic version mechanism is not claimed.
- **Production firmware download:** The current workflow uses a demonstration trigger and staged firmware rather than a complete production communication and download mechanism.
- **Automatic alternate-slot fallback for active-image corruption:** The current selected-active-image SHA-256 mismatch path enters BootSafe/fail-safe behavior.
- **Actual power-loss resilience:** The tested debugger-reset recovery scenario does not establish resilience to actual power interruption during Flash programming or metadata writes.

These are documented boundaries of the current project scope, not pending implementation or testing tasks for this release.

---

## 9. Implementation and Verification Status

**Status: Implemented and functionally verified for the documented project scope**

The memory layout, firmware metadata handling, image validation, SHA-256 verification, destination programming, and A/B update behavior have been exercised through the project's documented functional and recovery test cases.

Verification evidence includes debugger observations, ITM output, firmware version and metadata-state output, and LED indications where applicable.

### Current Memory Addresses

- **Bootloader start:** `0x08000000`
- **Slot A metadata:** `0x08008000`
- **Slot B metadata:** `0x0800C000`
- **Download / staging firmware:** `0x08010000`
- **Executable firmware Slot A:** `0x08020000`
- **Executable firmware Slot B:** `0x08040000`
- **Flash end:** `0x080FFFFF`

The project demonstrates the use of a dedicated staging region, two executable firmware slots, metadata-driven update states, SHA-256 integrity verification, candidate confirmation, and recovery to previously confirmed firmware for the documented failure scenarios.

This status reflects the implemented functionality and recorded test evidence. It does not imply exhaustive fault coverage, firmware authenticity, or formal standards compliance.

