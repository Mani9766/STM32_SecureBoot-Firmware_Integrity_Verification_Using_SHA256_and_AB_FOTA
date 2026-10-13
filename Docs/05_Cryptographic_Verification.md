
# Cryptographic Verification

## 1. Overview

This document describes the cryptographic mechanisms used to verify firmware integrity in the STM32F407 Secure Boot and A/B FOTA project.

The current implementation uses SHA-256 to verify firmware images against reference digests stored in firmware metadata. The Bootloader independently calculates the digest of the image being evaluated and compares it with the expected digest.

SHA-256 verification is performed during staged-image validation and again after the image has been programmed into the selected executable firmware slot.

Digital-signature-based firmware authenticity verification is not implemented in the current project scope.

---

## 2. Firmware Integrity Using SHA-256

SHA-256 is a cryptographic hash function that produces a fixed-length 256-bit (32-byte) digest for an input of arbitrary length.

Conceptually, firmware integrity verification works as follows:

```text
Firmware Image
      |
      v
   SHA-256
      |
      v
Calculated Digest
      |
      v
Compare with Expected Digest
```

A change to the firmware data is expected to produce a different digest.

The Bootloader uses this property to detect when a firmware image does not match the expected reference digest.

SHA-256 verification establishes a match with the stored reference digest. It does not independently establish who created the firmware or whether the reference digest is trustworthy.

---

## 3. SHA-256 Verification Process

The expected SHA-256 digest is stored in the metadata associated with the firmware image being evaluated.

During verification, the Bootloader independently calculates SHA-256 over the configured firmware image range in STM32 internal Flash.

```text
             Firmware Image in Flash
                       |
                       v
                 SHA-256 Calculation
                       |
                       v
                Calculated Digest
                       |
                       v
                 Compare Digests
                   /         \
                MATCH       MISMATCH
                  |             |
                  v             v
           Integrity Check   Reject Image
              Passed
```

A matching digest means that the calculated firmware digest corresponds to the expected digest stored in metadata.

A mismatch means that the image fails the integrity check and must not be accepted for execution.

The digest is calculated independently by the Bootloader rather than relying on a digest calculation performed by the application.

A successful digest comparison is one required validation result; metadata, image boundaries, version policy, and application handover checks must also be satisfied where applicable.

---

## 4. SHA-256 Implementation and Verification

The project contains a dedicated SHA-256 implementation used by the Bootloader.

The implementation has been verified using standard test vectors and additional boundary-oriented test cases.

Documented validation includes:

- Empty input.
- The standard `"abc"` test vector.
- 55-byte input.
- 56-byte input.
- Multi-block input.
- Firmware data stored in STM32 Flash.
- Active firmware SHA-256 verification.
- Candidate firmware SHA-256 verification.
- Matching-digest verification.
- SHA-256 mismatch detection.

These tests provide evidence for the standalone hash implementation and its integration with firmware image verification within the executed test scope.

The detailed test results and supporting evidence are maintained with the project's validation records.

---

## 5. Digest Storage and Metadata

The expected SHA-256 digest is stored in the firmware metadata associated with the image being evaluated.

The metadata includes a 32-byte SHA-256 digest, alongside other image information.

```text
Firmware Metadata
+----------------------------+
| Metadata Magic             |
+----------------------------+
| Firmware Image Size        |
+----------------------------+
| Firmware Version           |
+----------------------------+
| SHA-256 Digest (32 bytes)  |
+----------------------------+
| Update State               |
+----------------------------+
```

Separate metadata regions are maintained for Slot A and Slot B.

The metadata journal also uses sequence information, a record CRC, and a commit marker to help identify valid records and detect corruption or incomplete writes.

The expected digest and related metadata are used during firmware validation. The CRC and commit marker support corruption detection and journal consistency; they do not cryptographically authenticate metadata against an attacker capable of modifying Flash contents.

If an attacker can replace both a firmware image and its expected digest, SHA-256 comparison alone cannot establish that the image is authorized.

---

## 6. Active and Candidate Firmware Verification

The same SHA-256 mechanism is used to verify the active firmware and update candidate.

### 6.1 Active Firmware

During normal boot, the Bootloader validates the selected firmware metadata and image boundaries, calculates SHA-256 over the selected executable image, and compares the calculated digest with the expected digest.

Execution proceeds only when the required validation checks succeed.

If the selected active image fails SHA-256 verification, the current implementation enters the configured BootSafe/fail-safe behavior. It does not automatically try the alternate slot in this failure path.

### 6.2 Candidate Firmware and Destination Verification

The update workflow uses a download/staging region at `0x08010000` to hold incoming firmware.

The Bootloader validates the staged firmware using the applicable metadata, image-size, version-policy, boundary, and SHA-256 checks.

When the staged image passes validation, the Bootloader programs it into the selected inactive executable slot:

- Slot A: `0x08020000`
- Slot B: `0x08040000`

After programming, the Bootloader independently calculates SHA-256 over the destination image and compares it with the expected digest.

```text
Download / Staging Image
            |
            v
Validate Metadata, Size,
Version Policy and SHA-256
            |
            v
Select Inactive Slot
            |
            v
Erase and Program Destination
            |
            v
Calculate Destination SHA-256
            |
            v
Compare Expected Digest
       /            \
    MATCH          MISMATCH
      |                |
      v                v
Append            Reject Candidate
BOOT_PENDING      Do Not Trial Boot
      |
      v
Trial Boot from Executable Slot
```

A candidate that passes destination verification can proceed to `BOOT_PENDING` and trial boot.

The candidate executes from its destination Slot A or Slot B address, not directly from the staging region.

A failed staged-image or destination-image integrity check prevents the candidate from being accepted for trial boot. The previously confirmed firmware remains the intended recovery target when it is still valid.

---

## 7. SHA-256 Hashing vs. Digital Signatures

Hashing and digital signatures provide different security properties.

| Mechanism | Primary purpose |
|---|---|
| SHA-256 | Calculate a digest used to detect differences between firmware data and its expected reference digest. |
| Digital signature | Verify that signed data was authorized by the holder of a corresponding trusted private key, subject to the signature scheme and trust model. |
| Private signing key | Create a digital signature. |
| Trusted public key | Verify a digital signature. |

SHA-256 alone does not establish who created the firmware.

Digital-signature verification can add firmware authenticity assurance when the signature is correctly verified against a trusted public key and the signed data covers the relevant firmware and metadata.

Digital-signature-based firmware authentication is outside the current implementation scope.

---

## 8. Possible Future Digital-Signature Verification

Digital-signature verification could be evaluated as a separate future security enhancement.

The exact algorithm, signature format, signed-data format, verification library, and public-key storage strategy have not been selected.

### 8.1 Signing-Side Concept

```text
Firmware Image and Required Metadata
                 |
                 v
           Signing Process
                 |
                 | Uses protected private key
                 v
         Digital Signature
                 |
                 v
 Signed Firmware Package / Image
```

The private signing key must remain outside the embedded firmware and be protected by the signing environment.

### 8.2 Device-Side Verification Concept

```text
Firmware Image and Signed Data
                 |
                 +----------------------+
                 |                      |
                 v                      v
        Calculate Firmware       Read Signature
             Digest
                 |                      |
                 +-----------+----------+
                             |
                             v
                 Verify Signature Using
                    Trusted Public Key
                             |
                       +-----+-----+
                       |           |
                     VALID       INVALID
                       |           |
                       v           v
              Continue Required   Reject Image
              Validation Checks
```

The signature verifier validates the signature over the data defined by the selected signature scheme. A successful verification must be followed by the applicable image, metadata, version-policy, and application-handover checks.

This is a conceptual future architecture and does not describe a feature implemented in the current Bootloader.

---

## 9. Trust Model and Current Limitations

The current implementation uses a stored SHA-256 digest to check firmware integrity. It does not include cryptographic signature verification or a trusted signing-key mechanism.

Consequently:

- SHA-256 comparison can detect an image that does not match its expected digest.
- A matching digest does not prove that the image came from an authorized firmware publisher.
- The expected digest and security-relevant metadata are not cryptographically authenticated by the current design.
- Digital-signature verification and trusted-key management are not implemented.

A future signature-based design would need to define how the public key or trust anchor is protected, what data is signed, how invalid signatures are handled, and how signing keys are managed.

---

## 10. Verification Failure Behavior

The current implementation uses different responses for different failure scenarios.

### Staged-Image Integrity Failure

If the staged firmware fails its SHA-256 comparison, the candidate update is rejected and must not be selected for trial boot.

### Destination-Image Integrity Failure

If the programmed destination image fails SHA-256 verification, the candidate is rejected and must not proceed to trial boot.

### Candidate Trial-Boot Failure

If a candidate enters `BOOT_PENDING` but remains unconfirmed after a reset, the Bootloader detects the unconfirmed state and returns to the previously confirmed firmware according to the implemented recovery policy. The documented watchdog-reset scenario has been tested.

### Selected Active-Image Integrity Failure

If the selected active image fails SHA-256 verification, the Bootloader enters the configured BootSafe/fail-safe behavior. The current implementation does not automatically fall back to the alternate slot in this path.

These behaviors are supported by the documented functional and recovery test evidence.

---

## 11. Current Implementation and Verification Status

**Status: SHA-256 Implemented and Verified for the Documented Test Scope**

The current implementation includes:

- Dedicated SHA-256 algorithm implementation.
- Standard SHA-256 test-vector validation.
- Boundary-oriented and multi-block test cases.
- SHA-256 calculation over firmware stored in STM32 Flash.
- Active firmware integrity verification.
- Staged candidate integrity verification.
- Destination-image verification after Flash programming.
- Expected-digest comparison.
- SHA-256 mismatch detection.
- Rejection of invalid candidate firmware.
- Fail-safe behavior when the selected active image fails integrity verification.
- Candidate trial boot and rollback behavior following the tested unconfirmed-candidate reset scenario.

The status reflects the implemented mechanisms and the recorded test cases. It does not claim exhaustive fault coverage, firmware authenticity, or formal standards compliance.

---

## 12. Out-of-Scope Cryptographic Enhancements

The following capabilities are not implemented in the current project release:

- Firmware digital-signature verification.
- Trusted public-key integration and key management.
- Authenticated firmware metadata.
- Cryptographically enforced anti-rollback protection.

These capabilities may be considered in a future project phase according to its security requirements and threat model. They are not pending implementation or testing tasks for the current portfolio release.
