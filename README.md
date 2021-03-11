# Simple Bash Shell

## Brief Description

The following simple Linux bash shell is a team project built with C. The shell is similar to a normal Linux shell; It can run scripts that are already in the computer, run Linux basic commands such as ls, rm, and gedit, and it also contains a few built-in commands, which are explained below.

### The Ps all command

The following command enables the user to display any running or finished background process.

```bash
    ps_all
```

### The Search command

The search command allows the user to search for a specific keyword or phrase in source code files. The command can also work recursively using the -r option. Running it recursively will allow the user to also search the subdirectories. The command is limited to files with the following extenstions:

- .c or .C
- .h or .H

```bash
    search "main()"
    search -r "main()"
```

### The Bookmark command

The following command allows users to bookmark their frequently used commands. Rather than typing the command every time it needs to be executed, a user can simply use bookmark with the -i option followed by the index of the command. Using the -l with the command will list the bookmarked commands with their respective indexes.

```bash
    bookmark "ps -a"
    bookmark -i 0
```

### Other Functionalities

Similar to a normal bash shell, the following shell can execute I/O redirection commands, stop running the foreground process, and run the process in the background.

---

## Team Members

- Basil Osama Hamid ElKhalifa
- Muhammedcan Pirinççi
- Osama Azmat Mustafa

---
