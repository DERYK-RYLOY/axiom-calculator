# Third-party notices

## Jost typeface

Bundled in `app/fonts/Jost-*.ttf` and used as the geometric display fallback.

- Project: https://github.com/indestructible-type/Jost
- Copyright 2020 The Jost Project Authors
- Licence: SIL Open Font License 1.1 — full text in `app/fonts/OFL.txt`

## Alro typeface (not included)

The specimen image supplied with the brief shows **Alro** by **Artyway**,
distributed commercially by Font Bros: https://www.fontbros.com/font-family/alro

No Alro binaries are present in this repository. The stylesheet in
`app/index.html` declares `@font-face` rules that use a licensed local copy or
a user-supplied `app/fonts/Alro-*.woff2` when available, and otherwise fall back
to the bundled Jost files. Alro is a trademark of its respective owner and is
referenced here solely for identification.

## Runtime

No third-party JavaScript or Node packages are used. The web app, launcher and
test suite depend only on browser/Node standard APIs.
