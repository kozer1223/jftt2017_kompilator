# Compiler for the "Formal Languages and Translation Techniques" 2017/2018 Course
Simple compiler from a simple imperative language to a RAM machine-like assembly language written in C++ using Flex and Bison.
Created for the "Formal Languages and Translation Techniques" 2017/2018 Course at Wrocław University of Technology.
Author: Kacper Kozerski

## Compilation
1) Required programs:
 - `g++` with `C++11`
 - `bison`
 - `flex`
 - `GMP` library for `C++`
2) In order to compile the program, run `make`

## How to use
    ./compiler <input_code_filename >output_code_filename
Errors are written to stderr.

## Tools and library versions
 - `g++` 5.3.1
 - `bison` 3.0.4
 - `flex` 2.6.0
 - `gmp` 6.1.0
 
## Interperter
The folder `interpreter` contains the RAM machine interpreter written by dr Maciej Gębala.

## Testy
Example test programs can be found in the `tests` folder.

# Kompilator na JFTT 2017/2018
Prosty kompilator prostego języka imperatywnego na uproszczoną maszynę RAM napisany w języku C++ korzystający z narzędzi Flex i Bison, napisany na potrzeby kursu Języki Formalne i Techniki Translacji 2017/2018. Autor: Kacper Kozerski.

## Kompilacja
1) Do kompilacji programu potrzebne są:
 - `g++` z obsługą `C++11`
 - `bison`
 - `flex`
 - biblioteka `GMP` dla `C++`
2) Kompilacja kompilatora za pomocą komendy `make`

## Sposób użycia
    ./compiler <input_code_filename >output_code_filename
Błędy wyrzucane są na stderr.

## Wersje użytych programów i bibliotek
 - `g++` 5.3.1
 - `bison` 3.0.4
 - `flex` 2.6.0
 - `gmp` 6.1.0

## Interperter
Do kompilatora w folderze `interpreter` załączony jest interpreter
języka maszyny rejestrowej autorstwa dra Macieja Gębali.

## Testy
Przykładowe testowe programy znajdują się w folderze `tests`.
