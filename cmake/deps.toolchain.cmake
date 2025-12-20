# dependency on the toolchain is brought by CPM as a package
CPMAddPackage (
  NAME cmake_scripts
  GIT_REPOSITORY "https://github.com/kodezine/cmake_scripts"
  GIT_TAG ${GITHUB_BRANCH_toolchain}
  OPTIONS
    DOWNLOAD_ONLY TRUE
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

