
# Memory Architecture

## 1. Overview

The Secure Boot project uses dedicated STM32F407 internal Flash regions for the Bootloader, firmware metadata, download/staging image, and two executable firmware slots.

The Bootloader executes first after MCU reset. It validates firmware metadata and image boundaries, verifies firmware integrity using SHA-256, and transfers control to a validated application image according to the metadata state and implemented boot policy.

The architecture uses fixed physical locations for Slot A and Slot B. Their roles as the previously confirmed image, active image, or update candidate depend on the current update state.

The download/staging region holds incoming firmware before it is programmed into the selected inactive slot.

---

## 2. Flash Memory Layout

The current STM32F407 internal Flash organization is:

```text
STM32F407 Internal Flash (1 MiB)

+------------------+-----------+--------------------------------+
| Address Range    | Sector    | Purpose                        |
+------------------+-----------+--------------------------------+
| 0x08000000-      | Sector 0  | Bootloader                     |
| 0x08003FFF       |           |                                |
| 0x08004000-      | Sector 1  | Bootloader                     |
| 0x08007FFF       |           |                                |
| 0x08008000-      | Sector 2  | Slot A metadata                |
| 0x0800BFFF       |           |                                |
| 0x0800C000-      | Sector 3  | Slot B metadata                |
| 0x0800FFFF       |           |                                |
| 0x08010000-      | Sector 4  | Download / staging firmware    |
| 0x0801FFFF       |           |                                |
| 0x08020000-      | Sector 5  | Firmware Slot A                |
| 0x0803FFFF       |           |                                |
| 0x08040000-      | Sector 6  | Firmware Slot B                |
| 0x0805FFFF       |           |                                |
| 0x08060000-      | Sector 7  | Available / reserved           |
| 0x0807FFFF       |           |                                |
| 0x08080000-      | Sector 8  | Available / reserved           |
| 0x0809FFFF       |           |                                |
| 0x080A0000-      | Sector 9  | Available / reserved           |
| 0x080BFFFF       |           |                                |
| 0x080C0000-      | Sector 10 | Available / reserved           |
| 0x080DFFFF       |           |                                |
| 0x080E0000-      | Sector 11 | Available / reserved           |
| 0x080FFFFF       |           |                                |
+------------------+-----------+--------------------------------+
```

### Key Memory Boundaries

- **Bootloader:** `0x08000000 – 0x08007FFF`
- **Slot A metadata:** `0x08008000 – 0x0800BFFF`
- **Slot B metadata:** `0x0800C000 – 0x0800FFFF`
- **Download / staging firmware:** `0x08010000 – 0x0801FFFF`
- **Firmware Slot A:** `0x08020000 – 0x0803FFFF`
- **Firmware Slot B:** `0x08040000 – 0x0805FFFF`
- **Available / reserved Flash:** `0x08060000 – 0x080FFFFF`
- **Flash end address:** `0x080FFFFF`

The physical locations of Slot A and Slot B remain fixed. Their active and candidate roles can change during the update lifecycle.

The staging region holds the incoming image before promotion into the inactive firmware slot. It is not one of the two executable firmware slots.

The actual usable image size within a slot is constrained by the configured linker layout, image boundaries, and bootloader validation rules.

---

## 3. Bootloader Region

The Bootloader occupies dedicated Flash memory in Sectors 0 and 1.

### Responsibilities

- MCU reset and boot decision handling
- Firmware metadata reading and validation
- Application image-size and memory-boundary validation
- Firmware version checks
- SHA-256 integrity verification
- Staged-image validation
- Programming and verification of the destination slot
- Candidate trial boot and application confirmation
- Rollback to the previously confirmed firmware when a candidate fails to confirm
- BootSafe/fail-safe handling when the selected active image fails integrity verification
- Application handover following successful validation

The Bootloader is separated from the application firmware regions. The Bootloader's own firmware is not included in the application-image SHA-256 calculation described by this project.

**Recovery behavior:** Candidate rollback and active-image integrity failure are handled differently. An unconfirmed candidate can cause rollback to the previously confirmed firmware after reset. If the selected active image itself fails SHA-256 verification, the current implementation enters BootSafe/fail-safe behavior rather than automatically falling back to the alternate slot.

---

## 4. Metadata Regions

Firmware metadata is stored separately from the firmware images. Each executable slot has an associated metadata region.

### Slot A Metadata

- **Flash sector:** Sector 2
- **Base address:** `0x08008000`
- **Address range:** `0x08008000 – 0x0800BFFF`
- **Purpose:** Stores metadata records associated with firmware Slot A.

### Slot B Metadata

- **Flash sector:** Sector 3
- **Base address:** `0x0800C000`
- **Address range:** `0x0800C000 – 0x0800FFFF`
- **Purpose:** Stores metadata records associated with firmware Slot B.

### Metadata Contents

The metadata describes the firmware image associated with a slot and includes information such as:

- Metadata magic value
- Firmware image size
- Firmware version
- Expected SHA-256 digest
- Update state, such as `BOOT_PENDING` or `CONFIRMED`

The metadata journal also uses record sequence information, a record CRC, and a commit marker to help identify valid records and detect corruption or incomplete record writes.

Fixed image boundaries are defined by the Bootloader and linker configuration; they are not trusted solely from metadata.

A CRC and commit marker help detect accidental corruption and incomplete writes, but do not provide cryptographic authentication against an attacker who can modify Flash contents.
