---
default: patch
---

The host test harness of `grkit module test` gives a module's used settings their own handles, so a module that uses a setting of the core, such as `CORE.TONE`, can test it: used setting n has handle n (`GRKIT_TEST_USES` in `runtime/sdk/grkit_test.h`).
