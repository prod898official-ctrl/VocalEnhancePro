# VocalEnhancePro VST3

**Nowoczesna wtyczka do czyszczenia i ulepszania wokali** dla FL Studio (i innych DAW).

Kolorystyka: granatowo-czarno-fioletowa z neonowymi akcentami.

## Funkcje (pokretła)

| Pokretło | Co robi |
|----------|--------|
| **HIGH-PASS** | Usuwa brudne basy i szumy niskie (20–300 Hz) |
| **DE-ESSER** | Tłumi ostre „s” i sybilanty |
| **COMPRESS** | Delikatna kompresja + makeup gain |
| **WARMTH** | Dodaje ciepła i nasycenia (soft saturation) |
| **AIR** | Podnosi wysokie częstotliwości (air / presence) |
| **OUTPUT** | Głośność wyjściowa |
| **BYPASS** | Wyłącza przetwarzanie |

Wtyczka ma też wizualizację fali dźwiękowej na żywo.

## Jak pobrać gotową wtyczkę (Windows)

1. Wejdź na stronę repozytorium:  
   **https://github.com/prod898official-ctrl/VocalEnhancePro**
2. Kliknij zakładkę **Actions**
3. Wybierz najnowszy udany workflow **Build VST3 Windows**
4. Na dole strony pobierz artefakt **VocalEnhancePro-Windows** (ZIP)
5. Rozpakuj ZIP → znajdź folder `VocalEnhancePro.vst3`
6. Skopiuj cały folder `VocalEnhancePro.vst3` do:
   ```
   C:\Program Files\Common Files\VST3\
   ```
7. W FL Studio:  
   **Options → Manage plugins → Find plugins** (lub Restart FL Studio)

Po skanowaniu wtyczka pojawi się w liście efektów jako **VocalEnhancePro**.

## Budowanie samodzielnie (dla zaawansowanych)

Wymagania:
- Visual Studio 2022 (z C++)
- CMake 3.22+
- Git

```bash
git clone https://github.com/prod898official-ctrl/VocalEnhancePro.git
cd VocalEnhancePro
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Gotowy plik `.vst3` znajdziesz w:
`build/VocalEnhancePro_artefacts/Release/VST3/`

## Kolorystyka UI

- Tło: głęboki granat / czarny (`#0a0614` → `#1a1035`)
- Akcenty neonowe: fiolet (`#7c3aed`, `#c084fc`)
- Tekst: jasny fiolet / lavender

## Licencja

Open source – możesz używać, modyfikować i udostępniać.

---

Stworzone dla szybkiego, czystego wokalu w FL Studio.
