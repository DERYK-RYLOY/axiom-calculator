# Fonts

## Display face — Alro (optional)

The reference specimen supplied with the brief is **Alro** by **Artyway**,
sold through Font Bros: https://www.fontbros.com/font-family/alro

Alro is a **commercial** typeface. Its font binaries are therefore *not*
redistributed in this repository. The app is already wired to use it:

1. Purchase/download a licensed copy of `Alro-Regular.woff2` and
   `Alro-Bold.woff2` from Font Bros (the Web Font licence covers this use).
2. Drop both files into this folder.
3. Reload — headings and the brand name switch to Alro automatically.

## Bundled fallback — Jost (SIL OFL 1.1)

To guarantee the identical geometric look for everyone with zero setup, the
repository bundles **Jost** by indestructible-type, licensed under the SIL
Open Font License 1.1. See `OFL.txt` in this folder and
https://github.com/indestructible-type/Jost

Jost is a geometric sans in the same family as Alro (single-storey `a`,
circular `o`, tall `l`), so the visual character is preserved.

Font stack used by the app:

```css
font-family: 'Alro', 'Jost', 'Segoe UI', Roboto, Arial, sans-serif;
```
