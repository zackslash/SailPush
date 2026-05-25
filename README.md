# SailPush

Unofficial Pushover client for SailfishOS. Real-time notifications via WebSocket, background daemon with systemd integration.

## Deep Sleep

SailfishOS has no system-level push service. WebSocket connections break in deep sleep. For reliable notifications:

```bash
mcetool --set-suspend-policy=early
```

This keeps the CPU and network alive when the screen is off. Without it, notifications arrive when the device wakes up (delayed up to the polling interval).

## Install

Download the `.rpm` for your architecture from [Releases](https://github.com/zackslash/SailPush/releases):

```bash
devel-su pkcon install-local ./sailpush-<version>.rpm
systemctl --user daemon-reload
systemctl --user enable --now sailpush.service
```

Upgrading: install the new RPM over the old one. Credentials and settings are preserved.

## Build

Requires Sailfish OS SDK.

```bash
qmake5 && make        # local build
mb2 build             # RPM build
```

## Architecture

Two-process model: **UI** (QML) communicates with **daemon** (background, systemd) over D-Bus (`net.sailpush.Sailfish`). The daemon maintains a persistent WebSocket to Pushover servers and handles notifications.

## License

MIT. Unofficial client, not endorsed by Pushover, LLC.
