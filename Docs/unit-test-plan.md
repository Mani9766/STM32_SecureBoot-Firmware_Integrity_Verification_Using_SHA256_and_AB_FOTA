**Description**

Identify the software modules, functions, and security-critical decision paths requiring unit-level verification in the STM32F407 Secure Boot and A/B FOTA project.

**Unit testing scope includes:**

- **SHA-256 verification:** Digest calculation, digest comparison, and mismatch handling.
- **Metadata processing:** Metadata validation, magic value, image size and boundaries, CRC, commit marker, record selection, sequence handling, and state transitions.
- **Firmware version checks:** Acceptance and rejection of candidate versions according to the implemented version policy.
- **Image validation:** Image-size limits, address arithmetic overflow, permitted flash-region boundaries, and validation results.
- **Flash storage operations:** Destination-capacity checks, word-aligned programming size, permitted erase sectors, flash unlock/erase/program error handling, and failure propagation.
- **Boot decision logic:** Candidate acceptance or rejection, trial boot, confirmation, rollback, fallback, and fail-safe decisions.
- **Application handover:** Stack-pointer range and alignment, reset-handler range, Thumb-state validation, and vector-table configuration.
- **Update recovery:** Handling of incomplete transfers, failed promotion, invalid candidates, and retry behavior where unit-level verification is applicable.

Protocol-specific validation will be added to the test scope after protocol integration.

Existing implemented and tested functionality will be referenced using its available test evidence. New tests will focus on uncovered boundary conditions, failure paths, and recently added hardening checks.

Hardware-dependent flash operations and end-to-end recovery behavior will be distinguished from unit tests and verified through appropriate integration or hardware tests where necessary.
