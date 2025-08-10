# Lazypm — Changelog & Roadmap

This document tracks all significant changes and future plans for the `lazypm` project.

## Changelog

All notable changes are documented in this section.

### [Unreleased]
- N/A 

### [0.1.0] Core MVP - 2025-08-09

- [x] Display list of packages.
- [x] Install/update selected package.
- [x] Update all packages.
- [x] Uninstall selected package.
- [x] Filter list of packages.
- [x] Status messages.
- [x] Log debug info to a file.
- [x] Show list of all keybindings.
- [x] ~~Capture error when xbps itself needs an update.~~ Update `xbps` on startup.

## Roadmap

Planned and in-progress work for upcoming versions of `lazypm`.

### In Progress

- [ ] Modal to capture user password to run xbps-install and xbps-remove
  - For now, just run `sudo lazypm`

### Planned

- [ ] Keybinding to list depedencies of a package
  - also show what packages depend on this package?

### Under Consideration

- [ ] Run commands in `dry mode` to see results before running the command 
- [ ] Allow users to set color scheme
  - use env_vars or config file
- [ ] Allow users to set custom keybindings 
  - use env_vars or config file
- [ ] Allow users to specify default options to various xbps commands 
  - Example: by default, lazypm runs `xbps-install -Syu ...`, allow for user to change
`-Syu`
- [ ] Support `xbps-src`??

## Conventions

- Versioning follows `MAJOR.MINOR.PATCH`
- Dates use `YYYY-MM-DD` format
- Keep descriptions brief, clear, and action-based
- Use checkboxes to track roadmap progress

