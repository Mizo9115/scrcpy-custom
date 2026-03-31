# scrcpy Quick Reference

## Recommended Command

```bash
scrcpy --turn-screen-off --screen-off-key --stay-awake --always-on-top --no-audio
```

Short form:

```bash
scrcpy -Sw --screen-off-key --always-on-top --no-audio
```

This will:
- Mirror the phone to your PC
- Turn off the phone's physical screen immediately
- Keep the phone awake while scrcpy is running
- Pin the scrcpy window on top of all other windows
- Keep all audio (calls + media) on the phone's Bluetooth headphones


## Command-Line Flags

| Flag                     | Short | Description                                              |
|--------------------------|-------|----------------------------------------------------------|
| `--turn-screen-off`      | `-S`  | Turn the device display off when mirroring starts        |
| `--screen-off-key`      |       | Turn the device screen off by setting brightness to 0 (keeps device interactive; BT + PC mirror work) |
| `--stay-awake`           | `-w`  | Keep the device awake while scrcpy is running            |
| `--always-on-top`        |       | Keep the scrcpy window above all other windows           |
| `--no-audio`             |       | Disable audio forwarding (audio stays on the phone)      |
| `--audio-dup`            |       | Duplicate audio to both PC and phone (Android 13+)       |
| `--screen-off-timeout N` |       | Set a custom screen-off timeout in seconds               |
| `--power-off-on-close`   |       | Turn the phone screen off when scrcpy is closed          |
| `--no-power-on`          |       | Don't wake the screen when scrcpy starts                 |


## Sidebar (this fork)

Move the mouse to the **left edge** of the scrcpy window (within about 8px) to reveal a panel with four buttons (top to bottom):

1. **Screen dim / restore** — same as toggling display power (pairs with `--turn-screen-off` / `--screen-off-key` behavior on the device).
2. **Back** — injects Android `BACK`.
3. **Force-close** — `am force-stop` on the current foreground app (blocked for launcher / System UI).
4. **Always on top** — toggles the scrcpy window on the PC.

Requires control enabled (not `--no-control`). Rebuild both the **client** and **server** APK after pulling changes.

## Keyboard Shortcuts

MOD key = Left Alt or Left Super (Windows key)

| Shortcut         | Action                          |
|------------------|---------------------------------|
| MOD + o          | Turn device screen off          |
| MOD + Shift + o  | Turn device screen on           |
| MOD + p          | Press the power button          |
| MOD + t          | Toggle always-on-top            |


## Audio Options

- **`--no-audio`** — All audio stays on the phone. Best for Bluetooth headphones connected to the phone.
- **`--audio-dup`** — Audio plays on both the phone and PC (requires Android 13+, some apps may opt out).
- Default (no flag) — Audio is captured from the phone and played on the PC only.
