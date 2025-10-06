# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/mnt/c/Users/bah/Documents/RIT/Semester 7/GI/gi/build/_deps/eigen-src"
  "/mnt/c/Users/bah/Documents/RIT/Semester 7/GI/gi/build/_deps/eigen-build"
  "/mnt/c/Users/bah/Documents/RIT/Semester 7/GI/gi/build/_deps/eigen-subbuild/eigen-populate-prefix"
  "/mnt/c/Users/bah/Documents/RIT/Semester 7/GI/gi/build/_deps/eigen-subbuild/eigen-populate-prefix/tmp"
  "/mnt/c/Users/bah/Documents/RIT/Semester 7/GI/gi/build/_deps/eigen-subbuild/eigen-populate-prefix/src/eigen-populate-stamp"
  "/mnt/c/Users/bah/Documents/RIT/Semester 7/GI/gi/build/_deps/eigen-subbuild/eigen-populate-prefix/src"
  "/mnt/c/Users/bah/Documents/RIT/Semester 7/GI/gi/build/_deps/eigen-subbuild/eigen-populate-prefix/src/eigen-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/mnt/c/Users/bah/Documents/RIT/Semester 7/GI/gi/build/_deps/eigen-subbuild/eigen-populate-prefix/src/eigen-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/mnt/c/Users/bah/Documents/RIT/Semester 7/GI/gi/build/_deps/eigen-subbuild/eigen-populate-prefix/src/eigen-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
