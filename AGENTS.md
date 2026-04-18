# AGENTS

- Always name unit test files testUnitUnderTest.cpp where UnitUnderTest is the thing being tested.
- When writting tests, please name the test feature after the file name i.e. testUnitUnderTest.

## File Structure and Naming Conventions

- All C++ source files should be .cpp (not .cc)
- All headers should be .hpp (not .h)
- Library code should be in a separated directory from any application code that uses it or test code that exercises it

### Directory Structure

```
project/
├── lib/                    # Library code
│   ├── include/           # Library headers (.hpp)
│   └── src/               # Library sources (.cpp, .l, .y)
├── test/                  # Unit tests
│   └── test*.cpp         # Test files
├── examples/              # Application examples
│   └── *.cpp             # Example programs
└── CMakeLists.txt        # Build configuration
```