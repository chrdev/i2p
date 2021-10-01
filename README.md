# i2p

i2p is an img2pdf launcher working on Windows.

It runs DIR /B /ON /A-D-H and feeds the result to img2pdf.

### Usage

```
i2p output_file_name [input_ext1 input_ext2 ... | *] [img2pdf arguments]

Examples:
  i2p manual jpg png -D
    Generate manual.pdf from *.jpg and *.png files and pass -D to img2pdf

  i2p scan
    Generate scan.pdf from default image types, which are
    *.jpg *.jpeg *.png *.gif *.tif *.tiff *.jp2 *.jpa *.jpm *.jpx

Caution:
  The target pdf file may be overwritten by img2pdf without prompt!
```

### License 0BSD

BSD Zero Clause License

Copyright (c) [2021] [chrdev]

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
