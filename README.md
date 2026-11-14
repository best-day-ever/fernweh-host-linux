# Fernweh Linux Host

The Linux Host component of [BDE Fernweh](https://github.com/best-day-ever/fernweh),
a downstream source fork of [PLANK Host](https://github.com/instinctual/plank-host-linux),
which is derived from [Sunshine](https://github.com/LizardByte/Sunshine).
This is public ALPHA source. No new binary release is provided.
The parent [publication receipt](https://github.com/best-day-ever/fernweh/blob/main/docs/fernweh/review.md) records the history audit.

Build from the parent Fernweh gitlink and its exact transport/dependency patches.
The fork's baseline is Rocky 9.7 x86-64, NVIDIA graphics and X11; other inherited
Sunshine platforms are not Fernweh support claims. Runtime network identity,
keytab/MFA/PAM policy and shared-desktop policy belong in `/etc/plank/host.conf`.
No BDE directory, broker address or private deployment inventory is a compiler input.

- [Platform build instructions](https://github.com/best-day-ever/fernweh/blob/main/docs/fernweh/build.md#linux-host)
- [Broker integration](https://github.com/best-day-ever/fernweh/blob/main/docs/fernweh/broker.md)
- [GSSAPI admission contract](docs/plank-gssapi-admission.md)
- [Enhancements and limits](https://github.com/best-day-ever/fernweh/blob/main/docs/fernweh/enhancements.md)
- [Development and agent guidance](https://github.com/best-day-ever/fernweh/blob/main/docs/fernweh/development.md)
- [Contributing fixes to PLANK](https://github.com/best-day-ever/fernweh/blob/main/docs/fernweh/upstreaming.md)

Preserve upstream history, notices and the [GPLv3 license](LICENSE). GPU/NvFBC
SDK entitlement, capture/input/session tests and installed hardware acceptance
remain separate from a successful package build. The parent contains the RPM,
PAM/systemd configuration and the optional broker; this component alone is not
a complete service deployment.
