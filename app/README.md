# User app

## Dependencies

### msys2

Use the mingw environment using `msys2`. Use visual studio generator.

- `pacman -Sy python-pip python`
- `python -m pip install conan`
- `pacman -Sy mingw-w64-x86_64-boost`  # or use conan to download it.

#### Build

```
$ cmake .. -G 'Visual Studio 16 2019'
$ cmake --build .
```
