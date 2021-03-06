cmake_minimum_required(VERSION 3.2)
project(miniaudio C)

add_library(miniaudio INTERFACE)
target_include_directories(miniaudio INTERFACE .)
