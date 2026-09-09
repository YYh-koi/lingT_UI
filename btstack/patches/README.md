# BTstack PBAP local patch

`btstack-1.6.2-pbap-preserve-obex-status.patch` is the single audited local
status-preservation change on top of BTstack v1.6.2 `pbap_client.c`.

- Official base SHA-256: `0e7ed47854273b0424f45225740a52a21d701a0d076b1ad2af79ff42c55f2cb4`
- Checked-in patched SHA-256: `18b2de664a3f30ebe14856ae3d096b382a543ae09377249335de8cd2e4d812c1`
- Audit command: `git apply --check --reverse`

The patch also records the upstream `No newline at end of file` transition so
reverse application reconstructs the exact official byte stream.
