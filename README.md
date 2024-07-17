# File Appender Virus

This executable is a File Appender Virus, meaning that it will infect other PE executables by appending to the beginning of the file. The virus will preserve the original executable's icon.

# Behavior

The virus extracts from itself the original executable, makes a temporary copy of it, and runs it.

# Infection

After the original executable's execution is finished, the virus will check for a central file located at path "C:\\Users\\Public\\Documents\\atm.exe". 

If the file exists, then it means that the machine is already infected, otherwise it will create the respective file and then proceed to search and infect executables recursively, starting with path "C:\\"

# Payload

Finally, the payload is just a MessageBox displaying "You got pwned!"

# Installation

The GCC compiler ( provided by MSYS 2 ) was used to build the program on Windows 11.
Run the following commmand to compile the program

```
gcc -o v.exe v.c -O0
```

Afterwards, just run the executable v.exe

# Additional Notes

The project needs more testing, although currently it seems to be working on Windows 11 Pro 23H2 22631.3880 with Windows Defender turned off.

# Acknowledgements

For copying the icons and other resources from the resource directory, I used and modified the resource copier library from vzlomshikzloy. Unfortunately, I couldn't find the original web page from where I found the library.