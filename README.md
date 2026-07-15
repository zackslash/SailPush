<p align="center">
  <img src="icons/full.png" alt="SailPush" width="128">
</p>

# SailPush

Unofficial Pushover client for SailfishOS. Real-time notifications via WebSocket, background daemon with systemd integration.

## Background Delivery & Battery

SailfishOS has no system-level push service, and long-lived WebSocket
connections do not survive deep sleep (CPU/network suspend). SailPush handles
this with a **hybrid** approach and does **not** require the global
`mcetool --set-suspend-policy=early` override, which disables late suspend for
the entire device and drains battery.

How it works:

- **Screen on:** the daemon holds a persistent WebSocket to Pushover for
  instant delivery.
- **Screen off:** the daemon drops the WebSocket and polls Pushover on the
  configured interval (1–30 min, default 5). Pushover queues messages
  server-side, so nothing is lost — they arrive on the next poll.

Enable **Keep Connection Alive When Screen Off** in Settings if you want the
WebSocket kept alive while the screen is off (uses the same per-app keepalive
for real-time delivery at the cost of higher battery use).

### Limitations

- A plain `QTimer` does not wake the device from deep sleep, so the poll
  interval only fires while the device is awake. True deep-sleep polling
  (via libiphb / `Nemo.KeepAlive.BackgroundJob`) is not yet implemented.
- After a daemon restart, messages that arrived during the previous session
  may not produce a notification — open the app to review them.

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

The unit tests under `tests/` build and run on a regular Linux box with Qt5
(no Sailfish SDK needed for the pure-Qt suites):

```bash
cmake -S tests -B tests/build && cmake --build tests/build && ctest --test-dir tests/build
```

## Architecture

Two-process model: **UI** (QML) communicates with **daemon** (background, systemd) over D-Bus (`com.zackslash.sailpush`). The daemon maintains a persistent WebSocket to Pushover servers and handles notifications.

## License

MIT. Unofficial client, not endorsed by Pushover, LLC.
