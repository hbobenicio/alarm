# alarm

## Usage

```
HOUR=13 MIN=42 MSG="Don't forget to drink some water!" ./alarm
```

## Dependencies

### Build dependencies

- C11 compiler **(clang recommended)**
  - Ubuntu 18.04 LTS: `clang`
- pkg-config **(recommend)**, for finding gtk flags and libs. You can set the flags with other tools or manually though
  - Ubuntu 18.04 LTS: `pkg-config`
- gtk 3 **(required)**
  - Ubuntu 18.04 LTS: `libgtk-3-dev`
- alsa libasound 2 **(required)**
  - Ubuntu 18.04 LTS: `libasound2-dev`

### Runtime dependencies

- gtk 3
  - Ubuntu 18.04 LTS: `libgtk-3`
- alsa libasound 2
  - Ubuntu 18.04 LTS: `libasound2`

## Build

### Release profile

```
make release
```

### Debug profile

```
make debug
```

## Cleaning

`make clean` and `make distclean` are available.

## Extras

The target `vscode-include-paths` is available to help you configure vscode include path to satisfy intellisense autocompletion.

## References

- Alsa useful links:
  - https://www.alsa-project.org/alsa-doc/alsa-lib/index.html
  - https://www.linuxjournal.com/article/6735
