# Kompilator na JFFT 2017/2018
Autor: Kacper Kozerski

## Kompilacja
1) Do kompilacji programu potrzebne są:
 - `g++` z obsługą `C++11`
 - `bison`
 - `flex`
 - biblioteka `GMP` dla `C++`
2) Kompilacja kompilatora za pomocą komendy `make`

## Sposób użycia
    ./compiler <input >output
Błędy wyrzucane są na stderr.

## Wersje użytych programów i bibliotek
 - `g++` 5.3.1
 - `bison` 3.0.4
 - `flex` 2.6.0
 - `gmp` 6.1.0

## Interperter
Do kompilatora w folderze `interpreter` załączony jest interpreter
języka maszyny rejestrowej autorstwa dr Macieja Gębali.
