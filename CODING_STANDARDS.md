# Coding Standards

As far as possible, we try to use tools to ensure a consistent style in all languages, and strive to run these tools as CI jobs.
New code should follow these rules, and old code should be changed over time to satisfy them.

## All Languages

- Each file should have a well-defined purpose and be concerned with a single narrow concept.
- Add unit tests for new code. As far as possible, one test per source file or logical group of files.
- Names should be as clear and concise as possible, avoiding opaque abbreviations.

## Python

We adhere to the `pep8` standard, using `flake8`, `pylint` and `mypy` to catch further programming errors or standard violations. Type hints must be used everywhere.

## C++

The [.clang-format] file describes the C++ code formatting used in the project. The script
```console
$ scripts/format
```
can be run from the repository root to format all files. The CI builds will fail if formatting issues are found.

As well as the above formatting, the following conventions are also used in the C++ code:

### Formatting

- Long comments at the end of lines cause unnecessary line breaks. Comment above the line (do not add empty line between the comment and the line).
- Multi-line comments should use `//` style (clang-format does not cope well with other styles).
  ```
  // Your comment here line 1
  // line 2
  ```
- Do not omit braces for single-line statements in `if`, `for`, `while`, etc clauses. (Note that clang-format does not enforce this).
  ```
  // GOOD:
  if (condition) {
      x = 0;
  } else {
      x = 1;
  }
  ```
  instead of:
  ```
  // BAD:
  if (condition)
      x = 0;
  else
      x = 1;
  ```
- `//` can be used at the end of a line in an initializer to retain
  formatting:
  ```
    libsnark::pb_variable_array<FieldT> value = from_bits(
        {
            0, 0, 1, 0, 1, 1, 1, 1, //    <--- these breaks will remain
            0, 0, 0, 0, 0, 0, 0, 0, //
            0, 0, 0, 0, 0, 0, 0, 0, //
            0, 0, 0, 0, 0, 0, 0, 0, //
            ...
  ```
- Do not leave whitespace at the end of lines
- Documentation comments should use the `///` style, and should be present in the include files ONLY.

### Naming and ordering conventions

- Variables, functions and classes use `snake_case_names`.
- Macros are `ALL_CAPS`.
- Functions operating on some type `my_type` should be named `my_type_operation`, where the `my_type` argument appears first.
- Type parameters are Pascal or Camel-case with a trailing `T` (e.g. `typeT` or `MyTypeT`)
- Each source file should be concerned with a single "concept" (often single class or type + methods)
- Encodings are named:
  - `json`
  - `hex`
  - `bytes`
  - `string` (human-readable string, if different from hex and json)
  - `proto` (protobuf-defined objects)
  Note that `hex` implicitly means a string type. `json` refers to strings which contain "whole" json objects, e.g. `1234`, `"string"`, `["array", "of, "strings"]` or `{"an": "object", "with": "attributes"}`.
- Encoding methods are named `to_<encoding>()` and static parsing methods are named `from_<encoding>` (e.g. `to_json` or `from_proto`).
- Reading from and writing to streams is performed by methods `std::istream &read_<encoding>(std::istream &)` and `std::ostream &write_<encoding>(std::ostream &)` (e.g. `write_json` or `read_bytes`).
- Similarly, encoding, parsing, reading and writing functions are named `<type>_to_<encoding>`, `<type>_from_<encoding>`, `<type>_read_<encoding>` and `<type>_write_<encoding>` (e.g. `point_g1_affine_to_json`, `point_g1_affine_from_proto`).
- Conversion between encodings (where there is no high-level type) are named `<source>_to_<dest>` (e.g. `hex_to_bytes`).

### Other

- Avoid `auto` except when strictly necessary.
- Use `const` (including for local variables) where possible.
- Implement streaming and/or encoding methods based on what is sensible for the underlying type. There is no need to for all types to be serializable to all encodings if the rest of the code does not use them.
- In general, pass inputs by (const) reference, and return by value. Implement sensible move operators.
