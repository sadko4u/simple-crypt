# simple-crypt

Simple cryptographic application.

Generates an infinite pseudo-random sequence according to the entered passphrase
and encrypts input files using this pseudo-random sequence.

The encryption algorithm is symmetric and decrypts the file using the same passphrase.

```
USAGE: ./simple-crypt [args...] [files...]

Available arguments:
  -d, --dump              Output encrypted content to stdout instead of file
  -h, --help              Output usage
  -i, --inplace           Overwrite file immediately, do not use temporary files
  -k, --key               Specify encryption key
  -o, --output            Specify output file to write data
  -r, --recursive         Process directories recursively
  -v, --verbose           Output name of processed files
```
