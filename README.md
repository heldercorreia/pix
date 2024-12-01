# Pix

Rename photos and videos based on EXIF capture timestamp.

## Overview

Pix is a command-line tool that renames photo and video files based on their
EXIF metadata, specifically the capture timestamp. It supports custom formatting
and recursive directory traversal. This utility is helpful for organizing media
files chronologically.

## Features

- Rename files using the EXIF `DateTimeOriginal` and `SubSecTimeOriginal` tags.
- Customizable filename formats using `strftime`-compatible format specifiers (see [strftime.org](https://strftime.org/)).
- Preset formats for common naming conventions.
- Recursive directory processing.
- Simulate mode to preview changes without modifying files.

## Usage

```
Usage: pix [OPTIONS] FILE | DIRECTORY
Rename photos and videos based on EXIF capture timestamp.

Options:
  -p, --preset <NUMBER>      Use predefined format preset (default is 0):
                               0: %Y%m%d%H%M%S%f
                               1: %Y%m%d_%H%M%S_%f
                               2: %Y%m%d-%H%M%S-%f
                               3: %Y-%m-%d_%H-%M-%S_%f
                               4: %Y_%m_%d-%H_%M_%S-%f
  -f, --format <FORMAT>      Use custom format (strftime-compatible; overrides preset)
  -s, --simulate             Perform a dry run (do not rename files)
  -r, --recursive            Recursively process directories
  -v, --version              Show program version
  -h, --help                 Show this help message

A target FILE or DIRECTORY must be specified.
If a FILE is specified, only that file will be processed.
If a DIRECTORY is specified, files in that directory will be processed.
Use '.' to specify the current directory.
```

## Examples

### Rename a Single File

```bash
pix photo.jpg
```

Renames `photo.jpg` using the default preset format.

### Rename Files in a Directory (Non-Recursive)

```bash
pix /path/to/directory
```

Processes all supported files in `/path/to/directory`.

### Rename Files in a Directory Recursively

```bash
pix -r /path/to/directory
```

Processes all supported files in `/path/to/directory` and its subdirectories.

### Use a Custom Format

```bash
pix -f "%Y_%m_%d__%H%M%S" photo.jpg
```

Renames `photo.jpg` using the custom format.

### Simulate Actions Without Renaming

```bash
pix -s -r /path/to/directory
```

Performs a dry run and displays the intended renaming actions without modifying any files.

### Specify the Current Directory

```bash
pix .
```

Processes all supported files in the current directory (non-recursive).

### Recursive Processing in Current Directory

```bash
pix -r .
```

Processes all supported files in the current directory and subdirectories.

## Building Guide

Pix uses `CMake` for its build system.

### Prerequisites

- **C Compiler**: Ensure you have a C compiler that supports C17 (e.g., `gcc`, `clang`).
- **CMake**: Version 3.10 or higher.
- **libexif**: Install the development files for `libexif`.

#### Installing Dependencies on Debian/Ubuntu

```bash
sudo apt-get update
sudo apt-get install build-essential cmake libexif-dev
```

### Building Pix

1. **Clone the Repository**

   ```bash
   git clone https://github.com/heldercorreia/pix.git
   cd pix
   ```

2. **Create a Build Directory**

   ```bash
   mkdir build
   cd build
   ```

3. **Run CMake and Build**

   ```bash
   cmake ..
   make
   ```

4. **Install (Optional)**

   ```bash
   sudo make install
   ```

   This will install the `pix` executable to `/usr/local/bin`.

## Supported File Types

Pix recognizes the following image and video file extensions:

- Images: `.jpg`, `.jpeg`, `.png`, `.gif`, `.bmp`, `.tif`, `.tiff`, `.heic`, `.heif`, `.webp`
- Videos: `.mp4`, `.mov`, `.avi`, `.mkv`

## Custom Format Specifiers

Pix uses `strftime`-compatible format specifiers with an additional `%f` for microseconds extracted from `SubSecTimeOriginal`.

- **Example Format**: `%Y%m%d%H%M%S%f`
- **Microseconds Handling**:
  - If `SubSecTimeOriginal` exists:
    - Value is adjusted to always be 6 digits.
    - Padded with zeros on the right if less than 6 digits.
    - Truncated to 6 digits if more.
  - If it doesn't exist:
    - Defaults to `000000`.

## Contributing

Contributions are welcome! Please open an issue or submit a pull request on GitHub.

## License

This project is [licensed](LICENSE) under the MIT License.

---

## **Additional Notes**

- **Error Handling**: Always ensure you have backups of your files before running batch operations that modify or rename files.
- **Extending File Extensions**: To handle additional file types, modify the `extensions` array in the `is_image_or_video` function.

