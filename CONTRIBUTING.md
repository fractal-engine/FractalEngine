# Guidelines

## Getting Started

### Installing Xmake

As part of set up, make sure that Xmake is installed for building and running the project.

#### On Windows

1. **Using curl**:

     ```bash
    curl -fsSL https://xmake.io/shget.text | bash
     ```

2. **Using powershell**:

     ```bash
    cInvoke-Expression (Invoke-Webrequest 'https://xmake.io/psget.text' -UseBasicParsing).Content
     ```

#### On macOS

1. **Using Homebrew**:
   - If you have [Homebrew](https://brew.sh/) installed, you can install Xmake by running:
     ```bash
     brew install xmake
     ```

### Documentation

For more detailed instructions and advanced usage, refer to the [official Xmake documentation](https://xmake.io/#/guide/installation).


## Git commits

- **Do not merge `master` into your branch**: Instead of merging `master` into your branch, rebase your branch onto `master` to keep the commit history clean, this will make squashing much easier in the process.

- **Squash your commits**: For every pull request (PR), consider squashing your commits into a single commit. This keeps the Git history clean and makes it easier to track changes.
- **How to squash commits**:
  1. Use the command git `rebase -i master` to open the interactive rebase editor.
  2. Follow the instructions in the editor to merge, squash, or rewrite your commits as necessary.


## Code review
