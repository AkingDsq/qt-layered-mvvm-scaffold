# Downloaded Design from Figma to Qt

Save the downloaded design from Figma to Qt here, including the CMakeLists.txt.

The main project CMakeLists.txt does not pick this directory up automatically:
add `add_subdirectory(importedcontent)` to the root CMakeLists.txt (or to
`src/CMakeLists.txt`) once you actually drop generated code here, otherwise the
files stay out of the build.
