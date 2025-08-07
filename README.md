# lazypm

> A fast, minimal TUI wrapper for xbps — because managing Void packages should
> be as lazy as you are.

`lazypm` is a terminal user interface (TUI) for interacting with the `xbps`
package system on Void Linux. Inspired by [lazygit](https://github.com/jesseduffield/lazygit),
it provides a keyboard-driven interface for managing packages without needing to
memorize flags or commands.

## Table of Contents
- [Quick Start](#quick-start)
- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)

## Quick Start

1. Get lazypm binary file on your system. Checkout how to [install](#installation) lazypm below.

1. Run lazypm

```sh
lazypm
```

## Features

- Browse, search, install, remove, and upgrade packages
- View detailed package info
- TUI interface powered by [termbox2](https://github.com/termbox/termbox2)
- Minimal dependencies, fast startup

![output](https://github.com/user-attachments/assets/4883d958-7bfa-42bf-9d77-1f56ba432aa0)


## Installation

```sh
git clone https://github.com/navazjm/lazypm
cd lazypm
gcc ./build/nob.c -o ./build/nob
./build/nob
```

## Usage

```sh
lazypm
```

Navigate with arrow keys or hjkl. Press ? for help inside the UI.

## Contributing 

Want to contribute to lazypm? Awesome, we would love your input ♥

Before starting a new discussion or opening a new issue, please refer to our [release notes](./docs/release_notes.md). 

If you have a feature request, start a [discussion](https://github.com/navazjm/lazypm/discussions),
and we can work together to incorporate it into lazypm!

Encountered a defect? Please report it under [issues](https://github.com/navazjm/lazypm/issues).
Be sure to include detailed information to help us address the issue effectively.

Want to implement a feature request or fix a defect? Checkout our [contributing guide](./docs/contributing.md).

## License

lazypm is licensed under [MIT](./LICENSE)

