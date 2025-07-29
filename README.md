# Python Requirement for GenerateBridgeWrappers

This Unreal Engine project includes a Python script (`GenerateBridgeWrappers.py`)  
used to automatically generate bridge wrapper header files for editor interfaces.

---

## Python Required

In order to execute this script as part of your Unreal Editor module build process,  
**Python 3.13 or higher must be installed** on your system.

### Check if Python is available

Open a terminal and run:
python --version

If Python is not installed, visit [https://www.python.org/downloads/](https://www.python.org/downloads/) to install it.

---

## When is Python used?

During the editor module compilation (`ARPGEditor.Build.cs`),  
the build system automatically runs `GenerateBridgeWrappers.py`  
to scan header files and create static bridge wrapper headers  
under your project's `Intermediate/BridgeWrappers/` directory.

---

For further details on the bridge framework and how to use or configure the script,  
please refer to the inline documentation provided in `TNAEdBridgeRegistry.h`.

