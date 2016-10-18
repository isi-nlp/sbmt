cmake_minimum_required(VERSION 2.8)

project(guppy)

install(DIRECTORY . DESTINATION ./lib/guppy USE_SOURCE_PERMISSIONS FILES_MATCHING PATTERN *.py)